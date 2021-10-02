#include "standardFunctions.hpp"
#include "module.hpp"

#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>

namespace cobalt {

	void addMathFunctions(module& m) {
		m.addExternalFunctions("sin", std::function<number(number)>(
			[](number x) {
				return std::sin(x);
			}
		));
		
		m.addExternalFunctions("cos", std::function<number(number)>(
			[](number x) {
				return std::cos(x);
			}
		));
		
		m.addExternalFunctions("tan", std::function<number(number)>(
			[](number x) {
				return std::tan(x);
			}
		));
		
		m.addExternalFunctions("log", std::function<number(number)>(
			[](number x) {
				return std::log(x);
			}
		));
		
		m.addExternalFunctions("exp", std::function<number(number)>(
			[](number x) {
				return std::exp(x);
			}
		));
		
		m.addExternalFunctions("pow", std::function<number(number, number)>(
			[](number x, number y) {
				return std::pow(x, y);
			}
		));
		
		srand((unsigned int)time(0));
		
		m.addExternalFunctions("rnd", std::function<number(number)>(
			[](number x) {
				return rand() % int(x);
			}
		));
	}
	
	void addStringFunctions(module& m) {
		m.addExternalFunctions("strlen", std::function<number(const std::string&)>(
			[](const std::string& str) {
				return str.size();
			}
		));
		
		m.addExternalFunctions("substr", std::function<std::string(const std::string&, number, number)>(
			[](const std::string& str, number from, number count) {
				return str.substr(size_t(from), size_t(count));
			}
		));
	}
	
	void addTraceFunctions(module& m) {
		m.addExternalFunctions("trace", std::function<void(const std::string&)>(
			[](const std::string& str) {
				std::cout << str << std::endl;
			}
		));
	}
	
	void addStandardFunctions(module& m) {
		addMathFunctions(m);
		addStringFunctions(m);
		addTraceFunctions(m);
	}

}
