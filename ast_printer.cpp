#include "parser.hpp"
#include "ast_print.hpp"
#include <fstream>
#include <iostream>
using namespace small_lang;

// ------------------------------------------------------------
// Parse and print all globals from a file
// ------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.small> [--show-text]\n";
        return 1;
    }

    std::string filename = argv[1];
    bool show_text = (argc > 2 && std::string(argv[2]) == "--show-text");

    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: could not open file: " << filename << "\n";
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    ParseStream stream(source);

    std::cout << "=== Parsing file: " << filename << " ===\n\n";

    int global_index = 0;
    while (!stream.empty()) {
        stream.skip_whitespace();
        if (stream.empty())
            break;

        Global g;
        auto err = parse_global(stream, g);
        if (err) {
            std::cerr << "[parser error] " << err.what() << "\n";
            if (show_text)
                std::cerr << "At: " << err.context << "\n";
            return 1;
        }

        std::cout << "=== Global #" << global_index++ << " ===\n";
        print_global(g, 0, show_text);
        std::cout << "Text: \"" << (std::string_view)g << "\"\n\n";
    }

    std::cout << "✅ Done. Parsed " << global_index << " global(s).\n";
    return 0;
}
