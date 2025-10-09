# Grammar File Syntax
The input grammar file is a set of lines, where each line is one of the following
- a pragma definition, starting with % as the first character
- a rule definition, starting with the rule name which has the first character in lowercase
- a token definition, where the token name is in all caps.

e.g:
```
1: %class MyModule;
2: start := stmt SEMI;
3: stmt := VAR ID SEMI;

4: VAR := "var";
5: ID := "[\l][\l\d_]*";
6: SEMI := ";";
```

Here, line 1 is a pragma directive.

Lines 2 and 3 are rule definitions.

Lines 4, 5 and 6 are token definitions.

# Pragma definitions
Pragmas are a set of directives that alter the behaviour of the parser generator and the generated code.

A complete list of all pragmas can be found [below](#supported-pragma-directives-in-yantra).

# Token definitions
Tokens are the terminal nodes (or leaf nodes) in the Abstract Syntax Tree.

A token definition consists of:
- a token name, which must consist of uppercase letters,
- followed by a `:=`
- followed by a regular expression that defines the token.
- optionally followed by a precedence specifier
- ending with either a semi-colon

This is an example of a token that ends with a semi-colon.
```
STAR := "\*";
```

This is an example of a token that defines a precedence.
```
SLASH := "/" [STAR];
```
In this case, the SLASH token has the same precedence as the STAR token.

# Rule definitions
Rules are the most essential and fundamental element in a Yantra grammar file.
They define the basic structure of the grammar to be parsed.
They are the non-terminal nodes in the Abstract Syntax tree.

A rule consists of
- a rule name, which must start with a lowercase letter,
- followed by a `:=`
- followed by one or more nodes, which are references to either othre rules, or tokens.
- ending with either a semi-colon, or
- ending with one or more semantic actions, also known as functions.

This is an example of a rule that ends with a semi-colon.
```
start := stmt SEMI;
```

This is an example of a rule that ends with one function.
```
start := stmt SEMI
%{
    std::print("recognized start\n");
%}
```

This is an example of a rule that ends with multiple functions.
```
start := stmt SEMI
@CppWalker %{
    std::print("recognized start for C++ output\n");
%}
@JavaWalker %{
    std::print("recognized start for Java output\n");
%}
```

All nodes in a rule, whether terminal or non-terminal nodes, can have a variable name associated with it.
This name can be used to refer to that node from with the functions.
The name must follow the same naming convention as that of the node.

That is,
- if the node refers to a rule, the variable name must start with a lowercase letter (e.g: `rStatement`, `expr`, etc)
- if the node refers to a token, the variable must be all caps (`VAR`, `IDENTFIER`, `PLUS`, etc).

e.g:
```
stmt := VAR ID(I) EQUAL expr(e) SEMI
%{
    auto val = eval(e);
    vars[I.text] = val;
%}
```
Here, `eval()` is a user-defined function, defined against the `expr` rule.
`I` is a token recognised by the lexer.
Its member `std::string text` contains the text of the token as read from the input stream.

The token has another member `FilePos pos` that specifies where in the input stream this token was recognised.

# Supported pragma directives in Yantra
The following is the list of pragmas supported by Yantra
| Name                | Syntax | Repeatable | Scope | Description |
|---------------------|--------|-------------|------|-------------|
| class               | `%class MyModule;` | No | Grammar | Defines the name of the class representing this Parser |
| namespace           | `%namespace ast::MyGrammar;` | No | Grammar | Defines the namespace for the Parser class |
| pch_header          | `%pch_header "pch.hpp";` | No | Grammar | The precompiled header, if any, for the cpp file |
| std_header          | `%std_header no;` | No | Grammar | All necessary standard headers are included in the generated source file by default. <br/>Use this to disable, if for example, standard headers are included via the PCH header file, or any other header files |
| hdr_header          | `%hdr_header <fstream>;`<br/>`%hdr_header "config.hpp";` | Yes | Grammar | Header files to be added to the generated header file |
| src_header          | `%src_header <assert.h>;` | Yes | Grammar | Header files to be added to the generated source file |
| class_member        | `%class_member int value = 0;` | Yes | Grammar | Additional members to be added to the Parser class |
| encoding            | `%encoding utf8;` | No | Grammar | The character encoding for the parser input.<br/>Currently supports `utf8` by default, or `ascii`  |
| check_unused_tokens | `%check_unused_tokens no;` | No | Grammar | Yantra gives an error if any tokens are not used in the rules.<br/>Enabled by default, use this pragma to disable |
| auto_resolve        | `%auto_resolve no;` | No | Grammar | Yantra attempts to automatically resolve SHIFT-REDUCE conflicts.<br/>Enabled by default, use this pragma to disable |
| warn_resolve        | `%warn_resolve no;` | No | Grammar | If auto_resolve is enabled, Yantra gives a warning when conflicts are resolved.<br/>Enabled by default, use this pragma to disable |
| walkers             | `%walkers CppWalker JavaWalker;` | No | Grammar | Specify list of walkers used in this Parser.<br/>See [Walkers](020_concepts.md#walkers) in concepts for more details |
| default_walker      | `%default_walker JavaWalker;` | No | Grammar | Set the default walker to use with unnamed semantic actions |
| walker_output       | `%walker_output CppWalker text_file cpp;` | Yes | Walker | Set the output type for the specified walker.<br/>Can be `text_file` or `binary_file`, followed by the extension for the generated file<br/>See [Walker Output](020_concepts.md#walker-output) in concepts for more details |
| walker_traversal    | `%walker_traversal CppWalker top-down;` | Yes | Walker | Set the traversal type for the specified walker.<br/>Can be `top-down` or `manual`<br/>See [Walker Traversal](020_concepts.md#walker-traversal) in concepts for more details |
| members             | `%members CppWalker int i = 0;` | Yes | Walker | Set additional class members for the specified walker |
| error               | `%error %{ ... %}` | No | Grammar | Set codeblock for handling errors |
| start               | `%start entry_rule;` | No | Grammar| Set name of initial rule. Default value `start` |
| function            | `%function stmt_rule CppWalker::str() -> std::string;` | Yes | Rule | Define additional functions associated with a rule set<br/>See [Functions](020_concepts.md#functions) in concepts for more details |
| left                | `%left PLUS STAR;` | Yes | Lexer | Specify left association for given list of tokens |
| right               | `%right ASSIGN_EQ;` | Yes | Lexer | Specify right association for given list of tokens |
| token               | `%token SEMI VAR;` | Yes | Lexer | Specify no association for given list of tokens |
| fallback            | `%fallback ID VAR WHILE;` | Yes | Lexer | Specify fallabck for given list of tokens. If VAR is not a valid token in any rule, try it as an ID token |
| lexer_mode          | `%lexer_mode ML_COMMENT;` | Yes | Lexer | Start a new Lexer mode<br/>See [Lexer Modes](020_concepts.md#lexer-modes) in concepts for more details |
