#ifndef pushBackStream_h
#define pushBackStream_h
#include <stack>
#include <functional>

namespace cobalt {
	using get_character = std::function<int()>;

	class push_back_stream {
	private:
		const get_character& _input;
		std::stack<int> _stack;
		size_t _line_number;
		size_t _char_index;
	public:
		push_back_stream(const get_character* input);
		
		int operator()();
		
		void push_back(int c);
		
		size_t lineNumber() const;
		size_t charIndex() const;
	};
}

#endif /* pushBackStream_h */
