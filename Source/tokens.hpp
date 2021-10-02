#ifndef tokens_hpp
#define tokens_hpp

#include <optional>
#include <string_view>
#include <ostream>
#include <variant>

namespace cobalt {
	enum struct reservedToken {
		inc,
		dec,
		
		add,
		sub,
		concat,
		mul,
		div,
		idiv,
		mod,
		
		bitwise_not,
		bitwise_and,
		bitwise_or,
		bitwise_xor,
		shiftl,
		shiftr,
		
		assign,
		
		add_assign,
		sub_assign,
		concat_assign,
		mul_assign,
		div_assign,
		idiv_assign,
		mod_assign,
		
		and_assign,
		or_assign,
		xor_assign,
		shiftl_assign,
		shiftr_assign,
		
		logical_not,
		logical_and,
		logical_or,
		
		eq,
		ne,
		lt,
		gt,
		le,
		ge,
		
		question,
		colon,
		
		comma,
		
		semicolon,
		
		open_round,
		close_round,
		
		open_curly,
		close_curly,
		
		open_square,
		close_square,
		
		kw_sizeof,
		kw_tostring,
		
		kw_if,
		kw_else,
		kw_elif,

		kw_switch,
		kw_case,
		kw_default,

		kw_for,
		kw_while,
		kw_do,

		kw_break,
		kw_continue,
		kw_return,

		kw_function,
		
		kw_void,
		kw_number,
		kw_string,
		
		kw_public,
	};
	
	class push_back_stream;
	
	std::ostream& operator<<(std::ostream& os, reservedToken t);
	
	std::optional<reservedToken> getKeyword(std::string_view word);
	
	std::optional<reservedToken> getOperator(push_back_stream& stream);
	
	struct identifier{
		std::string name;
	};
	
	bool operator==(const identifier& id1, const identifier& id2);
	bool operator!=(const identifier& id1, const identifier& id2);
	
	struct eof{
	};
	
	bool operator==(const eof&, const eof&);
	bool operator!=(const eof&, const eof&);

	using tokenValue = std::variant<reservedToken, identifier, double, std::string, eof>;

	class token {
	private:
		tokenValue _value;
		size_t _line_number;
		size_t _char_index;
	public:
		token(tokenValue value, size_t lineNumber, size_t charIndex);
		
		bool isReservedToken() const;
		bool isIdentifier() const;
		bool isNumber() const;
		bool isString() const;
		bool isEof() const;
		
		reservedToken getReservedToken() const;
		const identifier& getIdentifier() const;
		double getNumber() const;
		const std::string& getString() const;
		const tokenValue& getValue() const;
		
		size_t getLineNumber() const;
		size_t getCharIndex() const;

		bool hasValue(const tokenValue& value) const;
	};
}

namespace std {
	std::string to_string(cobalt::reservedToken t);
	std::string to_string(const cobalt::tokenValue& t);}

#endif /* tokens_hpp */
