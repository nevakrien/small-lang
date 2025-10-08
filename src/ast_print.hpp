#pragma once
#include "ast.hpp"
#include <iostream>
#include <string>

namespace small_lang {

// ============================================================
// Op printing
// ============================================================
constexpr Op::operator std::string_view() const noexcept {
    switch (kind) {
        case Operator::Plus:        return "+";
        case Operator::Minus:       return "-";
        case Operator::Star:        return "*";
        case Operator::Slash:       return "/";
        case Operator::Percent:     return "%";
        case Operator::Lt:          return "<";
        case Operator::Gt:          return ">";
        case Operator::Le:          return "<=";
        case Operator::Ge:          return ">=";
        case Operator::EqEq:        return "==";
        case Operator::NotEq:       return "!=";
        case Operator::AndAnd:      return "&&";
        case Operator::OrOr:        return "||";
        case Operator::Not:         return "!";
        case Operator::BitAnd:      return "&";
        case Operator::BitOr:       return "|";
        case Operator::BitXor:      return "^";
        case Operator::Assign:      return "=";
        case Operator::PlusPlus:    return "++";
        case Operator::MinusMinus:  return "--";
        case Operator::Arrow:       return "->";
        case Operator::Dot:         return ".";
        default:                    return "<invalid>";
    }
}

inline std::ostream& operator<<(std::ostream& os, const Op& op) {
    return os << static_cast<std::string_view>(op);
}

// ============================================================
// Helper
// ============================================================
inline void print_token(std::ostream& os, const Token& tok, int indent, bool show_text) {
    if (!show_text) return;
    for (int i = 0; i < indent; i++) os << "  ";
    os << "[text: \"" << tok.text << "\"]\n";
}

// ============================================================
// Forward decls for stream_* functions (recursive core API)
// ============================================================
inline void stream(std::ostream&, const Expression&, int, bool);
inline void stream(std::ostream&, const Statement&, int, bool);
inline void stream(std::ostream&, const Block&, int, bool);
inline void stream(std::ostream&, const Global&, int, bool);

// ============================================================
// Individual AST node streaming
// ============================================================
inline void stream(std::ostream& os, const Invalid&, int indent, bool) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << "Invalid\n";
}

inline void stream(std::ostream& os, const Var& v, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << "Var: " << v.text << "\n";
    print_token(os, v, indent + 1, show_text);
}

inline void stream(std::ostream& os, const Num& n, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    if (std::to_string(n.value) == std::string(n.text))
        os << "Num: " << n.text << "\n";
    else
        os << "Num: " << n.value << "\n";
    print_token(os, n, indent + 1, show_text);
}

inline void stream(std::ostream& os, const PreOp& p, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << "PreOp: " << p.op << "\n";
    stream(os, *p.exp, indent + 1, show_text);
    print_token(os, p, indent + 1, show_text);
}

inline void stream(std::ostream& os, const TypeCast& c, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << "TypeCast: to "<<c.type.name<<"\n";
    stream(os, *c.exp, indent + 1, show_text);
    print_token(os, c, indent + 1, show_text);
}

inline void stream(std::ostream& os, const BinOp& b, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << "BinOp: " << b.op << "\n";
    stream(os, *b.a, indent + 1, show_text);
    stream(os, *b.b, indent + 1, show_text);
    print_token(os, b, indent + 1, show_text);
}

inline void stream(std::ostream& os, const SubScript& s, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << "SubScript:\n";
    for (int i = 0; i < indent; i++) os << "  ";
    os << " Array:\n";
    stream(os, *s.arr, indent + 2, show_text);
    for (int i = 0; i < indent; i++) os << "  ";
    os << " Index:\n";
    stream(os, *s.idx, indent + 2, show_text);
    print_token(os, s, indent + 1, show_text);
}

inline void stream(std::ostream& os, const Call& c, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << "Call:\n";
    for (int i = 0; i < indent; i++) os << "  ";
    os << "  func:\n";
    stream(os, *c.func, indent + 2, show_text);
    if (!c.args.empty()) {
        for (int i = 0; i < indent; i++) os << "  ";
        os << "  args:\n";
        for (const auto& a : c.args)
            stream(os, a, indent + 2, show_text);
    }
    print_token(os, c, indent + 1, show_text);
}

// ============================================================
// Expression dispatcher
// ============================================================
inline void stream(std::ostream& os, const Expression& exp, int indent, bool show_text) {
    std::visit([&](auto&& arg) { stream(os, arg, indent, show_text); }, exp.inner);
}

