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

	struct MissingVar {
		Var var;
	};

	struct NotAFunction {
	    const Expression& exp;
	    llvm::Type* got;
	};

	struct WrongArgCount {
	    const Call& call;
	    llvm::FunctionType* t;//can give count
	};


	struct BadType {
		const Expression& made;
	    llvm::Type* expected;
	    llvm::Type* got;
	};

	
	struct StatmentError;

	using CompileError = std::variant<MissingVar,NotAFunction,BadType,WrongArgCount,StatmentError>;
	struct StatmentError {
		const Statement& parent;
		std::unique_ptr<CompileError> source;
	};
	// struct HeledCompilerError{
	// 	std::unique_ptr<CompileError> inner;
	// 	HeledCompilerError(CompileError e) : inner(std::make_unique<CompileError>(std::move(e))) {}
	// 	const CompileError* get(){
	// 		return *inner;
	// 	}
	// };

	using vresult_t = std::expected<llvm::Value*,CompileError>;
	using result_t = std::expected<void,CompileError>;

	using VarEntry = std::variant<
	    llvm::AllocaInst*,     // stack-allocated variable
	    llvm::GlobalVariable*, // global
	    llvm::Value*           // direct LLVM value (const, func, etc.)
	>;

	struct CompileContext {
	    CompileContext(std::string name)
	        : ctx(std::make_unique<llvm::LLVMContext>()),
	          mod(std::make_unique<llvm::Module>(std::move(name), *ctx)),
	          builder(*ctx),
	          int_type(llvm::Type::getInt64Ty(*ctx))
	    {}

	    vresult_t compile(const Expression& exp);
	    result_t compile(const Statement& stmt);
	    result_t compile(const Global& global);


	    std::unique_ptr<llvm::LLVMContext> ctx;
	    std::unique_ptr<llvm::Module> mod;

	    llvm::IRBuilder<> builder;
	    llvm::IntegerType* int_type;
	    std::map<std::string_view, llvm::AllocaInst*> vars;
	    std::map<std::string_view, llvm::Value*> consts;
};

}//small_lang