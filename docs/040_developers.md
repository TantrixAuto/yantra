This diagram describes the end-to-end process by which all above classes interact to convert the `input.y` file to `output.cpp`

```mermaid
architecture-beta
    group yantra[Yantra]

    service input(disk)[input_y] in yantra
    service parser(server)[Parser] in yantra
    service grammar(database)[Grammar] in yantra
    service parser_builder(server)[ParserBuilder] in yantra
    service lexer_builder(server)[LexerBuilder] in yantra
    service parser_sm(database)[ParserFSM] in yantra
    service lexer_sm(database)[LexerFSM] in yantra
    service sm_tables(database)[FSM Tables] in yantra
    service cpp_gen(server)[CppGenerator] in yantra
    service output(disk)[output_cpp] in yantra

    input:R --> L:parser
    parser:R --> L:grammar
    grammar:T --> B:parser_builder
    grammar:B --> T:lexer_builder
    parser_builder:R --> L:parser_sm
    lexer_builder:R --> L:lexer_sm
    parser_sm:B --> T:sm_tables
    lexer_sm:T --> B:sm_tables
    sm_tables:R --> L:cpp_gen
    cpp_gen:R --> L:output
```

## Parser
The Parser class reads the input.y file and creates the Grammar object.

_(A Lexer class in the Parser reads the .y file and splits it into a stream of tokens first)_

## Grammar
The Grammar class holds the input grammar, which is read and parsed from the input.y file.
It also holds the State Machine tables which are built.

## LexerBuilder
The LexerBuilder class creates the Lexer State machine, for the generated parser, from the Grammar.

## ParserBuilder
The ParserBuilder class creates the LALR State machine, for the generated parser, from the Grammar.

## CppGenerator
The CppGenerator takes the lexer and parser State Machines and generates the output.cpp code that implements the Parser defined in the input grammar.
