#ifndef _NODE_H_
#define _NODE_H_

#include "Attributes.h"

////////////////////////////////////////////////// Type Identifiers ////////////////////////////////////
class TypeId {
public:
	virtual ~TypeId(){};
	virtual Type* typegen() = 0;
};


class BuiltinTyId : public TypeId {
public:
	std::string builtinTypeId;
	BuiltinTyId(std::string&& builtinTypeId) : builtinTypeId(std::move(builtinTypeId)) {}
	Type* typegen() override;
};


class EntityTyId : public TypeId {
public:
	std::string enttyTypeId;
	EntityTyId(std::string&& enttyTypeId) : enttyTypeId(std::move(enttyTypeId)) {}
	Type* typegen() override;
};


class ListTyId : public TypeId {
public:
	std::unique_ptr<TypeId> lstTypeId;
	ListTyId(std::unique_ptr<TypeId>&& lstTypeId) : lstTypeId(std::move(lstTypeId)) {}
	Type* typegen() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////


class CompoundStmt : public Node {
public:
	std::vector<std::unique_ptr<Node>> block_stmt;
	CompoundStmt(std::vector<std::unique_ptr<Node>>&& block_stmt) : block_stmt(std::move(block_stmt)) {}
	Value* codegen() override;
};


////////////////////////////////////////// Constant Expressions ////////////////////////////////////////
class BooleanLiteral : public Node {
public:
	std::string value;

	BooleanLiteral(std::string&& value) : value(std::move(value)) {}

	std::unique_ptr<Node> clone() override {
		return std::make_unique<BooleanLiteral>(std::string(value));
	}

	Value* codegen() override;
};


class NumericLiteral : public Node {
public:
	double value;

	NumericLiteral(double value) : value(value) {}

	std::unique_ptr<Node> clone() override { return std::make_unique<NumericLiteral>(value); }

	Value* codegen() override;
};


class Identifier : public Node {
public:
	std::string name;

	Identifier(std::string&& name) : name(std::move(name)) {}

	std::unique_ptr<Node> clone() override { return std::make_unique<Identifier>(std::string(name)); }

	bool injectCtx(bool ctx) override;
	Value* codegen() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////// Declarations ////////////////////////////////////////////////
class Declr : public Node {
public:
	std::string name;
	std::unique_ptr<TypeId> type;

	Declr(std::string&& name, std::unique_ptr<TypeId>&& type)
	    : name(std::move(name)), type(std::move(type)) {}

	Value* codegen() override;
};


class InitDeclr : public Node {
public:
	std::string name;  // variable name
	std::unique_ptr<Node> rhs;

	InitDeclr(std::string&& name, std::unique_ptr<Node>&& rhs)
	    : name(std::move(name)), rhs(std::move(rhs)) {}
	Value* codegen() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////// Liste Manipulations /////////////////////////////////////////
class AnonymousList : public Node {
public:
	std::vector<std::unique_ptr<Node>> initList;

	AnonymousList(std::vector<std::unique_ptr<Node>>&& initList) : initList(std::move(initList)) {}

	std::unique_ptr<Node> clone() override {
		decltype(initList) initList_clone;
		initList_clone.reserve(initList.size());

		for (const auto& item : initList) {
			initList_clone.push_back(item->clone());
		}

		return std::make_unique<AnonymousList>(std::move(initList_clone));
	}

	Value* codegen() override;
};


class ListConcatenation : public Node {
public:
	Value* list1;
	Value* list2;

	ListConcatenation(Value* lhs, Value* rhs) : list1(lhs), list2(rhs) {}

	std::unique_ptr<Node> clone() override { return std::make_unique<ListConcatenation>(list1, list2); }

	Value* codegen() override;
};


class ListIdxRef : public Node {
public:
	std::unique_ptr<Node> list;
	std::unique_ptr<Node> idx;

	ListIdxRef(std::unique_ptr<Node>&& list, std::unique_ptr<Node>&& index)
	    : list(std::move(list)), idx(std::move(index)) {}

	std::unique_ptr<Node> clone() override {
		return std::make_unique<ListIdxRef>(list->clone(), idx->clone());
	}

