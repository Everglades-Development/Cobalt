#include "expressionTree.hpp"
#include "helpers.hpp"
#include "errors.hpp"
#include "compilerContext.hpp"

namespace cobalt {
	namespace {
		bool is_convertible(typeHandle type_from, bool lvalue_from, typeHandle type_to, bool lvalue_to) {
			if (type_to == typeRegistry::getVoidHandle()) {
				return true;
			}
			if (lvalue_to) {
				return lvalue_from && type_from == type_to;
			}
			if (type_from == type_to) {
				return true;
			}
			if (const initListType* ilt = std::get_if<initListType>(type_from)) {
				if (lvalue_to) {
					return false;
				}
				if (type_to == typeRegistry::getVoidHandle()) {
					return true;
				}
				return std::visit(overloaded{
					[&](const arrayType& at) {
						for (typeHandle it : ilt->inner_type_id) {
							if (it != at.inner_type_id) {
								return false;
							}
						}
						return true;
					},
					[&](const tupleType& tt) {
						if (tt.inner_type_id.size() != ilt->inner_type_id.size()) {
							return false;
						}
						for (size_t i = 0; i < tt.inner_type_id.size(); ++i) {
							if (ilt->inner_type_id[i] != tt.inner_type_id[i]) {
								return false;
							}
						}
						return true;
					},
					[&](const type&) {
						return false;
					}
				}, *type_to);
			}
			return type_from == typeRegistry::getNumberHandle() && type_to == typeRegistry::getStringHandle();
		}
	}

	node::node(compilerContext& context, nodeValue value, std::vector<node_ptr> children, size_t lineNumber, size_t charIndex) :
		_value(std::move(value)),
		_children(std::move(children)),
		_line_number(lineNumber),
		_char_index(charIndex)
	{
		const typeHandle void_handle = typeRegistry::getVoidHandle();
		const typeHandle number_handle = typeRegistry::getNumberHandle();
		const typeHandle string_handle = typeRegistry::getStringHandle();
		
		std::visit(overloaded {
			[&](const std::string& value) {
				_type_id = string_handle;
				_lvalue = false;
			},
			[&](double value) {
				_type_id = number_handle;
				_lvalue = false;
			},
			[&](const identifier& value){
				if (const identifierInfo* info = context.find(value.name)) {
					_type_id = info->typeID();
					_lvalue = (info->getScope() != identifierScope::function);
				} else {
					throw undeclaredError(value.name, _line_number, _char_index);
				}
			},
			[&](nodeOperation value){
				switch (value) {
					case nodeOperation::param:
						_type_id = _children[0]->_type_id;
						_lvalue = false;
						break;
					case nodeOperation::preinc:
					case nodeOperation::predec:
						_type_id = number_handle;
						_lvalue = true;
						_children[0]->checkConversion(number_handle, true);
						break;
					case nodeOperation::postinc:
					case nodeOperation::postdec:
						_type_id = number_handle;
						_lvalue = false;
						_children[0]->checkConversion(number_handle, true);
						break;
					case nodeOperation::positive:
					case nodeOperation::negative:
					case nodeOperation::bnot:
					case nodeOperation::lnot:
						_type_id = number_handle;
						_lvalue = false;
						_children[0]->checkConversion(number_handle, false);
						break;
					case nodeOperation::size:
						_type_id = number_handle;
						_lvalue = false;
						break;
					case nodeOperation::tostring:
						_type_id = string_handle;
						_lvalue = false;
						break;
					case nodeOperation::add:
					case nodeOperation::sub:
					case nodeOperation::mul:
					case nodeOperation::div:
					case nodeOperation::idiv:
					case nodeOperation::mod:
					case nodeOperation::band:
					case nodeOperation::bor:
					case nodeOperation::bxor:
					case nodeOperation::bsl:
					case nodeOperation::bsr:
					case nodeOperation::land:
					case nodeOperation::lor:
						_type_id = number_handle;
						_lvalue = false;
						_children[0]->checkConversion(number_handle, false);
						_children[1]->checkConversion(number_handle, false);
						break;
					case nodeOperation::eq:
					case nodeOperation::ne:
					case nodeOperation::lt:
					case nodeOperation::gt:
					case nodeOperation::le:
					case nodeOperation::ge:
						_type_id = number_handle;
						_lvalue = false;
						if (!_children[0]->isNumber() || !_children[1]->isNumber()) {
							_children[0]->checkConversion(string_handle, false);
							_children[1]->checkConversion(string_handle, false);
						} else {
							_children[0]->checkConversion(number_handle, false);
							_children[1]->checkConversion(number_handle, false);
						}
						break;
					case nodeOperation::concat:
						_type_id = context.getHandle(simpleType::string);
						_lvalue = false;
						_children[0]->checkConversion(string_handle, false);
						_children[1]->checkConversion(string_handle, false);
						break;
					case nodeOperation::assign:
						_type_id = _children[0]->getTypeID();
						_lvalue = true;
						_children[0]->checkConversion(_type_id, true);
						_children[1]->checkConversion(_type_id, false);
						break;
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
						_type_id = number_handle;
						_lvalue = true;
						_children[0]->checkConversion(number_handle, true);
						_children[1]->checkConversion(number_handle, false);
						break;
					case nodeOperation::concat_assign:
						_type_id = string_handle;
						_lvalue = true;
						_children[0]->checkConversion(string_handle, true);
						_children[1]->checkConversion(string_handle, false);
						break;
					case nodeOperation::comma:
						for (int i = 0; i < int(_children.size()) - 1; ++i) {
							_children[i]->checkConversion(void_handle, false);
						}
						_type_id = _children.back()->getTypeID();
						_lvalue = _children.back()->is_lvalue();
						break;
					case nodeOperation::index:
						_lvalue = _children[0]->is_lvalue();
						if (const arrayType* at = std::get_if<arrayType>(_children[0]->getTypeID())) {
							_type_id = at->inner_type_id;
						} else if (const tupleType* tt = std::get_if<tupleType>(_children[0]->getTypeID())) {
							if (_children[1]->isNumber()) {
								double idx = _children[1]->getNumber();
								if (size_t(idx) == idx && idx >= 0 && idx < tt->inner_type_id.size()) {
									_type_id = tt->inner_type_id[size_t(idx)];
								
                                } else {
									throw semanticError("Invalid tuple index " + std::to_string(idx) , _line_number, _char_index);
								}
							} else {
								throw semanticError("Invalid tuple index", _line_number, _char_index);
							}
						} else {
							throw semanticError(to_string(_children[0]->_type_id) + " is not indexable",
							                     _line_number, _char_index);
						}
						break;
					case nodeOperation::ternary:
						_children[0]->checkConversion(number_handle, false);
						if (is_convertible(
							_children[2]->getTypeID(), _children[2]->is_lvalue(),
							_children[1]->getTypeID(), _children[1]->is_lvalue()
						)) {
							_children[2]->checkConversion(_children[1]->getTypeID(), _children[1]->is_lvalue());
							_type_id = _children[1]->getTypeID();
							_lvalue = _children[1]->is_lvalue();
						} else {
							_children[1]->checkConversion(_children[2]->getTypeID(), _children[2]->is_lvalue());
							_type_id = _children[2]->getTypeID();
							_lvalue = _children[2]->is_lvalue();
						}
						break;
					case nodeOperation::call:
						if (const functionType* ft = std::get_if<functionType>(_children[0]->getTypeID())) {
							_type_id = ft->return_type_id;
							_lvalue = false;
							if (ft->param_type_id.size() + 1 != _children.size()) {
								throw semanticError("Wrong number of arguments. "
								                     "Expected " + std::to_string(ft->param_type_id.size()) +
								                     ", given " + std::to_string(_children.size() - 1),
								                     _line_number, _char_index);
							}
							for (size_t i = 0; i < ft->param_type_id.size(); ++i) {
								if (_children[i+1]->is_lvalue() && !ft->param_type_id[i].by_ref) {
									throw semanticError(
										"Function doesn't receive the argument by reference",
										_children[i+1]->getLineNumber(), _children[i+1]->getCharIndex()
									);
								}
								_children[i+1]->checkConversion(ft->param_type_id[i].typeID, ft->param_type_id[i].by_ref);
							}
						} else {
							throw semanticError(to_string(_children[0]->_type_id) + " is not callable",
							                     _line_number, _char_index);
						}
						break;
					case nodeOperation::init:
					{
						initListType ilt;
						ilt.inner_type_id.reserve(_children.size());
						for (const node_ptr& child : _children) {
							ilt.inner_type_id.push_back(child->getTypeID());
						}
						_type_id = context.getHandle(ilt);
						_lvalue = false;
						break;
					}
				}
			}
		},_value);
	}
	
