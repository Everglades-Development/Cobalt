#ifndef errors_hpp
#define errors_hpp

#include <exception>
#include <functional>
#include <string>
#include <string_view>
#include <ostream>

namespace cobalt {
	class error: public std::exception {
	private:
		std::string _message;
		size_t _line_number;
		size_t _char_index;
	public:
		error(std::string message, size_t lineNumber, size_t charIndex) noexcept;
		
		const char* what() const noexcept override;
		size_t lineNumber() const noexcept;
		size_t charIndex() const noexcept;
	};
	
	error parsingError(std::string_view message, size_t lineNumber, size_t charIndex);
	error syntaxError(std::string_view message, size_t lineNumber, size_t charIndex);
	error semanticError(std::string_view message, size_t lineNumber, size_t charIndex);
	error compilerError(std::string_view message, size_t lineNumber, size_t charIndex);

	error unexpectedError(std::string_view unexpected, size_t lineNumber, size_t charIndex);
	error unexpectedSyntaxError(std::string_view unexpected, size_t lineNumber, size_t charIndex);
	error expectedSyntaxError(std::string_view expected, size_t lineNumber, size_t charIndex);
	error undeclaredError(std::string_view undeclared, size_t lineNumber, size_t charIndex);
	error wrongTypeError(std::string_view source, std::string_view destination, bool lvalue,
	                       size_t lineNumber, size_t charIndex);
	error alreadyDeclaredError(std::string_view name, size_t lineNumber, size_t charIndex);

	using get_character = std::function<int()>;
	void formatError(const error& err, const get_character& source, std::ostream& output);
	
	
	class runtimeError: public std::exception {
	private:
		std::string _message;
	public:
		runtimeError(std::string message) noexcept;
		
		const char* what() const noexcept override;
	};
	
	void runtimeAssertion(bool b, const char* message);
	
	class fileNotFound: public std::exception {
	private:
		std::string _message;
	public:
		fileNotFound(std::string message) noexcept;
		
		const char* what() const noexcept override;
	};
};

#endif /* errors_hpp */
