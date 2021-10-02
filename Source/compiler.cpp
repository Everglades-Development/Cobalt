#include "compiler.hpp"
#include "errors.hpp"
#include "compilerContext.hpp"
#include "expression.hpp"
#include "incompleteFunction.hpp"
#include "tokeniser.hpp"
#include "runtimeContext.hpp"
#include "helpers.hpp"
#include "pushBackStream.hpp"

namespace cobalt {
	namespace {
		struct possible_flow {
			size_t breakLevel;
			bool can_continue;
			typeHandle return_type_id;
			
			possible_flow add_switch() {
				return possible_flow{breakLevel+1, can_continue, return_type_id};
			}
			
			possible_flow add_loop() {
				return possible_flow{breakLevel+1, true, return_type_id};
			}
			
			static possible_flow in_function(typeHandle return_type_id) {
				return possible_flow{0, false, return_type_id};
			}
		};
	
		bool is_typename(const compilerContext&, const tokensIterator& it) {
			return std::visit(overloaded{
				[](reservedToken t) {
					switch (t) {
						case reservedToken::kw_number:
						case reservedToken::kw_string:
						case reservedToken::kw_void:
						case reservedToken::open_square:
							return true;
						default:
							return false;
					}
				},
				[](const tokenValue&) {
					return false;
				}
			}, it->getValue());
		}
	
		error unexpected_syntax(const tokensIterator& it) {
			return unexpectedSyntaxError(std::to_string(it->getValue()), it->getLineNumber(), it->getCharIndex());
		}
		
		std::vector<expression<lvalue>::ptr> compile_variable_declaration(compilerContext& ctx, tokensIterator& it) {
			typeHandle typeID = parseType(ctx, it);
		
			if (typeID == typeRegistry::getVoidHandle()) {
				throw syntaxError("Cannot declare void variable", it->getLineNumber(), it->getCharIndex());
			}
			
			std::vector<expression<lvalue>::ptr> ret;
			
			do {
				if (!ret.empty()) {
					++it;
				}
			
				std::string name = parseDeclarationName(ctx, it);
			
				if (it->hasValue(reservedToken::open_round)) {
					++it;
					ret.emplace_back(build_initialisation_expression(ctx, it, typeID, false));
					parseTokenValue(ctx, it, reservedToken::close_round);
				} else if (it->hasValue(reservedToken::assign)) {
					++it;
					ret.emplace_back(build_initialisation_expression(ctx, it, typeID, false));
				} else {
					ret.emplace_back(build_default_initialization(typeID));
				}
				
				ctx.createIdentifier(std::move(name), typeID);
			} while (it->hasValue(reservedToken::comma));
			
			return ret;
		}
		
		statement_ptr compile_simple_statement(compilerContext& ctx, tokensIterator& it);
		
		statement_ptr compile_block_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf);
		
