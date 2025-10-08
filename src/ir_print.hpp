#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include "compiler.hpp"
#include "ast_print.hpp"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

namespace small_lang {

// helper: get string representation of llvm::Type
inline std::string to_string(const Type& type) {
    llvm::Type* t = type.t;
    if (!t) return "(null)";
    std::string s;
    llvm::raw_string_ostream rso(s);
    t->print(rso);
    return rso.str();
}

// ============================================================
// Error pretty-printing
// ============================================================
inline std::ostream& operator<<(std::ostream& os, const CompileError& err);

inline std::ostream& operator<<(std::ostream& os, const MissingVar& e) {
    os << "MissingVar:\n"
       << e.var << "\n";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const NotAFunction& e) {
    os << "NotAFunction:\n"
       << "  expression: " << e.exp << "\n"
       << "  got type: " << to_string(e.got) << "\n";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CantBool& e) {
    os << "CantBool:\n"
       << "  got type: " << to_string(e.got) << "\n";
    return os;
}


inline std::ostream& operator<<(std::ostream& os, const WrongArgCount& e) {
    os << "WrongArgCount:\n"
       << "  call: " << e.call << "\n";
    if (e.t)
        os << "  expected arg count: " << e.t->args.size() << "\n";
    else
        os << "  expected arg count: (unknown)\n";
    return os;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const BadType<T>& e) {
    os << "BadType:\n"
       << "  made: " << e.made << "\n"
       << "  expected: " << to_string(e.expected) << "\n"
       << "  got: " << to_string(e.got) << "\n";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const StatmentError& e) {
    os << *e.source
    << "inside of statment:\n"
    << (std::string_view)e.parent <<"\n"

    ;
    return os;
}

// ============================================================
// CompileError variant printer
// ============================================================

inline std::ostream& operator<<(std::ostream& os, const CompileError& err) {
    std::visit([&](auto&& arg) { os << arg; }, err);
    return os;
}

} // namespace small_lang
