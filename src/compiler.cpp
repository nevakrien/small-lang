#include "compiler.hpp"
#include <stdexcept>
	
namespace small_lang {


struct ExpressionVisitor {
    CompileContext& ctx;

    vresult_t to_bool(llvm::Value* val) const{
	    llvm::Type* ty = val->getType();

	    if (ty->isIntegerTy()) {
	        llvm::Value* zero = llvm::ConstantInt::get(ty, 0);
	        return ctx.builder.CreateICmpNE(val, zero, "tobool");
	    }

	    if (ty->isFloatingPointTy()) {
	        llvm::Value* zero = llvm::ConstantFP::get(ty, 0.0);
	        return ctx.builder.CreateFCmpONE(val, zero, "tobool");
	    }

	    if (ty->isPointerTy()) {
	        llvm::Value* null = llvm::ConstantPointerNull::get(
	            llvm::cast<llvm::PointerType>(ty));
	        return ctx.builder.CreateICmpNE(val, null, "tobool");
	    }

	    return  std::unexpected(CantBool{val->getType()});
	}

	llvm::Value* int_to_bool(llvm::Value* val) const{
	    // Compare val != 0  → returns i1
	    llvm::Value* zero = llvm::ConstantInt::get(val->getType(), 0);
	    return ctx.builder.CreateICmpNE(val, zero, "tobool");
	}


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

    vresult_t operator()(const PreOp& pre_op) const {
        vresult_t ra = ctx.compile(*pre_op.exp);
    	if(!ra) return ra;
    	llvm::Value *a = *ra;

    	switch(pre_op.op.kind){
    	case Operator::Plus:
    		return a;
    	case Operator::Minus:
    		return ctx.builder.CreateNeg(a, "neg");
    	case Operator::Not:{
		    llvm::Value* zero = llvm::ConstantInt::get(a->getType(), 0);
		    return ctx.builder.CreateICmpEQ(a, zero, "logical_not");
		}
		case Operator::Invalid:
    		throw std::invalid_argument("uninit preop expression");
    		break;
    	default:
    		TODO
    	};
    }

    vresult_t operator()(const BinOp& bin_op) const {
    	llvm::Value *a,*b;

    	//auto mint specilization for degenracy
    	if(bin_op.op.kind==Operator::Assign) 
    	if (const auto var = std::get_if<Var>(&bin_op.a->inner))
    	if (ctx.vars.find(var->text)==ctx.vars.end())
    		{

    		vresult_t rb = ctx.compile(*bin_op.b);
	    	if(!rb) return rb;
	    	b = *rb;

	    	llvm::AllocaInst* slot =
		        ctx.builder.CreateAlloca(b->getType(), nullptr, var->text);

		    ctx.builder.CreateStore(b, slot);
		    ctx.vars[var->text] = slot;

	    	return b;
    	}

    	//all operators have these semantics so we init here
    	vresult_t ra = ctx.compile(*bin_op.a);
    	if(!ra) return ra;
    	a = *ra;

    	vresult_t rb = ctx.compile(*bin_op.b);
    	if(!rb) return rb;
    	b = *rb;

    	switch(bin_op.op.kind){
    	case Operator::Plus:
    		return ctx.builder.CreateAdd(a,b);//TODO we probably wana check types
    	case Operator::Minus:
            return ctx.builder.CreateSub(a, b);
        case Operator::Star:
            return ctx.builder.CreateMul(a, b);
        case Operator::Slash:
            return ctx.builder.CreateSDiv(a, b); // signed div for now
        case Operator::Percent:
            return ctx.builder.CreateSRem(a, b); // signed remainder

        // comparison (integer)
        case Operator::Lt:
            return ctx.builder.CreateICmpSLT(a, b);
        case Operator::Gt:
            return ctx.builder.CreateICmpSGT(a, b);
        case Operator::Le:
            return ctx.builder.CreateICmpSLE(a, b);
        case Operator::Ge:
            return ctx.builder.CreateICmpSGE(a, b);
        case Operator::EqEq:
            return ctx.builder.CreateICmpEQ(a, b);
        case Operator::NotEq:
            return ctx.builder.CreateICmpNE(a, b);

        //logical
        case Operator::AndAnd:
		    return ctx.builder.CreateAnd(int_to_bool(a), int_to_bool(b), "andtmp");
		case Operator::OrOr:
		    return ctx.builder.CreateOr(int_to_bool(a), int_to_bool(b), "ortmp");

        // bitwise
        case Operator::BitAnd:
            return ctx.builder.CreateAnd(a, b);
        case Operator::BitOr:
            return ctx.builder.CreateOr(a, b);
        case Operator::BitXor:
            return ctx.builder.CreateXor(a, b);


    	case Operator::Assign:{
    		llvm::Value* ptr = nullptr;
    		llvm::Type*  dest_ty = nullptr;

		    if (auto* load = llvm::dyn_cast<llvm::LoadInst>(a)) {
		        ptr = load->getPointerOperand();
		        dest_ty = load->getType();
		    } else {
		    	TODO
		    }
		    llvm::Type* src_ty = b->getType();

		    if (dest_ty != src_ty) {
		        if (dest_ty->isIntegerTy() && src_ty->isIntegerTy()) {
		            b = ctx.builder.CreateIntCast(b, dest_ty, /*isSigned=*/true, "assign_intcast");
		        } else if (dest_ty->isFloatingPointTy() && src_ty->isIntegerTy()) {
		            b = ctx.builder.CreateSIToFP(b, dest_ty, "assign_sitofp");
		        } else if (dest_ty->isIntegerTy() && src_ty->isFloatingPointTy()) {
		            b = ctx.builder.CreateFPToSI(b, dest_ty, "assign_fptosi");
		        } else {
		            return std::unexpected(BadType{bin_op,dest_ty,src_ty});
		        }
		    }

		    ctx.builder.CreateStore(b, ptr);
		    //returning b has the same semantics of C more or less
		    return b;
		}
    	case Operator::Invalid:
    		throw std::invalid_argument("uninit binop expression");
    		break;
    	default:
    		TODO
    	}
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
	    llvm::CallInst* call = ctx.builder.CreateCall(fnty, fn, args, "function call");
		call->setCallingConv(fn->getCallingConv());
		return call;
	}



};

vresult_t CompileContext::compile(const Expression& exp) {
    return std::visit(ExpressionVisitor{*this}, exp.inner);
}


struct StatmentVisitor {
    CompileContext& ctx;
    