	bool injectCtx(bool ctx) override;
	Value* codegen() override;
};


class ListSlice : public Node {
public:
	std::unique_ptr<Node> list;
	std::unique_ptr<Node> start;
	std::unique_ptr<Node> end;

	ListSlice(std::unique_ptr<Node>&& list, std::unique_ptr<Node>&& start, std::unique_ptr<Node>&& end)
	    : list(std::move(list)), start(std::move(start)), end(std::move(end)) {}

	std::unique_ptr<Node> clone() override {
		return std::make_unique<ListSlice>(list->clone(), start->clone(), end->clone());
	}

	Value* codegen() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////// Strings (basically lists) /////////////////////////////////////
class AnonymousString : public Node {
public:
	std::string str;

	AnonymousString(std::string&& str) : str(std::move(str)) {}

	std::unique_ptr<Node> clone() override {
		return std::make_unique<AnonymousString>(std::string(str));
	}

	Value* codegen() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////// Entity Manipulations //////////////////////////////////////////
class EntityDefn : public Node {
public:
	std::string enttyTypeId;
	std::vector<std::unique_ptr<TypeId>> memTypeIds;

	EntityDefn(std::string&& enttyTypeId, std::vector<std::unique_ptr<TypeId>>&& memTypeIds)
	    : enttyTypeId(std::move(enttyTypeId)), memTypeIds(std::move(memTypeIds)) {}
	Value* codegen() override;
};


class AnonymousEntity : public Node {
public:
	std::string enttyTypeId;
	// Entity constructor. Maps entity fields(actually their positions) to their
	// values
	std::unordered_map<uint8_t, std::unique_ptr<Node>> enttyCons;

	AnonymousEntity(std::string&& enttyTypeId,
	                std::unordered_map<uint8_t, std::unique_ptr<Node>>&& enttyCons)
	    : enttyTypeId(std::move(enttyTypeId)), enttyCons(std::move(enttyCons)) {}

	std::unique_ptr<Node> clone() override {
		decltype(enttyCons) enttyCons_clone;
		enttyCons_clone.reserve(enttyCons.size());

		for (const auto& pair : enttyCons) {
			enttyCons.emplace(std::make_pair(pair.first, pair.second->clone()));
		}

		return std::make_unique<AnonymousEntity>(std::string(enttyTypeId), std::move(enttyCons_clone));
	}

	Value* codegen() override;
};


class EntityOperation : public Node {
public:
	std::unique_ptr<Node> entty;
	std::string fieldId;

	EntityOperation(std::unique_ptr<Node>&& entty, std::string&& fieldId)
	    : entty(std::move(entty)), fieldId(std::move(fieldId)) {}

	std::unique_ptr<Node> clone() override {
		std::make_unique<EntityOperation>(entty->clone(), std::string(fieldId));
	}

	bool injectCtx(bool ctx) override;
	Value* codegen() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////// Binary Operations //////////////////////////////////////////////
class UnaryExpr : public Node {
public:
	std::string Op;
	std::unique_ptr<Node> rhs;

	UnaryExpr(std::string&& Op, std::unique_ptr<Node>&& rhs) : Op(std::move(Op)), rhs(std::move(rhs)) {}

	std::unique_ptr<Node> clone() override {
		return std::make_unique<UnaryExpr>(std::string(Op), rhs->clone());
	}

	Value* codegen() override;
};


class BinaryExpr : public Node {
public:
	std::string Op;
	std::unique_ptr<Node> lhs, rhs;

