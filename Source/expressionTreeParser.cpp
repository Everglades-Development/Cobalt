#include "expressionTreeParser.hpp"
#include "expressionTree.hpp"
#include "tokeniser.hpp"
#include "errors.hpp"
#include <stack>

namespace cobalt {
	namespace {
		enum struct operator_precedence {
			brackets,
			postfix,
			prefix,
			multiplication,
			addition,
			shift,
			comparison,
			equality,
			bitwise_and,
			bitwise_xor,
			bitwise_or,
			logical_and,
			logical_or,
			assignment,
			comma
		};
		
		enum struct operator_associativity {
			left_to_right,
			right_to_left
		};
		
		struct operator_info {
			nodeOperation operation;
			operator_precedence precedence;
			operator_associativity associativity;
			int number_of_operands;
			size_t lineNumber;
			size_t charIndex;
			
			operator_info(nodeOperation operation, size_t lineNumber, size_t charIndex) :
				operation(operation),
				lineNumber(lineNumber),
				charIndex(charIndex)
			{
				switch (operation) {
					case nodeOperation::init:
						precedence = operator_precedence::brackets;
						break;
					case nodeOperation::param: // This will never happen. Used only for the node creation.
					case nodeOperation::postinc:
					case nodeOperation::postdec:
					case nodeOperation::index:
					case nodeOperation::call:
						precedence = operator_precedence::postfix;
						break;
					case nodeOperation::preinc:
					case nodeOperation::predec:
					case nodeOperation::positive:
					case nodeOperation::negative:
					case nodeOperation::bnot:
					case nodeOperation::lnot:
					case nodeOperation::size:
					case nodeOperation::tostring:
						precedence = operator_precedence::prefix;
						break;
					case nodeOperation::mul:
					case nodeOperation::div:
					case nodeOperation::idiv:
					case nodeOperation::mod:
						precedence = operator_precedence::multiplication;
						break;
					case nodeOperation::add:
					case nodeOperation::sub:
					case nodeOperation::concat:
						precedence = operator_precedence::addition;
						break;
					case nodeOperation::bsl:
					case nodeOperation::bsr:
						precedence = operator_precedence::shift;
						break;
					case nodeOperation::lt:
					case nodeOperation::gt:
					case nodeOperation::le:
					case nodeOperation::ge:
						precedence = operator_precedence::comparison;
						break;
					case nodeOperation::eq:
					case nodeOperation::ne:
						precedence = operator_precedence::equality;
						break;
					case nodeOperation::band:
						precedence = operator_precedence::bitwise_and;
						break;
					case nodeOperation::bxor:
						precedence = operator_precedence::bitwise_xor;
						break;
					case nodeOperation::bor:
						precedence = operator_precedence::bitwise_or;
						break;
					case nodeOperation::land:
						precedence = operator_precedence::logical_and;
						break;
					case nodeOperation::lor:
						precedence = operator_precedence::logical_or;
						break;
					case nodeOperation::assign:
					case nodeOperation::add_assign:
					case nodeOperation::sub_assign:
					case nodeOperation::mul_assign:
					case nodeOperation::div_assign:
					case nodeOperation::idiv_assign:
					case nodeOperation::mod_assign:
					case nodeOperation::band_assign:
					case nodeOperation::bor_assign:
					case nodeOperation::bxor_assign:
					case nodeOperation::bsl_assign:
					case nodeOperation::bsr_assign:
					case nodeOperation::concat_assign:
					case nodeOperation::ternary:
						precedence = operator_precedence::assignment;
						break;
					case nodeOperation::comma:
						precedence = operator_precedence::comma;
						break;
				}
				
				switch (precedence) {
					case operator_precedence::prefix:
					case operator_precedence::assignment:
						associativity = operator_associativity::right_to_left;
						break;
					default:
						associativity = operator_associativity::left_to_right;
						break;
				}
				
				switch (operation) {
					case nodeOperation::init:
						number_of_operands = 0; //zero or more
						break;
					case nodeOperation::postinc:
					case nodeOperation::postdec:
					case nodeOperation::preinc:
					case nodeOperation::predec:
					case nodeOperation::positive:
					case nodeOperation::negative:
					case nodeOperation::bnot:
					case nodeOperation::lnot:
					case nodeOperation::size:
					case nodeOperation::tostring:
					case nodeOperation::call: //at least one
						number_of_operands = 1;
						break;
					case nodeOperation::ternary:
						number_of_operands = 3;
						break;
					default:
						number_of_operands = 2;
						break;
				}
			}
		};

