#include "compiler.hpp"
#include <stdexcept>

namespace small_lang {

struct ExpressionVisitor {
    CompileContext& ctx;

    vresult_t to_bool(Value val) const {
        // originally created bool comparisons depending on type
        // we keep structure but move to Value
        if (val.type.t->isIntegerTy()) {
            llvm::Value* zero = llvm::ConstantInt::get(val.type.t, 0);
            Value out{};
            out.v = ctx.builder.CreateICmpNE(val.v, zero, "tobool");
            TODO;
            return out;
        }

        if (val.type.t->isFloatingPointTy()) {
            llvm::Value* zero = llvm::ConstantFP::get(val.type.t, 0.0);
            Value out{};
            out.v = ctx.builder.CreateFCmpONE(val.v, zero, "tobool");
            TODO;
            return out;
        }

        if (val.type.t->isPointerTy()) {
            llvm::Value* null = llvm::ConstantPointerNull::get(
                llvm::cast<llvm::PointerType>(val.type.t));
            Value out{};
            out.v = ctx.builder.CreateICmpNE(val.v, null, "tobool");
            TODO;
            return out;
        }

        return std::unexpected(CantBool{val.type.t});
    }

    vresult_t int_to_bool(Value val) const {
        llvm::Value* zero = llvm::ConstantInt::get(val.type.t, 0);
        Value out{};
        out.v = ctx.builder.CreateICmpNE(val.v, zero, "tobool");
        TODO;
        return out;
    }

    vresult_t operator()(const Invalid&) const {
        throw std::invalid_argument("uninit expression");
    }

    vresult_t operator()(const Num& n) const {
        Value out{};
        out.v = llvm::ConstantInt::getSigned(ctx.int_type.t, n.value);
        out.type.t = ctx.int_type.t;
        return out;
    }

    vresult_t operator()(const Var& v) const {
        if (auto it = ctx.local_var_addrs.find(v.text); it != ctx.local_var_addrs.end()) {
            Value out{};
            Value* addr = it->second.get();
            out.type = *addr->type.stored;
            out.v = ctx.builder.CreateLoad(out.type.t, addr->v, v.text);
            out.address = addr;
            return out;
        }
        if (auto it = ctx.global_consts.find(v.text); it != ctx.global_consts.end())
            return *it->second;
        return std::unexpected(MissingVar{v});
    }

    vresult_t operator()(const TypeCast&) const {
        TODO;
    }

    vresult_t operator()(const PreOp& pre_op) const {
	    vresult_t ra = ctx.compile(*pre_op.exp);
	    if (!ra) return ra;
	    Value a = *ra;

	    // Check: only integer types allowed for now
	    if (!a.type.t->isIntegerTy())
	        TODO; // non-integer preops not handled yet

	    Value out{};
	    out.type = a.type;

	    switch (pre_op.op.kind) {
	    case Operator::Plus:
	        return a;
	    case Operator::Minus:
	        out.v = ctx.builder.CreateNeg(a.v, "neg");
	        return out;
	    case Operator::Not: {
	        llvm::Value* zero = llvm::ConstantInt::get(a.type.t, 0);
	        out.v = ctx.builder.CreateICmpEQ(a.v, zero, "logical_not");
	        out.type = Type{out.v->getType(),nullptr,nullptr};
	        return out;
	    }
	    case Operator::Invalid:
	        throw std::invalid_argument("uninit preop expression");
	    default:
	        TODO;
	    }
	    return out;
	}

	

	vresult_t do_write(Value mem,Value b) const{
		if(mem.type.stored->t!=b.type.t)
			TODO//junk write

		ctx.builder.CreateStore(b.v, mem.v);
		return b;
	}

	vresult_t do_assign(Value a,Value b) const{
		if(!a.address) TODO;//junk assigment
		return do_write(*a.address,b);
		
	}

