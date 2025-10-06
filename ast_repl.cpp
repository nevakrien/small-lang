#include "parser.hpp"
#include "ast_print.hpp"

// ------------------------------------------------------------
// Simple REPL
// ------------------------------------------------------------

int main() {
    std::cout << "Simple Parser REPL\n";
    std::cout << "Commands:\n";
    std::cout << "  :e <expr>  - Parse expression\n";
    std::cout << "  :s <stmt>  - Parse statement\n";
    std::cout << "  :t         - Toggle printing node text ranges\n";
    std::cout << "  :q         - Quit\n\n";

    bool show_text = false;
    std::string line;

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        if (line == ":q" || line == "quit" || line == "exit") break;
        if (line == ":t" || line == ":text") {
            show_text = !show_text;
            std::cout << "Node text display " << (show_text ? "ON" : "OFF") << ".\n";
            continue;
        }

        bool expr_mode = line.starts_with(":e ");
        bool stmt_mode = line.starts_with(":s ");
        std::string_view input = line;

        if (expr_mode || stmt_mode) input.remove_prefix(3);

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

    std::cout << "Bye!\n";
    return 0;
}