// ============================================================
// Statements
// ============================================================
inline void stream(std::ostream& os, const Return& r, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << "Return:\n";
    stream(os, r.val, indent + 1, show_text);
    print_token(os, r, indent + 1, show_text);
}

inline void stream(std::ostream& os, const If& i, int indent, bool show_text) {
    for (int k = 0; k < indent; k++) os << "  ";
    os << "If:\n";
    for (int k = 0; k < indent; k++) os << "  ";
    os << "  cond:\n";
    stream(os, i.cond, indent + 2, show_text);
    for (int k = 0; k < indent; k++) os << "  ";
    os << "  body:\n";
    stream(os, i.block, indent + 2, show_text);
    print_token(os, i, indent + 1, show_text);
}

inline void stream(std::ostream& os, const While& w, int indent, bool show_text) {
    for (int k = 0; k < indent; k++) os << "  ";
    os << "While:\n";
    for (int k = 0; k < indent; k++) os << "  ";
    os << "  cond:\n";
    stream(os, w.cond, indent + 2, show_text);
    for (int k = 0; k < indent; k++) os << "  ";
    os << "  body:\n";
    stream(os, w.block, indent + 2, show_text);
    print_token(os, w, indent + 1, show_text);
}

inline void stream(std::ostream& os, const Basic& b, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << "Basic Statement:\n";
    stream(os, b.inner, indent + 2, show_text);
    print_token(os, b, indent + 1, show_text);
}

inline void stream(std::ostream& os, const Block& blk, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << "Block:\n";
    for (const auto& s : blk.parts)
        stream(os, s, indent + 1, show_text);
    print_token(os, blk, indent + 1, show_text);
}

inline void stream(std::ostream& os, const Statement& stmt, int indent, bool show_text) {
    std::visit([&](auto&& arg) { stream(os, arg, indent, show_text); }, stmt.inner);
}

// ============================================================
// Functions and globals
// ============================================================
inline void stream(std::ostream& os, const FuncDec& fd, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << (fd.is_c ? "C-FuncDec: " : "FuncDec: ") << fd.name.text << "(";
    for (size_t i = 0; i < fd.args.size(); i++) {
        os << fd.args[i].text;
        if (i + 1 < fd.args.size()) os << ", ";
    }
    os << ")\n";
    print_token(os, fd, indent + 1, show_text);
}

inline void stream(std::ostream& os, const Function& fn, int indent, bool show_text) {
    for (int i = 0; i < indent; i++) os << "  ";
    os << (fn.is_c ? "C-Function: " : "Function: ") << fn.name.text << "(";
    for (size_t i = 0; i < fn.args.size(); i++) {
        os << fn.args[i].text;
        if (i + 1 < fn.args.size()) os << ", ";
    }
    os << ")\n";
    for (int i = 0; i < indent; i++) os << "  ";
    os << "  body:\n";
    stream(os, fn.body, indent + 2, show_text);
    print_token(os, fn, indent + 1, show_text);
}

inline void stream(std::ostream& os, const Global& g, int indent, bool show_text) {
    std::visit([&](auto&& arg){ stream(os, arg, indent, show_text); }, g.inner);
}

// ============================================================
// print
// ============================================================
inline void print_expression(const Expression& e, int indent = 0, bool show_text = false) {
    stream(std::cout, e, indent, show_text);
}

inline void print_statement(const Statement& s, int indent = 0, bool show_text = false) {
    stream(std::cout, s, indent, show_text);
}

inline void print_block(const Block& b, int indent = 0, bool show_text = false) {
    stream(std::cout, b, indent, show_text);
}

inline void print_global(const Global& g, int indent = 0, bool show_text = false) {
    stream(std::cout, g, indent, show_text);
}

// ============================================================
// Stream operator overloads
// ============================================================

inline std::ostream& operator<<(std::ostream& os, const Invalid& v)     { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const Var& v)         { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const Num& v)         { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const PreOp& v)       { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const TypeCast& v)    { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const BinOp& v)       { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const SubScript& v)   { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const Call& v)        { stream(os, v, 0, false); return os; }

inline std::ostream& operator<<(std::ostream& os, const Return& v)      { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const If& v)          { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const While& v)       { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const Basic& v)       { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const Block& v)       { stream(os, v, 0, false); return os; }

inline std::ostream& operator<<(std::ostream& os, const FuncDec& v)     { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const Function& v)    { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const Global& v)      { stream(os, v, 0, false); return os; }

inline std::ostream& operator<<(std::ostream& os, const Expression& v)  { stream(os, v, 0, false); return os; }
inline std::ostream& operator<<(std::ostream& os, const Statement& v)   { stream(os, v, 0, false); return os; }

} // namespace small_lang
