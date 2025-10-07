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
       	if (auto it = ctx.vars.find(v.text); it != ctx.vars.end())
	        return ctx.builder.CreateLoad(it->second->getAllocatedType(), it->second, v.text);
	    if (auto it = ctx.consts.find(v.text); it != ctx.consts.end())
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

	vresult_t operator()(const Call& c) const {
	    // compile the function expression
	    vresult_t r = ctx.compile(*c.func);
	    if (!r) return r;

	    llvm::Function* fn = llvm::dyn_cast<llvm::Function>(*r);
	    if (!fn)
	        return std::unexpected(NotAFunction{*c.func, (*r)->getType()});

	    llvm::FunctionType* fnty = fn->getFunctionType();

	   	// check argument count
	    if (fnty->getNumParams() != c.args.size())
	        return std::unexpected(WrongArgCount{c, fnty});


	    // compile arguments
	    std::vector<llvm::Value*> args;
	    args.reserve(c.args.size());
	    for (auto& arg : c.args) {
	        vresult_t v = ctx.compile(arg);
	        if (!v) return v;
	        args.push_back(*v);
	    }


	    // check argument types
	    for (size_t i = 0; i < fnty->getNumParams(); ++i) {
	        llvm::Type* expected = fnty->getParamType(i);
	        llvm::Type* got = args[i]->getType();
	        if (expected != got)
	            return std::unexpected(BadType{c.args[i], expected, got});
	    }

	    // all good, emit call
	    return ctx.builder.CreateCall(fnty, fn, args, "function call");
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
			if(!r) return r;
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

		ctx.consts[dec.name.text] = fn;

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


		ctx.vars.clear();

		auto it = f.args.begin();
		for (llvm::Argument &arg : fn->args()) {
		    llvm::AllocaInst* slot =
		        ctx.builder.CreateAlloca(ctx.int_type, nullptr, it->text);

		    ctx.builder.CreateStore(&arg, slot);
		    ctx.vars[it->text] = slot;
		    ++it;
		}

		assert(it == f.args.end());


		for(auto it =f.body.parts.begin();it!=f.body.parts.end();it++){
			result_t r = ctx.compile(*it);
			if(!r) return r;
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