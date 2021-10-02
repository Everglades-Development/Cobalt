#ifndef expressionTreeParser_hpp
#define expressionTreeParser_hpp

#include <memory>

#include "types.hpp"

namespace cobalt {
	struct node;
	using node_ptr=std::unique_ptr<node>;

	class tokensIterator;

	class compilerContext;

	node_ptr parseExpressionTree(compilerContext& context, tokensIterator& it, typeHandle typeID, bool allow_comma);
}

#endif /* expressionTreeParser_hpp */
