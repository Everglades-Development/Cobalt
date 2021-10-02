#include "variable.hpp"

namespace cobalt {
	namespace {
		string from_std_string(std::string str) {
			return std::make_shared<std::string>(std::move(str));
		}
	}
	
	template<typename T>
	variableImpl<T>::variableImpl(T value):
		value(std::move(value))
	{
	}
	
	template<typename T>
	variablePtr variableImpl<T>::clone() const {
		return std::make_shared<variableImpl<T> >(cloneVariableValue(value));
	}
	
	template<typename T>
	string variableImpl<T>::to_string() const {
		return convertToString(value);
	}
	
	template class variableImpl<number>;
	template class variableImpl<string>;
	template class variableImpl<function>;
	template class variableImpl<array>;
	
	number cloneVariableValue(number value) {
		return value;
	}
	
	string cloneVariableValue(const string& value) {
		return value;
	}
	
	function cloneVariableValue(const function& value) {
		return value;
	}

	array cloneVariableValue(const array& value) {
		array ret;
		for (const variablePtr& v : value) {
			ret.push_back(v->clone());
		}
		return ret;
	}
	
	string convertToString(number value) {
		if (value == int(value)) {
			return from_std_string(std::to_string(int(value)));
		} else {
			return from_std_string(std::to_string(value));
		}
	}
	
	string convertToString(const string& value) {
		return value;
	}
	
	string convertToString(const function& value) {
		return from_std_string("FUNCTION");
	}
	
	string convertToString(const array& value) {
		std::string ret = "[";
		const char* separator = "";
		for (const variablePtr& v : value) {
			ret += separator;
			ret += *(v->to_string());
			separator = ", ";
		}
		ret += "]";
		return from_std_string(std::move(ret));
	}
	
	string convertToString(const lvalue& var) {
		return var->to_string();
	}
}

