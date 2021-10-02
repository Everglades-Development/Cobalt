#ifndef standardFunctions_hpp
#define standardFunctions_hpp

namespace cobalt {

	class module;
	
	void add_math_functions(module& m);
	void add_string_functions(module& m);
	void add_trace_functions(module& m);
	
	void add_standard_functions(module& m);
}

#endif /* standardFunctions_hpp */
