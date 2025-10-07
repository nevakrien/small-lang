#include "jit.hpp"
#include "compiler.hpp"
#include "parser.hpp"
#include "ast_print.hpp"
#include "ir_print.hpp"

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>

#include <iostream>

namespace small_lang {


// ------------------------------------------------------------
// Modern optimizer (O2 pipeline, includes mem2reg etc.)
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


    // ----------------------------------------------------------


    llvm::ModulePassManager mpm =
        pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);

    mpm.run(mod, mam);
}

// ------------------------------------------------------------
// Run the JIT and call main()
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
int compile_source(std::string_view src, const RunOptions& opt) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

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
            std::cerr << "[compile error]\n" << res.error();
            return 1;
        }
    }

    // --- Pre-optimization IR ---
    if (opt.print_ir_pre) {
        std::cout << "\n[IR before optimization]\n";
        ctx.mod->print(llvm::outs(), nullptr);
        std::cout << "\n";
    }

    // --- Verify IR ---
    if (opt.verify_ir) {
        std::string verifyErrs;
        llvm::raw_string_ostream os(verifyErrs);
        if (llvm::verifyModule(*ctx.mod, &os)) {
            std::cerr << "[verify] Module verification failed:\n"
                      << os.str() << "\n";
            std::cerr << "[IR dump for debugging]\n";
            ctx.mod->print(llvm::errs(), nullptr);
            return 1;
        }
    }

    // --- Optimization ---
    if (opt.optimize_ir) {
        optimize_module(*ctx.mod);
        std::cout << "[optimize] done\n";
    }

    // --- Post-optimization IR ---
    if (opt.print_ir_post) {
        std::cout << "\n[IR after optimization]\n";
        ctx.mod->print(llvm::outs(), nullptr);
        std::cout << "\n";
    }

    return run_jit(ctx, opt);
}

}//small_lang