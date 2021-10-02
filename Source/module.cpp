#include "module.hpp"
#include <vector>
#include <cstdio>
#include "errors.hpp"
#include "pushBackStream.hpp"
#include "tokeniser.hpp"
#include "compiler.hpp"

namespace cobalt {
	namespace {
		class file{
			file(const file&) = delete;
			void operator=(const file&) = delete;
		private:
			FILE* _fp;
		public:
			file(const char* path):
				_fp(fopen(path, "rt"))
			{
				if (!_fp) {
					throw fileNotFound(std::string("'") + path + "' not found");
				}
			}
			
			~file() {
				if (_fp) {
					fclose(_fp);
				}
			}
			
			int operator()() {
				return fgetc(_fp);
			}
		};
	}

	class module_impl {
	private:
		std::vector<std::pair<std::string, function> > _external_functions;
		std::vector<std::string> _public_declarations;
		std::unordered_map<std::string, std::shared_ptr<function> > _public_functions;
		std::unique_ptr<runtimeContext> _context;
	public:
		module_impl(){
		}
		
		runtimeContext* getRuntimeContext() {
			return _context.get();
		}
		
		void addPublicFunctionDeclaration(std::string declaration, std::string name, std::shared_ptr<function> fptr) {
			_public_declarations.push_back(std::move(declaration));
			_public_functions.emplace(std::move(name), std::move(fptr));
		}
		
		void addExternalFunctionImpl(std::string declaration, function f) {
			_external_functions.emplace_back(std::move(declaration), std::move(f));
		}
		
		void load(const char* path) {
			file f(path);
			get_character get = [&](){
				return f();
			};
			push_back_stream stream(&get);
			
			tokensIterator it(stream);
			
			_context = std::make_unique<runtimeContext>(compile(it, _external_functions, _public_declarations));
			
			for (const auto& p : _public_functions) {
				*p.second = _context->get_public_function(p.first.c_str());
			}
		}
		
		bool tryLoad(const char* path, std::ostream* err) noexcept{
			try {
				load(path);
				return true;
			} catch(const fileNotFound& e) {
				if (err) {
					*err << e.what() << std::endl;
				}
			} catch(const error& e) {
				if (err) {
					file f(path);
					formatError(
						e,
						[&](){
							return f();
						},
						*err
					);
				}
			} catch(const runtimeError& e) {
				if (err) {
					*err << e.what() << std::endl;
				}
			}
			return false;
		}
		
		void resetGlobals() {
			if (_context) {
				_context->initialize();
			}
		}
	};
	
	module::module():
		_impl(std::make_unique<module_impl>())
	{
	}
	
	runtimeContext* module::getRuntimeContext() {
		return _impl->getRuntimeContext();
	}
	
	void module::addExternalFunctionImpl(std::string declaration, function f) {
		_impl->addExternalFunctionImpl(std::move(declaration), std::move(f));
	}

	void module::addPublicFunctionDeclaration(std::string declaration, std::string name, std::shared_ptr<function> fptr) {
		_impl->addPublicFunctionDeclaration(std::move(declaration), std::move(name), std::move(fptr));
	}
	
	void module::load(const char* path) {
		_impl->load(path);
	}
	
	bool module::tryLoad(const char* path, std::ostream* err) noexcept{
		return _impl->tryLoad(path, err);
	}
	
	void module::resetGlobals() {
		_impl->resetGlobals();
	}
	
	module::~module() {
	}
}