		operator_info get_operator_info(reservedToken token, bool prefix, size_t lineNumber, size_t charIndex) {
			switch(token) {
				case reservedToken::inc:
					return prefix ? operator_info(nodeOperation::preinc, lineNumber, charIndex)
					              : operator_info(nodeOperation::postinc, lineNumber, charIndex);
				case reservedToken::dec:
					return prefix ? operator_info(nodeOperation::predec, lineNumber, charIndex)
					              : operator_info(nodeOperation::postdec, lineNumber, charIndex);
				case reservedToken::add:
					return prefix ? operator_info(nodeOperation::positive, lineNumber, charIndex)
								  : operator_info(nodeOperation::add, lineNumber, charIndex);
				case reservedToken::sub:
					return prefix ? operator_info(nodeOperation::negative, lineNumber, charIndex)
					              : operator_info(nodeOperation::sub, lineNumber, charIndex);
				case reservedToken::concat:
					return operator_info(nodeOperation::concat, lineNumber, charIndex);
				case reservedToken::mul:
					return operator_info(nodeOperation::mul, lineNumber, charIndex);
				case reservedToken::div:
					return operator_info(nodeOperation::div, lineNumber, charIndex);
				case reservedToken::idiv:
					return operator_info(nodeOperation::idiv, lineNumber, charIndex);
				case reservedToken::mod:
					return operator_info(nodeOperation::mod, lineNumber, charIndex);
				case reservedToken::bitwise_not:
					return operator_info(nodeOperation::bnot, lineNumber, charIndex);
				case reservedToken::bitwise_and:
					return operator_info(nodeOperation::band, lineNumber, charIndex);
				case reservedToken::bitwise_or:
					return operator_info(nodeOperation::bor, lineNumber, charIndex);
				case reservedToken::bitwise_xor:
					return operator_info(nodeOperation::bxor, lineNumber, charIndex);
				case reservedToken::shiftl:
					return operator_info(nodeOperation::bsl, lineNumber, charIndex);
				case reservedToken::shiftr:
					return operator_info(nodeOperation::bsr, lineNumber, charIndex);
				case reservedToken::assign:
					return operator_info(nodeOperation::assign, lineNumber, charIndex);
				case reservedToken::add_assign:
					return operator_info(nodeOperation::add_assign, lineNumber, charIndex);
				case reservedToken::sub_assign:
					return operator_info(nodeOperation::sub_assign, lineNumber, charIndex);
				case reservedToken::concat_assign:
					return operator_info(nodeOperation::concat_assign, lineNumber, charIndex);
				case reservedToken::mul_assign:
					return operator_info(nodeOperation::mod_assign, lineNumber, charIndex);
				case reservedToken::div_assign:
					return operator_info(nodeOperation::div_assign, lineNumber, charIndex);
				case reservedToken::idiv_assign:
					return operator_info(nodeOperation::idiv_assign, lineNumber, charIndex);
				case reservedToken::mod_assign:
					return operator_info(nodeOperation::mod_assign, lineNumber, charIndex);
				case reservedToken::and_assign:
					return operator_info(nodeOperation::band_assign, lineNumber, charIndex);
				case reservedToken::or_assign:
					return operator_info(nodeOperation::bor_assign, lineNumber, charIndex);
				case reservedToken::xor_assign:
					return operator_info(nodeOperation::bxor_assign, lineNumber, charIndex);
				case reservedToken::shiftl_assign:
					return operator_info(nodeOperation::bsl_assign, lineNumber, charIndex);
				case reservedToken::shiftr_assign:
					return operator_info(nodeOperation::bsr_assign, lineNumber, charIndex);
				case reservedToken::logical_not:
					return operator_info(nodeOperation::lnot, lineNumber, charIndex);
				case reservedToken::logical_and:
					return operator_info(nodeOperation::land, lineNumber, charIndex);
				case reservedToken::logical_or:
					return operator_info(nodeOperation::lor, lineNumber, charIndex);
				case reservedToken::eq:
					return operator_info(nodeOperation::eq, lineNumber, charIndex);
				case reservedToken::ne:
					return operator_info(nodeOperation::ne, lineNumber, charIndex);
				case reservedToken::lt:
					return operator_info(nodeOperation::lt, lineNumber, charIndex);
				case reservedToken::gt:
					return operator_info(nodeOperation::gt, lineNumber, charIndex);
				case reservedToken::le:
					return operator_info(nodeOperation::le, lineNumber, charIndex);
				case reservedToken::ge:
					return operator_info(nodeOperation::ge, lineNumber, charIndex);
				case reservedToken::question:
					return operator_info(nodeOperation::ternary, lineNumber, charIndex);
				case reservedToken::comma:
					return operator_info(nodeOperation::comma, lineNumber, charIndex);
				case reservedToken::open_round:
					return operator_info(nodeOperation::call, lineNumber, charIndex);
				case reservedToken::open_square:
					return operator_info(nodeOperation::index, lineNumber, charIndex);
				case reservedToken::kw_sizeof:
					return operator_info(nodeOperation::size, lineNumber, charIndex);
				case reservedToken::kw_tostring:
					return operator_info(nodeOperation::tostring, lineNumber, charIndex);
				case reservedToken::open_curly:
					return operator_info(nodeOperation::init, lineNumber, charIndex);
				default:
					throw unexpectedSyntaxError(std::to_string(token), lineNumber, charIndex);
			}
		}

