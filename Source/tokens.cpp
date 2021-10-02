#include "tokens.hpp"
#include "lookup.hpp"
#include <string_view>
#include "helpers.hpp"
#include <stack>
#include "pushBackStream.hpp"

namespace cobalt {
	namespace {
		const lookup<std::string_view, reservedToken> operator_token_map {
			{"++", reservedToken::inc},
			{"--", reservedToken::dec},
			
			{"+", reservedToken::add},
			{"-", reservedToken::sub},
			{"..", reservedToken::concat},
			{"*", reservedToken::mul},
			{"/", reservedToken::div},
			{"\\", reservedToken::idiv},
			{"%", reservedToken::mod},
			
			{"~", reservedToken::bitwise_not},
			{"&", reservedToken::bitwise_and},
			{"|", reservedToken::bitwise_or},
			{"^", reservedToken::bitwise_xor},
			{"<<", reservedToken::shiftl},
			{">>", reservedToken::shiftr},
			
			{"=", reservedToken::assign},
			
			{"+=", reservedToken::add_assign},
			{"-=", reservedToken::sub_assign},
			{"..=", reservedToken::concat_assign},
			{"*=", reservedToken::mul_assign},
			{"/=", reservedToken::div_assign},
			{"\\=", reservedToken::idiv_assign},
			{"%=", reservedToken::mod_assign},
			
			{"&=", reservedToken::and_assign},
			{"|=", reservedToken::or_assign},
			{"^=", reservedToken::xor_assign},
			{"<<=", reservedToken::shiftl_assign},
			{">>=", reservedToken::shiftr_assign},
			
			{"!", reservedToken::logical_not},
			{"&&", reservedToken::logical_and},
			{"||", reservedToken::logical_or},
			
			{"==", reservedToken::eq},
			{"!=", reservedToken::ne},
			{"<", reservedToken::lt},
			{">", reservedToken::gt},
			{"<=", reservedToken::le},
			{">=", reservedToken::ge},
			
			{"?", reservedToken::question},
			{":", reservedToken::colon},
			
			{",", reservedToken::comma},
			
			{";", reservedToken::semicolon},
			
			{"(", reservedToken::open_round},
			{")", reservedToken::close_round},
			
			{"{", reservedToken::open_curly},
			{"}", reservedToken::close_curly},
			
			{"[", reservedToken::open_square},
			{"]", reservedToken::close_square},
		};
		
		const lookup<std::string_view, reservedToken> keyword_token_map {
			{"sizeof", reservedToken::kw_sizeof},
			{"tostring", reservedToken::kw_tostring},
		
			{"if", reservedToken::kw_if},
			{"else", reservedToken::kw_else},
			{"elif", reservedToken::kw_elif},

			{"switch", reservedToken::kw_switch},
			{"case", reservedToken::kw_case},
			{"default", reservedToken::kw_default},

			{"for", reservedToken::kw_for},
			{"while", reservedToken::kw_while},
			{"do", reservedToken::kw_do},

			{"break", reservedToken::kw_break},
			{"continue", reservedToken::kw_continue},
			{"return", reservedToken::kw_return},

			{"function", reservedToken::kw_function},
			
			{"void", reservedToken::kw_void},
			{"number", reservedToken::kw_number},
			{"string", reservedToken::kw_string},
			
			{"public", reservedToken::kw_public}
		};
		
		const lookup<reservedToken, std::string_view> token_string_map = ([](){
			std::vector<std::pair<reservedToken, std::string_view>> container;
			container.reserve(operator_token_map.size() + keyword_token_map.size());
			for (const auto& p : operator_token_map) {
				container.emplace_back(p.second, p.first);
			}
			for (const auto& p : keyword_token_map) {
				container.emplace_back(p.second, p.first);
			}
			return lookup<reservedToken, std::string_view>(std::move(container));
		})();
	}
	
	std::optional<reservedToken> getKeyword(std::string_view word) {
		auto it = keyword_token_map.find(word);
		return it == keyword_token_map.end() ? std::nullopt : std::make_optional(it->second);
	}
	
