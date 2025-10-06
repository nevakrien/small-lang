#pragma once

#include <string_view>
#include <cctype>
#include <cstring>
#include <cassert>
#include <variant>
#include <vector>
#include <memory>
#include <format>
#include <charconv>
#include <algorithm>

static constexpr std::string_view keywords[] = {
    "if", "else", "for", "while", "return",
    "break", "continue", "true", "false",
    "fn", "let", "const", "struct"
};

#define TODO assert(0 && "TODO");

struct Error {
    std::string message;
    std::string_view context; //position in the input stream where we errored

    constexpr Error() = default;
    constexpr Error(std::string_view msg, std::string_view ctx)
        : message(msg), context(ctx){}

    constexpr explicit operator bool() const noexcept {
        return !message.empty();
    }

    std::string_view what() const noexcept { return message; }
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

    // misc / extended
    Arrow,Dot,


};
struct Op {
    Operator kind = Operator::Invalid;

    constexpr Op() = default;
    constexpr Op(Operator k) : kind(k) {}

    constexpr operator std::string_view() const noexcept {
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

    constexpr explicit operator bool() const noexcept {
        return kind != Operator::Invalid;
    }

    constexpr int precedence() const noexcept {
	    switch (kind) {
	        case Operator::Assign: return 1;
	        case Operator::OrOr:   return 2;
	        case Operator::AndAnd: return 3;
	        case Operator::EqEq:
	        case Operator::NotEq:  return 4;
	        case Operator::Lt:
	        case Operator::Gt:
	        case Operator::Le:
	        case Operator::Ge:     return 5;
	        case Operator::Plus:
	        case Operator::Minus:  return 6;
	        case Operator::Star:
	        case Operator::Slash:
	        case Operator::Percent:return 7;
	        case Operator::Arrow:
	        case Operator::Dot:    return 8;
	        default:               return 0;
	    }
	}


    //associativity
    constexpr bool right_assoc() const noexcept {
        return kind == Operator::Assign;
    }

    // comparison support for convenience
    constexpr bool operator==(Operator k) const noexcept { return kind == k; }
    constexpr bool operator!=(Operator k) const noexcept { return kind != k; }
};


//base class used for text things
struct Token {
	std::string_view text;//view of the original text (can do pointer arithmetic)
	operator std::string_view() const noexcept {
        return text;
    }
};

struct Invalid : Token {};

struct Var : Token {
	Var(std::string_view name)  {
		text=name;
	}
};

struct Num : Token {
	uint64_t value;
};

struct ParseStream{
	std::string_view full;
	std::string_view current;

	void advance(int amount){
		current={current.data()+amount,current.size()-amount};
	}

	bool skip_whitespace(){
		bool skiped = false;
		while (!current.empty() && std::isspace(current.front())) {
	        skiped = true;
	        current.remove_prefix(1);
	    }

	    return skiped;	    
	}


	const char* marker(){
		return current.data();
	}

	bool empty(){
		skip_whitespace();
		return current.empty();
	}


	bool starts_with(std::string_view pre){
		if(current.size()<pre.size())
			return false;

		return std::memcmp(current.data(),pre.data(),pre.size())==0;
	}

	bool try_consume(std::string_view pre){
		skip_whitespace();
		if(starts_with(pre)){
			advance(pre.size());
			return true;
		}

		return false;
	}

	bool try_consume(std::string_view pre,Token& out){
		skip_whitespace();
		auto* start = current.data();
		if(!try_consume(pre))
			return false;

		out.text = {start,current.data()};
		return true;
	}

	Error consume(std::string_view pre,std::string_view expected){
		if(try_consume(pre))
			return Error();

		return Error(std::format("expected {} found {}",expected,found_token()),current);
	}

	Error consume(std::string_view pre){
		return consume(pre,pre);
	}

	std::string_view found_token(){
		std::string_view backup = current;

		skip_whitespace();
		if(current.empty()){
			current = backup;
			return "EOF";
		}

		for(auto e : keywords){
			if(current.starts_with(e))
				return e;
		}

		std::string_view ans = try_name();
		if(ans.size()){
			current = backup;
			return ans;
		}

		Op op = try_operator();
		if(op){
			current = backup;
			return op;
		}

		ans = {current.data(),1};//we arent empty
		current = backup;
		return ans ;
	}

	std::string_view try_name(){
		skip_whitespace();
		if(current.empty()|| !std::isalpha(current.front()))
			return {nullptr,0};

		const char* base = current.data();
		while(!current.empty() && 
			(std::isalpha(current.front()) 
				|| std::isdigit(current.front())
				|| current.front()=='_')
			)
		{
			current.remove_prefix(1);
		}

		std::string_view name{base,current.data()};
		for (auto kw : keywords) {
	        if (kw == name){
	        	current = {base,current.end()};
	            return {}; // treat as no identifier
	        }
	    }

		return name;
	}

	Error consume_name(std::string_view& name){
		name = try_name();
		if(!name.size())
			return Error(std::format("expected NAME found {}",found_token()),current);
		else
			return Error();
	}