		bool is_end_of_expression(const token& t, bool allow_comma) {
			if (t.isEof()) {
				return true;
			}

			if (t.isReservedToken()) {
				switch (t.getReservedToken()) {
					case reservedToken::semicolon:
					case reservedToken::close_round:
					case reservedToken::close_square:
					case reservedToken::close_curly:
					case reservedToken::colon:
						return true;
					case reservedToken::comma:
						return !allow_comma;
					default:
						return false;
				}
			}
			
			return false;
		}
		
		bool is_evaluated_before(const operator_info& l, const operator_info& r) {
			return l.associativity == operator_associativity::left_to_right ? l.precedence <= r.precedence : l.precedence < r.precedence;
		}
		
		void pop_one_operator(
			std::stack<operator_info>& operator_stack, std::stack<node_ptr>& operand_stack,
			compilerContext& context, size_t lineNumber, size_t charIndex
		) {
			if (operand_stack.size() < operator_stack.top().number_of_operands) {
				throw compilerError("Failed to parse an expression", lineNumber, charIndex);
			}
			
			std::vector<node_ptr> operands;
			operands.resize(operator_stack.top().number_of_operands);
			
			if (operator_stack.top().precedence != operator_precedence::prefix) {
				operator_stack.top().lineNumber = operand_stack.top()->getLineNumber();
				operator_stack.top().charIndex = operand_stack.top()->getCharIndex();
			}
			
			for (int i = operator_stack.top().number_of_operands - 1; i >= 0; --i) {
				operands[i] = std::move(operand_stack.top());
				operand_stack.pop();
			}
			
			operand_stack.push(std::make_unique<node>(
				context, operator_stack.top().operation, std::move(operands), operator_stack.top().lineNumber, operator_stack.top().charIndex)
			);
			
			operator_stack.pop();
		}
		
