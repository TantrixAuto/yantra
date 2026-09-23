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
- Lexer-driven parser, useful for incremental parsing.
- An optional amalgamated mode, where the entire parser is generated as a single cpp file, along with a full-featured main() function.

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

This produces the `ycc` executable in `bin/`. Generate a parser from a grammar passed directly on the command line:

```bash
bin/ycc -c ascii -s 'start := stmts; stmts := stmts stmt; stmts := stmt; stmt := ID; ID := "[A-Za-z]+"; WS := "\s"!;' -a -n hello
```

This writes `hello.cpp` (an amalgamated, self-contained parser with its own `main()`) and `hello.log`. See the [Build Instructions](docs/050_build.md) and [Tutorial](tutorial/) below for compiling and running the generated parser, and for a real walk-through of the grammar syntax.

## License

Yantra is licensed under the [MIT License](LICENSE).

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
