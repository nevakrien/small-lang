#include "compiler.hpp"
#include "utils.hpp"
#include <stdexcept>
#include <algorithm>

#include "ir_print.hpp"


#define FORWARD_UNEXPECTED(src) std::unexpected(std::move(src).error())


static std::string concat_statements(std::string_view a, std::string_view b) {
    std::string combined;
    combined.reserve(a.size() + b.size());

    // Copy first chunk as-is
    combined.append(a.data(), a.size());

    // Copy second, collapsing whitespace runs to ≤4
    size_t run = 0;
    size_t len = std::min(b.size(),(size_t)100);

    for (size_t i =0;i<len;i++) {
        if (std::isspace(static_cast<unsigned char>(b[i]))) {
            if (run < 4) {
                combined.push_back(' '); // normalize all whitespace to a single space
                ++run;
            }
        } else {
            run = 0;
            combined.push_back(b[i]);
        }
    }

    return combined;
}



namespace small_lang {

Type* CompileContext::get_type(const TypeDec& t){
	if(t.name=="bool")
		return &bool_type;

	if(t.name=="int")
		return &int_type;

	return nullptr;
}

struct VisitorBase{
	CompileContext& ctx;

	bool types_exactly_equal(const Type& a, const Type& b) const {
	    if (a.t == b.t)
	        return true;

	    if (a.t->getTypeID() != b.t->getTypeID())
	        return false;

	    // function type comparison
	    if (a.func && b.func) {
	        const FunctionType* fa = a.func;
	        const FunctionType* fb = b.func;

	        if (fa->cc != fb->cc)
	            return false;
	        if (!types_exactly_equal(fa->ret, fb->ret))
	            return false;
	        if (fa->args.size() != fb->args.size())
	            return false;

	        for (size_t i = 0; i < fa->args.size(); ++i)
	            if (!types_exactly_equal(fa->args[i], fb->args[i]))
	                return false;

	        return true;
	    }

	    if (a.stored && b.stored)
	        return types_exactly_equal(*a.stored, *b.stored);

	    //TODO extend to support aggregate/struct comparison
	    return false;
	}

	void promote_integer_pair(Value& a, Value& b,bool isSigned = false) const {
	    auto* ta = llvm::cast<llvm::IntegerType>(a.type.t);
	    auto* tb = llvm::cast<llvm::IntegerType>(b.type.t);

	    unsigned wa = ta->getBitWidth();
	    unsigned wb = tb->getBitWidth();

	    if (wa == wb)
	        return; // already same width

	    // choose larger width
	    unsigned target_width = std::max(wa, wb);
	    llvm::Type* target_type = llvm::Type::getIntNTy(*ctx.ctx, target_width);

	    if (wa < target_width) {
	        a.v = ctx.builder.CreateIntCast(a.v, target_type,isSigned, "cast_up_a");
	        a.type = Type{target_type,nullptr,nullptr};
	    }

	    if (wb < target_width) {
	        b.v = ctx.builder.CreateIntCast(b.v, target_type, isSigned, "cast_up_b");
	        b.type = Type{target_type,nullptr,nullptr};
	    }
	}

	template <typename D>
	result_t implicit_cast(Value& val, const Type& target_type,const D& debug,bool isSigned = false) const {
	    llvm::Type* src = val.type.t;
	    llvm::Type* dst = target_type.t;

	    // only handle integers for now
	    if (src->isIntegerTy() && dst->isIntegerTy()) {
	        unsigned sw = llvm::cast<llvm::IntegerType>(src)->getBitWidth();
	        unsigned dw = llvm::cast<llvm::IntegerType>(dst)->getBitWidth();

	        if (sw == dw)
	            return {}; // same width, nothing to do

	        if (sw < dw) {
	            val.v = ctx.builder.CreateIntCast(val.v, dst,isSigned, "int_extend");
	            val.type = Type{dst,nullptr,nullptr};
	            return {};
	        }

	        // narrowing is not implicit
	        return std::unexpected(BadType<D>{debug, target_type, val.type});
	    }

	    // TODO: add the rest
	    return std::unexpected(BadType<D>{debug, target_type, val.type});
	}

