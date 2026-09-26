# Yantra Parser Generator

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![Version](https://img.shields.io/badge/version-0.4.0-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-23-blue.svg)

![Yantra Logo](docs/icon.png)

Yantra is a powerful compiler compiler and LALR(1) parser generator written in C++, with the following core features:
- An integrated lexer
- Built-in support for UNICODE/UTF8 input
- Built-in AST builder
- Built-in AST walker(s)
- Bottom-up parsing (being LALR), and top-down walking (traversal)
- Multi-mode lexer, useful for implementing nested multi-line comments, etc.
- Lexer-driven (push-based) parser: reads input one character at a time and feeds tokens to the parser as they complete, useful for processing input as it arrives (e.g. from a socket).
- An optional amalgamated mode, where the entire parser is generated as a single cpp file, along with a full-featured main() function.
- Or, in non-amalgamated mode, the parser is generated as separate .hpp and .cpp files, ready to drop into an existing project.

The name ***Yantra*** is Sanskrit for ***machine***, as in ***state machine*** in this context.

## Quick Start

Yantra has no dependencies beyond the C++ standard library, so building it is a plain CMake build:

```bash
git clone git@github.com:TantrixAuto/yantra.git
cd yantra
mkdir build && cd build
cmake ..
cmake --build .
```

This produces the `ycc` executable in `bin/`. Save a grammar file, `hello.y`:

```
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := ID;

ID := "[A-Za-z]+";
WS := "\s"!;
```

Then generate a parser from it:

```bash
bin/ycc -c ascii -f hello.y -a
```

This writes `hello.cpp` (an amalgamated, self-contained parser with its own `main()`) and `hello.log`. Compile it with any C++23 compiler:

```bash
# clang
clang++ --std=c++23 -o hello hello.cpp

# gcc
g++ --std=c++23 -o hello hello.cpp

# MSVC (cl.exe, from a Developer Command Prompt)
cl /std:c++23 /EHsc /nologo hello.cpp
```

The grammar above recognizes one or more whitespace-separated alphabetic words. `-s <string>` feeds that string directly to the parser as input (as opposed to `-f <filename>`, which reads from a file, or `-i`, which reads interactively from the console):

```bash
# succeeds silently
$ ./hello -s "hello world"
$ echo $?
0
```

```bash
# -t1 prints the parsed AST
$ ./hello -s "hello world" -t1
0:start_1(1:stmts_1(2:stmts_2(3:stmt_1(4:ID(hello))) 2:stmt_1(3:ID(world))) 1:_tEND())
```

```bash
# fails: ID only matches letters, "123" isn't valid input for this grammar
$ ./hello -s "hello 123"
s1-err:?a1.in(001,007):TOKEN_ERROR{{token: }}
hello 123
$ echo $?
1
```

### A small AST walker

Yantra parses the entire input into an AST first, *then* walks it top-down calling your semantic actions, unlike most parser generators, where actions run bottom-up as each rule is reduced. That ordering is what lets a parent rule's action run before its children are visited. Save this as `calc.y`:

```
%class Calculator;

start := expr;

expr := expr(a) PLUS expr(b)
%{
    std::cout << "Adding" << std::endl;
%}

expr := NUMBER(N)
%{
    std::cout << "Number: " << N.text << std::endl;
%}

NUMBER := "\d+";
PLUS := "\+";
WS := "\s+"!;
```

Generate and compile it the same way as above (`bin/ycc -c ascii -f calc.y -a`, then any of the three compiler commands), then run it:

```bash
$ ./calc -s "1 + 2 + 3"
Adding
Number: 1
Adding
Number: 2
Number: 3
```

`1 + 2 + 3` parses left-associatively as `(1 + 2) + 3`. So the outer `Adding`, the root of the tree, prints *first*, followed by its left child (`Number: 1`) and then its right child, which is itself another `Adding` node with its own two children. A hand-written recursive-descent or bottom-up parser would have to build extra AST classes and a separate walking pass to get this ordering. Here it falls out of the grammar directly.

See the [Build Instructions](docs/050_build.md) and [Tutorial](tutorial/) below for a real walk-through of the grammar syntax.

## How is this different?

- **vs. Bison / Yacc / Lemon** (the classic LALR(1) family. Lemon, from SQLite, is Yantra's direct stated inspiration): these run semantic actions *during* parsing, as each rule reduces, bottom-up. Yantra always builds the full AST first, then walks it top-down in a separate pass, so a parent rule's action can run before its children are visited. A single grammar can also define more than one walker (e.g. one that emits C++, another that emits Java, from the same parse). Getting either of those out of the Bison family means hand-building your own AST and walker on top.
- **vs. ANTLR**: ANTLR's visitor pattern is genuinely similar in spirit. It also lets you walk a fully-built parse tree after parsing completes. The real differences are narrower: Yantra targets C++ only (ANTLR generates for many languages), ships its own integrated lexer with mode-stack support instead of a separate lexer generator, and uses classic LALR(1) table-driven parsing rather than ANTLR's adaptive LL(*) algorithm. ANTLR is far more mature and widely used. Yantra is a much smaller, newer, single-maintainer project.
- **vs. tree-sitter**: a different problem entirely. It's built for incremental, error-tolerant parsing embedded in editors and IDEs (what GitHub, Neovim, etc. use it for), not for generating a compiler/codegen backend. Yantra doesn't do incremental reparsing and isn't trying to.

See [Known Limitations](KNOWN_LIMITATIONS.md) for an honest list of what Yantra doesn't do yet.

## Essential Reading
The following are a set of key links to get familiar with Yantra.

It is recommended that they be read in the given order.

| Name                                          | Description                                                  |
|-----------------------------------------------|--------------------------------------------------------------|
| [Tutorial](tutorial/)                         | A step-by-step walk-through for getting started with Yantra. |
| [Overview](docs/010_overview.md)              | A high-level overview of Yantra.                             |
| [Concepts](docs/020_concepts.md)              | Key concepts of Yantra.                                      |
| [User Manual](docs/030_manual.md)             | User manual describing the grammar.<br/>List of pragmas.     |
| [Developer Reference](docs/040_developers.md) | Description of major classes.                                |
| [Build Instructions](docs/050_build.md)       | Steps to build Yantra.                                       |
| [Grammar Quickstart](docs/060_quickstart.md)  | A quick introduction to the grammar file structure.          |

## Essential links
### Sample project
See https://github.com/TantrixAuto/lingo for standalone sample project that uses yantra.

### Language Server Extension
This is a language server extension created by [Raj Chaudhuri](https://github.com/rajch) that provides syntax highlighting for Yantra files in vscode, qtcreator, and any other IDE that supports the Language Server Protocol.

https://github.com/rajware/yantra-language-server

## License

Yantra is licensed under the [MIT License](LICENSE).

## Author

Renji Panicker ([@renjipanicker](https://github.com/renjipanicker))
