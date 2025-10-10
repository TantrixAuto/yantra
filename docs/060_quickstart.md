# Introduction to Yantra grammar file

The high-level structure of a Yantra grammar file is as follows:
```
Grammar related pragmas
Walker related pragmas
Rules
Tokens
```

The most common grammar related pragmas include
- %class: for specifying the class name of the generated Parser, default YantraModule
- %namespace: for specifying the namespace of the Parser class, default none
- %class_member: additional class members to be added to the Parser class
and so on.

Walker-related pragmas include:
- %walkers: for specifying one or more walkers
- %walker_traversal: spefify whether a walker should automatically traverse top-down, or the user will manually do so.
- %walker_output: Specify the output file type for a walker, if any

The next section is the list of rules, along with the accompanying functions.

Finally, we have the tokens, which is a set of regexs, classified by lexer lexer modes.