	const nodeValue& node::getValue() const {
		return _value;
	}
		
	bool node::is_node_operation() const {
		return std::holds_alternative<nodeOperation>(_value);
	}
	
	bool node::isIdentifier() const {
		return std::holds_alternative<identifier>(_value);
	}
	
	bool node::isNumber() const {
		return std::holds_alternative<double>(_value);
	}
	
	bool node::isString() const {
		return std::holds_alternative<std::string>(_value);
	}
	
	nodeOperation node::getNodeOperation() const {
		return std::get<nodeOperation>(_value);
	}
	
	std::string_view node::getIdentifier() const {
		return std::get<identifier>(_value).name;
	}
	
	double node::getNumber() const {
		return std::get<double>(_value);
	}
	
	std::string_view node::getString() const {
		return std::get<std::string>(_value);
	}
	
	const std::vector<node_ptr>& node::getChildren() const {
		return _children;
	}
	
	typeHandle node::getTypeID() const {
		return _type_id;
	}
	
	bool node::is_lvalue() const {
		return _lvalue;
	}
	
	size_t node::getLineNumber() const {
		return _line_number;
	}
	
	size_t node::getCharIndex() const {
		return _char_index;
	}
	
	void node::checkConversion(typeHandle typeID, bool lvalue) const{
		if (!is_convertible(_type_id, _lvalue, typeID, lvalue)) {
			throw wrongTypeError(std::to_string(_type_id), std::to_string(typeID), lvalue,
			                       _line_number, _char_index);
		}
	}
}
