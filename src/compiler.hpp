#pragma once
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <map>
#include <string>
#include <expected>

#include "ast.hpp"

namespace small_lang {

	struct CompileError{

	};

	class CompileContext {
	public:
	    CompileContext(std::string name)
	        : ctx(),
	          builder(ctx),
	          mod(std::move(name), ctx)
	    {}

	    std::expected<llvm::Value*,CompileError> compile();

	private:
	    llvm::LLVMContext ctx;
	    llvm::IRBuilder<> builder;
	    llvm::Module mod;
	    std::map<std::string, llvm::Value*> namedValues;
	};

}//small_lang