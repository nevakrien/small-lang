#include "parser.hpp"
#include "ast_print.hpp"

#include <replxx.hxx>   // Replxx header
#include <iostream>
#include <vector>
#include <string>
#include <string_view>

using Replxx = replxx::Replxx;

// ------------------------------------------------------------
// Simple Parser REPL using Replxx
// ------------------------------------------------------------
int main() {
    std::cout << "Simple Parser REPL\n";
    std::cout << "Commands:\n";
    std::cout << "  :e <expr>  - Parse expression\n";
    std::cout << "  :s <stmt>  - Parse statement\n";
    std::cout << "  :t         - Toggle printing node text ranges\n";
    std::cout << "  :q         - Quit\n\n";

    bool show_text = false;

    // --- Initialize Replxx
    Replxx rx;
    rx.set_indent_multiline(true);

    rx.set_max_history_size(1000);
    // rx.history_load(".parser_history");
    
    // --- Completion callback for command prefixes
    std::vector<std::string> cmds = {":e", ":s", ":t", ":q", "quit", "exit"};
    rx.set_completion_callback(
        [&](std::string const& context, int) {
            Replxx::completions_t out;
            for (auto const& c : cmds)
                if (c.rfind(context, 0) == 0)
                    out.emplace_back(c.substr(context.size()));
            return out;
        }
    );

    while (true) {
        // Get user input with prompt
        const char* input_c = rx.input("> ");
        if (!input_c) break;  // EOF or Ctrl-D

        std::string line(input_c);
        if (line.empty()) continue;
        rx.history_add(line);

        if (line == ":q" || line == "quit" || line == "exit") break;

        if (line == ":t" || line == ":text") {
            show_text = !show_text;
            std::cout << "Node text display " << (show_text ? "ON" : "OFF") << ".\n";
            continue;
        }

        bool expr_mode = line.rfind(":e ", 0) == 0;
        bool stmt_mode = line.rfind(":s ", 0) == 0;
        std::string_view input = line;
        if (expr_mode || stmt_mode)
            input.remove_prefix(3);

        if (expr_mode) {
            ParseStream stream{input, input};
            Expression exp;
            Error err = parse_expression(stream, exp);

            if (err) {
                std::cout << "Error: " << err.what() << "\n";
                if (show_text)
                    std::cout << "At: " << err.context << "\n";
            } else {
                std::cout << "Parsed expression:\n";
                print_expression(exp, 0, show_text);
                std::cout << "Text: \"" << (std::string_view)exp << "\"\n";
            }
        } else {
            ParseStream stream{input, input};
            statement stmt;
            Error err = parse_statement(stream, stmt);

            if (err) {
                std::cout << "Error: " << err.what() << "\n";
                if (show_text)
                    std::cout << "At: " << err.context << "\n";
            } else {
                std::cout << "Parsed statement:\n";
                print_statement(stmt, 0, show_text);
                std::cout << "Text: \"" << (std::string_view)stmt << "\"\n";
            }
        }

        std::cout << "\n";
    }

    // rx.history_save(".parser_history");
    std::cout << "Bye!\n";
    return 0;
}