	namespace {
		class maximal_munch_comparator{
		private:
			size_t _idx;
		public:
			maximal_munch_comparator(size_t idx) :
				_idx(idx)
			{
			}
			
			bool operator()(char l, char r) const {
				return l < r;
			}
			
			bool operator()(std::pair<std::string_view, reservedToken> l, char r) const {
				return l.first.size() <= _idx || l.first[_idx] < r;
			}
			
			bool operator()(char l, std::pair<std::string_view, reservedToken> r) const {
				return r.first.size() > _idx && l < r.first[_idx];
			}
			
			bool operator()(std::pair<std::string_view, reservedToken> l, std::pair<std::string_view, reservedToken> r) const {
				return r.first.size() > _idx && (l.first.size() < _idx || l.first[_idx] < r.first[_idx]);
			}
		};
	}
	
	std::optional<reservedToken> getOperator(push_back_stream& stream) {
		auto candidates = std::make_pair(operator_token_map.begin(), operator_token_map.end());
		
		std::optional<reservedToken> ret;
		size_t match_size = 0;
		
		std::stack<int> chars;
		
		for (size_t idx = 0; candidates.first != candidates.second; ++idx) {
			chars.push(stream());
			
			candidates = std::equal_range(candidates.first, candidates.second, char(chars.top()), maximal_munch_comparator(idx));
			
			if (candidates.first != candidates.second && candidates.first->first.size() == idx + 1) {
				match_size = idx + 1;
				ret = candidates.first->second;
			}
		}
		
		while (chars.size() > match_size) {
			stream.push_back(chars.top());
			chars.pop();
		}
		
		return ret;
	}
	
	token::token(tokenValue value, size_t lineNumber, size_t charIndex) :
		_value(std::move(value)),
		_line_number(lineNumber),
		_char_index(charIndex)
	{
	}
	
	bool token::isReservedToken() const {
		return std::holds_alternative<reservedToken>(_value);
	}
	
	bool token::isIdentifier() const {
		return std::holds_alternative<identifier>(_value);
	}
	
	bool token::isNumber() const {
		return std::holds_alternative<double>(_value);
	}
	
	bool token::isString() const {
		return std::holds_alternative<std::string>(_value);
	}
	
	bool token::isEof() const {
		return std::holds_alternative<eof>(_value);
	}
	
	reservedToken token::getReservedToken() const {
		return std::get<reservedToken>(_value);
	}
	
	const identifier& token::getIdentifier() const {
		return std::get<identifier>(_value);
	}
	
	double token::getNumber() const {
		return std::get<double>(_value);
	}
	
	const std::string& token::getString() const {
		return std::get<std::string>(_value);
	}
	
	const tokenValue& token::getValue() const {
		return _value;
	}
	
	size_t token::getLineNumber() const {
		return _line_number;
	}

	size_t token::getCharIndex() const {
		return _char_index;
	}
	
	bool token::hasValue(const tokenValue& value) const {
		return _value == value;
	}
	
	bool operator==(const identifier& id1, const identifier& id2) {
		return id1.name == id2.name;
	}
	
	bool operator!=(const identifier& id1, const identifier& id2) {
		return id1.name != id2.name;
	}
	
	bool operator==(const eof&, const eof&) {
		return true;
	}
	
	bool operator!=(const eof&, const eof&) {
		return false;
	}
}

namespace std {
	using namespace cobalt;
	std::string to_string(reservedToken t) {
		return std::string(token_string_map.find(t)->second);
	}
	
	std::string to_string(const tokenValue& t) {
		return std::visit(overloaded{
			[](reservedToken rt) {
				return to_string(rt);
			},
			[](double d) {
				return to_string(d);
			},
			[](const std::string& str) {
				return str;
			},
			[](const identifier& id) {
				return id.name;
			},
			[](eof) {
				return std::string("<EOF>");
			}
		}, t);
	}
}
