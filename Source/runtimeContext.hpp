#ifndef runtimeContext_hpp
#define runtimeContext_hpp
#include <variant>
#include <vector>
#include <deque>
#include <stack>
#include <string>
#include <unordered_map>
#include "variable.hpp"
#include "lookup.hpp"
#include "expression.hpp"

namespace cobalt {
	class runtimeContext {
	private:
		std::vector<function> _functions;
		std::unordered_map<std::string, size_t> _public_functions;
		std::vector<expression<lvalue>::ptr> _initializers;
		std::vector<variablePtr> _globals;
		std::deque<variablePtr> _stack;
		size_t _retval_idx;
		
		class scope {
		private:
			runtimeContext& _context;
			size_t _stack_size;
		public:
			scope(runtimeContext& context);
			~scope();
		};
		
	public:
		runtimeContext(
			std::vector<expression<lvalue>::ptr> initializers,
			std::vector<function> functions,
			std::unordered_map<std::string, size_t> public_functions
		);
	
		void initialize();

		variablePtr& global(int idx);
		variablePtr& retval();
		variablePtr& local(int idx);

		const function& get_function(int idx) const;
		const function& get_public_function(const char* name) const;

		scope enterScope();
		void push(variablePtr v);
		
		variablePtr call(const function& f, std::vector<variablePtr> params);
	};
}

#endif /*runtimeContext_hpp*/
