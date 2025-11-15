# About this folder:
- Contains small, focused "recipes" (subfolders) showing common build, run, and test workflows.
- run the specified recipe commands from the project's build directory.
- refer to the `build.sh` file for a well-functioning set of commands
- run the `build.sh` script to test all recipes

## Usage modes
There are several way in which the generated parser can be used in your project.
- You can generate an amalgamated single-file parser that includes a main() function,
- or you can generate the parser as a separate .hpp and .cpp files, which can be used in your main() function.
- You can define the semantic actions inline in the .y file,
- or you can define in an external .cpp file.

The following four recipes demonstrates each of these usage modes.

The grammar used in all these recipes is the same.
It is a simple expression parser that recognises the PLUS and MUL operators.

```
start := expr;
expr := addexpr;
addexpr := addexpr PLUS mulexpr;
addexpr := mulexpr;
mulexpr := mulexpr STAR valexpr;
mulexpr := valexpr;
valexpr := LPAREN expr RPAREN;
valexpr := NUM;

STAR := "\*";
PLUS := "\+";
LPAREN := "\(";
RPAREN := "\)";
NUM := "[0-9]+";
WS := "\s"!;
```

It uses a single walker called `Compiler`. This `Compiler` can be either:
- `inline` where the actions are provided in the .y file
- `external` where the actions are provided in a separate .cpp file.

The generated parser can be either:
- `amalgamated` where the parser and main are in one integrated .cpp file
- `separated` where the main is implemented by the coder in a separate main.cpp file


The following 4 recipes are all the combinations of the above.
| Folder               | Description |
|----------------------|-------------|
| [inline-amalgamated](inline-amalgamated/)      | This is the most basic usage. It produces a single .cpp file combining the main() and all the actions.|
| > Generate           | `./bin/ycc -f ../../recipes/inline-amalgamated/grammar.y -a`|
| > Build              | `clang++ -g -Wall --std=c++20 grammar.cpp`|
| > Run                | `./a.out -s "2 * 3 + 1"`|
| [external-amalgamated](external-amalgamated/)  | This produces a .cpp file containing the main(), and all the actions are in a separate .cpp file.|
| > Generate           | `./bin/ycc -f ../../recipes/external-amalgamated/grammar.y -a`|
| > Build              | `clang++ -g -Wall --std=c++20 -I . grammar.cpp ../../recipes/external-amalgamated/compiler_impl.cpp`|
| > Run                | `./a.out -s "2 + 1 * 3"`|
| [inline-separate](inline-separate/)           | This produces a .cpp file containing all the actions, and main() is in a separate .cpp file.|
| > Generate           | `./bin/ycc -f ../../recipes/inline-separate/grammar.y`|
| > Build              | `clang++ -g -Wall --std=c++20 -I . grammar.cpp ../../recipes/inline-separate/main.cpp`|
| > Run                | `./a.out "2 + 1 * 3"`|
| [external-separate](external-separate/)        | This produces a .cpp file containing only the parser, while all the actions and main() is in a separate .cpp file.|
| > Generate           | `./bin/ycc -f ../../recipes/external-separate/grammar.y`|
| > Build              | `clang++ -g -Wall --std=c++20 -I . grammar.cpp ../../recipes/external-separate/main.cpp`|
| > Run                | `./a.out "2 + 1 * 3"`|

## Multiple walkers
The recipe `multiple-walkers` demonstrates how to implement multiple walkers in a single .y file.
It implements two walkers, `CppWalker` and `JavaWalker`.

| Folder               | Description |
|----------------------|-------------|
| [multiple-walkers](multiple-walkers/)      | This is minimal example of using multiplle walkers againsta single grammar. It produces a single .cpp file combining the main() and all the walkers.|
| > Generate                            | `./bin/ycc -f ../../recipes/multiple-walkers/grammar.y -a`|
| > Build                               | `clang++ -g -Wall --std=c++20 grammar.cpp`|
| > Run with default walker(CppWalker)  | `./a.out -s "a = a::b;"`|
| > Run with CppWalker                  | `./a.out -s "a = a::b;" -w CppWalker`|
| > Run with JavaWalker                 | `./a.out -s "a = a::b;" -w JavaWalker`|
