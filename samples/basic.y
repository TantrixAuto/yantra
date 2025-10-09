// define two AST walkers, one for generating C++ code and the other for Java
%walkers CppWalker JavaWalker;

// this is the start rule. It recognizes a list of statements
start := stmts;

// this pair of rules recognizes a list of statements
stmts := stmts stmt;
stmts := stmt;

// this rule recognizes a simple assignment statement.
// it has code blocks for each of the two defined walkers.
stmt := ID(I) EQUAL qualifiedID(qid) SEMI
@CppWalker
%{
    std::cout << I.text << " = " << str(qid) << std::endl;
%}
@JavaWalker
%{
    std::cout << I.text << " = " << str(qid) << std::endl;
%}

// This pair of rules recognises a qualifiedID, separated by double-colons
// Each rule defines a str() function
%function qualifiedID CppWalker::str() -> std::string;
%function qualifiedID JavaWalker::str() -> std::string;
qualifiedID := qualifiedID(qid) DBLCOLON ID(I)
@CppWalker::str
%{
    return str(qid) + "::" + I.text;
%}
@JavaWalker::str
%{
    return str(qid) + "." + I.text;
%}

qualifiedID := ID(I)
@CppWalker::str
%{
    return I.text;
%}
@JavaWalker::str
%{
    return I.text;
%}

//This defines all the lexer tokens
ID := "[\l][\l\d_]*";
DBLCOLON := "::";
EQUAL := "=";
SEMI := ";";
WS := "\s"!;

