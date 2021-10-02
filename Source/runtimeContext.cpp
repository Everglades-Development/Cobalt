#include "runtimeContext.hpp"
#include "errors.hpp"

namespace cobalt {
	runtimeContext::runtimeContext(
		std::vector<expression<lvalue>::ptr> initializers,
		std::vector<function> functions,
		std::unordered_map<std::string, size_t> public_functions
	) :
		_functions(std::move(functions)),
		_public_functions(std::move(public_functions)),
		_initializers(std::move(initializers)),
		_retval_idx(0)
	{
		_globals.reserve(_initializers.size());
		initialize();
	}
	
	void runtimeContext::initialize() {
		_globals.clear();
		
		for (const auto& initializer : _initializers) {
			_globals.emplace_back(initializer->evaluate(*this));
		}
	}
	
	variablePtr& runtimeContext::global(int idx) {
		runtimeAssertion(idx < _globals.size(), "Uninitialized global variable access");
		return _globals[idx];
	}

	variablePtr& runtimeContext::retval() {
		return _stack[_retval_idx];
	}

	variablePtr& runtimeContext::local(int idx) {
		return _stack[_retval_idx + idx];
	}
	
	const function& runtimeContext::get_function(int idx) const {
		return _functions[idx];
	}
	
	const function& runtimeContext::get_public_function(const char* name) const{
		return _functions[_public_functions.find(name)->second];
	}
	
	runtimeContext::scope runtimeContext::enterScope() {
		return scope(*this);
	}
	
	void runtimeContext::push(variablePtr v) {
		_stack.push_back(std::move(v));
	}

	variablePtr runtimeContext::call(const function& f, std::vector<variablePtr> params) {
		for (size_t i = params.size(); i > 0; --i) {
			_stack.push_back(std::move(params[i-1]));
		}
		size_t old_retval_idx = _retval_idx;
		
		_retval_idx = _stack.size();
		_stack.resize(_retval_idx + 1);
		
		runtimeAssertion(bool(f), "Uninitialized function call");
		
		f(*this);
		
		variablePtr ret = std::move(_stack[_retval_idx]);
		
		_stack.resize(_retval_idx - params.size());
		
		_retval_idx = old_retval_idx;
		
		return ret;
	}
	
	runtimeContext::scope::scope(runtimeContext& context):
		_context(context),
		_stack_size(context._stack.size())
	{
	}
	
	runtimeContext::scope::~scope() {
		_context._stack.resize(_stack_size);
	}
}
