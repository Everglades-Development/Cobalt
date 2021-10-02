#ifndef expression_hpp
#define expression_hpp

#include "variable.hpp"
#include "types.hpp"

#include <string>

namespace cobalt {
	class runtimeContext;
	class tokensIterator;
	class compilerContext;

	template <typename R>
	class expression {
		expression(const expression&) = delete;
		void operator=(const expression&) = delete;
	protected:
		expression() = default;
	public:
		using ptr = std::unique_ptr<const expression>;
		
		virtual R evaluate(runtimeContext& context) const = 0;
		virtual ~expression() = default;
	};
	
	expression<void>::ptr build_void_expression(compilerContext& context, tokensIterator& it);
	expression<number>::ptr build_number_expression(compilerContext& context, tokensIterator& it);
	expression<lvalue>::ptr build_initialisation_expression(
		compilerContext& context,
		tokensIterator& it,
		typeHandle typeID,
		bool allow_comma
	);
	expression<lvalue>::ptr build_default_initialization(typeHandle typeID);
}

#endif /* expression_hpp */
