%walkers Compiler;
%walker_interface Compiler CompilerBase;

start := expr(e);

%function expr Compiler::eval() -> int;
expr := addexpr(e);

%function addexpr Compiler::eval() -> int;
addexpr := addexpr(a) PLUS mulexpr(v);

addexpr := mulexpr(e);

%function mulexpr Compiler::eval() -> int;
mulexpr := mulexpr(m) STAR valexpr(a);

mulexpr := valexpr(e);

%function valexpr Compiler::eval() -> int;
valexpr := LPAREN expr(e) RPAREN;

valexpr := NUM(N);

STAR := "\*";
PLUS := "\+";

LPAREN := "\(";
RPAREN := "\)";

NUM := "[0-9]+";
WS := "\s"!;
