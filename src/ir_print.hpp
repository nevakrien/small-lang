#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include "compiler.hpp"
#include "ast_print.hpp"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

namespace small_lang {

inline std::string to_string(const Type& type) {
    if (!type.t)
        return "(null type)";

    std::string s;
    llvm::raw_string_ostream rso(s);

    // Base LLVM type
    rso << "[llvm:";
    type.t->print(rso);
    rso << "]";

    // Stored type (e.g. pointer to element type)
    if (type.stored && type.stored->t) {
        rso << " -> stored(";
        type.stored->t->print(rso);
        rso << ")";
    }

    // Function type (if attached)
    if (type.func && type.func->ft) {
        rso << " -> func(";
        // Return type
        if (type.func->ret.t)
            type.func->ret.t->print(rso);
        else
            rso << "(void)";

        rso << " (";
        for (size_t i = 0; i < type.func->args.size(); ++i) {
            if (i) rso << ", ";
            if (type.func->args[i].t)
                type.func->args[i].t->print(rso);
            else
                rso << "(?)";
        }
        rso << ")";
        rso << " cc=" << static_cast<int>(type.func->cc) << ")";
    }

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