	template <typename D>
	result_t exiplicit_cast(Value& val, const Type& target_type,const D& debug,bool isSigned = false) const {
	    llvm::Type* src = val.type.t;
	    llvm::Type* dst = target_type.t;

	    // only handle integers for now
	    if (src->isIntegerTy() && dst->isIntegerTy()) {
	        unsigned sw = llvm::cast<llvm::IntegerType>(src)->getBitWidth();
	        unsigned dw = llvm::cast<llvm::IntegerType>(dst)->getBitWidth();

	        if (sw == dw)
	            return {}; // same width, nothing to do

	        val.v = ctx.builder.CreateIntCast(val.v, dst, isSigned, "int_extend");
            val.type = Type{dst,nullptr,nullptr};
            return {};
	    }

	    // TODO: add the rest
	    return std::unexpected(BadType<D>{debug, target_type, val.type});
	}

    vresult_t to_bool(Value val) const {
        // originally created bool comparisons depending on type
        // we keep structure but move to Value
        Value out{};
        out.type = ctx.bool_type;

        if (val.type.t->isIntegerTy()) {
            llvm::Value* zero = llvm::ConstantInt::get(val.type.t, 0);
            out.v = ctx.builder.CreateICmpNE(val.v, zero, "tobool");
            return out;
        }

        if (val.type.t->isFloatingPointTy()) {
            llvm::Value* zero = llvm::ConstantFP::get(val.type.t, 0.0);
            out.v = ctx.builder.CreateFCmpONE(val.v, zero, "tobool");
            return out;
        }

        if (val.type.t->isPointerTy()) {
            llvm::Value* null = llvm::ConstantPointerNull::get(
                llvm::cast<llvm::PointerType>(val.type.t));
            out.v = ctx.builder.CreateICmpNE(val.v, null, "tobool");
            return out;
        }

        return std::unexpected(CantBool{val.type.t,nullptr,nullptr});
    }
};

struct ExpressionVisitor : VisitorBase{
	Value& out;
    result_t operator()(const Invalid&) const {
        throw std::invalid_argument("uninit expression");
    }

    result_t operator()(const Num& n) const {
        out.v = llvm::ConstantInt::getSigned(ctx.int_type.t, n.value);
        out.type.t = ctx.int_type.t;
        return {};
    }

    result_t operator()(const Var& v) const {
        if (auto it = ctx.local_var_addrs.find(v.text); it != ctx.local_var_addrs.end()) {
            Value* addr = it->second.get();
            out.type = *addr->type.stored;
            out.v = ctx.builder.CreateLoad(out.type.t, addr->v, v.text);
            out.address = addr;
            return {};
        }
        if (auto it = ctx.global_consts.find(v.text); it != ctx.global_consts.end()){
        	out = *it->second;
            return {};
        }
        return std::unexpected(MissingVar{v});
    }

    result_t operator()(const TypeCast& cast) const {
    	Type* type = ctx.get_type(cast.type);
    	if(!type)
        	TODO;

        result_t r = ctx.compile(*cast.exp,out);
        if(!r) return FORWARD_UNEXPECTED(r);

        result_t r2 = exiplicit_cast(out,*type,cast);
        if(!r2) return FORWARD_UNEXPECTED(r2);
        return {};
    }

    result_t pointer_preop(Value a,const PreOp& pre_op) const{
	    switch (pre_op.op.kind) {
	    case Operator::BitAnd:{
	    	if(!a.address)
	    		TODO

	    	out = *a.address;
	    	return {};
	    }
	    case Operator::Star:{
	    	if(!a.type.stored)
	    		TODO

	        out.type = *a.type.stored;
	        out.v = ctx.builder.CreateLoad(out.type.t, a.v);

	        auto au = std::make_unique<Value>(a);
	        out.address = au.get();
	        ctx.local_arena.emplace_back(std::move(au));
	        return {};
	    }
	    case Operator::Not: {
	        llvm::Value* null = llvm::ConstantPointerNull::get(
                llvm::cast<llvm::PointerType>(a.type.t));
            out.v = ctx.builder.CreateICmpNE(a.v, null, "not");
            out.type = ctx.bool_type;
            return {};
        }
	    case Operator::Invalid:
	        throw std::invalid_argument("uninit preop expression");

	    default:
	        TODO;
	    }
	    return {};
    }