	Op try_operator() {
	    skip_whitespace();
	    if (current.empty())
	        return {};

	    // Try longest operators first — order matters!
	    if (try_consume("++")) return Op(Operator::PlusPlus);
	    if (try_consume("--")) return Op(Operator::MinusMinus);
	    if (try_consume("->")) return Op(Operator::Arrow);
	    if (try_consume("&&")) return Op(Operator::AndAnd);
	    if (try_consume("||")) return Op(Operator::OrOr);
	    if (try_consume("==")) return Op(Operator::EqEq);
	    if (try_consume("!=")) return Op(Operator::NotEq);
	    if (try_consume("<=")) return Op(Operator::Le);
	    if (try_consume(">=")) return Op(Operator::Ge);

	    // Then all single-character operators
	    if (try_consume("+")) return Op(Operator::Plus);
	    if (try_consume("-")) return Op(Operator::Minus);
	    if (try_consume("*")) return Op(Operator::Star);
	    if (try_consume("/")) return Op(Operator::Slash);
	    if (try_consume("%")) return Op(Operator::Percent);
	    if (try_consume(".")) return Op(Operator::Dot);
	    if (try_consume("&")) return Op(Operator::BitAnd);
	    if (try_consume("|")) return Op(Operator::BitOr);
	    if (try_consume("^")) return Op(Operator::BitXor);
	    if (try_consume("!")) return Op(Operator::Not);
	    if (try_consume("=")) return Op(Operator::Assign);
	    if (try_consume("<")) return Op(Operator::Lt);
	    if (try_consume(">")) return Op(Operator::Gt);

	    // Nothing matched
	    return {};
	}



	Num try_number() {
	    skip_whitespace();
	    if (current.empty())
	        return Num{};

	    const char* start = current.data();
	    const char* ptr   = start;


	    // Require at least one digit after optional sign
	    if (ptr == current.end() || !std::isdigit(static_cast<unsigned char>(*ptr)))
	        return Num{};

	    // Consume all digits
	    ptr = std::find_if_not(ptr, current.end(), [](unsigned char c) { return std::isdigit(c); });

	    Num ans;
	    ans.text  = {start, ptr};
	    ans.value = 0;

	    std::from_chars(start, ptr, ans.value);

	    current.remove_prefix(static_cast<size_t>(ptr - current.data()));
	    return ans;
	}


};

struct Expression;



struct PreOp : Token {
    std::unique_ptr<Expression> exp;
    std::string_view op;

