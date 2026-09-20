# Code Review Findings

Full-codebase review of `src/`: memory, bugs, performance, formatting, and
LALR(1) functional correctness. No code was changed as part of the original
review; items below are checked off as fixes land, with a couple of
additional findings turned up while verifying those fixes end-to-end (see
"Generated-parser build correctness" and "Test harness" below).

## Critical — LALR(1) construction correctness

- [x] **`hasItemSet` compares configs by position, not as a set** — [grammar_yg.hpp:756-788](src/grammar_yg.hpp#L756-L788)
  - **Problem**: `hasItemSet` walks `is->configs` and the candidate list pairwise and bails on the first positional mismatch. Two item sets containing the *same* (rule, dot-position) items but assembled in a different order — which `expandConfigs` can produce, since its traversal order depends on `unordered_map` iteration order in `CanonicalItemSet::shifts`/`gotos` — are treated as *different* states. Effect: the generator can emit duplicate LALR states instead of merging them, at minimum bloating the table, and at worst letting two "duplicate" states resolve conflicts differently.
  - **Suggested solution(s)**:
    - Compare the two config lists as an unordered set, e.g. sort both by `(&rule, cpos)` before the positional comparison, or build a `std::set`/`std::unordered_set` keyed on that pair and compare sets.
    - Add a regression grammar to `unittests/` that is likely to hit non-deterministic item ordering (e.g. a rule reachable via two different shift paths into the same item set) and diff generated state counts before/after the fix.

- [x] **`addShift`'s dedup key ignores dot-position** — [parser_builder.cpp:365-374](src/parser_builder.cpp#L365-L374)
  - **Problem**: the "already has a shift for this rule" check only compares `&(xcfg->rule) == &(config.rule)`, never `cpos`. If the same rule legitimately needs a shift on the same token from two different dot-positions within one item set (e.g. a rule with a repeated symbol, or overlapping recursion), the second shift target is silently dropped instead of being added alongside the first — a missing transition in the generated table for that grammar shape.
  - **Suggested solution(s)**:
    - Change the dedup check to compare the *source* config identity (rule **and** `cpos`), not rule alone.
    - Write a small regression grammar with a repeated symbol in one rule (e.g. `X -> a X a`) and run it through `unittests/unittests.sh`, checking the generated parser accepts/rejects the expected strings.

## Critical — Generated-parser build correctness

- [x] **Missing `#include <print>` in every generated parser** — [prototype.cpp:48-62](src/prototype.cpp#L48-L62)
  - **Problem**: found while verifying the two LALR fixes above end-to-end under a modern toolchain. `prototype.cpp` is stringified verbatim into every parser yantra generates. Its `stdHeaders` block — the one actually emitted into generated output, as opposed to a separate `SKIP`-only block used only when compiling `prototype.cpp` standalone — didn't include `<print>`, even though generated code calls the free/stdout `std::print`/`std::println` form (not the `ostream&`-taking one) about 30 times, e.g. in the amalgamated `main()`'s CLI/error-reporting output. Under C++23 with `<print>` missing, `std::print(fmt, args...)` silently resolves to the wrong overload (the `ostream&`-requiring one declared in `<ostream>`), which fails to compile and cascades into unrelated-looking `-Wformat-extra-args`/`-Wnon-pod-varargs` errors on top. Every parser yantra generates in amalgamated mode was affected.
  - **Fix applied**: added `#include <print>` to the `stdHeaders` block.

## Test harness (`unittests/unittests.sh`)

Found and fixed while getting the two LALR fixes above to actually verify
end-to-end — turned out `unittests.sh` had been silently failing before it
ever reached a single real assertion, for an unknown amount of time (see
below), so none of this was new breakage from the LALR fixes themselves.

- [x] **Hardcoded `-std=c++20`, but generated code needs C++23** — [unittests.sh:39,43,59](unittests/unittests.sh#L43)
  - **Problem**: yantra's own standard is C++23 ([yantra.cmake](yantra.cmake)), and everything it generates uses `std::println`. Compiling generated output at `-std=c++20` made `std::println` not exist in `namespace std` at all, failing every single test at the compile step before its AST assertion ever ran.
  - **Fix applied**: bumped to `-std=c++23` (the clang path, its PCH precompile step, and the MSYS2/`cl.exe` path for parity).

- [x] **`-Weverything` flags `std::atoi` in one test's embedded semantic action** — [unittests.sh:44](unittests/unittests.sh#L44)
  - **Problem**: `-Wunsafe-buffer-usage-in-libc-call` (an aggressive Clang-21 hardening warning) turns `std::atoi(N.text.c_str())`, inside one test grammar's own `%{ ... %}` semantic-action code, into a hard error under `-Werror -Weverything`.
  - **Fix applied**: added `-Wno-unsafe-buffer-usage-in-libc-call`, alongside the file's existing list of `-Weverything` suppressions for the same reason (overly strict for general code).

- [x] **Stale expected-output string for `STAR`'s associativity** — [unittests.sh:1210](unittests/unittests.sh#L1210)
  - **Problem**: `STAR := "\*";` uses plain `:=`, which [defaults to right-associative](src/parser.cpp#L2512-L2522) — not left, as the test's own expected AST for `11 * 12 * 13` assumed. `PLUS` right next to it correctly used `:=>` (left), so this reads as a copy-paste typo rather than an intentional choice. Confirmed this predates both LALR fixes above — reproduced identically against a build from before the `addShift` fix — it had simply never been caught because the `-std=c++20` issue meant no test's actual parse output was ever compared until now.
  - **Fix applied**: changed to `STAR :=> "\*";`, matching `PLUS`.

**Net effect**: `unittests/unittests.sh` now passes 318/318 — quite possibly the first time it's run to a real pass/fail verdict end-to-end, rather than failing at the compile step on every single test.

## High — Performance

- [x] **Every `write`/`writeln`/`iwrite`/`xwriteln` call flushes the stream** — [text_writer.hpp:27,36,45,55,63](src/text_writer.hpp#L27)
  - **Problem**: `TextFileWriter` wraps `std::ofstream`; each of these methods ends with `ss.flush()`. Since generated files can involve tens of thousands of `tw.writeln(...)` calls, this forces an OS-level flush on every single line, defeating stream buffering and directly slowing down code generation for any non-trivial grammar.
  - **Suggested solution(s)**:
    - Drop the per-call `flush()` — `std::ofstream` flushes on close/destruction, and RAII already guarantees that here.
    - If an explicit flush is needed for interleaved-log debugging, gate it behind a debug/verbose option instead of doing it unconditionally.

- [ ] **Linear scan through sorted Unicode range tables** — [encoding_utf8.cpp:292-309](src/encoding_utf8.cpp#L292-L309), used by [`isLetter`/`isDigit`/`isWord`](src/encoding_utf8.cpp#L318-L327)
  - **Problem**: `check()` does a plain `for` loop over `letters[]`/`digits[]`/`whitespace[]` (hundreds of entries, already sorted ascending by `.from`). This isn't just yantra's own bootstrap lexer — `encoding_utf8.cpp` is stringified at build time (`STRINGIFY(... "encoding_utf8.cpp" ...)` in [src/CMakeLists.txt](src/CMakeLists.txt)) and spliced into **every Unicode-enabled generated parser** via the `utf8Encoding` segment in [cpp_generator.cpp:2006](src/cpp_generator.cpp#L2006). So this O(n) per-character scan runs in the hot path of every parser yantra generates with Unicode on.
  - **Suggested solution(s)**:
    - Since the tables are already sorted by `.from`, replace the linear scan with `std::upper_bound`/binary search on `.from`, then check `.to`.
    - Verify table sortedness with a `static_assert`/one-time debug check if practical, since correctness of the binary search depends on it.

## Medium — Robustness / hardening

- [ ] **Potential `size_t` underflow in FIRST/FOLLOW construction** — [parser_builder.cpp:745](src/parser_builder.cpp#L745)
  - **Problem**: `for(size_t idx = 0; idx < r2->nodes.size() - 1; ++idx)` underflows to a huge loop bound if `r2->nodes.size()` is ever 0, causing out-of-bounds `nodes` access. Currently only protected by `assert(k > 0)` on a *different* loop variable (`rule`, not `r2`) two lines above, relying on both loops ranging over the same `grammar.rules` collection. In a `-DNDEBUG` release build the assert vanishes.
  - **Suggested solution(s)**:
    - Guard directly: `if (r2->nodes.size() == 0) continue;` (or assert on `r2` itself, not just `rule`).
    - Consider a `GeneratorError` throw instead of `assert` here, since this loop processes user-supplied grammar data, not just internal invariants.

- [ ] **Load-bearing invariants guarded only by `assert`, which disappears under `-DNDEBUG`**
  - **Problem**: several safety checks that matter at runtime — not just during development — are `assert`-only:
    - [prototype.cpp:518-519](src/prototype.cpp#L518-L519) — `_reduce()`'s `valueStack.size() >= len` / `stateStack.size() >= len`. This is the shift/reduce stack of *every generated parser*, not just yantra itself.
    - [grammar_yg.hpp:732](src/grammar_yg.hpp#L732) — `createConfig`'s `p <= r.nodes.size()`.
    
    `yantra.cmake` doesn't disable `NDEBUG` for Release builds, so these nets disappear exactly where they'd matter most.
  - **Suggested solution(s)**:
    - Triage which of these are true "can't happen, internal bug" invariants (keep as `assert`) vs. which could be reached by a malformed/adversarial grammar or generated-parser input (convert to a real runtime check that throws).
    - At minimum, document the NDEBUG tradeoff so it's a deliberate choice, not an oversight.

- [ ] **`TextFileWriter::_open` silently no-ops on an empty filename** — [text_writer.hpp:99-102](src/text_writer.hpp#L99-L102)
  - **Problem**: unlike the non-empty path, which throws on failure to open ([text_writer.hpp:107-109](src/text_writer.hpp#L107-L109)), an empty `fname` just returns without opening or erroring. A caller that computes an empty path by mistake gets silently-dropped output with no diagnostic.
  - **Suggested solution(s)**:
    - Throw the same `unable to open output file` error (or a clearer "empty output filename" error) instead of silently returning.
    - Alternatively, if an empty filename is a legitimate "don't write this file" signal somewhere, make that explicit at the call site rather than implicit inside `_open`.

- [ ] **Inconsistent error types**
  - **Problem**: most of the codebase throws `GeneratorError` with file/line/grammar-position context; a few spots throw bare `std::runtime_error` with no position info — [prototype.cpp:816](src/prototype.cpp#L816) (`readFile`, shipped into every generated parser), [text_writer.hpp:108](src/text_writer.hpp#L108). This degrades diagnostics specifically for generated-parser users.
  - **Suggested solution(s)**:
    - Route these through `GeneratorError` (or an equivalent generated-parser-side error type with position/context) for consistency.

## Medium — Algorithmic performance (likely fine today, worth knowing)

- [ ] **Naive fixed-point FIRST/FOLLOW computation** — [parser_builder.cpp:736-810](src/parser_builder.cpp#L736-L810)
  - **Problem**: `buildLinks()` repeats an `O(rules² × avg_nodes)` pass until no changes occur, rather than using a worklist keyed on what actually changed. Standard textbook approach, but scales poorly for grammars with hundreds of rules.
  - **Suggested solution(s)**:
    - Replace with a worklist algorithm: only reprocess rules whose FIRST/FOLLOW dependencies actually changed in the previous iteration, rather than re-scanning every rule every pass.

- [ ] **`O(n²)` item-set closure construction** — [parser_builder.cpp:238-250](src/parser_builder.cpp#L238-L250), [parser_builder.cpp:255-290](src/parser_builder.cpp#L255-L290)
  - **Problem**: `hasRuleInConfigList` is a linear scan called from inside `expandConfigs`'s work loop, giving `O(n²)` closure construction per item set, repeated for every state during the whole LALR build.
  - **Suggested solution(s)**:
    - Track seen `(rule*, cpos)` pairs in a `std::set`/`std::unordered_set` alongside `configs`, and check membership there instead of scanning the vector.

## Low — Acknowledged / minor

- [ ] **`_findSmallestSuperset` returns the first match, not the smallest** — [lexer_builder.cpp:251-253](src/lexer_builder.cpp#L251-L253)
  - **Problem**: already flagged by the author's own `TODO`; affects DFA-merge optimality (not correctness) during lexer state minimization.
  - **Suggested solution(s)**:
    - When picking this back up, compare candidate supersets by their transition-set size (or a cheap proxy for it) and keep the smallest instead of returning on first match.

## Formatting / cleanliness

- [ ] **Dead/commented-out code** — [parser_builder.cpp:56-61](src/parser_builder.cpp#L56-L61) (old `addShift`), [parser_builder.cpp:575](src/parser_builder.cpp#L575) (old `throw`)
  - **Problem**: commented-out code left in place instead of deleted; adds noise and risks accidental reactivation.
  - **Suggested solution(s)**: delete it; git history already preserves it if it's ever needed again.

- [ ] **Stray `unused(epsilons);` immediately followed by real use of `epsilons`** — [parser_builder.cpp:81](src/parser_builder.cpp#L81) (used again at [:84](src/parser_builder.cpp#L84))
  - **Problem**: confusing leftover from an earlier version of the function; `epsilons` genuinely is used a few lines later, so the `unused()` call is misleading.
  - **Suggested solution(s)**: remove the `unused(epsilons);` line.

- [ ] **`unused(&CanonicalItemSet::hasGoto);` hack to silence an unused-function warning** — [parser_builder.cpp:136](src/parser_builder.cpp#L136)
  - **Problem**: taking the address of a method just to suppress a warning reads like it's doing something functional; easy to misread.
  - **Suggested solution(s)**: mark the `hasGoto` declaration `[[maybe_unused]]` instead, and drop this line.

- [ ] **Typo in doc comment** — [parser_builder.cpp:9](src/parser_builder.cpp#L9)
  - **Problem**: "chek if opssible" → should read "check if possible".
  - **Suggested solution(s)**: fix the typo.

- [ ] **Repetitive CLI flag-parsing** — [main.cpp:226-307](src/main.cpp#L226-L307)
  - **Problem**: the same "advance, bounds-check, error" triplet is repeated ~8 times across flag branches. Not a bug (every branch correctly increments `i` exactly once), but it's duplicated boilerplate.
  - **Suggested solution(s)**: factor out a small `nextArg(errorMsg)` helper that advances `i`, bounds-checks, and returns the arg or calls `help()`.

## What I did *not* find

- No raw `new`/`delete`/`malloc`/`free` anywhere in `src/` — ownership is consistently RAII (`unique_ptr`-owned vectors for `Config`/`ItemSet`/etc., `std::ofstream`/`std::ifstream` for files).
- `Parser::values` in [prototype.cpp:494](src/prototype.cpp#L494) grows monotonically for the life of one parse (never trimmed mid-parse). This is expected/necessary since AST nodes hold pointers into it — not a leak, but worth knowing if parsing very large single files under memory pressure.
- Compiler hygiene is already strong: [yantra.cmake](yantra.cmake) enables `-Wall -Wextra -Werror -pedantic` (plus `-Weverything` on Clang) — no gap there.

---

**Status**: all Critical items fixed (both LALR construction bugs, plus the
generated-parser `<print>` bug and test-harness issues found while
verifying them) — `unittests/unittests.sh` passes 318/318. None of this has
been committed or pushed.

Suggested order of attack for what's left: the two **High** performance items (cheap, high-value wins), then the **Medium** hardening items as time allows.
