#ifndef _NODE_H_
#define _NODE_H_

#include "Attributes.h"


class Scope;
extern std::vector<std::unique_ptr<Scope>> ScopeStack; 

class Scope {
    public:
        // symbol table for variables of this scope
        std::unordered_map<std::string, Value*> SymbolTable; 
        
        // record of all heap allocations for this scope
        std::vector<Value*> HeapAllocas;

        // holds return value for function. Used to implement multiple return values in function definition
        Value* returnValue = nullptr; 
        
};



class TypeId {
    public:
        virtual ~TypeId() {};
        virtual Type *typegen() = 0;
};


class PrimitiveTyId : public TypeId {
    public:
        std::string prityId;

        PrimitiveTyId(std::string ptyId)
            : prityId(std::move(ptyId)) {}
        Type *typegen() override;
};


class EntityTyId : public TypeId {
    public:
        std::string enttyId;
        
        EntityTyId(std::string etyId)
            : enttyId(std::move(etyId)) {}
        Type *typegen() override;
};


class ListTyId : public TypeId {
    public:
        std::unique_ptr<TypeId> lsttyId;

        ListTyId(std::unique_ptr<TypeId> ltyId)
            : lsttyId(std::move(ltyId)) {}
        Type *typegen() override;
};



/// Rule of thumb. if a Node class has a Value* member or a constructor with Value* parameters, the Parser will
/// never directly call that constructor or affect/initialize that member

class Node {
    public:
        bool ctx = 1; // 1 for load, 0 for no load 

        virtual void injectCtx(std::string ctxStr) {
            ctx = (ctxStr == "Load") ? 1 : 0;
        }
        virtual ~Node() {}
        virtual Value *codegen() = 0;
};

    
class IntegerNode : public Node {
    public:
        long value;
        IntegerNode(long value) : value(value) {}
        Value *codegen() override;
};


class FloatNode : public Node {
    public:
        double value;
        FloatNode(double value) : value(value) {}
        Value *codegen() override;
};


class Identifier : public Node {
    public:
        std::string name;
        Identifier(std::string Name) : name(Name) {}
        Value *codegen() override;

};



/***************************************** Declarations *************************************************************/
class Declr : public Node {
    public:
        std::string name;
        std::unique_ptr<TypeId> type;

        Declr(std::string nm, std::unique_ptr<TypeId> ty) 
            : name(std::move(nm)), type(std::move(ty)) {}
        Value *codegen() override;       
};


class InitDeclr : public Node {
    public:
        std::string name; // variable name
        std::unique_ptr<Node> rhs;

        InitDeclr(std::string id, std::unique_ptr<Node> rhs) 
            : name(std::move(id)), rhs(std::move(rhs)) {}
         Value *codegen() override;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**************************************** Liste Manipulations *******************************************************/
class AnonymousList : public Node {
    public:
        std::vector<std::unique_ptr<Node>> initializerList;

        AnonymousList(std::vector<std::unique_ptr<Node>> iList)
            : initializerList(std::move(iList)) {}
        Value *codegen() override;
};


class ListConcatenation : public Node {
    public:
        Value* list1;
        Value* list2;

        ListConcatenation(Value* lhs, Value* rhs)
            : list1(lhs), list2(rhs) {}
        Value* codegen() override;
};


class ListSlice : public Node {
    public:
        std::unique_ptr<Node> list;
        std::unique_ptr<Node> start;
        std::unique_ptr<Node> end;

        ListSlice(std::unique_ptr<Node> lst, std::unique_ptr<Node> lower, std::unique_ptr<Node> upper)
            : list(std::move(lst)), start(std::move(lower)), end(std::move(upper)) {}
        Value *codegen() override;
};


class ListOperation : public Node {
    public:
        std::unique_ptr<Node> list; // variable name
        std::unique_ptr<Node> idx;
        std::unique_ptr<ListSlice> slice;
        
        ListOperation(std::unique_ptr<Node> lst, std::unique_ptr<Node> index) 
            : list(std::move(lst)), idx(std::move(index)) {}
        ListOperation(std::unique_ptr<Node> lst, std::unique_ptr<Node> sliceBegin, std::unique_ptr<Node> sliceEnd) 
            : list(std::move(lst)) {
                slice = std::make_unique<ListSlice>(std::move(lst), std::move(sliceBegin), std::move(sliceEnd));
            }
        Value *codegen() override;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**************************************** Entity Manipulations ******************************************************/
class AnonymousEntity : public Node {
    public:
        std::string enttypeStr;
        // Entity constructor. Maps entity fields(actually their positions) to their values
        std::unordered_map<unsigned int, std::unique_ptr<Node>> entityCons;