		node_ptr parse_expression_tree_impl(compilerContext& context, tokensIterator& it, bool allow_comma, bool allow_empty) {
			std::stack<node_ptr> operand_stack;
			std::stack<operator_info> operator_stack;
			
			bool expected_operand = true;
			
			for (; !is_end_of_expression(*it, allow_comma); ++it) {
				if (it->isReservedToken()) {
					operator_info oi = get_operator_info(
						it->getReservedToken(), expected_operand, it->getLineNumber(), it->getCharIndex()
					);
					
					if (oi.operation == nodeOperation::call && expected_operand) {
						//open round bracket is misinterpreted as a function call
						++it;
						operand_stack.push(parse_expression_tree_impl(context, it, true, false));
						if (it->hasValue(reservedToken::close_round)) {
							expected_operand = false;
							continue;
						} else {
							throw syntaxError("Expected closing ')'", it->getLineNumber(), it->getCharIndex());
						}
					}
					
					if (oi.operation == nodeOperation::init && expected_operand) {
						++it;
						std::vector<node_ptr> children;
						if (!it->hasValue(reservedToken::close_curly)) {
							while (true) {
								children.push_back(parse_expression_tree_impl(context, it, false, false));
								if (it->hasValue(reservedToken::close_curly)) {
									break;
								} else if (it->hasValue(reservedToken::comma)) {
									++it;
								} else {
									throw syntaxError("Expected ',', or closing '}'", it->getLineNumber(), it->getCharIndex());
								}
							}
						}
						operand_stack.push(std::make_unique<node>(
							context,
							nodeOperation::init,
							std::move(children),
							it->getLineNumber(),
							it->getCharIndex()
						));
						
						expected_operand = false;
						continue;
					}
					
					if ((oi.precedence == operator_precedence::prefix) != expected_operand) {
						throw unexpectedSyntaxError(
							std::to_string(it->getValue()),
							it->getLineNumber(),
							it->getCharIndex()
						);
					}
					
					if (!operator_stack.empty() && is_evaluated_before(operator_stack.top(), oi)) {
						pop_one_operator(operator_stack, operand_stack, context, it->getLineNumber(), it->getCharIndex());
					}
					
					switch (oi.operation) {
						case nodeOperation::call:
							++it;
							if (!it->hasValue(reservedToken::close_round)) {
								while (true) {
									bool remove_lvalue = !it->hasValue(reservedToken::bitwise_and);
									if (!remove_lvalue) {
										++it;
									}
									node_ptr argument = parse_expression_tree_impl(context, it, false, false);
									if (remove_lvalue) {
										size_t lineNumber = argument->getLineNumber();
										size_t charIndex = argument->getCharIndex();
										std::vector<node_ptr> argument_vector;
										argument_vector.push_back(std::move(argument));
										argument = std::make_unique<node>(
											context,
											nodeOperation::param,
											std::move(argument_vector),
											lineNumber,
											charIndex
										);
									} else if (!argument->is_lvalue()) {
										throw wrongTypeError(
											std::to_string(argument->getTypeID()),
											std::to_string(argument->getTypeID()),
											true,
											argument->getLineNumber(),
											argument->getCharIndex()
										);
									}
									
									operand_stack.push(std::move(argument));

									++oi.number_of_operands;
									
									if (it->hasValue(reservedToken::close_round)) {
										break;
									} else if (it->hasValue(reservedToken::comma)) {
										++it;
									} else {
										throw syntaxError("Expected ',', or closing ')'", it->getLineNumber(), it->getCharIndex());
									}
								}
							}
							break;
						case nodeOperation::index:
							++it;
							operand_stack.push(parse_expression_tree_impl(context, it, true, false));
							if (!it->hasValue(reservedToken::close_square)) {
								throw syntaxError("Expected closing ]'", it->getLineNumber(), it->getCharIndex());
							}
							break;
						case nodeOperation::ternary:
							++it;
							operand_stack.push(parse_expression_tree_impl(context, it, true, false));
							if (!it->hasValue(reservedToken::colon)) {
								throw syntaxError("Expected ':'", it->getLineNumber(), it->getCharIndex());
							}
							break;
						default:
							break;
					}
					
					operator_stack.push(oi);
					
					expected_operand = (oi.precedence != operator_precedence::postfix);
				} else {
					if (!expected_operand) {
						throw unexpectedSyntaxError(
							std::to_string(it->getValue()),
							it->getLineNumber(),
							it->getCharIndex()
						);
					}
					if (it->isNumber()) {
						operand_stack.push(std::make_unique<node>(
							context, it->getNumber(), std::vector<node_ptr>(), it->getLineNumber(), it->getCharIndex())
						);
					} else if (it->isString()) {
						operand_stack.push(std::make_unique<node>(
							context, it->getString(), std::vector<node_ptr>(), it->getLineNumber(), it->getCharIndex())
						);
					} else {
						operand_stack.push(std::make_unique<node>(
							context, it->getIdentifier(), std::vector<node_ptr>(), it->getLineNumber(), it->getCharIndex())
						);
					}
					expected_operand = false;
				}
			}
			
			if (expected_operand) {
				if (allow_empty && operand_stack.empty() && operator_stack.empty()) {
					return node_ptr();
				} else {
					throw syntaxError("Operand expected", it->getLineNumber(), it->getCharIndex());
				}
			}
			
			while(!operator_stack.empty()) {
				pop_one_operator(operator_stack, operand_stack, context, it->getLineNumber(), it->getCharIndex());
			}
			
			if (operand_stack.size() != 1 || !operator_stack.empty()) {
				throw compilerError("Failed to parse an expression", it->getLineNumber(), it->getCharIndex());
			}
			
			return std::move(operand_stack.top());
		}
	}
	
	node_ptr parseExpressionTree(
		compilerContext& context, tokensIterator& it, typeHandle typeID, bool allow_comma
	) {
		node_ptr ret = parse_expression_tree_impl(context, it, allow_comma, typeID == typeRegistry::getVoidHandle());
		if (ret) {
			ret->checkConversion(typeID, false);
		}
		return ret;
	}
}
