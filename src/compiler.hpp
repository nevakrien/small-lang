#pragma once
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <map>
#include <string>
#include <expected>
#include <memory>

#include "ast.hpp"


namespace small_lang {


struct FunctionType;
struct Type {
	llvm::Type* t;
	
	//optionals (live in function/global storage)
	Type* stored;
	FunctionType* func;
};

struct FunctionType {
	llvm::FunctionType* ft;
	llvm::CallingConv::ID cc;

	Type ret;
	std::vector<Type> args;//dont realloc this
};

struct Value {
	llvm::Value* v;
	Type type;

	//optionals (live in function/global storage)
	Value* address;
};

//=============ERRORS=========
struct MissingVar {
	Var var;
};

struct NotAFunction {
    const Expression& exp;
    Type got;
};

struct CantBool {
    Type got;
};

struct WrongArgCount {
    const Call& call;
    FunctionType* t;//can give count
};

template <typename T>
struct BadType {
    const T& made;
    Type expected;
    Type got;
};


struct StatmentError;

using CompileError = std::variant<MissingVar,NotAFunction,CantBool,BadType<Expression>,BadType<BinOp>,BadType<Return>,WrongArgCount,StatmentError>;
struct StatmentError {
	const Statement& parent;
	std::unique_ptr<CompileError> source;
};

// using vresult_t = std::expected<llvm::Value*,CompileError>;
using vresult_t = std::expected<Value,CompileError>;
using result_t = std::expected<void,CompileError>;


struct CompileContext {
    CompileContext(std::string name)
        : ctx(std::make_unique<llvm::LLVMContext>()),
          mod(std::make_unique<llvm::Module>(std::move(name), *ctx)),
          builder(*ctx),
          int_type(Type{llvm::Type::getInt64Ty(*ctx),nullptr,nullptr}),
          bool_type(Type{llvm::Type::getInt1Ty(*ctx), nullptr, nullptr})
    {}

    vresult_t compile(const Expression& exp);
    result_t compile(const Statement& stmt);
    result_t compile(const Global& global);


    FunctionType* current_func = nullptr;
    std::unique_ptr<llvm::LLVMContext> ctx;
    std::unique_ptr<llvm::Module> mod;

    llvm::IRBuilder<> builder;
    Type int_type;
    Type bool_type;
    // std::map<std::string_view, llvm::AllocaInst*> vars;
    // std::map<std::string_view, llvm::Value*> consts;

    std::map<std::string_view, std::unique_ptr<Value>> local_var_addrs;
    std::map<std::string_view, std::unique_ptr<Value>> global_consts;
    std::vector<std::unique_ptr<Type>> local_type_arena;
    std::vector<std::unique_ptr<Value>> local_arena;
    std::vector<std::unique_ptr<FunctionType>> func_defs;    

    void clear_locals(){
    	local_var_addrs.clear();
    	local_type_arena.clear();
    	local_arena.clear();
    }
};

}//small_lang