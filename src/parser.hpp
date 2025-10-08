#pragma once


#include <cctype>
#include <cstring>
#include <cassert>
#include <format>
#include <charconv>
#include <algorithm>

#include "ast.hpp"
#include "ast_print.hpp"

namespace small_lang {

static constexpr std::string_view keywords[] = {
    "if", "else", "for", "while", "return",
    "fn", "cfn",
    //not used but like comeon
    "break", "continue", "true", "false",
    "let","as","is", "const", "struct"
};


struct ParseError {
    std::string message;
    std::string_view context; //position in the input stream where we ParseErrored

    constexpr ParseError() = default;
    constexpr ParseError(std::string_view msg, std::string_view ctx)
        : message(msg), context(ctx){}

    constexpr explicit operator bool() const noexcept {
        return !message.empty();
    }

    std::string_view what() const noexcept { return message; }
};


static constexpr Bp CALL_BP = 16;
static constexpr Bp SUBSCRIPT_BP = 16;
static constexpr Bp CAST_BP = 15;



constexpr Bp  Op::bp_prefix() const noexcept {
    switch (kind) {
        case Operator::Plus:        // unary +
        case Operator::Minus:       // unary -
        case Operator::Not:         // logical not
        case Operator::BitAnd:      // address-of
        case Operator::Star:        // deref
        case Operator::PlusPlus:    // pre-increment
        case Operator::MinusMinus:  // pre-decrement
            return 16; // tight binding (C-style unary)
        default:
            return 0;   // not a prefix op
    }
}

constexpr Bp  Op::bp_infix_left() const noexcept {
    switch (kind) {
        case Operator::Dot:
        case Operator::Arrow:       return 20;
        case Operator::Star:
        case Operator::Slash:
        case Operator::Percent:     return 14;
        case Operator::Plus:
        case Operator::Minus:       return 13;
        case Operator::Lt:
        case Operator::Gt:
        case Operator::Le:
        case Operator::Ge:          return 11;
        case Operator::EqEq:
        case Operator::NotEq:       return 10;
        case Operator::BitAnd:      return 9;
        case Operator::BitXor:      return 8;
        case Operator::BitOr:       return 7;
        case Operator::AndAnd:      return 6;
        case Operator::OrOr:        return 5;
        case Operator::Assign:      return 3;
        default:
            return 0;
    }
}

constexpr Bp  Op::bp_infix_right() const noexcept {
    switch (kind) {
        // left-associative ops use same as left
        case Operator::Dot:
        case Operator::Arrow:       return 20;
        case Operator::Star:
        case Operator::Slash:
        case Operator::Percent:     return 14;
        case Operator::Plus:
        case Operator::Minus:       return 13;
        case Operator::Lt:
        case Operator::Gt:
        case Operator::Le:
        case Operator::Ge:          return 11;
        case Operator::EqEq:
        case Operator::NotEq:       return 10;
        case Operator::BitAnd:      return 9;
        case Operator::BitXor:      return 8;
        case Operator::BitOr:       return 7;
        case Operator::AndAnd:      return 6;
        case Operator::OrOr:        return 5;

         // right-assoc
        case Operator::Assign:      return 4;
        default:
            return 0;
    }
}

constexpr Bp  Op::bp_postfix() const noexcept {
    switch (kind) {
    	// case Operator::Paren:
        // case Operator::Bracket:     return 16;

        case Operator::PlusPlus:    
        case Operator::MinusMinus:  return 15;
        default:
            return 0;
    }
}


struct ParseStream{
	std::string_view full;
	std::string_view current;

	ParseStream(std::string_view text) : full(text),current(text) {}


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

	ParseError consume(std::string_view pre,std::string_view expected){
		if(try_consume(pre))
			return ParseError();

		return ParseError(std::format("expected {} found {}",expected,found_token()),current);
	}

