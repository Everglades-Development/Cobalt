#ifndef tokeniser_hpp
#define tokeniser_hpp

#include <functional>
#include <string_view>
#include <iostream>
#include <variant>
#include <deque>

#include "tokens.hpp"

namespace cobalt {
	class push_back_stream;

	class tokensIterator {
		tokensIterator(const tokensIterator&) = delete;
		void operator=(const tokensIterator&) = delete;
	private:
		std::function<token()> _get_next_token;
		token _current;
	public:
		tokensIterator(push_back_stream& stream);
		tokensIterator(std::deque<token>& tokens);
		
		const token& operator*() const;
		const token* operator->() const;
		
		tokensIterator& operator++();
		
		explicit operator bool() const;
	};
}


#endif /* tokeniser_hpp */
