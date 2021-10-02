#include "incompleteFunction.hpp"
#include "compiler.hpp"
#include "compilerContext.hpp"
#include "errors.hpp"
#include "tokeniser.hpp"

namespace cobalt {
	functionDeclaration parseFunctionDeclaration(compilerContext& ctx, tokensIterator& it) {
		functionDeclaration ret;
		
		parseTokenValue(ctx, it, reservedToken::kw_function);
		
		functionType ft;
		ft.return_type_id = parseType(ctx, it);
		ret.name = parseDeclarationName(ctx, it);
		
		{
			auto _ = ctx.function();
			
			parseTokenValue(ctx, it, reservedToken::open_round);
			
			while(!it->hasValue(reservedToken::close_round)) {
				if (!ret.params.empty()) {
					parseTokenValue(ctx, it, reservedToken::comma);
				}
				
				typeHandle t = parseType(ctx, it);
				bool byref = false;
				if (it->hasValue(reservedToken::bitwise_and)) {
					byref = true;
					++it;
				}
				ft.param_type_id.push_back({t, byref});
				
				if (!it->hasValue(reservedToken::close_round) && !it->hasValue(reservedToken::comma)) {
					ret.params.push_back(parseDeclarationName(ctx, it));
				} else {
					ret.params.push_back("@"+std::to_string(ret.params.size()));
				}
			}
			++it;
		}
		
		ret.typeID = ctx.getHandle(ft);
		
		return ret;
	}

	incompleteFunction::incompleteFunction(compilerContext& ctx, tokensIterator& it) {
		_decl = parseFunctionDeclaration(ctx, it);
		
		_tokens.push_back(*it);
		
		parseTokenValue(ctx, it, reservedToken::open_curly);
		
		int nesting = 1;
		
		while (nesting && !it->isEof()) {
			if (it->hasValue(reservedToken::open_curly)) {
				++nesting;
			}
			
			if (it->hasValue(reservedToken::close_curly)) {
				--nesting;
			}
			
			_tokens.push_back(*it);
			++it;
		}
		
		if (nesting) {
			throw unexpectedSyntaxError("end of file", it->getLineNumber(), it->getCharIndex());
		}
		
		ctx.createFunction(_decl.name, _decl.typeID);
	}
	
	incompleteFunction::incompleteFunction(incompleteFunction&& orig) noexcept:
		_tokens(std::move(orig._tokens)),
		_decl(std::move(orig._decl))
	{
	}
	
	const functionDeclaration& incompleteFunction::getDecl() const {
		return _decl;
	}
	
	function incompleteFunction::compile(compilerContext& ctx) {
		auto _ = ctx.function();
		
		const functionType* ft = std::get_if<functionType>(_decl.typeID);
		
		for (int i = 0; i < int(_decl.params.size()); ++i) {
			ctx.createParam(std::move(_decl.params[i]), ft->param_type_id[i].typeID);
		}
		
		tokensIterator it(_tokens);
		
		shared_statement_ptr stmt = compileFunctionBlock(ctx, it, ft->return_type_id);
		
		return [stmt=std::move(stmt)] (runtimeContext& ctx) {
			stmt->execute(ctx);
		};
	}
}
