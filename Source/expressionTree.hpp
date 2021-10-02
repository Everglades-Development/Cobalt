#ifndef expressionTree_hpp
#define expressionTree_hpp
#include <memory>
#include <variant>
#include <vector>
#include "tokens.hpp"
#include "types.hpp"

namespace cobalt {

	enum struct nodeOperation {
		param,

		preinc,
		predec,
		postinc,
		postdec,
		positive,
		negative,
		bnot,
		lnot,
		size,
		tostring,
		
		add,
		sub,
		mul,
		div,
		idiv,
		mod,
		band,
		bor,
		bxor,
		bsl,
		bsr,
		concat,
		assign,
		add_assign,
		sub_assign,
		mul_assign,
		div_assign,
		idiv_assign,
		mod_assign,
		band_assign,
		bor_assign,
		bxor_assign,
		bsl_assign,
		bsr_assign,
		concat_assign,
		eq,
		ne,
		lt,
		gt,
		le,
		ge,
		comma,
		land,
		lor,
		index,
		
		ternary,
		
		call,
		
		init,
	};
	
	struct node;
	using node_ptr=std::unique_ptr<node>;
	
	using nodeValue=std::variant<nodeOperation, std::string, double, identifier>;
	
	class compilerContext;
	
	struct node {
	private:
		nodeValue _value;
		std::vector<node_ptr> _children;
		typeHandle _type_id;
		bool _lvalue;
		size_t _line_number;
		size_t _char_index;
	public:
		node(compilerContext& context, nodeValue value, std::vector<node_ptr> children, size_t lineNumber, size_t charIndex);
		
		const nodeValue& getValue() const;
		
		bool is_node_operation() const;
		bool isIdentifier() const;
		bool isNumber() const;
		bool isString() const;
		
		nodeOperation getNodeOperation() const;
		std::string_view getIdentifier() const;
		double getNumber() const;
		std::string_view getString() const;

		const std::vector<node_ptr>& getChildren() const;
		
		typeHandle getTypeID() const;
		bool is_lvalue() const;
		
		size_t getLineNumber() const;
		size_t getCharIndex() const;
		
		void checkConversion(typeHandle typeID, bool lvalue) const;
	};

}

#endif /* expressionTree_hpp */
