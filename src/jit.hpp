#pragma once

#include<string_view>

namespace small_lang {

// ------------------------------------------------------------
// Run options
// ------------------------------------------------------------
struct RunOptions {
    bool print_globals = false;
    bool print_ir_pre  = false;   // print IR before optimization
    bool print_ir_post = false;   // print IR after optimization
    bool verify_ir     = true;
    bool optimize_ir   = true;
    bool run_main      = true;
};

int compile_source(std::string_view src, const RunOptions& opt);

}