    result_t operator()(const PreOp& pre_op) const {
	    Value a;
	    result_t r = ctx.compile(*pre_op.exp,a);
	    if (!r) return FORWARD_UNEXPECTED(r);

	    if(a.type.t->isPointerTy())
	    	return pointer_preop(a,pre_op);

	    // Check: only integer types allowed for now
	    if (!a.type.t->isIntegerTy())
	        TODO; // non-integer preops not handled yet

	    out.type = a.type;

	    switch (pre_op.op.kind) {
	    case Operator::BitAnd:{
	    	if(!a.address)
	    		TODO

	    	out = *a.address;
	    	return {};
	    }
	    case Operator::Plus:{
	    	out = a;
	        return {};
	    }
	    case Operator::Minus:
	        out.v = ctx.builder.CreateNeg(a.v, "neg");
	        return {};
	    case Operator::Not: {
	        llvm::Value* zero = llvm::ConstantInt::get(a.type.t, 0);
	        out.v = ctx.builder.CreateICmpEQ(a.v, zero, "logical_not");
	        out.type = Type{out.v->getType(),nullptr,nullptr};
	        return {};
	    }
	    case Operator::Invalid:
	        throw std::invalid_argument("uninit preop expression");

	    default:
	    	//std::cout << pre_op;
	        TODO;
	    }
	    return {};
	}