	ParseError consume(std::string_view pre){
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

	ParseError consume_name(Var& name){
		return consume_name(name.text);
	}
	ParseError consume_name(std::string_view& name){
		name = try_name();
		if(!name.size())
			return ParseError(std::format("expected NAME found {}",found_token()),current);
		else
			return ParseError();
	}

	Op peek_operator() {
	    skip_whitespace();
	    if (current.empty())
	        return {};

	    // Longest operators first
	    if (starts_with("++")) return Op(Operator::PlusPlus);
	    if (starts_with("--")) return Op(Operator::MinusMinus);
	    if (starts_with("->")) return Op(Operator::Arrow);
	    if (starts_with("&&")) return Op(Operator::AndAnd);
	    if (starts_with("||")) return Op(Operator::OrOr);
	    if (starts_with("==")) return Op(Operator::EqEq);
	    if (starts_with("!=")) return Op(Operator::NotEq);
	    if (starts_with("<=")) return Op(Operator::Le);
	    if (starts_with(">=")) return Op(Operator::Ge);

	    // Single-character operators
	    if (starts_with("+")) return Op(Operator::Plus);
	    if (starts_with("-")) return Op(Operator::Minus);
	    if (starts_with("*")) return Op(Operator::Star);
	    if (starts_with("/")) return Op(Operator::Slash);
	    if (starts_with("%")) return Op(Operator::Percent);
	    if (starts_with(".")) return Op(Operator::Dot);
	    if (starts_with("&")) return Op(Operator::BitAnd);
	    if (starts_with("|")) return Op(Operator::BitOr);
	    if (starts_with("^")) return Op(Operator::BitXor);
	    if (starts_with("!")) return Op(Operator::Not);
	    if (starts_with("=")) return Op(Operator::Assign);
	    if (starts_with("<")) return Op(Operator::Lt);
	    if (starts_with(">")) return Op(Operator::Gt);

	    return {};
	}

	Op try_operator() {
	    skip_whitespace();
	    const Op op = peek_operator();
	    if (!op) return {};

	    // Consume actual characters based on the matched operator text
	    advance(static_cast<std::string_view>(op).size());
	    return op;
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


inline ParseError parse_statement(ParseStream& stream,Statement& out);
inline ParseError parse_expression(ParseStream& stream,Expression& exp,Bp min_bp = 0);


inline ParseError parse_atom(ParseStream& stream,Expression& out){
	stream.skip_whitespace();

	Num n = stream.try_number();
	if(n.text.size()){
		out.inner = std::move(n);
		return ParseError();
	}

	auto name = stream.try_name();
	if(name.size()){
		out.inner = Var(name);
		return ParseError();
	}

	return ParseError(std::format("expected VALUE found {}\n",stream.found_token()),stream.current);
}

inline ParseError parse_paren_expression(ParseStream& stream,Expression& out){
	ParseError res;
	
	stream.skip_whitespace();
	const char* start = stream.marker();

	res =  stream.consume("(");
	if(res) return res;

	res = parse_expression(stream,out);
	if(res) return res;
	
	
	res =  stream.consume(")");
	if(res) return res;

	out.tok().text = {start,stream.marker()};
	return res;
}

inline ParseError parse_call_args(ParseStream& stream,Call& out){
	Expression tmp;
	ParseError err;

	err=stream.consume("(");
	if(err) return err;

	//check for easy empty
	
	if(stream.try_consume(")"))
		return ParseError();
	
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

inline ParseError parse_type(ParseStream& stream,TypeDec& type){
	stream.skip_whitespace();
	const char* start = stream.marker();

	ParseError res = stream.consume("@");
	if(res) return res;

	res = stream.consume_name(type.name);
	if(res) return res;

	type.text = {start,stream.marker()};
	return res;
}

//HEAVILY inspired by https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
inline ParseError parse_expression(ParseStream& stream,Expression& out,Bp min_bp){
	ParseError res;
	
	stream.skip_whitespace();
	const char* start = stream.marker();

	//recursively get the start
	Op op = stream.try_operator();
	if(op){
		res = parse_expression(stream,out,op.bp_prefix());
		if(res) return res;
		out.inner = PreOp(op,std::move(out),{start,stream.marker()});
	}
	else if(stream.starts_with("(")){
		res = parse_paren_expression(stream,out);
		if(res) return res;
	}
	else if(stream.starts_with("@")){
		TypeCast cast;
		res = parse_type(stream,cast.type);
		if(res) return res;

		cast.exp = std::make_unique<Expression>();
		res = parse_expression(stream,*cast.exp,CAST_BP);
		if(res) return res;

		cast.text = {start,stream.marker()};
		out.inner = std::move(cast);
	}
	else{
		res = parse_atom(stream,out);
		if(res) return res;
	}


	for(;;){
		//special cases first
		stream.skip_whitespace();
		if(stream.starts_with("(")){
			if(CALL_BP < min_bp) break;
			Call call;
			res = parse_call_args(stream,call);
			if(res) return res;

			call.func = std::make_unique<Expression>(std::move(out));
			call.text = {start,stream.marker()};
			out.inner = std::move(call);
			
			continue;
		}

		if(stream.starts_with("[")){			
			if(SUBSCRIPT_BP < min_bp) break;
			stream.try_consume("[");

			SubScript sub;

			sub.idx = std::make_unique<Expression>();
			res = parse_expression(stream,*sub.idx);
			if(res) return res;

			res = stream.consume("]");
			if(res) return res;


			sub.arr = std::make_unique<Expression>(std::move(out));
			sub.text = {start,stream.marker()};
			out.inner = std::move(sub);
			
			continue;
		}

		//common case
		op = stream.peek_operator();
		if(!op) break;

		Bp b = op.bp_postfix();
		if(b){
			if(b<min_bp) break;
			stream.try_operator();//skip the operator
			out.inner = PreOp(op,std::move(out),{start,stream.marker()});

			continue;
		}

		Bp lbp = op.bp_infix_left();
		if(!lbp || lbp<min_bp) break;
		stream.try_operator();



		
		BinOp bin;
		bin.op = op;
		bin.b = std::make_unique<Expression>();
		res = parse_expression(stream,*bin.b,op.bp_infix_right());
		if(res) return res;

		bin.a = std::make_unique<Expression>(std::move(out));
		bin.text = {start,stream.marker()};

		out.inner = std::move(bin);

	}

	return ParseError();
}



inline ParseError parse_proper_block(ParseStream& stream,Block& out){
	ParseError res = ParseError();	
	stream.skip_whitespace();
	const char* start = stream.marker();

	res = stream.consume("{");
	if(res) return res;

	Statement stmt;
	for(;;){
		
		if(stream.try_consume("}")){
			out.text = {start,stream.marker()};
			return res;
		}

		if(stream.empty()){
			return ParseError("expected statement or '}' found EOF\n",stream.current);
		}

		res=parse_statement(stream,stmt);
		if(res) return res;
		out.parts.push_back(std::move(stmt));
	}
}

inline ParseError parse_block(ParseStream& stream,Block& out){
	if(stream.try_consume(";",out)){
		return ParseError();
	}

	stream.skip_whitespace();
	if(stream.starts_with("{"))
		return parse_proper_block(stream,out);
	
	Statement stmt;
	auto res = parse_statement(stream,stmt);
	if(res) return res;
	out.parts.push_back(std::move(stmt));
	return res;
}


inline ParseError parse_statement(ParseStream& stream,Statement& out){
	ParseError res;
	stream.skip_whitespace();
	const char* start = stream.marker();
	
	if(stream.starts_with("{")){
		auto& b = out.inner.emplace<Block>();
		return parse_proper_block(stream,b);
	}


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


inline ParseError parse_func_args(ParseStream& stream,FuncDec& out){
	Var tmp;
	ParseError err;

	err=stream.consume("(");
	if(err) return err;

	//check for easy empty
	
	if(stream.try_consume(")"))
		return ParseError();
	
	err=stream.consume_name(tmp);
	if(err) return err;
	out.args.push_back(std::move(tmp));

	
	while(stream.try_consume(",")){
		err=stream.consume_name(tmp);
		if(err) return err;
		out.args.push_back(std::move(tmp));

		
	}

	return stream.consume(")");
}


inline ParseError parse_global(ParseStream& stream,Global& out){
	ParseError res;
	stream.skip_whitespace();
	const char* start = stream.marker();

	FuncDec sig;
	sig.is_c = stream.try_consume("cfn");
	
	if(sig.is_c || stream.try_consume("fn")){
		res = stream.consume_name(sig.name);
		if(res) return res;

		res = parse_func_args(stream,sig);
		if(res) return res;


		if(stream.try_consume(";")){
			sig.text = { start, stream.marker() };
			out.inner = std::move(sig);
			return res;
		}

		Function& func = out.inner.emplace<Function>();
		static_cast<FuncDec&>(func) = std::move(sig);

		res = parse_proper_block(stream,func.body);
		func.text = { start, stream.marker() };
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

};//small_lang