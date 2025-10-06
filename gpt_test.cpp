/**
 *this file is autogened to see that we can actually run llvm
 *  
 */

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <variant>
#include <iostream>

// -------------------- Context --------------------
struct CodegenContext {
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    std::map<std::string, llvm::Value*> named_values;

    CodegenContext(std::string module_name)
        : builder(context), module(std::make_unique<llvm::Module>(module_name, context)) {}

    llvm::Value* log_error(const char* msg) const {
        std::cerr << "Codegen error: " << msg << "\n";
        return nullptr;
    }
};

// -------------------- AST Types --------------------
// Forward declaration
struct Expr;

struct NumberExpr { double value; };
struct VariableExpr { std::string name; };

struct BinaryExpr {
    char op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct CallExpr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> args;
};

// This wrapper allows recursive structure
struct Expr {
    using Variant = std::variant<NumberExpr, VariableExpr, BinaryExpr, CallExpr>;
    Variant node;

    // convenience constructors
    template<typename T>
    Expr(T val) : node(std::move(val)) {}
};

// Forward declare the codegen function (needed inside visitor)
llvm::Value* codegen(CodegenContext& ctx, const Expr& expr);

// -------------------- Codegen Visitor --------------------
struct CodegenVisitor {
    CodegenContext& ctx;

    llvm::Value* operator()(const NumberExpr& n) const {
        return llvm::ConstantFP::get(ctx.context, llvm::APFloat(n.value));
    }

    llvm::Value* operator()(const VariableExpr& v) const {
        auto it = ctx.named_values.find(v.name);
        if (it == ctx.named_values.end())
            return ctx.log_error("unknown variable");
        return it->second;
    }

    llvm::Value* operator()(const BinaryExpr& b) const {
        llvm::Value* L = codegen(ctx, *b.lhs);
        llvm::Value* R = codegen(ctx, *b.rhs);
        if (!L || !R) return nullptr;

        switch (b.op) {
            case '+': return ctx.builder.CreateFAdd(L, R, "addtmp");
            case '-': return ctx.builder.CreateFSub(L, R, "subtmp");
            case '*': return ctx.builder.CreateFMul(L, R, "multmp");
            case '<': {
                llvm::Value* cmp = ctx.builder.CreateFCmpULT(L, R, "cmptmp");
                return ctx.builder.CreateUIToFP(
                    cmp, llvm::Type::getDoubleTy(ctx.context), "booltmp");
            }
            default:
                return ctx.log_error("invalid binary operator");
        }
    }

    llvm::Value* operator()(const CallExpr& c) const {
        llvm::Function* fn = ctx.module->getFunction(c.callee);
        if (!fn) return ctx.log_error("unknown function");

        if (fn->arg_size() != c.args.size())
            return ctx.log_error("argument count mismatch");

        std::vector<llvm::Value*> argsV;
        argsV.reserve(c.args.size());
        for (auto& a : c.args) {
            llvm::Value* v = codegen(ctx, *a);
            if (!v) return nullptr;
            argsV.push_back(v);
        }

        return ctx.builder.CreateCall(fn, argsV, "calltmp");
    }
};

// -------------------- Dispatcher --------------------
llvm::Value* codegen(CodegenContext& ctx, const Expr& expr) {
    return std::visit(CodegenVisitor{ctx}, expr.node);
}

// -------------------- Main --------------------
int main() {
    CodegenContext cg("main_module");

    // define a dummy function (for call exprs)
    llvm::FunctionType* FT = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(cg.context),
        { llvm::Type::getDoubleTy(cg.context), llvm::Type::getDoubleTy(cg.context) },
        false);
    llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "add", cg.module.get());

    // build: (2 + 3) * 4
    auto expr = Expr(BinaryExpr{
        '*',
        std::make_unique<Expr>(
            BinaryExpr{
                '+',
                std::make_unique<Expr>(NumberExpr{2.0}),
                std::make_unique<Expr>(NumberExpr{3.0})
            }),
        std::make_unique<Expr>(NumberExpr{4.0})
    });

    llvm::Value* val = codegen(cg, expr);
    if (val)
        cg.module->print(llvm::outs(), nullptr);
}
