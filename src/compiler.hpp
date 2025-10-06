#pragma once
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <map>
#include <string>
#include <expected>

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
	        : ctx(),
	          builder(ctx),
	          mod(std::move(name), ctx),
	          int_type(llvm::Type::getInt64Ty(ctx))
	    {}

	    vresult_t compile(const Expression& exp);
	    result_t compile(const Statement& stmt);
	    result_t compile(const Global& global);


	    llvm::LLVMContext ctx;
	    llvm::IRBuilder<> builder;
	    llvm::Module mod;
	    llvm::IntegerType* int_type;
	    std::map<std::string_view, llvm::Value*> vars;
};

}//small_lang