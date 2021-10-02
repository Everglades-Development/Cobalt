#ifndef standardFunctions_hpp
#define standardFunctions_hpp

namespace cobalt {

	class module;
	
	void addMathFunctions(module& m);
	void addStringFunctions(module& m);
	void addTraceFunctions(module& m);
	
	void addStandardFunctions(module& m);
}

#endif /* standardFunctions_hpp */