	BinaryExpr(std::string&& Op, std::unique_ptr<Node>&& lhs, std::unique_ptr<Node>&& rhs)
	    : Op(std::move(Op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

	std::unique_ptr<Node> clone() override {
		return std::make_unique<BinaryExpr>(std::string(Op), lhs->clone(), rhs->clone());
	}

	Value* codegen() override;
};


class Assignment : public Node {
public:
	std::unique_ptr<Node> lhs;
	std::unique_ptr<Node> rhs;

	Assignment(std::unique_ptr<Node>&& lhs, std::unique_ptr<Node>&& rhs)
	    : lhs(std::move(lhs)), rhs(std::move(rhs)) {}

	std::unique_ptr<Node> clone() override {
		return std::make_unique<Assignment>(lhs->clone(), rhs->clone());
	}

	Value* codegen() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////// Control structures ///////////////////////////////////////
class IfStmt : public Node {
public:
	// The Else body can be a node ptr (in the case of else if) or a vector of
	// statements (in the case of lone else)
	std::unique_ptr<Node> Cond;
	std::unique_ptr<Node> ThenBody, ElseBody;

	IfStmt(std::unique_ptr<Node>&& Cond, std::unique_ptr<CompoundStmt>&& ThenBody)
	    : Cond(std::move(Cond)), ThenBody(std::move(ThenBody)) {}
	IfStmt(std::unique_ptr<Node>&& Cond, std::unique_ptr<CompoundStmt>&& ThenBody,
	       std::unique_ptr<Node>&& ElseBody)
	    : Cond(std::move(Cond)), ThenBody(std::move(ThenBody)), ElseBody(std::move(ElseBody)) {}
	Value* codegen() override;
};


class WhileStmt : public Node {
public:
	std::unique_ptr<Node> Cond;
	std::unique_ptr<CompoundStmt> Body;

	WhileStmt(std::unique_ptr<Node>&& Cond, std::unique_ptr<CompoundStmt>&& Body)
	    : Cond(std::move(Cond)), Body(std::move(Body)) {}
	Value* codegen() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////// Functions /////////////////////////////////////////////////
class Prototype : public Node {
public:
	std::string fnName;
	std::unique_ptr<TypeId> fnTypeId;
	std::vector<std::string> paramNames;
	std::vector<std::unique_ptr<TypeId>> paramTypeIds;

	Prototype(std::string&& fnName, std::unique_ptr<TypeId>&& fnTypeId)
	    : fnName(std::move(fnName)), fnTypeId(std::move(fnTypeId)) {}

	Prototype(std::string&& fnName, std::unique_ptr<TypeId>&& fnTypeId,
	          std::vector<std::string>&& paramNames, std::vector<std::unique_ptr<TypeId>>&& paramTypeIds)

	    : fnName(std::move(fnName)),
	      fnTypeId(std::move(fnTypeId)),
	      paramNames(std::move(paramNames)),
	      paramTypeIds(std::move(paramTypeIds)) {}

	Value* codegen() override;
};


class FunctionDefn : public Node {
public:
	std::unique_ptr<Prototype> prototype;
	std::unique_ptr<CompoundStmt> Body;

	FunctionDefn(std::unique_ptr<Prototype>&& prototype, std::unique_ptr<CompoundStmt>&& Body)
	    : prototype(std::move(prototype)), Body(std::move(Body)) {}
	Value* codegen() override;
};


class ReturnStmt : public Node {
public:
	std::unique_ptr<Node> returnExpr;
	ReturnStmt(std::unique_ptr<Node>&& returnExpr) : returnExpr(std::move(returnExpr)) {}
	Value* codegen() override;
};


class FunctionCallExpr : public Node {
public:
	std::string Callee;  // this holds the name of the function
	std::vector<std::unique_ptr<Node>> Args;

	FunctionCallExpr(std::string&& Callee, std::vector<std::unique_ptr<Node>>&& args)
	    : Callee(std::move(Callee)), Args(std::move(args)) {}
	FunctionCallExpr(std::string&& Callee) : Callee(std::move(Callee)) {}

	std::unique_ptr<Node> clone() override {
		decltype(Args) Args_clone;
		Args_clone.reserve(Args.size());

		for (const auto& Arg : Args) {
			Args_clone.push_back(Arg->clone());
		}

		return std::make_unique<FunctionCallExpr>(std::string(Callee), std::move(Args_clone));
	}

	Value* codegen() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////


class IncludeStmt : public Node {
public:
	std::string rtModule;
	std::vector<std::string> userModule;
	std::string alias;

	IncludeStmt(std::string&& rtModule, std::string&& alias)
	    : rtModule(std::move(rtModule)), alias(std::move(alias)) {}
	IncludeStmt(std::vector<std::string>&& userModule, std::string&& alias)
	    : userModule(std::move(userModule)), alias(std::move(alias)) {}
	Value* codegen() override;
};

#endif