	vresult_t operator()(const BinOp& bin_op) const {
	    Value a, b;

	    // auto-mint specialization (degenerate assign)
	    if (bin_op.op.kind == Operator::Assign)
	    if (const auto var = std::get_if<Var>(&bin_op.a->inner))
	    if (ctx.local_var_addrs.find(var->text) == ctx.local_var_addrs.end()) {
	        vresult_t rb = ctx.compile(*bin_op.b);
	        if (!rb) return rb;
	        b = *rb;

	        auto slot = std::make_unique<Value>();
	        slot->v = ctx.builder.CreateAlloca(b.type.t, nullptr, var->text);
	        
	        Type* type = ctx.local_type_arena.emplace_back(std::make_unique<Type>(b.type)).get();//TODO better alocation scheme
	        slot->type = {slot->v->getType(),type,nullptr};
	        
	        ctx.builder.CreateStore(b.v, slot->v);
	        ctx.local_var_addrs[var->text] = std::move(slot);
	        return b;
	    }

	    vresult_t ra = ctx.compile(*bin_op.a);
	    if (!ra) return ra;
	    a = *ra;

	    vresult_t rb = ctx.compile(*bin_op.b);
	    if (!rb) return rb;
	    b = *rb;

	    // Check: currently only integer types allowed for all binops
	    if (!a.type.t->isIntegerTy() || a.type.t != b.type.t){
	        if(bin_op.op.kind == Operator::Assign)
	        	return do_assign(a,b);

	 		TODO
	    }

	    Value out{};
	    out.type = a.type;

	    switch (bin_op.op.kind) {
	    // --- arithmetic ---
	    case Operator::Plus:
	        out.v = ctx.builder.CreateAdd(a.v, b.v);
	        return out;
	    case Operator::Minus:
	        out.v = ctx.builder.CreateSub(a.v, b.v);
	        return out;
	    case Operator::Star:
	        out.v = ctx.builder.CreateMul(a.v, b.v);
	        return out;
	    case Operator::Slash:
	        out.v = ctx.builder.CreateSDiv(a.v, b.v);
	        return out;
	    case Operator::Percent:
	        out.v = ctx.builder.CreateSRem(a.v, b.v);
	        return out;

	    // --- comparison ---
	    case Operator::Lt:
	        out.v = ctx.builder.CreateICmpSLT(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return out;
	    case Operator::Gt:
	        out.v = ctx.builder.CreateICmpSGT(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return out;
	    case Operator::Le:
	        out.v = ctx.builder.CreateICmpSLE(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return out;
	    case Operator::Ge:
	        out.v = ctx.builder.CreateICmpSGE(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return out;
	    case Operator::EqEq:
	        out.v = ctx.builder.CreateICmpEQ(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return out;
	    case Operator::NotEq:
	        out.v = ctx.builder.CreateICmpNE(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return out;

	    // --- logical ---
	    case Operator::AndAnd: {
	        auto lhs = int_to_bool(a);
	        if (!lhs) return lhs;
	        auto rhs = int_to_bool(b);
	        if (!rhs) return rhs;
	        out.v = ctx.builder.CreateAnd(lhs->v, rhs->v, "andtmp");
	        out.type = lhs->type;
	        return out;
	    }
	    case Operator::OrOr: {
	        auto lhs = int_to_bool(a);
	        if (!lhs) return lhs;
	        auto rhs = int_to_bool(b);
	        if (!rhs) return rhs;
	        out.v = ctx.builder.CreateOr(lhs->v, rhs->v, "ortmp");
	        out.type = lhs->type;
	        return out;
	    }

	    // --- bitwise ---
	    case Operator::BitAnd:
	        out.v = ctx.builder.CreateAnd(a.v, b.v);
	        return out;
	    case Operator::BitOr:
	        out.v = ctx.builder.CreateOr(a.v, b.v);
	        return out;
	    case Operator::BitXor:
	        out.v = ctx.builder.CreateXor(a.v, b.v);
	        return out;

	    // --- assignment ---
	    case Operator::Assign: {
	        TODO//error a is an int
	    }

	    case Operator::Invalid:
	        throw std::invalid_argument("uninit binop expression");

	    default:
	        TODO;
	    }

	    return out; // only reached for TODOs / incomplete branches
	}




    vresult_t operator()(const SubScript&) const {
        TODO;
    }

    vresult_t operator()(const Call& c) const {
        vresult_t rf = ctx.compile(*c.func);
        if (!rf) return rf;
        Value fn = *rf;

        // fn.v should be a function pointer, type should be FunctionType
        TODO;
    }
};

vresult_t CompileContext::compile(const Expression& exp) {
    return std::visit(ExpressionVisitor{*this}, exp.inner);
}

struct StatmentVisitor {
    CompileContext& ctx;

    result_t compile_block(const Block& b) const {
        for (auto& stmt : b.parts) {
            result_t r = ctx.compile(stmt);
            if (!r) return r;
        }
        return {};
    }

    result_t operator()(const Invalid&) const {
        throw std::invalid_argument("uninit statement");
    }

    result_t operator()(const While&) const { TODO; }

    result_t operator()(const If&) const { TODO; }

    result_t operator()(const Return& r) const {
        vresult_t value = ctx.compile(r.val);
        if (!value)
            return std::unexpected<CompileError>(std::move(value).error());
        ctx.builder.CreateRet(value->v);
        return {};
    }

    result_t operator()(const Block& b) const { return compile_block(b); }

    result_t operator()(const Basic& b) const {
        return ctx.compile(b.inner).transform([](auto&&) {});
    }
};

result_t CompileContext::compile(const Statement& stmt) {
    result_t r = std::visit(StatmentVisitor{*this}, stmt.inner);
    if (r || std::holds_alternative<StatmentError>(r.error())) return r;
    return std::unexpected(
        StatmentError{stmt, std::make_unique<CompileError>(std::move(r).error())});
}

struct GlobalVisitor {
    CompileContext& ctx;

    Value* generate_func(const FuncDec& dec) const {
        std::vector<llvm::Type*> arg_llvm_types;
        std::vector<Type> arg_types;
        
        for (auto& a : dec.args){
            arg_types.push_back(ctx.int_type);
            arg_llvm_types.push_back(ctx.int_type.t);
        }

        Type ret = ctx.int_type;

        auto sig = llvm::FunctionType::get(ret.t, arg_llvm_types, false);
        llvm::Function* fn = llvm::Function::Create(
            sig, llvm::Function::ExternalLinkage, dec.name.text, *ctx.mod);

        if (dec.is_c)
            fn->setCallingConv(llvm::CallingConv::C);
        else
            fn->setCallingConv(llvm::CallingConv::Fast);


        auto* ft = ctx.func_defs.emplace_back(std::make_unique<FunctionType>(
        		FunctionType{
        			 fn->getFunctionType(),
        			 fn->getCallingConv(),
        			 ret,
        			 std::move(arg_types),
        		}
        	)).get();

        auto val = std::make_unique<Value>(
        	Value{fn,
        		Type{fn->getType(),nullptr,ft}
        	}
        );

        Value* ans = val.get();
        ctx.global_consts[dec.name.text] = std::move(val);
        return ans;
    }

    result_t operator()(const Invalid&) const {
        throw std::invalid_argument("uninit global statement");
    }

    result_t operator()(const FuncDec& dec) const {
        generate_func(dec);
        return {};
    }

    result_t operator()(const Function& f) const {
        Value* fn_val = generate_func(f);
        llvm::Function* fn = static_cast<llvm::Function*>(fn_val->v);
        FunctionType& fn_type = *fn_val->type.func;

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(*ctx.ctx, "entry", fn);
        ctx.builder.SetInsertPoint(entry);

        ctx.clear_locals();
        
        assert(f.args.size()==fn_type.args.size());

        auto it = f.args.begin();
        auto it_types = fn_type.args.begin();

        for (llvm::Argument& arg : fn->args()) {
            auto slot = std::make_unique<Value>();
            slot->v = ctx.builder.CreateAlloca(it_types->t, nullptr, it->text);
            slot->type.t = slot->v->getType();
            slot->type.stored = &*it_types;

            ctx.builder.CreateStore(&arg, slot->v);
            ctx.local_var_addrs[it->text] = std::move(slot);
            ++it;
            ++it_types;
        }
        assert(it == f.args.end());

        for (auto& stmt : f.body.parts) {
            result_t r = ctx.compile(stmt);
            if (!r) return r;
        }

        if (f.body.parts.empty() ||
            !std::holds_alternative<Return>(f.body.parts.back().inner))
            TODO;

        return {};
    }

    result_t operator()(const Basic& b) const { return StatmentVisitor{ctx}(b); }
};

result_t CompileContext::compile(const Global& global) {
    return std::visit(GlobalVisitor{*this}, global.inner);
}

} // namespace small_lang