    PreOp() = default;
    PreOp(std::string_view o) : op(o) {}
    PreOp(std::string_view o, Expression expr);
};


struct BinOp : Token{
	std::unique_ptr<Expression> a;
	std::unique_ptr<Expression> b;
	std::string_view op;
};

struct Call : Token{
	std::unique_ptr<Expression> func;
	std::vector<Expression> args;
};

using ExpressionVariant = std::variant<Invalid,Var,Num,PreOp,BinOp,Call>;
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

PreOp::PreOp(std::string_view o, Expression expr)
    : exp(std::make_unique<Expression>(std::move(expr))),
      op(o) {}



inline Error parse_expression(ParseStream& stream,Expression& exp);

inline Error parse_call_args(ParseStream& stream,Call& out){
	Expression tmp;
	Error err;

	err=stream.consume("(");
	if(err) return err;

	//check for easy empty
	
	if(stream.try_consume(")"))
		return Error();
	
	err=parse_expression(stream,tmp);
	if(err) return err;
	out.args.push_back(std::move(tmp));

	
	while(stream.try_consume(",")){
		
		
		err=parse_expression(stream,tmp);
		if(err) return err;
		out.args.push_back(std::move(tmp));

		
	}

	return stream.consume(")");
}


inline Error parse_paren_expression(ParseStream& stream,Expression& out){
	Error res;
	
	stream.skip_whitespace();
	const char* start = stream.marker();

	res = stream.consume("(");
	if(res) return res;

	res = parse_expression(stream,out);
	if(res) return res;
	
	
	res =  stream.consume(")");
	if(res) return res;

	out.tok().text = {start,stream.marker()};
	return res;
}

inline Error parse_atom(ParseStream& stream,Expression& out){
	if(stream.starts_with("("))
		return parse_paren_expression(stream,out);

	Num n = stream.try_number();
	if(n.text.size()){
		out.inner = Invalid{};
		out.inner = std::move(n);
		return Error();
	}

	auto name = stream.try_name();
	if(name.size()){
		out.inner = Var(name);
		return Error();
	}

	return Error(std::format("expected VALUE found {}\n",stream.found_token()),stream.current);
}

inline Error parse_pres_and_atom(ParseStream& stream,Expression& out){
	stream.skip_whitespace();
	const char* start = stream.marker();
	auto pre_op = stream.try_operator(); 

	if(!pre_op)
		return parse_atom(stream,out);

	PreOp& handle = out.inner.emplace<PreOp>(pre_op);
	handle.exp=std::make_unique<Expression>();//we need the mem
	Error res = parse_pres_and_atom(stream,*handle.exp);
	handle.text = {start,stream.current.data()};

	return res;
}

inline Error parse_expression_core(ParseStream& stream,Expression& out){
	stream.skip_whitespace();
	const char* start = stream.marker();

	Error res = parse_pres_and_atom(stream,out);
	if(res) return res;


	auto op = stream.try_operator();
	if(!op)
		return Error();

	struct LHS {
		Op op;
		Expression exp ;
	};

	std::vector<LHS> parts;
	parts.emplace_back(LHS{
		op,
	    std::move(out)
	});

	auto collapse_ops = [&](int prec_limit) {
	    while (parts.size() >= 2 && parts.back().op.precedence() >= prec_limit) {
	        auto rhs = std::move(parts.back().exp);
	        auto op  = parts.back().op;
	        parts.pop_back();

	        auto& lhs = parts.back().exp;

	        BinOp bin;
	        bin.a  = std::make_unique<Expression>(std::move(lhs));
	        bin.b  = std::make_unique<Expression>(std::move(rhs));
	        bin.op = op;
	        lhs.inner = std::move(bin);
	    }
	};


	for (; op; op = stream.try_operator()) {
	    Expression right;
	    res = parse_pres_and_atom(stream, right);
	    if (res) return res;

	    int prec = op.precedence();

	    collapse_ops(prec);
	    parts.emplace_back(LHS{ op,std::move(right) });
	}

	collapse_ops(-1024);
	out = std::move(parts.front().exp);

	out.tok().text = {start,stream.marker()};
	return res;
}

inline Error parse_expression(ParseStream& stream,Expression& out){
	Error res = parse_expression_core(stream,out);
	if(res) return res;

	const char* start = out.tok().text.begin();
	while(stream.starts_with("(")){
		Call call;
		call.func = std::make_unique<Expression>(std::move(out));
		res = parse_call_args(stream,call);
		if(res) return res;

		call.text = {start,stream.marker()};
		out.inner = std::move(call);
	}

	return res;
}

struct statement;

struct Basic : Token {
	Expression inner;
};

struct Return : Token {
	Expression val;
};

struct Block : Token {
	std::vector<statement> parts;
};

struct Condstatement : Token {
	Expression cond;
	Block block;
};

struct While : Condstatement {};
struct If : Condstatement {
	Block else_part;
};


using statementVariant = std::variant<Invalid,While,If,Return,Block,Basic>;
struct statement {
	statementVariant inner;
	operator std::string_view() const noexcept {
        return std::visit([](auto && arg){
        	return (std::string_view)arg;
        },inner);
    }
};




inline Error parse_statement(ParseStream& stream,statement& out);

inline Error parse_proper_block(ParseStream& stream,Block& out){
	Error res = Error();	
	stream.skip_whitespace();
	const char* start = stream.marker();

	res = stream.consume("{");
	if(res) return res;

	statement stmt;
	for(;;){
		
		if(stream.try_consume("}")){
			out.text = {start,stream.marker()};
			return res;
		}

		if(stream.empty()){
			return Error("expected statement or '}' found EOF\n",stream.current);
		}

		res=parse_statement(stream,stmt);
		if(res) return res;
		out.parts.push_back(std::move(stmt));
	}
}

inline Error parse_block(ParseStream& stream,Block& out){
	if(stream.try_consume(";",out)){
		return Error();
	}

	stream.skip_whitespace();
	if(stream.starts_with("{"))
		return parse_proper_block(stream,out);
	
	statement stmt;
	auto res = parse_statement(stream,stmt);
	if(res) return res;
	out.parts.push_back(std::move(stmt));
	return res;
}


inline Error parse_statement(ParseStream& stream,statement& out){
	Error res;
	stream.skip_whitespace();
	const char* start = stream.marker();
	
	if(stream.try_consume("while")){
		While& handle = out.inner.emplace<While>();
		res = parse_expression(stream,handle.cond);
		if(res) return res;

		res = parse_block(stream,handle.block);
		if(res) return res;

		handle.text = {start,stream.marker()};
		return res;
	}

	if(stream.try_consume("if")){
		If& handle = out.inner.emplace<If>();
		res = parse_expression(stream,handle.cond);
		if(res) return res;

		res = parse_block(stream,handle.block);
		if(res) return res;

		if(stream.try_consume("else")){
			res = parse_block(stream,handle.block);
			handle.text = {start,stream.marker()};
		}

		return res;
	}

	if(stream.try_consume("return")){
		Return& handle = out.inner.emplace<Return>();
		res = parse_expression(stream,handle.val);
		if(res) return res;

		
		stream.try_consume(";");

		handle.text = {start,stream.marker()};
		return res;
	}

	Basic& handle = out.inner.emplace<Basic>();
	res = parse_expression(stream, handle.inner);
	if (res) return res;

	res = stream.consume(";");
	if (res) return res;

	handle.text = { start, stream.marker() };
	return res;


}