		statement_ptr compile_for_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf);
		
		statement_ptr compile_while_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf);
		
		statement_ptr compile_do_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf);
		
		statement_ptr compile_if_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf);
		
		statement_ptr compile_switch_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf);
		
		statement_ptr compile_var_statement(compilerContext& ctx, tokensIterator& it);
		
		statement_ptr compile_break_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf);
		
		statement_ptr compile_continue_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf);
		
		statement_ptr compile_return_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf);
		
		statement_ptr compile_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf, bool in_switch) {
			if (it->isReservedToken()) {
				switch (it->getReservedToken()) {
					case reservedToken::kw_for:
						return compile_for_statement(ctx, it, pf.add_loop());
					case reservedToken::kw_while:
						return compile_while_statement(ctx, it, pf.add_loop());
					case reservedToken::kw_do:
						return compile_do_statement(ctx, it, pf.add_loop());
					case reservedToken::kw_if:
						return compile_if_statement(ctx, it, pf);
					case reservedToken::kw_switch:
						return compile_switch_statement(ctx, it, pf.add_switch());
					case reservedToken::kw_break:
						return compile_break_statement(ctx, it, pf);
					case reservedToken::kw_continue:
						return compile_continue_statement(ctx, it, pf);
					case reservedToken::kw_return:
						return compile_return_statement(ctx, it, pf);
					default:
						break;
				}
			}
			
			if (is_typename(ctx, it)) {
				if (in_switch) {
					throw syntaxError("Declarations in switch block are not allowed", it->getLineNumber(), it->getCharIndex());
				} else {
					return compile_var_statement(ctx, it);
				}
			}
			
			if (it->hasValue(reservedToken::open_curly)) {
				return compile_block_statement(ctx, it, pf);
			}
			
			return compile_simple_statement(ctx, it);
		}
		
		statement_ptr compile_simple_statement(compilerContext& ctx, tokensIterator& it) {
			statement_ptr ret = createSimpleStatement(build_void_expression(ctx, it));
			parseTokenValue(ctx, it, reservedToken::semicolon);
			return ret;
		}
		
		statement_ptr compile_for_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf) {
			auto _ = ctx.scope();
		
			parseTokenValue(ctx, it, reservedToken::kw_for);
			parseTokenValue(ctx, it, reservedToken::open_round);
			
			std::vector<expression<lvalue>::ptr> decls;
			expression<void>::ptr expr1;
			
			if (is_typename(ctx, it)) {
				decls = compile_variable_declaration(ctx, it);
			} else {
				expr1 = build_void_expression(ctx, it);
			}
		
			parseTokenValue(ctx, it, reservedToken::semicolon);
			
			expression<number>::ptr expr2 = build_number_expression(ctx, it);
			parseTokenValue(ctx, it, reservedToken::semicolon);
			
			expression<void>::ptr expr3 = build_void_expression(ctx, it);
			parseTokenValue(ctx, it, reservedToken::close_round);
			
			statement_ptr block = compile_block_statement(ctx, it, pf);
			
			if (!decls.empty()) {
				return createForStatement(std::move(decls), std::move(expr2), std::move(expr3), std::move(block));
			} else {
				return createForStatement(std::move(expr1), std::move(expr2), std::move(expr3), std::move(block));
			}
		}
		
		statement_ptr compile_while_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf) {
			parseTokenValue(ctx, it, reservedToken::kw_while);

			parseTokenValue(ctx, it, reservedToken::open_round);
			expression<number>::ptr expr = build_number_expression(ctx, it);
			parseTokenValue(ctx, it, reservedToken::close_round);
			
			statement_ptr block = compile_block_statement(ctx, it, pf);
			
			return createWhileStatement(std::move(expr), std::move(block));
		}
		
		statement_ptr compile_do_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf) {
			parseTokenValue(ctx, it, reservedToken::kw_do);
			
			statement_ptr block = compile_block_statement(ctx, it, pf);
			
			parseTokenValue(ctx, it, reservedToken::kw_while);
			
			parseTokenValue(ctx, it, reservedToken::open_round);
			expression<number>::ptr expr = build_number_expression(ctx, it);
			parseTokenValue(ctx, it, reservedToken::close_round);
			
			return createDoStatement(std::move(expr), std::move(block));
		}
		
		statement_ptr compile_if_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf) {
			auto _ = ctx.scope();
			parseTokenValue(ctx, it, reservedToken::kw_if);
			
			parseTokenValue(ctx, it, reservedToken::open_round);
			
			std::vector<expression<lvalue>::ptr> decls;
			
			if (is_typename(ctx, it)) {
				decls = compile_variable_declaration(ctx, it);
				parseTokenValue(ctx, it, reservedToken::semicolon);
			}
			
			std::vector<expression<number>::ptr> exprs;
			std::vector<statement_ptr> stmts;
			
			exprs.emplace_back(build_number_expression(ctx, it));
			parseTokenValue(ctx, it, reservedToken::close_round);
			stmts.emplace_back(compile_block_statement(ctx, it, pf));
			
			while (it->hasValue(reservedToken::kw_elif)) {
				++it;
				parseTokenValue(ctx, it, reservedToken::open_round);
				exprs.emplace_back(build_number_expression(ctx, it));
				parseTokenValue(ctx, it, reservedToken::close_round);
				stmts.emplace_back(compile_block_statement(ctx, it, pf));
			}
			
			if (it->hasValue(reservedToken::kw_else)) {
				++it;
				stmts.emplace_back(compile_block_statement(ctx, it, pf));
			} else {
				stmts.emplace_back(createBlockStatement({}));
			}
			
			return createIfStatement(std::move(decls), std::move(exprs), std::move(stmts));
		}
		
		 statement_ptr compile_switch_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf) {
		 	auto _ = ctx.scope();
			parseTokenValue(ctx, it, reservedToken::kw_switch);
			
			parseTokenValue(ctx, it, reservedToken::open_round);
			
			std::vector<expression<lvalue>::ptr> decls;
			
			if (is_typename(ctx, it)) {
				decls = compile_variable_declaration(ctx, it);
				parseTokenValue(ctx, it, reservedToken::semicolon);
			}
			
			expression<number>::ptr expr = build_number_expression(ctx, it);
			parseTokenValue(ctx, it, reservedToken::close_round);
			
			std::vector<statement_ptr> stmts;
			std::unordered_map<number, size_t> cases;
			size_t dflt = size_t(-1);
			
			parseTokenValue(ctx, it, reservedToken::open_curly);
			
			while (!it->hasValue(reservedToken::close_curly)) {
				if (it->hasValue(reservedToken::kw_case)) {
					++it;
					if (!it->isNumber()) {
						throw unexpected_syntax(it);
					}
					cases.emplace(it->getNumber(), stmts.size());
					++it;
					parseTokenValue(ctx, it, reservedToken::colon);
				} else if (it->hasValue(reservedToken::kw_default)) {
					++it;
					dflt = stmts.size();
					parseTokenValue(ctx, it, reservedToken::colon);
				} else {
					stmts.emplace_back(compile_statement(ctx, it, pf, true));
				}
			}
			
			++it;
			
			if (dflt == size_t(-1)) {
				dflt = stmts.size();
			}
			
			return createSwitchStatement(std::move(decls), std::move(expr), std::move(stmts), std::move(cases), dflt);
		}
	
		statement_ptr compile_var_statement(compilerContext& ctx, tokensIterator& it) {
			std::vector<expression<lvalue>::ptr> decls = compile_variable_declaration(ctx, it);
			parseTokenValue(ctx, it, reservedToken::semicolon);
			return createLocalDeclarationState(std::move(decls));
		}
		
		statement_ptr compile_break_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf) {
			if (pf.breakLevel == 0) {
				throw unexpected_syntax(it);
			}
			
			parseTokenValue(ctx, it, reservedToken::kw_break);
			
			double breakLevel;
			
			if (it->isNumber()) {
				breakLevel = it->getNumber();
			
				if (breakLevel < 1 || breakLevel != int(breakLevel) || breakLevel > pf.breakLevel) {
					throw syntaxError("Invalid break value", it->getLineNumber(), it->getCharIndex());
				}
				
				++it;
			} else {
				breakLevel = 1;
			}
			
			
			parseTokenValue(ctx, it, reservedToken::semicolon);
			
			return createBreakStatement(int(breakLevel));
		}
		
		statement_ptr compile_continue_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf){
			if (!pf.can_continue) {
				throw unexpected_syntax(it);
			}
			parseTokenValue(ctx, it, reservedToken::kw_continue);
			parseTokenValue(ctx, it, reservedToken::semicolon);
			return createContinueStatement();
		}
		
		statement_ptr compile_return_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf){
			parseTokenValue(ctx, it, reservedToken::kw_return);
			
			if (pf.return_type_id == typeRegistry::getVoidHandle()) {
				parseTokenValue(ctx, it, reservedToken::semicolon);
				return createReturnVoidStatement();
			} else {
				expression<lvalue>::ptr expr = build_initialisation_expression(ctx, it, pf.return_type_id, true);
				parseTokenValue(ctx, it, reservedToken::semicolon);
				return createReturnStatement(std::move(expr));
			}
		}
		
		
		std::vector<statement_ptr> compile_block_contents(compilerContext& ctx, tokensIterator& it, possible_flow pf) {
			std::vector<statement_ptr> ret;
			
			if (it->hasValue(reservedToken::open_curly)) {
				parseTokenValue(ctx, it, reservedToken::open_curly);
				
				while (!it->hasValue(reservedToken::close_curly)) {
					ret.push_back(compile_statement(ctx, it, pf, false));
				}
				
				parseTokenValue(ctx, it, reservedToken::close_curly);
			} else {
				ret.push_back(compile_statement(ctx, it, pf, false));
			}
			
			return ret;
		}
		
		statement_ptr compile_block_statement(compilerContext& ctx, tokensIterator& it, possible_flow pf) {
			auto _ = ctx.scope();
			std::vector<statement_ptr> block = compile_block_contents(ctx, it, pf);
			return createBlockStatement(std::move(block));
		}
	}

	void parseTokenValue(compilerContext&, tokensIterator& it, const tokenValue& value) {
		if (it->hasValue(value)) {
			++it;
			return;
		}
		throw expectedSyntaxError(std::to_string(value), it->getLineNumber(), it->getCharIndex());
	}
	
	std::string parseDeclarationName(compilerContext& ctx, tokensIterator& it) {
		if (!it->isIdentifier()) {
			throw unexpected_syntax(it);
		}

		std::string ret = it->getIdentifier().name;
		
		if (!ctx.canDeclare(ret)) {
			throw alreadyDeclaredError(ret, it->getLineNumber(), it->getCharIndex());
		}

		++it;
		
		return ret;
	}

	typeHandle parseType(compilerContext& ctx, tokensIterator& it) {
		if (!it->isReservedToken()) {
			throw unexpected_syntax(it);
		}
		
		typeHandle t = nullptr;
		
		switch (it->getReservedToken()) {
			case reservedToken::kw_void:
				t = ctx.getHandle(simpleType::nothing);
				++it;
				break;
			case reservedToken::kw_number:
				t = ctx.getHandle(simpleType::number);
				++it;
				break;
			case reservedToken::kw_string:
				t = ctx.getHandle(simpleType::string);
				++it;
				break;
			case reservedToken::open_square:
				{
					tupleType tt;
					++it;
					while (!it->hasValue(reservedToken::close_square)) {
						if (!tt.inner_type_id.empty()) {
							parseTokenValue(ctx, it, reservedToken::comma);
						}
						tt.inner_type_id.push_back(parseType(ctx, it));
					}
					++it;
					t = ctx.getHandle(std::move(tt));
				}
				break;
			default:
				throw unexpected_syntax(it);
		}
		
		while (it->isReservedToken()) {
			switch (it->getReservedToken()) {
				case reservedToken::open_square:
					parseTokenValue(ctx, ++it, reservedToken::close_square);
					t = ctx.getHandle(arrayType{t});
					break;
				case reservedToken::open_round:
					{
						functionType ft;
						ft.return_type_id = t;
						++it;
						while (!it->hasValue(reservedToken::close_round)) {
							if (!ft.param_type_id.empty()) {
								parseTokenValue(ctx, it, reservedToken::comma);
							}
							typeHandle param_type = parseType(ctx, it);
							if (it->hasValue(reservedToken::bitwise_and)) {
								ft.param_type_id.push_back({param_type, true});
								++it;
							} else {
								ft.param_type_id.push_back({param_type, false});
							}
						}
						++it;
						t = ctx.getHandle(ft);
					}
					break;
				default:
					return t;
			}
		}
		
		return t;
	}
	
	shared_statement_ptr compileFunctionBlock(compilerContext& ctx, tokensIterator& it, typeHandle return_type_id) {
		std::vector<statement_ptr> block = compile_block_contents(ctx, it, possible_flow::in_function(return_type_id));
		if (return_type_id != typeRegistry::getVoidHandle()) {
			block.emplace_back(createReturnStatement(build_default_initialization(return_type_id)));
		}
		return createSharedBlockStatement(std::move(block));
	}
	
	runtimeContext compile(
		tokensIterator& it,
		const std::vector<std::pair<std::string, function> >& external_functions,
		std::vector<std::string> public_declarations
	) {
		compilerContext ctx;
		
		for (const std::pair<std::string, function>& p : external_functions) {
			get_character get = [i = 0, &p]() mutable {
				if (i < p.first.size()){
					return int(p.first[i++]);
				} else {
					return -1;
				}
			};
			
			push_back_stream stream(&get);
			
			tokensIterator function_it(stream);
		
			functionDeclaration decl = parseFunctionDeclaration(ctx, function_it);
			
			ctx.createFunction(decl.name, decl.typeID);
		}
		
		std::unordered_map<std::string, typeHandle> public_function_types;
		
		for (const std::string& f : public_declarations) {
			get_character get = [i = 0, &f]() mutable {
				if (i < f.size()){
					return int(f[i++]);
				} else {
					return -1;
				}
			};
			
			push_back_stream stream(&get);
			
			tokensIterator function_it(stream);
		
			functionDeclaration decl = parseFunctionDeclaration(ctx, function_it);
			
			public_function_types.emplace(decl.name, decl.typeID);
		}

		std::vector<expression<lvalue>::ptr> initializers;
		
		std::vector<incompleteFunction> incomplete_functions;
		std::unordered_map<std::string, size_t> public_functions;
		
		while (it) {
			if (!std::holds_alternative<reservedToken>(it->getValue())) {
				throw unexpected_syntax(it);
			}
		
			bool public_function = false;
			
			switch (it->getReservedToken()) {
				case reservedToken::kw_public:
					public_function = true;
					if (!(++it)->hasValue(reservedToken::kw_function)) {
						throw unexpected_syntax(it);
					}
				case reservedToken::kw_function:
					{
						size_t lineNumber = it->getLineNumber();
						size_t charIndex = it->getCharIndex();
						const incompleteFunction& f = incomplete_functions.emplace_back(ctx, it);
						
						if (public_function) {
							auto it = public_function_types.find(f.getDecl().name);
						
							if (it != public_function_types.end() && it->second != f.getDecl().typeID) {
								throw semanticError(
									"Public function doesn't match it's declaration " + std::to_string(it->second),
									lineNumber,
									charIndex
								);
							} else {
								public_function_types.erase(it);
							}
						
							public_functions.emplace(
								f.getDecl().name,
								external_functions.size() + incomplete_functions.size() - 1
							);
						}
						break;
					}
				default:
					for (expression<lvalue>::ptr& expr : compile_variable_declaration(ctx, it)) {
						initializers.push_back(std::move(expr));
					}
					parseTokenValue(ctx, it, reservedToken::semicolon);
					break;
			}
		}
		
		if (!public_function_types.empty()) {
			throw semanticError(
				"Public function '" + public_function_types.begin()->first + "' is not defined.",
				it->getLineNumber(),
				it->getCharIndex()
			);
		}
		
		std::vector<function> functions;
		
		functions.reserve(external_functions.size() + incomplete_functions.size());
		
		for (const std::pair<std::string, function>& p : external_functions) {
			functions.emplace_back(p.second);
		}
		
		for (incompleteFunction& f : incomplete_functions) {
			functions.emplace_back(f.compile(ctx));
		}
		
		return runtimeContext(std::move(initializers), std::move(functions), std::move(public_functions));
	}
}
