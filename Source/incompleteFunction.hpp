#ifndef incompleteFunction_hpp
#define incompleteFunction_hpp

#include "tokens.hpp"
#include "types.hpp"
#include <deque>
#include <functional>

namespace cobalt {
	class compilerContext;
	class runtimeContext;
	class tokensIterator;
	using function = std::function<void(runtimeContext&)>;

	struct functionDeclaration{
		std::string name;
		typeHandle typeID;
		std::vector<std::string> params;
	};
	
	functionDeclaration parseFunctionDeclaration(compilerContext& ctx, tokensIterator& it);

	class incompleteFunction {
	private:
		functionDeclaration _decl;
		std::deque<token> _tokens;
		size_t _index;
	public:
		incompleteFunction(compilerContext& ctx, tokensIterator& it);
		
		incompleteFunction(incompleteFunction&& orig) noexcept;
		
		const functionDeclaration& getDecl() const;
		
		function compile(compilerContext& ctx);
	};
}

#endif /* incompleteFunction_hpp */
