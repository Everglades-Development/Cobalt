#ifndef compilerContext_hpp
#define compilerContext_hpp

#include <unordered_map>
#include <memory>
#include <string>

#include "types.hpp"

namespace cobalt {

	enum struct identifierScope {
		global_variable,
		local_variable,
		function,
	};

	class identifierInfo {
	private:
		typeHandle _type_id;
		size_t _index;
		identifierScope _scope;
	public:
		identifierInfo(typeHandle typeID, size_t index, identifierScope scope);
		
		typeHandle typeID() const;
		
		size_t index() const;
		
		identifierScope getScope() const;
	};
	
	class identifierLookup {
	private:
		std::unordered_map<std::string, identifierInfo> _identifiers;
	protected:
		const identifierInfo* insertIdentifier(std::string name, typeHandle typeID, size_t index, identifierScope scope);
		size_t identifiersSize() const;
	public:
		virtual const identifierInfo* find(const std::string& name) const;
		
		virtual const identifierInfo* createIdentifier(std::string name, typeHandle typeID) = 0;
		
		bool canDeclare(const std::string& name) const;
		
		virtual ~identifierLookup();
	};
	
	class globalVariableLookup: public identifierLookup {
	public:
		const identifierInfo* createIdentifier(std::string name, typeHandle typeID) override;
	};
	
	class localVariableLookup: public identifierLookup {
	private:
		std::unique_ptr<localVariableLookup> _parent;
		int _next_identifier_index;
	public:
		localVariableLookup(std::unique_ptr<localVariableLookup> parent_lookup);
		
		const identifierInfo* find(const std::string& name) const override;

		const identifierInfo* createIdentifier(std::string name, typeHandle typeID) override;
		
		std::unique_ptr<localVariableLookup> detach_parent();
	};
	
	class paramLookup: public localVariableLookup {
	private:
		int _next_param_index;
	public:
		paramLookup();
		
		const identifierInfo* createParam(std::string name, typeHandle typeID);
	};
	
	class functionLookup: public identifierLookup {
	public:
		const identifierInfo* createIdentifier(std::string name, typeHandle typeID) override;
	};
	
	class compilerContext {
	private:
		functionLookup _functions;
		globalVariableLookup _globals;
		paramLookup* _params;
		std::unique_ptr<localVariableLookup> _locals;
		typeRegistry _types;
		
		class scopeRaii {
		private:
			compilerContext& _context;
		public:
			scopeRaii(compilerContext& context);
			~scopeRaii();
		};
		
		class functionRaii {
		private:
			compilerContext& _context;
		public:
			functionRaii(compilerContext& context);
			~functionRaii();
		};
		
		void enterFunction();
		void enterScope();
		void leaveScope();
	public:
		compilerContext();
		
		typeHandle getHandle(const type& t);
		
		const identifierInfo* find(const std::string& name) const;
		
		const identifierInfo* createIdentifier(std::string name, typeHandle typeID);
		
		const identifierInfo* createParam(std::string name, typeHandle typeID);
		
		const identifierInfo* createFunction(std::string name, typeHandle typeID);
		
		bool canDeclare(const std::string& name) const;
		
		scopeRaii scope();
		functionRaii function();
	};
}

#endif /*compilerContext_hpp*/
