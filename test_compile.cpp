#include "compiler.hpp"
#include "parser.hpp"
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Verifier.h>
#include <iostream>

using namespace small_lang;

int main() {
    // --- 1. Initialize LLVM targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    std::cout << "=== Small-Lang test ===\n";

    // --- 2. Input code
    std::string_view src = R"(
    	fn helper(a){
    		return a;
    	}
        cfn main() {
            return helper(0);
        }
    )";

    std::cout << "[source]\n" << src << "\n";

    // --- 3. Parse
    ParseStream stream(src);
    Global helper;
    if (auto err = parse_global(stream, helper); err) {
        std::cerr << "[parser error] " << err.what() << "\n";
        return 1;
    }

    Global global;
    if (auto err = parse_global(stream, global); err) {
        std::cerr << "[parser error] " << err.what() << "\n";
        return 1;
    }

    std::cout << "[parse] success\n";

    // --- 4. Compile to LLVM IR
    CompileContext ctx("jit_test");
    auto cresh = ctx.compile(helper);
    if (!cresh) {
        std::cerr << "[compile error]\n";
        return 1;
    }

    auto cres = ctx.compile(global);
    if (!cres) {
        std::cerr << "[compile error]\n";
        return 1;
    }

    // --- 5. Verify IR
    if (llvm::verifyModule(*ctx.mod, &llvm::errs())) {
        std::cerr << "[verify] module invalid!\n";
        return 1;
    }

    std::cout << "[IR dump]\n";
    ctx.mod->print(llvm::outs(), nullptr);
    std::cout << "\n";

    // --- 6. Create JIT and add the module
    auto jitExp = llvm::orc::LLJITBuilder().create();
    if (!jitExp) {
        llvm::errs() << toString(jitExp.takeError()) << "\n";
        return 1;
    }
    auto jit = std::move(*jitExp);

    // transfer ownership to ORC
    llvm::orc::ThreadSafeModule tsm(std::move(ctx.mod), std::move(ctx.ctx));
    if (auto err = jit->addIRModule(std::move(tsm))) {
        llvm::errs() << toString(std::move(err)) << "\n";
        return 1;
    }

    std::cout << "[JIT] module added\n";

    // --- 7. Lookup and run main()
    auto sym = jit->lookup("main");
    if (!sym) {
        llvm::errs() << "[JIT error] " << toString(sym.takeError()) << "\n";
        return 1;
    }

    using MainFn = int64_t (*)();
    MainFn mainFn = sym->toPtr<MainFn>();  // modern API (LLVM ≥ 15)

    std::cout << "[Run]\n";
    int64_t ret = mainFn();

    std::cout << "main() returned " << ret << "\n";
    std::cout << "=== done ===\n";
}
