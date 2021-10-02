#ifndef variable_hpp
#define variable_hpp

#include <memory>
#include <deque>
#include <vector>
#include <functional>
#include <string>

namespace cobalt {

	class variable;
	
	using variablePtr = std::shared_ptr<variable>;

	template <typename T>
	class variableImpl;
	
	class runtimeContext;
	
	using number = double;
	using string = std::shared_ptr<std::string>;
	using array = std::deque<variablePtr>;
	using function = std::function<void(runtimeContext&)>;
	using tuple = array;
	using initializer_list = array;
	
	using lvalue = variablePtr;
	using lnumber = std::shared_ptr<variableImpl<number> >;
	using lstring = std::shared_ptr<variableImpl<string> >;
	using larray = std::shared_ptr<variableImpl<array> >;
	using lfunction = std::shared_ptr<variableImpl<function> >;
	using ltuple = std::shared_ptr<variableImpl<tuple> >;

	class variable: public std::enable_shared_from_this<variable> {
	private:
		variable(const variable&) = delete;
		void operator=(const variable&) = delete;
	protected:
		variable() = default;
	public:
		virtual ~variable() = default;

		template <typename T>
		T staticPointerDowncast() {
			return std::static_pointer_cast<
				variableImpl<typename T::element_type::valueType>
			>(shared_from_this());
		}
		
		virtual variablePtr clone() const = 0;
		
		virtual string to_string() const = 0;
	};
	
	template<typename T>
	class variableImpl: public variable {
	public:
		using valueType = T;
		
		valueType value;
		
		variableImpl(valueType value);
		
		variablePtr clone() const override;
	
		string to_string() const override;
	};
	
	number cloneVariableValue(number value);
	string cloneVariableValue(const string& value);
	function cloneVariableValue(const function& value);
	array cloneVariableValue(const array& value);
	
	template <class T>
	T cloneVariableValue(const std::shared_ptr<variableImpl<T> >& v) {
		return cloneVariableValue(v->value);
	}
	
	string convertToString(number value);
	string convertToString(const string& value);
	string convertToString(const function& value);
	string convertToString(const array& value);
	string convertToString(const lvalue& var);
}

#endif /* variable_hpp */