    result_t compile_block(const Block& b) const {
        for(auto it =b.parts.begin();it!=b.parts.end();it++){
			result_t r = ctx.compile(*it);
			if(!r) return r;
		}

		return {};
    }

    result_t operator()(const Invalid&) const {
        throw std::invalid_argument("uninit statment");
    }

    result_t operator()(const While&) const {
        TODO
    }

    result_t operator()(const If& i) const {
        vresult_t rcond = ctx.compile(i.cond);
        if(!rcond) return std::unexpected(std::move(rcond).error());

        vresult_t rcond_bool = ExpressionVisitor{ctx}.to_bool(*rcond);
        if(!rcond_bool) std::unexpected(std::move(rcond_bool).error());

        llvm::Value* cond = *rcond_bool;
       	llvm::Function* func = ctx.builder.GetInsertBlock()->getParent();

		auto bthen  = llvm::BasicBlock::Create(*ctx.ctx, "then", func);
		auto belse  = llvm::BasicBlock::Create(*ctx.ctx, "else", func);
		auto bmerge = llvm::BasicBlock::Create(*ctx.ctx, "merge", func);
        ctx.builder.CreateCondBr(cond, bthen, belse);

        ctx.builder.SetInsertPoint(bthen);
        result_t rthen = compile_block(i.block);
        if(!rthen) return rthen;
        if (!bthen->getTerminator())
        	ctx.builder.CreateBr(bmerge);

        ctx.builder.SetInsertPoint(belse);
        result_t relse = compile_block(i.else_part);
        if(!relse) return relse;
        if (!belse->getTerminator())
        	ctx.builder.CreateBr(bmerge);

        ctx.builder.SetInsertPoint(bmerge);
        return {};
    }

    result_t operator()(const Return& r) const {
        vresult_t value = ctx.compile(r.val);
        if(!value) 
        	return std::unexpected<CompileError>( std::move(value).error());
        

        ctx.builder.CreateRet(*value);
        return {};
    }

    result_t operator()(const Block& b) const {
        return compile_block(b);
    }

    result_t operator()(const Basic& b) const {
        return ctx.compile(b.inner).transform([](auto&&) {});
    }
};

result_t CompileContext::compile(const Statement& stmt) {
    result_t r = std::visit(StatmentVisitor{*this}, stmt.inner);
    if(r || std::holds_alternative<StatmentError>(r.error())) return r;

    return std::unexpected(StatmentError{stmt,std::make_unique<CompileError>(std::move(r).error())});
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

    	if (dec.is_c)
		    fn->setCallingConv(llvm::CallingConv::C);
		else
		    fn->setCallingConv(llvm::CallingConv::Fast);

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