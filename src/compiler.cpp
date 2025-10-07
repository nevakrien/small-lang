#include "compiler.hpp"
#include <stdexcept>
	
namespace small_lang {


struct ExpressionVisitor {
    CompileContext& ctx;

    vresult_t operator()(const Invalid&) const {
        throw std::invalid_argument("uninit expression");
    }

    vresult_t operator()(const Num& n) const {
        return llvm::ConstantInt::getSigned(ctx.int_type,n.value);
    }

    vresult_t operator()(const Var& v) const {
       	auto it = ctx.vars.find(v.text);
       	if(it != ctx.vars.end())
       		return it->second;

       	return  std::unexpected(MissingVar{v});       	
    } 

    vresult_t operator()(const PreOp&) const {
        TODO
    }

    vresult_t operator()(const BinOp&) const {
        TODO
    }

    vresult_t operator()(const SubScript&) const {
        TODO
    }

    vresult_t operator()(const Call&) const {
        TODO
    }
};

vresult_t CompileContext::compile(const Expression& exp) {
    return std::visit(ExpressionVisitor{*this}, exp.inner);
}


struct StatmentVisitor {
    CompileContext& ctx;

    result_t operator()(const Invalid&) const {
        throw std::invalid_argument("uninit statment");
    }

    result_t operator()(const While&) const {
        TODO
    }

    result_t operator()(const If&) const {
        TODO
    }

    result_t operator()(const Return& r) const {
        vresult_t value = ctx.compile(r.val);
        if(!value) 
        	return std::unexpected(value.error());

        ctx.builder.CreateRet(*value);
        return {};
    }

    result_t operator()(const Block& b) const {
        for(auto it =b.parts.begin();it!=b.parts.end();it++){
			result_t r = ctx.compile(*it);
			if(r) return r;
		}

		return {};
    }

    result_t operator()(const Basic& b) const {
        return ctx.compile(b.inner).transform([](auto&&) {});
    }
};

result_t CompileContext::compile(const Statement& stmt) {
    return std::visit(StatmentVisitor{*this}, stmt.inner);
}



struct GlobalVisitor {
    CompileContext& ctx;
    
    llvm::Function* generate_func(const FuncDec& dec) const {
    	std::vector<llvm::Type*> arg_types; //can be optimized
    	for(auto it = dec.args.begin();it!=dec.args.end();it++){
    		arg_types.push_back(ctx.int_type);
    	}

    	auto sig = llvm::FunctionType::get(ctx.int_type, arg_types,false);

    	llvm::Function* fn = llvm::Function::Create(
		    sig,
		    llvm::Function::ExternalLinkage,//we figure this later
		    dec.name.text,
		    *ctx.mod
		);

    	if(dec.is_c)
			fn->setCallingConv(llvm::CallingConv::C);

		return fn;
	}


    result_t operator()(const Invalid&) const {
        throw std::invalid_argument("uninit global statment");
    }

    result_t operator()(const FuncDec& dec) const {
    	generate_func(dec);
    	return {};
    }

    result_t operator()(const Function& f) const {
        llvm::Function* fn = generate_func(f);
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(*ctx.ctx, "entry", fn);
		ctx.builder.SetInsertPoint(entry);

		for(auto it =f.body.parts.begin();it!=f.body.parts.end();it++){
			result_t r = ctx.compile(*it);
			if(r) return r;
		}

		if(f.body.parts.empty() || !std::holds_alternative<Return>(f.body.parts.back().inner))
			TODO

		return {};

    }

    result_t operator()(const Basic& b) const {
        return StatmentVisitor{ctx}(b);
    }
};

result_t CompileContext::compile(const Global& global) {
    return std::visit(GlobalVisitor{*this}, global.inner);
}



}//small_lang