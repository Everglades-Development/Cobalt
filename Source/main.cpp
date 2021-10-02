#include <iostream>
#include "module.hpp"
#include "standardFunctions.hpp"

int main() {
	std::string path = __FILE__;
	path = path.substr(0, path.find_last_of("/\\") + 1) + "..\\Samples\\ascTest.cbt";
	
	using namespace cobalt;
	
	module m;
	
	addStandardFunctions(m);
	
	/*
	m.addExternalFunctions("greater", std::function<number(number, number)>([](number x, number y){
		return x > y;
	}));
	*/
	
	auto s_main = m.createPublicFunctionCaller<void>("main");
	
	if (m.tryLoad(path.c_str(), &std::cerr)) {
		s_main();
	}
	
	return 0;
}