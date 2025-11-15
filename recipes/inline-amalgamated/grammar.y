%walkers Compiler;

start := expr(e)
%{
    auto v = eval(e);
    if(v > 0) {}
    std::println("Result(inline-amalgamated): {}", v);
%}

%function expr Compiler::eval() -> int;
expr := addexpr(e)
@Compiler::eval
%{
    auto v = eval(e);
    return v;
%}

%function addexpr Compiler::eval() -> int;
addexpr := addexpr(a) PLUS mulexpr(v)
@Compiler::eval
%{
    auto av = eval(a);
    auto vv = eval(v);
    auto rv = av + vv;
    return rv;
%}

addexpr := mulexpr(e)
@Compiler::eval
%{
    auto v = eval(e);
    return v;
%}

%function mulexpr Compiler::eval() -> int;
mulexpr := mulexpr(m) STAR valexpr(a)
@Compiler::eval
%{
    auto mv = eval(m);
    auto av = eval(a);
    auto v = mv * av;
    return v;
%}

mulexpr := valexpr(e)
@Compiler::eval
%{
    auto v = eval(e);
    return v;
%}

%function valexpr Compiler::eval() -> int;
valexpr := LPAREN expr(e) RPAREN
@Compiler::eval
%{
    auto v = eval(e);
    return v;
%}

valexpr := NUM(N)
@Compiler::eval
%{
    auto v = std::stoi(N.text);
    return v;
%}

STAR := "\*";
PLUS := "\+";

LPAREN := "\(";
RPAREN := "\)";

NUM := "[0-9]+";
WS := "\s"!;
