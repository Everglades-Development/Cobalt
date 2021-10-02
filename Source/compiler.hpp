#ifndef compiler_hpp
#define compiler_hpp

#include "types.hpp"
#include "tokens.hpp"
#include "statement.hpp"

#include <vector>
#include <functional>

namespace cobalt {
	class compilerContext;
	class tokensIterator;
	class runtimeContext;
	
	using function = std::function<void(runtimeContext&)>;

	runtimeContext compile(
		tokensIterator& it,
		const std::vector<std::pair<std::string, function> >& external_functions,
		std::vector<std::string> public_declarations
	);
	
	typeHandle parseType(compilerContext& ctx, tokensIterator& it);

	std::string parseDeclarationName(compilerContext& ctx, tokensIterator& it);
	
	void parseTokenValue(compilerContext& ctx, tokensIterator& it, const tokenValue& value);
	
	shared_statement_ptr compileFunctionBlock(compilerContext& ctx, tokensIterator& it, typeHandle return_type_id);
}

#endif /* compiler_hpp */
