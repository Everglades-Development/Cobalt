#include "compilerContext.hpp"

namespace cobalt{
	identifierInfo::identifierInfo(typeHandle typeID, size_t index, identifierScope scope) :
		_type_id(typeID),
		_index(index),
		_scope(scope)
	{
	}
	
	typeHandle identifierInfo::typeID() const {
		return _type_id;
	}
	
	size_t identifierInfo::index() const {
		return _index;
	}
	
	identifierScope identifierInfo::getScope() const {
		return _scope;
	}

	const identifierInfo* identifierLookup::insertIdentifier(std::string name, typeHandle typeID, size_t index, identifierScope scope) {
		return &_identifiers.emplace(std::move(name), identifierInfo(typeID, index, scope)).first->second;
	}
	
	size_t identifierLookup::identifiersSize() const {
		return _identifiers.size();
	}

	const identifierInfo* identifierLookup::find(const std::string& name) const {
		if (auto it = _identifiers.find(name); it != _identifiers.end()) {
			return &it->second;
		} else {
			return nullptr;
		}
	}
	
	bool identifierLookup::canDeclare(const std::string& name) const {
		return _identifiers.find(name) == _identifiers.end();
	}
	
	identifierLookup::~identifierLookup() {
	}

	const identifierInfo* globalVariableLookup::createIdentifier(std::string name, typeHandle typeID) {
		return insertIdentifier(std::move(name), typeID, identifiersSize(), identifierScope::global_variable);
	}

	localVariableLookup::localVariableLookup(std::unique_ptr<localVariableLookup> parent_lookup) :
		_parent(std::move(parent_lookup)),
		_next_identifier_index(_parent ? _parent->_next_identifier_index : 1)
	{
	}
	
	const identifierInfo* localVariableLookup::find(const std::string& name) const {
		if (const identifierInfo* ret = identifierLookup::find(name)) {
			return ret;
		} else {
			return _parent ? _parent->find(name) : nullptr;
		}
	}

	const identifierInfo* localVariableLookup::createIdentifier(std::string name, typeHandle typeID) {
		return insertIdentifier(std::move(name), typeID, _next_identifier_index++, identifierScope::local_variable);
	}
	
	std::unique_ptr<localVariableLookup> localVariableLookup::detach_parent() {
		return std::move(_parent);
	}

	paramLookup::paramLookup() :
		localVariableLookup(nullptr),
		_next_param_index(-1)
	{
	}
	
	const identifierInfo* paramLookup::createParam(std::string name, typeHandle typeID) {
		return insertIdentifier(std::move(name), typeID, _next_param_index--, identifierScope::local_variable);
	}
	
	const identifierInfo* functionLookup::createIdentifier(std::string name, typeHandle typeID) {
		return insertIdentifier(std::move(name), typeID, identifiersSize(), identifierScope::function);
	}

	compilerContext::compilerContext() :
		_params(nullptr)
	{
	}
	
	const type* compilerContext::getHandle(const type& t) {
		return _types.getHandle(t);
	}
	
	const identifierInfo* compilerContext::find(const std::string& name) const {
		if (_locals) {
			if (const identifierInfo* ret = _locals->find(name)) {
				return ret;
			}
		}
		if (const identifierInfo* ret = _functions.find(name)) {
			return ret;
		}
		return _globals.find(name);
	}
	
	const identifierInfo* compilerContext::createIdentifier(std::string name, typeHandle typeID) {
		if (_locals) {
			return _locals->createIdentifier(std::move(name), typeID);
		} else {
			return _globals.createIdentifier(std::move(name), typeID);
		}
	}
	
	const identifierInfo* compilerContext::createParam(std::string name, typeHandle typeID) {
		return _params->createParam(name, typeID);
	}
	
	const identifierInfo* compilerContext::createFunction(std::string name, typeHandle typeID) {
		return _functions.createIdentifier(name, typeID);
	}
	
	void compilerContext::enterScope() {
		_locals = std::make_unique<localVariableLookup>(std::move(_locals));
	}
	
	void compilerContext::enterFunction() {
		std::unique_ptr<paramLookup> params = std::make_unique<paramLookup>();
		_params = params.get();
		_locals = std::move(params);
	}
	
	void compilerContext::leaveScope() {
		if (_params == _locals.get()) {
			_params = nullptr;
		}
		
		_locals = _locals->detach_parent();
	}
	
	bool compilerContext::canDeclare(const std::string& name) const {
		return _locals ? _locals->canDeclare(name) : (_globals.canDeclare(name) && _functions.canDeclare(name));
	}
	
	compilerContext::scopeRaii compilerContext::scope() {
		return scopeRaii(*this);
	}
	
	compilerContext::functionRaii compilerContext::function() {
		return functionRaii(*this);
	}
	
	compilerContext::scopeRaii::scopeRaii(compilerContext& context):
		_context(context)
	{
		_context.enterScope();
	}
	
	compilerContext::scopeRaii::~scopeRaii() {
		_context.leaveScope();
	}
	
	compilerContext::functionRaii::functionRaii(compilerContext& context):
		_context(context)
	{
		_context.enterFunction();
	}
	
	compilerContext::functionRaii::~functionRaii() {
		_context.leaveScope();
	}
}
