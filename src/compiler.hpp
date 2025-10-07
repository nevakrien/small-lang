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

	struct NonSenseOp {
		const Expression& exp;
	};

	using CompileError = std::variant<MissingVar,NonSenseOp>;

	using vresult_t = std::expected<llvm::Value*,CompileError>;
	using result_t = std::expected<void,CompileError>;

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
	    std::map<std::string_view, llvm::Value*> vars;
};

}//small_lang