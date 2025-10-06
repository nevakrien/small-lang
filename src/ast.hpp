#pragma once

namespace small_lang {

//base class used for text things
struct Token {
	std::string_view text;//view of the original text (can do pointer arithmetic)
	operator std::string_view() const noexcept {
        return text;
    }
};

struct Invalid : Token {};

struct Var : Token {
	Var() = default;
	Var(std::string_view name)  {
		text=name;
	}
};

struct Num : Token {
	uint64_t value;
};


enum class Operator {
    Invalid,

    // arithmetic
    Plus, Minus, Star, Slash, Percent,

    // comparison
    Lt, Gt, Le, Ge, EqEq, NotEq,

    // logical
    AndAnd, OrOr, Not,

    // bitwise
    BitAnd, BitOr, BitXor,

    // assignment and increment
    Assign, PlusPlus, MinusMinus,

    // misc
    Arrow, Dot,
};

typedef unsigned int Bp;
struct Op {
    Operator kind = Operator::Invalid;

    constexpr Op() = default;
    constexpr Op(Operator k) : kind(k) {}

    constexpr explicit operator bool() const noexcept { return kind != Operator::Invalid; }
    constexpr bool operator==(Operator k) const noexcept { return kind == k; }
    constexpr bool operator!=(Operator k) const noexcept { return kind != k; }

    //in print_ast
    constexpr operator std::string_view() const noexcept;
    
    //in parser
    constexpr Bp bp_prefix() const noexcept;
    constexpr Bp bp_infix_left() const noexcept;
    constexpr Bp bp_infix_right() const noexcept;
    constexpr Bp bp_postfix() const noexcept;
};

struct Expression;


struct PreOp : Token {
    std::unique_ptr<Expression> exp;
    Op op;

    PreOp() = default;
    PreOp(Op o) : op(o) {}
    PreOp(Op o, Expression expr,std::string_view text);
};


struct BinOp : Token{
	std::unique_ptr<Expression> a;
	std::unique_ptr<Expression> b;
	Op op;
};

struct SubScript : Token{
	std::unique_ptr<Expression> arr;
	std::unique_ptr<Expression> idx;
};


struct Call : Token{
	std::unique_ptr<Expression> func;
	std::vector<Expression> args;
};

using ExpressionVariant = std::variant<Invalid,Var,Num,PreOp,BinOp,SubScript,Call>;
struct Expression {
	ExpressionVariant inner;
	Expression() = default;

	 // construct from the variant directly
    Expression(ExpressionVariant v)
        : inner(std::move(v)) {}

    // construct from any alternative type directly
    template <typename T>
    Expression(T&& value)
        : inner(std::forward<T>(value)) {}

	operator std::string_view() const noexcept {
        return std::visit([](auto && arg){
        	return (std::string_view)arg;
        },inner);
    }

    // access the base Token reference
    Token& tok() noexcept {
        return std::visit([](auto& arg) -> Token& {
            return static_cast<Token&>(arg);
        }, inner);
    }

    const Token& tok() const noexcept {
        return std::visit([](auto const& arg) -> const Token& {
            return static_cast<const Token&>(arg);
        }, inner);
    }
};

PreOp::PreOp(Op o, Expression expr,std::string_view t)
    : exp(std::make_unique<Expression>(std::move(expr))),
      op(o) {
      	text = t;
      }


struct Statement;

struct Basic : Token {
	Expression inner;
};

struct Return : Token {
	Expression val;
};

struct Block : Token {
	std::vector<Statement> parts;
};

struct CondStatement : Token {
	Expression cond;
	Block block;
};

struct While : CondStatement {};
struct If : CondStatement {
	Block else_part;
};


using statementVariant = std::variant<Invalid,While,If,Return,Block,Basic>;
struct Statement {
	statementVariant inner;
	operator std::string_view() const noexcept {
        return std::visit([](auto && arg){
        	return (std::string_view)arg;
        },inner);
    }
};


}//small_lang