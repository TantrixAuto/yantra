# Known Limitations

Yantra is a young, actively-developed project (`0.4.0`, pre-1.0). This is
a list of things it currently doesn't do, so you can decide up front
whether it fits your use case.

## Scope

- **C++ output only, for now.** Yantra generates C++ parsers exclusively; other target languages aren't supported yet.
- **Not built for incremental or error-tolerant parsing.** There's no
  incremental reparse and no error-recovery/resynchronization: a syntax
  or lexer error stops parsing at that point rather than attempting to
  continue and report further errors. If you need a parser embedded in
  an editor or IDE, look at tree-sitter instead.

## Grammar language

- **`%namespace` only accepts a single identifier**, not a scoped `::`
  path. `%namespace MyGrammar;` works, `%namespace ast::MyGrammar;`
  does not.
- **Precedence pragmas (`%left`/`%right`/`%token`) apply per token, not
  per rule alternative.** For a grammar with more than a couple of
  precedence levels, a separate rule per level (a "subrule") is the
  more reliable way to express precedence than a single ambiguous rule
  plus pragmas. See [Concepts: Precedence](docs/020_concepts.md#precedence)
  for the full explanation and a worked example.

## Reporting an issue

If you hit something not listed here, or one of these turns out to be
inaccurate, please open an issue.
