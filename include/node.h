#ifndef _NODE_H_
#define _NODE_H_

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>

#include <memory>
#include <string>
#include <iostream>
//#include <vector>

using namespace llvm;

class Scope;
extern std::vector<std::shared_ptr<Scope>> scopeStack;

extern llvm::LLVMContext context;
extern std::shared_ptr<llvm::Module> module;
extern IRBuilder<> builder;

class Scope{
    public:
        BasicBlock* block; // holds the starting block for each scope
        std::map<std::string, Value*> symbolTable; // symbol table for this scope
        Scope(BasicBlock* bb)
            :block(bb){}
};


class Node{
    public:
        virtual ~Node() {};
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


class ReturnExpr : public Node {
    public:
        std::unique_ptr<Node> returnValue;
        ReturnExpr(std::unique_ptr<Node> rValue)
            :returnValue(std::move(rValue)) {}
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


class ArrayDeclaration : public Node {
    public:
        std::unique_ptr<Identifier> arrType;
        std::unique_ptr<Identifier> arrName;
        uint64_t arrSize;
        std::vector<std::unique_ptr<Node>> initializerList; 

        ArrayDeclaration(std::unique_ptr<Identifier> type, std::unique_ptr<Identifier> name, uint64_t size)
            : arrType(std::move(type)), arrName(std::move(name)), arrSize(size) {}
        ArrayDeclaration(std::unique_ptr<Identifier> type, std::unique_ptr<Identifier> name, uint64_t size, std::vector<std::unique_ptr<Node>> v)
            : arrType(std::move(type)), arrName(std::move(name)), arrSize(size), initializerList(std::move(v)) {}
        Value *codegen() override;
};

class ArrayOperation : public Node {
    public:
        std::unique_ptr<Node> index;
        std::unique_ptr<Identifier> arr;
        std::unique_ptr<Node> rhsExpression;

        ArrayOperation(std::unique_ptr<Identifier> arrName, std::unique_ptr<Node> id) // Used for load operations
            : arr(std::move(arrName)), index(std::move(id)) {}
        ArrayOperation(std::unique_ptr<Identifier> arrName, std::unique_ptr<Node> id, std::unique_ptr<Node> rvalue)
            : arr(std::move(arrName)), index(std::move(id)), rhsExpression(std::move(rvalue)) {} // Used for store operations
        Value *codegen() override;
};


class Assignment : public Node {
    public:
        std::unique_ptr<Identifier> id;
        std::unique_ptr<Node> rhsExpression;
        Assignment(std::unique_ptr<Identifier> lhs, std::unique_ptr<Node> rhs) : 
            id(std::move(lhs)), rhsExpression(std::move(rhs)) { }
        Value *codegen() override;
};


class VariableDeclaration : public Node {
    public:
        std::unique_ptr<Identifier> type; //the type of the variable, id, does not change.
        std::unique_ptr<Identifier> id; // variable name
        std::unique_ptr<Node> assignmentExpr;

        VariableDeclaration(std::unique_ptr<Identifier> type, std::unique_ptr<Identifier> id) :
            type(std::move(type)), id(std::move(id)) { assignmentExpr = nullptr;}
        VariableDeclaration(std::unique_ptr<Identifier> type, std::unique_ptr<Identifier> id, std::unique_ptr<Node> assignmentExpr) :
            type(std::move(type)), id(std::move(id)), assignmentExpr(std::move(assignmentExpr)) { }
        Value *codegen() override;
};


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



class FunctionDefinition : public Node {
    public:
        std::unique_ptr<Prototype> prototype;
        std::vector<std::unique_ptr<Node>> Body;
        FunctionDefinition(std::unique_ptr<Prototype> proto, std::vector<std::unique_ptr<Node>> body) 
                :prototype(std::move(proto)), Body(std::move(body)){}
        Value *codegen() override;
};

#endif