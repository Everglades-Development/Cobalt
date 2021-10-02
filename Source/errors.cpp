#include "errors.hpp"
#include <sstream>

namespace cobalt {
	error::error(std::string message, size_t lineNumber, size_t charIndex) noexcept :
		_message(std::move(message)),
		_line_number(lineNumber),
		_char_index(charIndex)
	{
	}
		
	const char* error::what() const noexcept {
		return _message.c_str();
	}
		
	size_t error::lineNumber() const noexcept {
		return _line_number;
	}
	
	size_t error::charIndex() const noexcept {
		return _char_index;
	}
	
	error parsingError(std::string_view message, size_t lineNumber, size_t charIndex) {
		std::string errorMessage("Parsing error: ");
		errorMessage += message;
		return error(std::move(errorMessage), lineNumber, charIndex);
	}
	
	error syntaxError(std::string_view message, size_t lineNumber, size_t charIndex) {
		std::string errorMessage("Syntax error: ");
		errorMessage += message;
		return error(std::move(errorMessage), lineNumber, charIndex);
	}
	
	error semanticError(std::string_view message, size_t lineNumber, size_t charIndex) {
		std::string errorMessage("Semantic error: ");
		errorMessage += message;
		return error(std::move(errorMessage), lineNumber, charIndex);
	}
	
	error compilerError(std::string_view message, size_t lineNumber, size_t charIndex) {
		std::string errorMessage("Compiler error: ");
		errorMessage += message;
		return error(std::move(errorMessage), lineNumber, charIndex);
	}
	
	error unexpectedError(std::string_view unexpected, size_t lineNumber, size_t charIndex) {
		std::string message("Unexpected '");
		message += unexpected;
		message += "'";
		return parsingError(message, lineNumber, charIndex);
	}
	
	error unexpectedSyntaxError(std::string_view unexpected, size_t lineNumber, size_t charIndex) {
		std::string message("Unexpected '");
		message += unexpected;
		message += "'";
		return syntaxError(message, lineNumber, charIndex);
	}
	
	error expectedSyntaxError(std::string_view expected, size_t lineNumber, size_t charIndex) {
		std::string message("Expected '");
		message += expected;
		message += "'";
		return syntaxError(message, lineNumber, charIndex);
	}
	
	error undeclaredError(std::string_view undeclared, size_t lineNumber, size_t charIndex) {
		std::string message("Undeclared identifier '");
		message += undeclared;
		message += "'";
		return semanticError(message, lineNumber, charIndex);
	}

	error wrongTypeError(std::string_view source, std::string_view destination,
	                       bool lvalue, size_t lineNumber,
		size_t charIndex) {
		std::string message;
		if (lvalue) {
			message += "'";
			message += source;
			message += "' is not a lvalue";
		} else {
			message += "Cannot convert '";
			message +=  source;
			message += "' to '";
			message +=  destination;
			message += "'";
		}
		return semanticError(message, lineNumber, charIndex);
	}
	
	error alreadyDeclaredError(std::string_view name, size_t lineNumber, size_t charIndex) {
		std::string message = "'";
		message += name;
		message += "' is already declared";
		return semanticError(message, lineNumber, charIndex);
	}

	void formatError(const error& err, const get_character& source, std::ostream& output) {
		output << "(" << (err.lineNumber() + 1) << ") " << err.what() << std::endl;
		
		size_t charIndex = 0;
		
		for (size_t lineNumber = 0; lineNumber < err.lineNumber(); ++charIndex) {
			int c = source();
			if (c < 0) {
				return;
			} else if (c == '\n') {
				++lineNumber;
			}
		}

		size_t index_in_line = err.charIndex() - charIndex;
		
		std::string line;
		for (size_t idx = 0;; ++idx) {
			int c = source();
			if (c < 0 || c == '\n' || c == '\r') {
				break;
			}
			line += char(c == '\t' ? ' ' : c);
		}
		
		output << line << std::endl;
		
		for (size_t idx = 0; idx < index_in_line; ++idx) {
			output << " ";
		}
		
		output << "^" << std::endl;
	}
	
	runtimeError::runtimeError(std::string message) noexcept:
		_message(std::move(message))
	{
	}
		
	const char* runtimeError::what() const noexcept {
		return _message.c_str();
	}
	
	void runtimeAssertion(bool b, const char* message) {
		if (!b) {
			throw runtimeError(message);
		}
	}
	
	fileNotFound::fileNotFound(std::string message) noexcept:
		_message(std::move(message))
	{
	}
		
	const char* fileNotFound::what() const noexcept {
		return _message.c_str();
	}
}
