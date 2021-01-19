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

        // last block in a function definition.
        BasicBlock* fnExitBB;
        // holds return value for function. Helps to handle multiple return values in function definition
        Value* returnValue = nullptr; 
        
};


////////////////////////////////////////////////// Types //////////////////////////////////////////////////////////////
class TypeId {
    public:
        virtual ~TypeId() {};
        virtual Type *typegen() = 0;
};


class BuiltinTyId : public TypeId {
    public:
        std::string prityId;

        BuiltinTyId(std::string ptyId)
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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



class Node {
    
    public:
        // ctx is 1 for load (default context), 0 for store. This has nothing to do with llvm's context
        bool ctx = 1; 

        // Some Node classes have no context e.x Declr, FunctionDefn, CompoundStmt. While some can only allow a load
        // context e.x ListSlice, FunctionCallExpr.
        // Node classes that can have a load or store ctx (i.e Nodes allowed on the left hand side of an assigment) 
        // override this method.
        virtual bool injectCtx(bool ctx) {
            return false;
        }

        virtual ~Node() {}
        virtual Value *codegen() = 0;
};


class CompoundStmt : public Node {
    public:
        std::vector<std::unique_ptr<Node>> stmtBlock;

        CompoundStmt(std::vector<std::unique_ptr<Node>> sblock)
            : stmtBlock(std::move(sblock)) {}
        Value *codegen() override;
};



////////////////////////////////////////// Constant Expressions ///////////////////////////////////////////////////////  
class BooleanLiteral : public Node {
    public:
        std::string value;

        BooleanLiteral(std::string value) 
            : value(std::move(value)) {}
        Value *codegen() override;
};       


class NumericLiteral : public Node {
    public:
        double value;

        NumericLiteral(double value) 
            : value(value) {}
        Value *codegen() override;
};


class Identifier : public Node {
    public:
        std::string name;

        Identifier(std::string Name) : name(Name) {}
        bool injectCtx(bool ctx) override;
        Value *codegen() override;

};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////// Declarations ///////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////// Liste Manipulations /////////////////////////////////////////////////////////
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


class ListIdxRef : public Node {
    public:
        std::unique_ptr<Node> list; 
        std::unique_ptr<Node> idx;
        
        ListIdxRef(std::unique_ptr<Node> lst, std::unique_ptr<Node> index) 
            : list(std::move(lst)), idx(std::move(index)) {}
        bool injectCtx(bool ctx) override;
        Value *codegen() override;
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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////// Strings (basically lists) ////////////////////////////////////////////////////
class AnonymousString: public Node{
    public:
        std::string str;

        AnonymousString(std::string str)
            : str(std::move(str)) {}
        Value *codegen() override;
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////// Entity Manipulations /////////////////////////////////////////////////////////
class EntityDefn : public Node {
    public:
        std::string entityName;
        std::vector<std::unique_ptr<TypeId>> memtypeIds;

        EntityDefn(std::string eName, std::vector<std::unique_ptr<TypeId>> memTIds)
            : entityName(std::move(eName)), memtypeIds(std::move(memTIds)) {}
        Value *codegen() override;
};


class AnonymousEntity : public Node {
    public:
        std::string enttypeStr;
        // Entity constructor. Maps entity fields(actually their positions) to their values
        std::unordered_map<uint8_t, std::unique_ptr<Node>> entityCons;

        AnonymousEntity(std::string eType, std::unordered_map<uint8_t, std::unique_ptr<Node>> eInit)
            : enttypeStr(std::move(eType)), entityCons(std::move(eInit)) {}
        Value *codegen() override;
};


class EntityOperation : public Node {
    public:
        std::unique_ptr<Node> entity;
        std::string selectedField;
        
        EntityOperation(std::unique_ptr<Node> e, std::string eField)
            : entity(std::move(e)), selectedField(std::move(eField)) {}
        bool injectCtx(bool ctx) override;
        Value *codegen() override;
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////// Binary Operations /////////////////////////////////////////////////////////////
class BinaryExpr : public Node {
    // Expression class for Binary Operations
    public:
        std::string Op;
        std::unique_ptr<Node> lhs, rhs;

        BinaryExpr(std::string op, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
            : Op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////// Control structures //////////////////////////////////////////////////////
class IfStmt : public Node {
    public:
        // The Else body can be a node ptr (in the case of else if) or a vector of statements (in the case of lone else)
        std::unique_ptr<Node> Cond;
        std::unique_ptr<Node> ThenBody, ElseBody;

        IfStmt(std::unique_ptr<Node> cond, std::unique_ptr<CompoundStmt> thenb)
            : Cond(std::move(cond)), ThenBody(std::move(thenb)) {}
        IfStmt(std::unique_ptr<Node> cond, std::unique_ptr<CompoundStmt> thenb, std::unique_ptr<Node> elseb)
            : Cond(std::move(cond)), ThenBody(std::move(thenb)), ElseBody(std::move(elseb)) {}
        Value *codegen() override;
};


class WhileStmt : public Node {
    public:
        std::unique_ptr<Node> Cond;
        std::unique_ptr<CompoundStmt> Body;
        
        WhileStmt(std::unique_ptr<Node> cond, std::unique_ptr<CompoundStmt> body)
            : Cond(std::move(cond)), Body(std::move(body)) {}
        Value *codegen() override;
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////   



//////////////////////////////////////////// Functions ////////////////////////////////////////////////////////////////
class Prototype : public Node {
    public:
        std::string fnName;
        std::unique_ptr<TypeId> fnTypeId;
        std::vector<std::string> paramNames;
        std::vector<std::unique_ptr<TypeId>> paramTypeIds;

        Prototype(std::string funcName, std::unique_ptr<TypeId> ftypeid)
                : fnName(std::move(funcName)), fnTypeId(std::move(ftypeid)) {}
        
        Prototype(std::string funcName, std::unique_ptr<TypeId> ftypeid,
                    std::vector<std::string> pnames,std::vector<std::unique_ptr<TypeId>> ptypeids)

                : fnName(std::move(funcName)), fnTypeId(std::move(ftypeid)),
                    paramNames(std::move(pnames)), paramTypeIds(std::move(ptypeids)) {}
                    
        Value *codegen() override;
};


class FunctionDefn : public Node {
    public:
        std::unique_ptr<Prototype> prototype;
        std::unique_ptr<CompoundStmt> Body;

        FunctionDefn(std::unique_ptr<Prototype> proto, std::unique_ptr<CompoundStmt> body) 
                :prototype(std::move(proto)), Body(std::move(body)) {}
        Value *codegen() override;
};


class ReturnStmt : public Node {
    public:
        std::unique_ptr<Node> returnExpr;
        
        ReturnStmt(std::unique_ptr<Node> rValue)
            : returnExpr(std::move(rValue)) {}
        Value *codegen() override;
};


class FunctionCallExpr : public Node {
    public:
        std::string callee; // this holds the name of the function
        std::vector<std::unique_ptr<Node>> Args;

        FunctionCallExpr(std::string Callee, std::vector<std::unique_ptr<Node>> args)
            : callee(std::move(Callee)), Args(std::move(args)){}
        FunctionCallExpr(std::string Callee)
            : callee(std::move(Callee)) {}
        Value *codegen() override;
}; 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#endif