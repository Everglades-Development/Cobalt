#ifndef module_hpp
#define module_hpp

#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <iostream>
#include "variable.hpp"
#include "runtimeContext.hpp"

namespace cobalt {
	namespace details {
		template<typename R, typename Unpacked, typename Left>
		struct unpacker;
		
		template<typename R, typename... Unpacked, typename Left0, typename... Left>
		struct unpacker<R, std::tuple<Unpacked...>, std::tuple<Left0, Left...> >{
			R operator()(
				runtimeContext& ctx,
				const std::function<R(Unpacked..., Left0, Left...)>& f,
				std::tuple<Unpacked...> t
			) const {
				using next_unpacker = unpacker<R, std::tuple<Unpacked..., Left0>, std::tuple<Left...> >;
				if constexpr(std::is_convertible<const std::string&, Left0>::value) {
					return next_unpacker()(
						ctx,
						f,
						std::tuple_cat(
							std::move(t),
							std::tuple<Left0>(
								*ctx.local(
									-1 - int(sizeof...(Unpacked))
								)->staticPointerDowncast<lstring>()->value
							)
						)
					);
				} else {
					static_assert(std::is_convertible<number, Left0>::value);
					return next_unpacker()(
						ctx,
						f,
						std::tuple_cat(
							std::move(t),
							std::tuple<Left0>(
								ctx.local(
									-1 - int(sizeof...(Unpacked))
								)->staticPointerDowncast<lnumber>()->value
							)
						)
					);
				}
			}
		};
	
		template<typename R, typename... Unpacked>
		struct unpacker<R, std::tuple<Unpacked...>, std::tuple<> >{
			R operator()(
				runtimeContext& ctx,
				const std::function<R(Unpacked...)>& f,
				std::tuple<Unpacked...> t
			) const {
				return std::apply(f, t);
			}
		};
		
		template<typename R, typename... Args>
		function createExternalFunction(std::function<R(Args...)> f) {
			return [f=std::move(f)](runtimeContext& ctx) {
				if constexpr(std::is_same<R, void>::value) {
					unpacker<R, std::tuple<>, std::tuple<Args...> >()(ctx, f, std::tuple<>());
				} else {
					R retval = unpacker<R, std::tuple<>, std::tuple<Args...> >()(ctx, f, std::tuple<>());
					if constexpr(std::is_convertible<R, std::string>::value) {
						ctx.retval() = std::make_shared<variableImpl<string> >(std::make_shared<std::string>(std::move(retval)));
					} else {
						static_assert(std::is_convertible<R, number>::value);
						ctx.retval() = std::make_shared<variableImpl<number> >(retval);
					}
				}
			};
		}
		
		template<typename T>
		struct retval_declaration{
			static constexpr const char* result() {
				if constexpr(std::is_same<T, void>::value) {
					return "void";
				} else if constexpr(std::is_convertible<T, std::string>::value) {
					return "string";
				} else {
					static_assert(std::is_convertible<T, number>::value);
					return "number";
				}
			}
		};
		
		template<typename T>
		struct argumentDeclaration{
			static constexpr const char* result() {
				if constexpr(std::is_convertible<const std::string&, T>::value) {
					return "string";
				} else {
					static_assert(std::is_convertible<number, T>::value);
					return "number";
				}
			}
		};
		
		struct functionArgumentString{
			std::string str;
			functionArgumentString(const char* p):
				str(p)
			{
			}
			
			functionArgumentString& operator+=(const functionArgumentString& oth) {
				str += ", ";
				str += oth.str;
				return *this;
			}
		};
		
		template<typename R, typename... Args>
		std::string createFunctionDeclaration(const char* name) {
			if constexpr(sizeof...(Args) == 0) {
				return std::string("function ") + retval_declaration<R>::result() + " " + name + "()";
			} else {
				return std::string("function ") + retval_declaration<R>::result() + " " + name +
					"(" +
					(
						functionArgumentString(argumentDeclaration<Args>::result()) += ...
					).str +
					")";
			}
		}
		
		inline variablePtr to_variable(number n) {
			return std::make_shared<variableImpl<number> >(n);
		}
		
		inline variablePtr to_variable(std::string str) {
			return std::make_shared<variableImpl<string> >(std::make_shared<std::string>(std::move(str)));
		}
		
		template <typename T>
		T moveFromVariable(const variablePtr& v) {
			if constexpr (std::is_same<T, std::string>::value) {
				return std::move(*v->staticPointerDowncast<lstring>()->value);
			} else {
				static_assert(std::is_same<number, T>::value);
				return v->staticPointerDowncast<lnumber>()->value;
			}
		}
	}
	
	class module_impl;
	
	class module {
	private:
		std::unique_ptr<module_impl> _impl;
		void addExternalFunctionImpl(std::string declaration, function f);
		void addPublicFunctionDeclaration(std::string declaration, std::string name, std::shared_ptr<function> fptr);
		runtimeContext* getRuntimeContext();
	public:
		module();
		
		template<typename R, typename... Args>
		void addExternalFunctions(const char* name, std::function<R(Args...)> f) {
			addExternalFunctionImpl(
				details::createFunctionDeclaration<R, Args...>(name),
				details::createExternalFunction(std::move(f))
			);
		}
		
		template<typename R, typename... Args>
		auto createPublicFunctionCaller(std::string name) {
			std::shared_ptr<function> fptr = std::make_shared<function>();
			std::string decl = details::createFunctionDeclaration<R, Args...>(name.c_str());
			addPublicFunctionDeclaration(std::move(decl), std::move(name), fptr);
			
			return [this, fptr](Args... args){
				if constexpr(std::is_same<R, void>::value) {
					getRuntimeContext()->call(
						*fptr,
						{details::to_variable(std::move(args))...}
					);
				} else {
					return details::moveFromVariable<R>(getRuntimeContext()->call(
						*fptr,
						{details::to_variable(args)...}
					));
				}
			};
		}
		
		void load(const char* path);
		bool tryLoad(const char* path, std::ostream* err = nullptr) noexcept;
		
		void resetGlobals();
		
		~module();
	};
}
#endif /* module_hpp */
