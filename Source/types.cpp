#include "types.hpp"
#include "helpers.hpp"

namespace cobalt {
	bool typeRegistry::typesLess::operator()(const type& t1, const type& t2) const {
		const size_t idx1 = t1.index();
		const size_t idx2 = t2.index();
		
		if (idx1 != idx2) {
			return idx1 < idx2;
		}
		
		switch (idx1) {
			case 0:
				return std::get<0>(t1) < std::get<0>(t2);
			case 1:
				return std::get<1>(t1).inner_type_id < std::get<1>(t2).inner_type_id;
			case 2:
			{
				const functionType& ft1 = std::get<2>(t1);
				const functionType& ft2 = std::get<2>(t2);
				
				if (ft1.return_type_id != ft2.return_type_id) {
					return ft1.return_type_id < ft2.return_type_id;
				}
				
				if (ft1.param_type_id.size() != ft2.param_type_id.size()) {
					return ft1.param_type_id.size() < ft2.param_type_id.size();
				}
				
				for (size_t i = 0; i < ft1.param_type_id.size(); ++i) {
					if (ft1.param_type_id[i].typeID != ft2.param_type_id[i].typeID) {
						return ft1.param_type_id[i].typeID < ft2.param_type_id[i].typeID;
					}
					if (ft1.param_type_id[i].by_ref != ft2.param_type_id[i].by_ref) {
						return ft2.param_type_id[i].by_ref;
					}
				}
				return false;
			}
			case 3:
			{
				const tupleType& tt1 = std::get<3>(t1);
				const tupleType& tt2 = std::get<3>(t2);
				
				if (tt1.inner_type_id.size() != tt2.inner_type_id.size()) {
					return tt1.inner_type_id.size() < tt2.inner_type_id.size();
				}
				
				for (size_t i = 0; i < tt1.inner_type_id.size(); ++i) {
					if (tt1.inner_type_id[i] != tt2.inner_type_id[i]) {
						return tt1.inner_type_id[i] < tt2.inner_type_id[i];
					}
				}
				return false;
			}
			case 4:
			{
				const initListType& ilt1 = std::get<4>(t1);
				const initListType& ilt2 = std::get<4>(t2);
				
				if (ilt1.inner_type_id.size() != ilt2.inner_type_id.size()) {
					return ilt1.inner_type_id.size() < ilt2.inner_type_id.size();
				}
				
				for (size_t i = 0; i < ilt1.inner_type_id.size(); ++i) {
					if (ilt1.inner_type_id[i] != ilt2.inner_type_id[i]) {
						return ilt1.inner_type_id[i] < ilt2.inner_type_id[i];
					}
				}
				return false;
			}
		}
		
		return false;
	}

	typeRegistry::typeRegistry(){
	}
	
	typeHandle typeRegistry::getHandle(const type& t) {
		return std::visit(overloaded{
			[](simpleType t) {
				switch (t) {
					case simpleType::nothing:
						return typeRegistry::getVoidHandle();
					case simpleType::number:
						return typeRegistry::getNumberHandle();
					case simpleType::string:
						return typeRegistry::getStringHandle();
				}
			},
			[this](const auto& t) {
				return &(*(_types.insert(t).first));
			}
		}, t);
	}
	
	type typeRegistry::void_type = simpleType::nothing;
	type typeRegistry::number_type = simpleType::number;
	type typeRegistry::string_type = simpleType::string;
}

namespace std {
	using namespace cobalt;
	std::string to_string(typeHandle t) {
		return std::visit(overloaded{
			[](simpleType st) {
				switch (st) {
					case simpleType::nothing:
						return std::string("void");
					case simpleType::number:
						return std::string("number");
					case simpleType::string:
						return std::string("string");
				}
			},
			[](const arrayType& at) {
				std::string ret = to_string(at.inner_type_id);
				ret += "[]";
				return ret;
			},
			[](const functionType& ft) {
				std::string ret = to_string(ft.return_type_id) + "(";
				const char* separator = "";
				for (const functionType::param& p: ft.param_type_id) {
					ret +=  separator + to_string(p.typeID) + (p.by_ref ? "&" : "");
					separator = ",";
				}
				ret += ")";
				return ret;
			},
			[](const tupleType& tt) {
				std::string ret = "[";
				const char* separator = "";
				for (typeHandle it : tt.inner_type_id) {
					ret +=  separator + to_string(it);
					separator = ",";
				}
				ret += "]";
				return ret;
			},
			[](const initListType& ilt) {
				std::string ret = "{";
				const char* separator = "";
				for (typeHandle it : ilt.inner_type_id) {
					ret +=  separator + to_string(it);
					separator = ",";
				}
				ret += "}";
				return ret;
			}
		}, *t);
	}
}
