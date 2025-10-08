#include "jit.hpp"
#include <iostream>

using namespace small_lang;


// ------------------------------------------------------------
// main
// ------------------------------------------------------------
int main() {
    std::cout << "=== Small-Lang test ===\n";

    std::string_view src = R"(
        cfn main() {
        	a = 5;
        	pa = &a;
        	*pa = 0;
        	return a;
        }
    )";


    RunOptions opt;
    opt.print_globals = true;
    opt.print_ir_pre  = true;
    opt.print_ir_post = true;
    opt.verify_ir     = true;
    opt.optimize_ir   = true;
    opt.run_main      = true;

    std::cout << "[source]\n" << src << "\n";
    return compile_source(src, opt);
}