	result_t operator()(const BinOp& bin_op) const {
	    Value a, b;

	    // auto-mint specialization (degenerate assign)
	    if (bin_op.op.kind == Operator::Assign)
	    if (const auto var = std::get_if<Var>(&bin_op.a->inner))
	    if (ctx.local_var_addrs.find(var->text) == ctx.local_var_addrs.end()) {
	        result_t rb = ctx.compile(*bin_op.b,b);
	        if (!rb) return FORWARD_UNEXPECTED(rb);

	        auto slot = std::make_unique<Value>();
	        slot->v = ctx.builder.CreateAlloca(b.type.t, nullptr, var->text);
	        
	        Type* type = ctx.local_type_arena.emplace_back(std::make_unique<Type>(b.type)).get();//TODO better alocation scheme
	        slot->type = {slot->v->getType(),type,nullptr};
	        
	        ctx.builder.CreateStore(b.v, slot->v);
	        ctx.local_var_addrs[var->text] = std::move(slot);
	        out = b;
	        return {};
	    }

	    result_t ra = ctx.compile(*bin_op.a,a);
	    if (!ra) return FORWARD_UNEXPECTED(ra);

	    result_t rb = ctx.compile(*bin_op.b,b);
	    if (!rb) return FORWARD_UNEXPECTED(rb);

	    if(bin_op.op.kind == Operator::Assign){
	    	if(!a.address) TODO;//junk assigment
			Value& mem = *a.address;
			result_t r = implicit_cast(b,*mem.type.stored,bin_op);
			if(!r) return FORWARD_UNEXPECTED(r);

			ctx.builder.CreateStore(b.v, mem.v);
			out = b;
			return {};
	    }

	  	// --- type normalization ---
		if (a.type.t->isIntegerTy() && b.type.t->isIntegerTy()) {
		    promote_integer_pair(a, b);
		} else {
		    // non-integer combination → reject for now
		    TODO;
		}


	    out.type = a.type;

	    switch (bin_op.op.kind) {
	    // --- arithmetic ---
	    case Operator::Plus:
	        out.v = ctx.builder.CreateAdd(a.v, b.v);
	        return {};
	    case Operator::Minus:
	        out.v = ctx.builder.CreateSub(a.v, b.v);
	        return {};
	    case Operator::Star:
	        out.v = ctx.builder.CreateMul(a.v, b.v);
	        return {};
	    case Operator::Slash:
	        out.v = ctx.builder.CreateSDiv(a.v, b.v);
	        return {};
	    case Operator::Percent:
	        out.v = ctx.builder.CreateSRem(a.v, b.v);
	        return {};

	    // --- comparison ---
	    case Operator::Lt:
	        out.v = ctx.builder.CreateICmpSLT(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return {};
	    case Operator::Gt:
	        out.v = ctx.builder.CreateICmpSGT(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return {};
	    case Operator::Le:
	        out.v = ctx.builder.CreateICmpSLE(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return {};
	    case Operator::Ge:
	        out.v = ctx.builder.CreateICmpSGE(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return {};
	    case Operator::EqEq:
	        out.v = ctx.builder.CreateICmpEQ(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return {};
	    case Operator::NotEq:
	        out.v = ctx.builder.CreateICmpNE(a.v, b.v);
	        out.type = Type{out.v->getType(), nullptr, nullptr};
	        return {};

	    // --- logical ---
	    case Operator::AndAnd: {
	        auto lhs = to_bool(a);
	        if (!lhs) return FORWARD_UNEXPECTED(lhs);
	        auto rhs = to_bool(b);
	        if (!rhs) return FORWARD_UNEXPECTED(rhs);
	        out.v = ctx.builder.CreateAnd(lhs->v, rhs->v, "andtmp");
	        out.type = lhs->type;
	        return {};
	    }
	    case Operator::OrOr: {
	        auto lhs = to_bool(a);
	        if (!lhs) return FORWARD_UNEXPECTED(lhs);
	        auto rhs = to_bool(b);
	        if (!rhs) return FORWARD_UNEXPECTED(rhs);
	        out.v = ctx.builder.CreateOr(lhs->v, rhs->v, "ortmp");
	        out.type = lhs->type;
	        return {};
	    }

	    // --- bitwise ---
	    case Operator::BitAnd:
	        out.v = ctx.builder.CreateAnd(a.v, b.v);
	        return {};
	    case Operator::BitOr:
	        out.v = ctx.builder.CreateOr(a.v, b.v);
	        return {};
	    case Operator::BitXor:
	        out.v = ctx.builder.CreateXor(a.v, b.v);
	        return {};

	    // --- caught ---
	    case Operator::Assign: 

	    	UNREACHABLE();
	    	break;

	    case Operator::Invalid:
	        throw std::invalid_argument("uninit binop expression");

	    default:
	        TODO;
	    }

	    return {}; // only reached for TODOs / incomplete branches
	}




    result_t operator()(const SubScript&) const {
        TODO;
    }

    result_t operator()(const Call& c) const {
	    Value fn_val;
	    result_t rf = ctx.compile(*c.func,fn_val);
	    if (!rf) return FORWARD_UNEXPECTED(rf);

	    // must be a function
	    if (!fn_val.type.func || !fn_val.type.func->ft)
	        return std::unexpected(NotAFunction{*c.func, fn_val.type.t});

	    FunctionType* fnty = fn_val.type.func;

	    // argument count check
	    if (fnty->args.size() != c.args.size())
	        return std::unexpected(WrongArgCount{c, fnty});

	    // compile arguments
	    std::vector<llvm::Value*> arg_vals;
	    arg_vals.reserve(c.args.size());
	    for (size_t i = 0; i < c.args.size(); ++i) {
	        Value a;
	        result_t ra = ctx.compile(c.args[i],a);
	        if (!ra) return FORWARD_UNEXPECTED(ra);
	        arg_vals.push_back(a.v);

	        const Type& expected = fnty->args[i];
	        const Type& got = a.type;
	        if (!types_exactly_equal(expected, got))
	            return std::unexpected(BadType{c.args[i], expected, got});
	    }

	    // create call instruction
	    llvm::CallInst* call = ctx.builder.CreateCall(fnty->ft, fn_val.v, arg_vals, "function call");
	    call->setCallingConv(fnty->cc);

	    // wrap result
	    out.v = call;
	    out.type = fnty->ret;
	    return {};
	}

};

result_t CompileContext::compile(const Expression& exp,Value& out) {
	//print("[compiling exp:]\n");
	// print_expression(exp,1,true);
    result_t r = std::visit(ExpressionVisitor{*this,out}, exp.inner);
    if(!r) return FORWARD_UNEXPECTED(r);
    return {};
}

struct StatmentVisitor :VisitorBase {
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

    result_t operator()(const If& i) const {
	    // --- 1. Evaluate condition ---
	    Value cond_val;
	    result_t rcond = ctx.compile(i.cond,cond_val);
	    if (!rcond) return rcond;


	    // --- 2. Convert to boolean ---
	    vresult_t rcond_bool = to_bool(cond_val);
	    if (!rcond_bool)
	        return FORWARD_UNEXPECTED(rcond_bool);

	    Value cond_bool = *rcond_bool;
	    llvm::Value* cond = cond_bool.v;
	    llvm::Function* func = ctx.builder.GetInsertBlock()->getParent();

		auto bthen  = llvm::BasicBlock::Create(*ctx.ctx, concat_statements("then",i.block.text), func);
		auto belse  = llvm::BasicBlock::Create(*ctx.ctx, concat_statements("else",i.else_part.text), func);
        ctx.builder.CreateCondBr(cond, bthen, belse);

        //print("in %p \n",ctx.builder.GetInsertBlock());
        //print("made %p %p %p\n",bthen,belse,bmerge);
        //print("setting insertion at %p \n",bthen);
        ctx.builder.SetInsertPoint(bthen);
        result_t rthen = compile_block(i.block);
        if(!rthen) return rthen;
        auto then_end = ctx.builder.GetInsertBlock();


        //print("in %p \n",ctx.builder.GetInsertBlock());
        //print("setting insertion at %p \n",belse);
        ctx.builder.SetInsertPoint(belse);
        result_t relse = compile_block(i.else_part);
        if(!relse) return relse;
        auto else_end = ctx.builder.GetInsertBlock();

        

        //cant have empty block
        //this is a partial solutin we need a better check on blocks being dead
        bool then_open = !then_end->getTerminator();
        bool else_open = !else_end->getTerminator();
        if (then_open||else_open) {
		    auto bmerge = llvm::BasicBlock::Create(*ctx.ctx, "merge", func);
	        
	        if (then_open){
	        	//print("adding merge in %p to %p\n",ctx.builder.GetInsertBlock(),bmerge);
	        	ctx.builder.SetInsertPoint(then_end);
	        	ctx.builder.CreateBr(bmerge);
	        }

	        if (else_open){
	        	//print("adding merge in %p to %p\n",ctx.builder.GetInsertBlock(),bmerge);
	        	ctx.builder.SetInsertPoint(else_end);
	        	ctx.builder.CreateBr(bmerge);
	        }

	        ctx.builder.SetInsertPoint(bmerge);
		}

        return {};
	}

    result_t operator()(const Return& r) const {
        Value value;
        result_t res = ctx.compile(r.val,value);
        if (!res) return res;
        
        result_t res2 = implicit_cast(value,ctx.current_func->ret,r);
        if(!res2) return res2;

        //std::cout << "in "<<ctx.builder.GetInsertBlock() << r.text << "\n";
        ctx.builder.CreateRet(value.v);
        return {};
    }

    result_t operator()(const Block& b) const { return compile_block(b); }

    result_t operator()(const Basic& b) const {
    	Value out;
        return ctx.compile(b.inner,out);
    }
};

result_t CompileContext::compile(const Statement& stmt) {
    //print("[compiling stmt:]\n");
	// print_statement(stmt,1,true);
    result_t r = std::visit(StatmentVisitor{*this}, stmt.inner);
    if (r || std::holds_alternative<StatmentError>(r.error())) return r;
    return std::unexpected(
        StatmentError{stmt, std::make_unique<CompileError>(std::move(r).error())});
}

struct GlobalVisitor : VisitorBase {
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
        		Type{fn->getType(),nullptr,ft},
        		nullptr
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

        ctx.current_func = fn_val->type.func;

        for (auto& stmt : f.body.parts) {
            result_t r = ctx.compile(stmt);
            if (!r) return r;//dont reset function so error can use it
        }

        if (f.body.parts.empty() ||
            !std::holds_alternative<Return>(f.body.parts.back().inner))
            TODO;

        ctx.current_func = nullptr;
        return {};
    }

    result_t operator()(const Basic& b) const { return StatmentVisitor{ctx}(b); }
};

result_t CompileContext::compile(const Global& global) {
    return std::visit(GlobalVisitor{*this}, global.inner);
}

} // namespace small_lang