        AnonymousEntity(std::string eType, std::unordered_map<unsigned int, std::unique_ptr<Node>> eInit)
            : enttypeStr(std::move(eType)), entityCons(std::move(eInit)) {}
        Value *codegen() override;
};


class EntityOperation : public Node {
    public:
        std::unique_ptr<Node> entity;
        std::string selectedField;
        
        EntityOperation(std::unique_ptr<Node> e, std::string eField)
            : entity(std::move(e)), selectedField(std::move(eField)) {}
        Value *codegen() override;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class ReturnExpr : public Node {
    public:
        std::unique_ptr<Node> returnExpr;
        ReturnExpr(std::unique_ptr<Node> rValue)
            : returnExpr(std::move(rValue)) {}
        Value *codegen() override;
};


class BinaryExpr : public Node {
    // Expression class for Binary Operations
    public:
        char Op;
        std::unique_ptr<Node> LHS, RHS;
        BinaryExpr(char op, std::unique_ptr<Node> LHS, std::unique_ptr<Node> RHS)
            : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
        Value *codegen() override;
};


class FunctionCallNode : public Node {
    std::unique_ptr<Identifier> Callee; // this holds the name of the function
    std::vector<std::unique_ptr<Node>> Args;

    public:
        FunctionCallNode(std::unique_ptr<Identifier> Callee, std::vector<std::unique_ptr<Node>> args)
            : Callee(std::move(Callee)), Args(std::move(args)){}
        FunctionCallNode(std::unique_ptr<Identifier> Callee)
            : Callee(std::move(Callee)) {}
        Value *codegen() override;
};   


class Assignment : public Node {
    public:
        std::unique_ptr<Node> lhs;
        std::unique_ptr<Node> rhs;

        Assignment(std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
            : lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        Value *codegen() override;
};



/**************************************** Control structures ********************************************************/
class IfNode : public Node {
    public:
        // The Else body can be a node ptr (in the case of else if) or a vector of statements (in the case of lone else)
        std::unique_ptr<Node> Cond, ElseIf;
        std::vector<std::unique_ptr<Node>> Then, LoneElse;

        IfNode(std::unique_ptr<Node> cond, std::vector<std::unique_ptr<Node>> then)
            :Cond(std::move(cond)), Then(std::move(then)) {}
        IfNode(std::unique_ptr<Node> cond, std::vector<std::unique_ptr<Node>> then, std::unique_ptr<Node> elif)
            :Cond(std::move(cond)), Then(std::move(then)), ElseIf(std::move(elif)) {}
        IfNode(std::unique_ptr<Node> cond, std::vector<std::unique_ptr<Node>> then, std::vector<std::unique_ptr<Node>> ele)
            :Cond(std::move(cond)), Then(std::move(then)), LoneElse(std::move(ele)) {}
        Value *codegen() override;
};


class WhileNode : public Node {
    public:
        std::unique_ptr<Node> Cond;
        std::vector<std::unique_ptr<Node>> Body;
        
        WhileNode(std::unique_ptr<Node> cond, std::vector<std::unique_ptr<Node>> body)
            : Cond(std::move(cond)), Body(std::move(body)) {}
        Value *codegen() override;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////   


class EntityDefn : public Node {
    public:
        std::string entityName;
        std::vector<std::unique_ptr<TypeId>> memtypeIds;

        EntityDefn(std::string eName, std::vector<std::unique_ptr<TypeId>> memTIds)
            : entityName(std::move(eName)), memtypeIds(std::move(memTIds)) {}
        Value *codegen() override;
};


class Prototype : public Node {
    public:
        std::unique_ptr<Identifier> returnType;
        std::unique_ptr<Identifier> funcName;
        std::vector<std::unique_ptr<Identifier>> argTypes;
        std::vector<std::unique_ptr<Identifier>> argNames;
        
        Prototype(std::unique_ptr<Identifier> return_type, std::unique_ptr<Identifier> funcName, std::vector<std::unique_ptr<Identifier>> arg_types, 
                  std::vector<std::unique_ptr<Identifier>> arg_names)
                :returnType(std::move(return_type)), funcName(std::move(funcName)), argTypes(std::move(arg_types)), argNames(std::move(arg_names)) {}
        Value *codegen() override;
};


class FunctionDefn : public Node {
    public:
        std::unique_ptr<Prototype> prototype;
        std::vector<std::unique_ptr<Node>> Body;
        FunctionDefn(std::unique_ptr<Prototype> proto, std::vector<std::unique_ptr<Node>> body) 
                :prototype(std::move(proto)), Body(std::move(body)){}
        Value *codegen() override;
};

#endif