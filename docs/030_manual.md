### Grammar File Syntax
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

### Pragma definitions
Pragmas are a set of directives that alter the behaviour of the parser generator and the generated code.

A complete list of all pragmas can be found [below](#supported-pragma-directives-in-yantra).

### Token definitions
Tokens are the terminal nodes (or leaf nodes) in the Abstract Syntax Tree.

A token definition consists of:
- a token name, which must consist of uppercase letters,
- followed by a `:=`
- followed by a regular expression that defines the token.
- optionally followed by a lexer mode specifier
- ending with either a semi-colon

This is an example of a token that ends with a semi-colon.
```
STAR := "\*";
```

### Lexer Modes
Yantra supports multiple lexer modes, where the same regex can result in a different token, depending on the current lexer mode.

Consider the typical use-case of multi-line comments.
```
ENTER_MLCOMMENT := "/\*"! [ML_COMMENT_MODE];
```
This line says that when a `/*` is recognised, switch to `ML_COMMENT_MODE` mode.

The ! at the end of the regex indicates that the ENTER_MLCOMMENT token will not be used by the grammar.

Further down in the same file, we define the ML_COMMENT_MODE mode:
```
%lexer_mode ML_COMMENT_MODE;
```

In the ML_COMMENT_MODE mode, we redefine the ENTER_MLCOMMENT token, as follows:
```
ENTER_MLCOMMENT := "/\*"! [ML_COMMENT_MODE];
```
This says that if a `/*` is seen in the `ML_COMMENT_MODE`, then re-enter the `ML_COMMENT_MODE` recursively.

Next, we define the regex to leave the multi-line comment mode, as follows:
```
LEAVE_MLCOMMENT := "\*/"! [^];
```
Here, the caret ^ tells the lexer to go to the previous lexer mode.

This is a very significant concept. Lexer modes are saved in a stack.

Whenever the lexer enters a mode, it pushes the current mode onto the stack and goes to the new mode.

When it detects a caret, it pops the current mode and goes to the previous mode.

This allows for, in this example, nested multi-line comments.

e.g:
```
/*
This is a multi-line comment.
    /*
    This is another multi-line comment inside the first one.
    */
We are back inside the first comment.
*/
```

Finally, we have a regex that catches and ignore all other characters inside the comment.
```
CMT := ".*"!;
```


### Rule definitions
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

### Walkers
Parser generators such as YACC, BISON and LEMON allow us to attach a semantic action (typically a C or C++ code block) with a production, and this action is invoked as soon as the production is reduced.

The C code block is typically used to create AST nodes, or perform some other relevant action.
Because the actions are executed when productions are reduced, the executions happen in a bottom-up order.

On the other hand, Yantra automatically creates an AST, and the semantic actions are invoked while walking the AST in top-down order.

This takes away a lot of boilerplate code that the developer would have otherwise had to do by hand.

This approach now allows us to define one (or more) Walkers to walk the generated AST.

Consider this common use-case:
- we want to develop a new language, and write a parser for it
- the parser will read a source file and generate C++ or Java code

We can do this using Yantra by defining two walkers, say CppWalker and JavaWalker, and attaching per-walker semantic actions to each of them.

Every rule can have multiple walkers associated with them.

### Lexer calls Parser
In parser generators such as YACC and BISON, the parser calls the lexer to get the next available token.

Yantra follows the LEMON approach, where the lexer calls the parser when it recognises a token.

This has several advantages, the biggest advantage being that we can now perform incremental parsing.
Imagine reading a large XML, YAML or JSON input over a socket, and parsing it.

With the YACC approach, we would have to:
- first read the entire incoming data from the socket into memory or a disk file
- call the parser
- parser calls the lexer for the next token

With the LEMON approach, we can:
- read the incoming data in small chunks (say 2k)
- feed it into the lexer
- lexer scans the input and calls the parser as and when it recognises each token.
- parser receives the token and updates its internal state machine using the appropriae action (SHIFT, REDUCE or GOTO)

This eliminates the need for any intermediate memory or file buffers.

### Supported pragma directives in Yantra
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
