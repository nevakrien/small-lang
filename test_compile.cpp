#include "compiler.hpp"
#include "parser.hpp"
#include "ast_print.hpp"

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <iostream>

using namespace small_lang;

// ------------------------------------------------------------
// Run options (application layer config)
// ------------------------------------------------------------
struct RunOptions {
    bool print_globals = true;
    bool print_ir      = true;
    bool verify_ir     = true;
    bool optimize_ir   = true;
    bool run_main      = true;
};

// ------------------------------------------------------------
// Modern optimizer
// ------------------------------------------------------------
static void optimize_module(llvm::Module& mod) {
    llvm::PassBuilder pb;

    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cgam;
    llvm::ModuleAnalysisManager mam;

    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);

    // Full standard O2 pipeline — includes mem2reg automatically
    llvm::ModulePassManager mpm =
        pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);

    mpm.run(mod, mam);
}

// ------------------------------------------------------------
// Helper: run the JIT and execute main()
// ------------------------------------------------------------
static int run_jit(CompileContext& ctx, const RunOptions& opt) {
    auto jitExp = llvm::orc::LLJITBuilder().create();
    if (!jitExp) {
        llvm::errs() << toString(jitExp.takeError()) << "\n";
        return 1;
    }
    auto jit = std::move(*jitExp);

    llvm::orc::ThreadSafeModule tsm(std::move(ctx.mod), std::move(ctx.ctx));
    if (auto err = jit->addIRModule(std::move(tsm))) {
        llvm::errs() << toString(std::move(err)) << "\n";
        return 1;
    }

    std::cout << "[JIT] module added\n";

    if (!opt.run_main)
        return 0;

    auto sym = jit->lookup("main");
    if (!sym) {
        llvm::errs() << "[JIT error] " << toString(sym.takeError()) << "\n";
        return 1;
    }

    using MainFn = int64_t (*)();
    MainFn mainFn = sym->toPtr<MainFn>();

    std::cout << "[Run]\n";
    int64_t ret = mainFn();
    std::cout << "main() returned " << ret << "\n";
    return 0;
}

// ------------------------------------------------------------
// Compile + verify + (optionally) optimize + JIT
// ------------------------------------------------------------
static int compile_source(std::string_view src, const RunOptions& opt) {
    ParseStream stream(src);
    CompileContext ctx("jit_test");

    while (true) {
        stream.skip_whitespace();
        if (stream.empty()) break;

        Global g;
        if (auto err = parse_global(stream, g)) {
            std::cerr << "[parser error] " << err.what() << "\n";
            return 1;
        }

        if (opt.print_globals) {
            std::cout << "parsed global:\n";
            print_global(g);
        }

        result_t res = ctx.compile(g);
        if (!res) {
            std::cerr << "[compile error]\n";
            return 1;
        }

        std::cout << "[compiled]\n";
    }

    if (opt.verify_ir) {
        if (llvm::verifyModule(*ctx.mod, &llvm::errs())) {
            std::cerr << "[verify] module invalid!\n";
            return 1;
        }
    }

    if (opt.optimize_ir) {
        optimize_module(*ctx.mod);
        std::cout << "[optimize] done\n";
    }

    if (opt.print_ir) {
        std::cout << "[IR dump]\n";
        ctx.mod->print(llvm::outs(), nullptr);
        std::cout << "\n";
    }

    return run_jit(ctx, opt);
}

// ------------------------------------------------------------
// main
// ------------------------------------------------------------
int main() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    std::cout << "=== Small-Lang test ===\n";

    std::string_view src = R"(
        fn helper(a) {
            return a;
        }

        cfn main() {
            return helper(0);
        }
    )";

    RunOptions opt;
    opt.print_globals = true;
    opt.print_ir      = true;
    opt.verify_ir     = true;
    opt.optimize_ir   = true;
    opt.run_main      = true;

    std::cout << "[source]\n" << src << "\n";
    return compile_source(src, opt);
}
