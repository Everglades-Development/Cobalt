#include "tokeniser.hpp"
#include <map>
#include <string>
#include <cctype>
#include <stack>
#include <cstdlib>
#include "pushBackStream.hpp"
#include "errors.hpp"

namespace cobalt {
	namespace {
		enum struct character_type {
			eof,
			space,
			alphanum,
			punct,
		};
		
		character_type get_character_type(int c) {
			if (c < 0 ) {
				return character_type::eof;
			}
			if (std::isspace(c)) {
				return character_type::space;
			}
			if (std::isalpha(c) || std::isdigit(c) || c == '_') {
				return character_type::alphanum;
			}
			return character_type::punct;
		}
		
		token fetch_word(push_back_stream& stream) {
			size_t lineNumber = stream.lineNumber();
			size_t charIndex = stream.charIndex();

			std::string word;
			
			int c = stream();
			
			bool isNumber = isdigit(c);
			
			do {
				word.push_back(char(c));
				c = stream();
				
				if (c == '.' && word.back() == '.') {
					stream.push_back(word.back());
					word.pop_back();
					break;
				}
			} while (get_character_type(c) == character_type::alphanum || (isNumber && c == '.'));
			
			stream.push_back(c);
			
			if (std::optional<reservedToken> t  = getKeyword(word)) {
				return token(*t, lineNumber, charIndex);
			} else {
				if (std::isdigit(word.front())) {
					char* endptr;
					double num = strtol(word.c_str(), &endptr, 0);
					if (*endptr != 0) {
						num = strtod(word.c_str(), &endptr);
						if (*endptr != 0) {
							size_t remaining = word.size() - (endptr - word.c_str());
							throw unexpectedError(
								std::string(1, char(*endptr)),
								stream.lineNumber(),
								stream.charIndex() - remaining
							);
						}
					}
					return token(num, lineNumber, charIndex);
				} else {
					return token(identifier{std::move(word)}, lineNumber, charIndex);
				}
			}
		}
		
		token fetch_operator(push_back_stream& stream) {
			size_t lineNumber = stream.lineNumber();
			size_t charIndex = stream.charIndex();

			if (std::optional<reservedToken> t = getOperator(stream)) {
				return token(*t, lineNumber, charIndex);
			} else {
				std::string unexpected;
				size_t err_line_number = stream.lineNumber();
				size_t err_char_index = stream.charIndex();
				for (int c = stream(); get_character_type(c) == character_type::punct; c = stream()) {
					unexpected.push_back(char(c));
				}
				throw unexpectedError(unexpected, err_line_number, err_char_index);
			}
		}
		
		token fetch_string(push_back_stream& stream) {
			size_t lineNumber = stream.lineNumber();
			size_t charIndex = stream.charIndex();

			std::string str;
			
			bool escaped = false;
			int c = stream();
			for (; get_character_type(c) != character_type::eof; c = stream()) {
				if (c == '\\') {
					escaped = true;
				} else {
					if (escaped) {
						switch(c) {
							case 't':
								str.push_back('\t');
								break;
							case 'n':
								str.push_back('\n');
								break;
							case 'r':
								str.push_back('\r');
								break;
							case '0':
								str.push_back('\0');
								break;
							default:
								str.push_back(c);
								break;
						}
						escaped = false;
					} else {
						switch (c) {
							case '\t':
							case '\n':
							case '\r':
								stream.push_back(c);
								throw parsingError("Expected closing '\"'", stream.lineNumber(), stream.charIndex());
							case '"':
								return token(std::move(str), lineNumber, charIndex);
							default:
								str.push_back(c);
						}
					}
				}
			}
			stream.push_back(c);
			throw parsingError("Expected closing '\"'", stream.lineNumber(), stream.charIndex());
		}
		
		void skip_line_comment(push_back_stream& stream) {
			int c;
			do {
				c = stream();
			} while (c != '\n' && get_character_type(c) != character_type::eof);
			
			if (c != '\n') {
				stream.push_back(c);
			}
		}
		
		void skip_block_comment(push_back_stream& stream) {
			bool closing = false;
			int c;
			do {
				c = stream();
				if (closing && c == '/') {
					return;
				}
				closing = (c == '*');
			} while (get_character_type(c) != character_type::eof);

			stream.push_back(c);
			throw parsingError("Expected closing '*/'", stream.lineNumber(), stream.charIndex());
		}
	
		token tokenize(push_back_stream& stream) {
			while (true) {
				size_t lineNumber = stream.lineNumber();
				size_t charIndex = stream.charIndex();
				int c = stream();
				switch (get_character_type(c)) {
					case character_type::eof:
						return {eof(), lineNumber, charIndex};
					case character_type::space:
						continue;
					case character_type::alphanum:
						stream.push_back(c);
						return fetch_word(stream);
					case character_type::punct:
						switch (c) {
							case '"':
								return fetch_string(stream);
							case '/':
							{
								char c1 = stream();
								switch(c1) {
									case '/':
										skip_line_comment(stream);
										continue;
									case '*':
										skip_block_comment(stream);
										continue;
									default:
										stream.push_back(c1);
								}
							}
							default:
								stream.push_back(c);
								return fetch_operator(stream);
						}
						break;
				}
			}
		}
	}
	
	tokensIterator::tokensIterator(push_back_stream& stream):
		_current(eof(), 0, 0),
		_get_next_token([&stream](){
			return tokenize(stream);
		})
	{
		++(*this);
	}
	
	tokensIterator::tokensIterator(std::deque<token>& tokens):
		_current(eof(), 0, 0),
		_get_next_token([&tokens](){
			if (tokens.empty()) {
				return token(eof(), 0, 0);
			} else {
				token ret = std::move(tokens.front());
				tokens.pop_front();
				return ret;
			}
		})
	{
		++(*this);
	}

	tokensIterator& tokensIterator::operator++() {
		_current = _get_next_token();
		return *this;
	}
	
	const token& tokensIterator::operator*() const {
		return _current;
	}
	
	const token* tokensIterator::operator->() const {
		return &_current;
	}

	tokensIterator::operator bool() const {
		return !_current.isEof();
	}
}

