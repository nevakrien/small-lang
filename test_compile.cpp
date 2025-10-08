#include "jit.hpp"
#include <iostream>
#include <vector>
#include <string_view>

using namespace small_lang;

struct TestCase {
    std::string name;
    std::string_view src;
    int64_t expected;
};

// run one case, optionally with debug
static bool run_case(const TestCase& t, bool verbose_on_fail = true) {
    RunOptions opt;
    opt.print_globals = false;
    opt.print_ir_pre  = false;
    opt.print_ir_post = false;
    opt.verify_ir     = true;
    opt.optimize_ir   = true;
    opt.run_main      = true;

    int64_t ret = -9999;
    bool ok = false;

    try {
        ok = !compile_source(t.src, opt, ret);
        ok |= (ret == t.expected);
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION in test " << t.name << ": " << e.what() << "\n";
        ok = false;
    }

    if (!ok && verbose_on_fail) {
        std::cerr << "\n=== FAIL: " << t.name << " ===\n";
        std::cerr << "Expected " << t.expected << " but got " << ret << "\n";
        std::cerr << "=== Rerunning with full debug ===\n\n";

        opt.print_globals = opt.print_ir_pre = opt.print_ir_post = true;
        try {
            compile_source(t.src, opt, ret);
        } catch (...) {
            std::cerr << "(Exception again during debug rerun)\n";
        }
    }
    return ok;
}

int main() {
    std::cout << "=== Small-Lang Battery ===\n";

    std::vector<TestCase> tests = {

        // --- pointer & memory semantics ---
        { "basic deref store",
R"(
cfn main() {
    a = 5;
    pa = &a;
    *pa = 0;
    return a;
}
)", 0 },

        { "double indirection",
R"(
cfn main() {
    a = 7;
    p = &a;
    pp = &p;
    **pp = 9;
    return a;
}
)", 9 },

        { "pointer aliasing",
R"(
cfn main() {
    a = 1;
    b = 2;
    p = &a;
    q = &b;
    *p = *q;
    return a;
}
)", 2 },

        // --- logical & comparison ---
        { "logical chain",
R"(
cfn main() {
    return (!1 && 0) || (1 && 1);
}
)", 1 },

        { "comparison equal and not equal",
R"(
cfn main() {
    a = 3;
    b = 3;
    c = (a == b);
    d = (a != b);
    return c*10 + d;
}
)", 10 },

        // --- branching ---
        { "simple if",
R"(
cfn main() {
    a = 1;
    b = 2;
    if (a < b) a = 9;
    return a;
}
)", 9 },

        { "if else",
R"(
cfn main() {
    a = 5;
    b = 6;
    if (a > b) c = 111;
    else c = 222;
    return c;
}
)", 222 },

        // --- function call & dispatch ---
        { "call simple function",
R"(
cfn add1(x) { return x + 1; }
cfn main() { return add1(41); }
)", 42 },

        { "function pointer swap",
R"(
cfn inc(x) { return x + 1; }
cfn dec(x) { return x - 1; }
cfn main() {
    f = inc;
    g = dec;
    p = &f;
    *p = g;          // swap function pointer
    return f(5);     // f now points to dec
}
)", 4 },

        // --- nested control flow ---
        { "nested ifs",
R"(
cfn main() {
    a = 1;
    b = 2;
    c = 3;
    if (a < b) {
        if (b < c) return 99;
        else return 77;
    }
    return 11;
}
)", 99 },

        // --- math & precedence ---
        { "arithmetic precedence",
R"(
cfn main() {
    return 1 + 2 * 3 + 4;
}
)", 11 },

        { "minus and negation",
R"(
cfn main() {
    a = -5;
    return a + 8;
}
)", 3 },

        // --- mixed pointer & arithmetic ---
        { "pointer arithmetic simulation (by manual indirection)",
R"(
cfn main() {
    x = 10;
    p = &x;
    *p = *p + 5;
    return x;
}
)", 15 },

        // --- dynamic dispatch simulation ---
        { "dynamic function pointer swap mid-execution",
R"(
cfn inc(x) { return x + 1; }
cfn dec(x) { return x - 1; }

cfn main() {
    f = inc;
    r = f(10);
    f = dec;
    r = r + f(10);
    return r;
}
)", 20 },

        // --- chained logic ---
        { "long boolean chain",
R"(
cfn main() {
    return (1 && 1 && 1) || (0 && 1);
}
)", 1 },

        // --- equality truth check like earlier ---
        { "logical inversion equality",
R"(
cfn main() {
    a = !1 && 0;
    b = a == 1;
    b = a == b;
    return b;
}
)", 1 },
    };

    int passed = 0;
    for (auto& t : tests) {
        if (run_case(t))
            ++passed;
        else
            std::cerr << "❌ " << t.name << " failed\n";
    }

    std::cout << "\n=== " << passed << " / " << tests.size() << " passed ===\n";
    return (passed == (int)tests.size()) ? 0 : 1;
}
