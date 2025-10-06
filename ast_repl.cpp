#include "parser.hpp"
#include "ast_print.hpp"
using namespace small_lang;

// ------------------------------------------------------------
// Unified parser REPL with line-buffer + instant mode toggle
// ------------------------------------------------------------

int main() {
    std::cout << "Simple Parser REPL\n";
    std::cout << "Commands:\n";
    std::cout << "  :e           - Switch to expression mode\n";
    std::cout << "  :s           - Switch to statement mode\n";
    std::cout << "  :g           - Switch to global mode\n";
    std::cout << "  :t           - Toggle showing text ranges\n";
    std::cout << "  :i           - Toggle instant-commit mode (parse each line)\n";
    std::cout << "  :c           - Clear current input buffer\n";
    std::cout << "  :q           - Quit\n\n";
    std::cout << "Tip: Type code normally; a blank line commits and parses it.\n\n";

    enum class Mode { Expr, Stmt, Global };
    Mode mode = Mode::Stmt;
    bool show_text = false;
    bool instant_mode = false;

    std::string buffer;
    std::string line;

    while (true) {
        std::cout << (buffer.empty() ? "> " : "| ");
        if (!std::getline(std::cin, line))
            break;

        // --- Handle commands ---
        if (!line.empty() && line[0] == ':') {
            if (line == ":q" || line == ":quit" || line == ":exit") {
                std::cout << "Bye!\n";
                break;
            }
            else if (line == ":t" || line == ":text") {
                show_text = !show_text;
                std::cout << "Node text display " << (show_text ? "ON" : "OFF") << ".\n";
            }
            else if (line == ":c" || line == ":clear") {
                buffer.clear();
                std::cout << "Buffer cleared.\n";
            }
            else if (line == ":i") {
                instant_mode = !instant_mode;
                std::cout << "Instant commit mode " << (instant_mode ? "ON" : "OFF") << ".\n";
            }
            else if (line == ":e") {
                mode = Mode::Expr;
                std::cout << "Mode: Expression\n";
            }
            else if (line == ":s") {
                mode = Mode::Stmt;
                std::cout << "Mode: Statement\n";
            }
            else if (line == ":g") {
                mode = Mode::Global;
                std::cout << "Mode: Global\n";
            }
            else {
                std::cout << "Unknown command.\n";
            }
            continue;
        }

        // --- Determine when to commit ---
        bool should_commit = false;
        if (instant_mode) {
            buffer = line + "\n";  // replace buffer each line
            should_commit = true;
        }
        else if (line.empty()) {
            if (buffer.empty()) continue;
            should_commit = true;
        }
        else {
            buffer += line + "\n";
        }

        // --- Commit and parse if needed ---
        if (!should_commit)
            continue;

        std::string_view input = buffer;
        ParseStream stream{input, input};

        if (mode == Mode::Expr) {
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
        }
        else if (mode == Mode::Stmt) {
            Statement stmt;
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
        else if (mode == Mode::Global) {
            Global g;
            Error err = parse_global(stream, g);
            if (err) {
                std::cout << "Error: " << err.what() << "\n";
                if (show_text)
                    std::cout << "At: " << err.context << "\n";
            } else {
                std::cout << "Parsed global:\n";
                print_global(g, 0, show_text);
                std::cout << "Text: \"" << (std::string_view)g << "\"\n";
            }
        }

        buffer.clear();
        std::cout << "\n";
    }

    return 0;
}
