#ifndef types_h
#define types_h
#include <vector>
#include <variant>
#include <set>
#include <ostream>

namespace cobalt {
	enum struct simpleType {
		nothing,
		number,
		string,
	};
	
	struct arrayType;
	struct functionType;
	struct tupleType;
	struct initListType;
	
	using type = std::variant<simpleType, arrayType, functionType, tupleType, initListType>;
	using typeHandle = const type*;
	
	struct arrayType {
		typeHandle inner_type_id;
	};
	
	struct functionType {
		struct param {
			typeHandle typeID;
			bool by_ref;
		};
		typeHandle return_type_id;
		std::vector<param> param_type_id;
	};
	
	struct tupleType {
		std::vector<typeHandle> inner_type_id;
	};
	
	struct initListType {
		std::vector<typeHandle> inner_type_id;
	};
	
	class typeRegistry {
	private:
		struct typesLess{
			bool operator()(const type& t1, const type& t2) const;
		};
		std::set<type, typesLess> _types;
		
		static type void_type;
		static type number_type;
		static type string_type;
	public:
		typeRegistry();
		
		typeHandle getHandle(const type& t);
		
		static typeHandle getVoidHandle() {
			return &void_type;
		}
		
		static typeHandle getNumberHandle() {
			return &number_type;
		}
		
		static typeHandle getStringHandle() {
			return &string_type;
		}
	};
}

namespace std {
	std::string to_string(cobalt::typeHandle t);
}

#endif /* types_h */
