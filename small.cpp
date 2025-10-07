#include "jit.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <filesystem>

using namespace small_lang;

static void print_help(const char* prog) {
    std::cout <<
        "Usage: " << prog << " [options] <file>\n"
        "Options:\n"
        "  --no-run           Do not execute main()\n"
        "  --no-opt           Disable IR optimization\n"
        "  --no-verify        Disable IR verification\n"
        "  --print-globals    Print globals table\n"
        "  --print-ir-pre     Print IR before optimization\n"
        "  --print-ir-post    Print IR after optimization\n"
        "  -h, --help         Show this message\n";
}

// ------------------------------------------------------------
// main
// ------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    RunOptions opt; // defaults are true for verify/optimize/run_main

    std::filesystem::path input_path;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--no-run") opt.run_main = false;
        else if (arg == "--no-opt") opt.optimize_ir = false;
        else if (arg == "--no-verify") opt.verify_ir = false;
        else if (arg == "--print-globals") opt.print_globals = true;
        else if (arg == "--print-ir-pre") opt.print_ir_pre = true;
        else if (arg == "--print-ir-post") opt.print_ir_post = true;
        else if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg.starts_with('-')) {
            std::cerr << "Unknown flag: " << arg << "\n";
            return 1;
        } else {
            input_path = arg;
        }
    }

    if (input_path.empty()) {
        std::cerr << "Error: no input file provided.\n";
        print_help(argv[0]);
        return 1;
    }

    if (!std::filesystem::exists(input_path)) {
        std::cerr << "Error: file not found: " << input_path << "\n";
        return 1;
    }

    std::ifstream file(input_path);
    if (!file) {
        std::cerr << "Error: failed to open file: " << input_path << "\n";
        return 1;
    }

    std::string src((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

    std::cout << "=== Small-Lang ===\n";
    std::cout << "[source: " << input_path << "]\n";

    return compile_source(src, opt);
}
