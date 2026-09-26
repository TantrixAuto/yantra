# Known Limitations

Yantra is a young, actively-developed project (still at `0.y.z`, pre-1.0). This is a list of things it currently doesn't do, so you can decide up front
whether it fits your use case.

## Scope

- **C++ output only, for now.** Yantra generates C++ parsers exclusively; other target languages aren't supported yet.
- **Not built for incremental or error-tolerant parsing.** There's no
  incremental reparse and no error-recovery/resynchronization: a syntax
  or lexer error stops parsing at that point rather than attempting to
  continue and report further errors.

## Grammar language

- **`%namespace` only accepts a single identifier**, not a scoped `::`
  path. `%namespace MyGrammar;` works, `%namespace ast::MyGrammar;`
  does not.

## Precedence

Precedence lives on tokens, the same model Yacc, Bison, and Lemon use.
A rule borrows its precedence from a token in one of two ways:

- **Automatically**, via its anchor (the rule's last terminal), same
  convention Yacc/Bison/Lemon use.
- **Explicitly**, via an override: `[TOKEN]` in Yantra, `%prec` in
  Bison, also `[TOKEN]` in Lemon.

None of these tools let a rule have independent precedence, so this
isn't a Yantra-specific gap.

For more than a couple of precedence levels, subrules are more
maintainable than pragmas and overrides. See
[Concepts: Precedence](docs/020_concepts.md#precedence).

## Reporting an issue

If you hit something not listed here, or one of these turns out to be
inaccurate, please open an issue.
