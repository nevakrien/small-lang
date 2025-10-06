#pragma once

#include "parser.hpp"
#include <iostream>
#include <string>

// ------------------------------------------------------------
// Debug printing utilities
// ------------------------------------------------------------

inline void print_expression(const Expression& exp, int indent = 0, bool show_text = false);

inline void print_text(const Token& tok, int indent, bool show_text) {
    if (!show_text) return;
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << "[text: \"" << tok.text << "\"]\n";
}

inline void print_expression(const Expression& exp, int indent, bool show_text) {
    auto ind = [indent]() {
        for (int i = 0; i < indent; i++) std::cout << "  ";
    };

    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, Invalid>) {
            ind(); std::cout << "Invalid\n";
        }
        else if constexpr (std::is_same_v<T, Var>) {
            ind(); std::cout << "Var: " << arg.text << "\n";
            print_text(arg, indent + 1, show_text);
        }
        else if constexpr (std::is_same_v<T, Num>) {
		    ind();
		    if (std::to_string(arg.value) == std::string(arg.text))
		        std::cout << "Num: " << arg.text << "\n";
		    else
		        std::cout << "Num: " << arg.value << "\n";
		    print_text(arg, indent + 1, show_text);
		}

        else if constexpr (std::is_same_v<T, PreOp>) {
            ind(); std::cout << "PreOp: " << arg.op << "\n";
            print_expression(*arg.exp, indent + 1, show_text);
            print_text(arg, indent + 1, show_text);
        }
        else if constexpr (std::is_same_v<T, BinOp>) {
            ind(); std::cout << "BinOp: " << arg.op << "\n";
            print_expression(*arg.a, indent + 1, show_text);
            print_expression(*arg.b, indent + 1, show_text);
            print_text(arg, indent + 1, show_text);
        }
        else if constexpr (std::is_same_v<T, Call>) {
            ind(); std::cout << "Call:\n";
            ind(); std::cout << "  func:\n";
            print_expression(*arg.func, indent + 2, show_text);
            if (!arg.args.empty()) {
                ind(); std::cout << "  args:\n";
                for (const auto& a : arg.args)
                    print_expression(a, indent + 2, show_text);
            }
            print_text(arg, indent + 1, show_text);
        }
    }, exp.inner);
}

inline void print_statement(const statement& stmt, int indent = 0, bool show_text = false);

inline void print_block(const Block& blk, int indent, bool show_text) {
    auto ind = [indent]() {
        for (int i = 0; i < indent; i++) std::cout << "  ";
    };

    ind(); std::cout << "Block:\n";
    for (const auto& s : blk.parts)
        print_statement(s, indent + 1, show_text);
    print_text(blk, indent + 1, show_text);
}

inline void print_statement(const statement& stmt, int indent, bool show_text) {
    auto ind = [indent]() {
        for (int i = 0; i < indent; i++) std::cout << "  ";
    };

    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, Invalid>) {
            ind(); std::cout << "Invalid Statement\n";
        }
        else if constexpr (std::is_same_v<T, Return>) {
            ind(); std::cout << "Return:\n";
            print_expression(arg.val, indent + 1, show_text);
            print_text(arg, indent + 1, show_text);
        }
        else if constexpr (std::is_same_v<T, Block>) {
            print_block(arg, indent, show_text);
        }
        else if constexpr (std::is_same_v<T, If>) {
            ind(); std::cout << "If:\n";
            ind(); std::cout << "  cond:\n";
            print_expression(arg.cond, indent + 2, show_text);
            ind(); std::cout << "  body:\n";
            print_block(arg.block, indent + 2, show_text);
            print_text(arg, indent + 1, show_text);
        }
        else if constexpr (std::is_same_v<T, While>) {
            ind(); std::cout << "While:\n";
            ind(); std::cout << "  cond:\n";
            print_expression(arg.cond, indent + 2, show_text);
            ind(); std::cout << "  body:\n";
            print_block(arg.block, indent + 2, show_text);
            print_text(arg, indent + 1, show_text);
        }
        else if constexpr (std::is_same_v<T, Basic>) {
            ind(); std::cout << "Basic Statement:\n";
            print_expression(arg.inner, indent + 2, show_text);
            print_text(arg, indent + 1, show_text);
        }
    }, stmt.inner);
}

