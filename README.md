Yantra is a powerful compiler compiler and LALR(1) parser generator written in C++, with the following core features:
- an integrated lexer
- Built-in support for UNICODE/UTF8 input
- Built-in AST builder
- Built-in AST walker(s).
- Bottom-up parsing (being LALR), and top-down walking (traversal).
- Multi-mode lexer, useful for implementing nested multi-line comments, etc.
- Lexer-driven parser, useful for incremental parsing.
- An optional amalgamated mode, where the entire parser is generated as a single cpp file, along with a full-featured main() function.

The name ***Yantra*** is Sanskrit for ***machine***, as in ***state machine*** in this context.

# Essential Reading
The folloing are a set of key links to get familiar with Yantra.

It is recommended that they be read in the given order.

| Name                                           | Description.                          |
|------------------------------------------------|---------------------------------------|
| [Overview](docs/010_overview.md)               | - A high-level overview of Yantra.    |
| [Concepts](docs/020_concepts.md)               | - Key concepts of Yantra.             |
| [User Manual](docs/030_manual.md)              | - User manual describing the grammar.<br/>- List of pragmas. |
| [Developer Reference](docs/040_developers.md)  | - Description of major classes.       |
| [Build Instructions](docs/050_build.md)        | - Steps to build Yantra.              |
| [Grammar Quickstart](docs/060_quickstart.md)   | - A quick introduction to the grammar file structure. |

# Sample project
See https://github.com/TantrixAuto/lingo for standalone sample project that uses yantra.
