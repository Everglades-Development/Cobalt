#ifndef statement_hpp
#define statement_hpp
#include <memory>
#include <vector>
#include <unordered_map>
#include "expression.hpp"

namespace cobalt {
	enum struct flow_type{
		f_normal,
		f_break,
		f_continue,
		f_return,
	};
	
	class flow {
	private:
		flow_type _type;
		int _break_level;
		flow(flow_type type, int breakLevel);
	public:
		flow_type type() const;
		int breakLevel() const;
		
		static flow normalFlow();
		static flow breakFlow(int breakLevel);
		static flow continueFlow();
		static flow returnFlow();
		flow consumeBreak();
	};
	
	class runtimeContext;
	
	class statement {
		statement(const statement&) = delete;
		void operator=(const statement&) = delete;
	protected:
		statement() = default;
	public:
		virtual flow execute(runtimeContext& context) = 0;
		virtual ~statement() = default;
	};
	
	using statement_ptr = std::unique_ptr<statement>;
	using shared_statement_ptr = std::shared_ptr<statement>;
	
	statement_ptr createSimpleStatement(expression<void>::ptr expr);
	
	statement_ptr createLocalDeclarationState(std::vector<expression<lvalue>::ptr> decls);
	
	statement_ptr createBlockStatement(std::vector<statement_ptr> statements);
	shared_statement_ptr createSharedBlockStatement(std::vector<statement_ptr> statements);
	
	statement_ptr createBreakStatement(int breakLevel);
	
	statement_ptr createContinueStatement();
	
	statement_ptr createReturnStatement(expression<lvalue>::ptr expr);
	
	statement_ptr createReturnVoidStatement();
	
	statement_ptr createIfStatement(
		std::vector<expression<lvalue>::ptr> decls,
		std::vector<expression<number>::ptr> exprs,
		std::vector<statement_ptr> statements
	);
	
	statement_ptr createSwitchStatement(
		std::vector<expression<lvalue>::ptr> decls,
		expression<number>::ptr expr,
		std::vector<statement_ptr> statements,
		std::unordered_map<number, size_t> cases,
		size_t dflt
	);
	
	
	statement_ptr createWhileStatement(expression<number>::ptr expr, statement_ptr statement);
	
	statement_ptr createDoStatement(expression<number>::ptr expr, statement_ptr statement);
	
	statement_ptr createForStatement(
		expression<void>::ptr expr1,
		expression<number>::ptr expr2,
		expression<void>::ptr expr3,
		statement_ptr statement
	);
	
	statement_ptr createForStatement(
		std::vector<expression<lvalue>::ptr> decls,
		expression<number>::ptr expr2,
		expression<void>::ptr expr3,
		statement_ptr statement
	);
}


#endif /* statement_hpp */
