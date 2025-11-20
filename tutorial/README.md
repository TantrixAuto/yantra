# Yantra Quick Start Tutorial

Welcome! This tutorial will guide you through creating your first parser with Yantra by building an integer arithmetic expression evaluator. We'll start simple and progressively add features, demonstrating Yantra's key capabilities along the way.

## What You'll Build

By the end of this tutorial, you'll have a working parser that can parse and evaluate expressions like:
```
42
2 + 3
10 - 5 * 2
(10 - 5) * 2
```

## Prerequisites

- Yantra installed and `ycc` executable in your PATH
- A C++ compiler (GCC, Clang, or MSVC)
- Basic understanding of context-free grammars (helpful but not required)
- Text editor of your choice

## Yantra basics

The following steps describe the basic Yantra workflow:

1. You create a _grammar file_ using a text editor.
2. You use *ycc* to "compile" the grammar file. This produces C++ source code for a set of classes that can parse your grammar. The source code is organized using header and implementation files, as is standard in C++. You can then include the generated source code in your own program, typically by writing a `main()` function that invokes the generated classes. You can also opt for an _amalgamated_ output, which produces all source code in one single file, and includes a default `main()` function that provides command-line options for receiving input, and invokes the parser as appropriate.
3. You use a C++ compiler to compile the generated source code (along with your own code if you did not select the amalgamated output option). You run the resulting executable, and test it using input formatted according to your grammar.

In this tutorial, we will use the amalgamated option in step 2.

## Stage 1: The Simplest Parser

### Create our first grammar file
Let's start with the absolute minimum: a parser that recognizes a single integer.

Create a file named `calc.yantra`:

```
%class Calculator;

start := numexpr;

numexpr := NUMBER;

NUMBER := "\d+";
WS := "\s+"!;
```

### Understand the grammar file

A Yantra grammar file can consist of several different elements, including ___pragmas___, ___rules___ and ___tokens___.

#### Token

A ___token___ is a single meaningful unit of input. It is a categorized sequence of characters. In a Yantra grammer, a token is identified using a regular expression, and is given a name in all caps.

In this grammar, you can see two tokens, NUMBER and WS. NUMBER is identified using a regular expression that matches one or more decimal digits. WS is identified using a regular expression consisting of one or more whitespace characters. 

Note the `!` after the regular expression in the WS token definition. This tells *ycc* that this token is not meaningful in the grammar. Any token that is marked with this symbol will be completely ignored in input. In this case, all whitespace in input will be ignored by the generated parser.

#### Rule

A ___rule___ is a formal statement that defines the syntax or structure of valid input. In a Yantra grammar, a rule is defined using one or more tokens or rules, in strict sequence, and is given a name that begins with a lower case letter.

One of the rules defined in a grammar is considered the ___start symbol___. The generated parser begins parsing the rules from that one. Unless otherwise specified, *ycc* assumes that the start symbol is a rule named `start`.

In this grammar, you can see two rules defined: `numexpr` and `start`. `numexpr` is defined as the NUMBER token, and nothing else. This means that the parser will accept only a single number as valid input, and all other input will cause a syntax error (except whitespace, which will be ignored). `start` is defined as `numexpr`, which means that the complete input can be just one number.

#### Pragma

A ___pragma___ is directive for *ycc*, which can cause it to perform additional actions while generating the parser source code from the grammar. Pragmas in Yantra have names beginning with the `%` symbol.

In this grammar, you can see a pragma called `%class`, which tells *ycc* what name to give the data structure that it generates to represent the entire valid input. This is optional: ycc will derive a default name (YantraModule) if you do not provide it.

### Generate the parser

Run the following:

```bash
ycc -f calc.yantra -a
```

The `-a` flag generates a single, amalgamated source code file with a built-in `main()` function. So, this will generate two files: **calc.cpp** and **calc.log**. The log file contains details of how the grammar itself was parsed - we can ignore it for now. The .cpp file contains the generated parser class, and a `main()` function which invokes that class to parse input.

### Compile and test the parser

To compile the generated parser, run the following based on your operating system:

```bash
# Linux/Mac
clang++ --std=c++20  -o calc calc.cpp

# Windows
cl calc.cpp
```

To test the generated parser executable, run the following:

```bash
./calc -s "42"
```

The `main()` function (generated due to the `-a` option passed to **ycc**) allows input to be provided to the parser as a string, using the `-s` command-line option as shown above.

Running the above command should not cause any errors, because we passed a valid number. Now, test with the following:

```bash
./calc -s "  42  "
./calc -s "ABC"
./calc -s "12 A"
./calc -s "-12"
```

The first one should succeed, because all whitespace is ignored as per our grammar. All the others should fail.

### Make the most of it

For the rest of this tutorial, we will repeatedly perform these steps, viz.: modify the grammer, invoke **ycc** to generate C++ source code for the parser,  compile the code, and involve the resulting *calc* binary (*calc.exe* on Windows) for testing. To automate this process, we will create a _Makefile_ for Linux/Mac/UNIXlikes, and a batch file for Windows if you are not using WSL. Here they are:

#### Makefile (for UNIXlikes)

```
calc: calc.cpp
	clang++ --std=c++20  -o $@ $<

calc.cpp: calc.yantra
	ycc -f $< -a

.PHONY: clean
clean:
	rm -f calc.log calc.cpp calc
```

Save that as a file called *Makefile*. From now on, just run `make` after you make changes to the grammar file.

#### Batch file (for Windows)

```
@echo off
setlocal

if "%~1"=="" goto calc
if /i "%~1"=="calc" goto calc
if /i "%~1"=="clean" goto clean

echo.
echo ERROR: Invalid target.
echo Usage: %~nx0 [calc | clean]
goto :eof

:calc

echo Generating calc.cpp from calc.yantra...
ycc -f calc.yantra -a
if errorlevel 1 (
    echo ERROR: ycc generation failed. Is 'ycc' in your system PATH?
    exit /b 1
)


echo Compiling 'calc.cpp' to 'calc.exe'...
cl calc.cpp
if errorlevel 1 (
    echo.
    echo ERROR: C++ Compilation failed.
    exit /b 1
)

goto :eof

:clean
echo Deleting calc.log...
del /f /q calc.log 2>nul

echo Deleting calc.cpp...
del /f /q calc.cpp 2>nul

echo Deleting calc executable (calc.exe)...
del /f /q calc.exe 2>nul

goto :eof


endlocal
```

Save that as **make.cmd**. From now on, just run `make.cmd` after you make changes to the grammar file.

### Get Some Action

When our parser detects _valid_ input, it currently does nothing. Let's change that. Change the **calc.yantra** grammar file as follows:

```
%class Calculator;

start := numexpr;

numexpr := NUMBER (NUM)
%{
    std::cout << "Number: " << NUM.text << std::endl; 
%}

NUMBER := "\d+";
WS := "\s+"!;
```

The relevant changes are in the `numexpr` rule, on line 5. First, generate and compile using `make`. Then run:

```
./calc -s "42"
```

and notice that the parsed number is printed to the output. How does this work?

#### Mr. Semantic, Really Fantastic

If you look at the modified `numpexpr` rule, you will notice two new elements. First, the new symbol in brackets after the token name: `(NUM)`. This is a _variable_. Second, the C++ code block immediately after the rule definition, enclosed in %{ and %} (note the absence of a semicolon after the rule definition). This is called a _semantic action_.

A semantic action is a function that is executed as soon as the generated parser parses a rule. Elements of the rule, such as other rules or tokens, are passed to the function as variables. You can choose which parts of the rule your function is interested in, and declare a variable for that element by putting a variable name in parenthesis immediately after it. Yantra requires that variables declared for tokens have names in all caps, and variables declared for rules have names beginning with a lower-case letter - same rules as the element names themselves.

The variables contain objects, different ones for rules and tokens. In this example, you can see that a token variable has a member called `.text`, which contains the actual token scanned from input.

Yantra adds the function to the generated code, where it is invoked when that particular rule gets matched, which means when a number is read from input.

#### Mr. Walker

The way it all works is this: any Yantra-generated parser reads the input and transforms it into a data structure called Abstract Syntax Tree (AST). Yantra can also generate one or more ***Walker*** classes, which traverse the parsed AST from top down, and call any defined functions (semantic actions) as a particular rule is parsed in the input. The semantic actions that we define in a grammar thus become members of Walker classes.

It is possible to define multiple Walkers in a grammar, for example one to generate amd64 assembly and another one for arm64. These would then need to be formally named, and you would be able to attach one semantic action per Walker to each rule. If, like now, we simply attach a single semantic action per rule, and do not name the Walker explicitly, Yantra generates a default one (called Walker_Walker).

### Exit Stage 1

At this point, we have created a grammar file that recognizes a single integer number as the only valid input. We have used the **ycc** tool to generate an amalgamated C++ source code file which contains:

* a generated parser which parses input complying to that grammar, providing an accurate error message if it doesn't
* a walker which traverses the parsed AST, calling semantic actions where defined. In out case, the parsed number is output.
* a boilerplate `main()` function which calls the parser (and the walker), providing some pre-provided options like the source of input and the walker to use

We have compiled the amalgamated C++ source code file to produce an executable binary, which we will call the _target compiler_ from now on. Finally, we have run the target compiler using valid and invalid input, and checked if it provides appropriate responses.

## Stage 2: Adding Basic Operators

### Adding some

Now, let's enhance our grammar to understand addition and subtraction.

Update `calc.yantra`:

```
%class Calculator;

start := numexpr;

addexpr := numexpr PLUS numexpr
%{
    std::cout << "Adding:" << std::endl;
%}

subexpr := numexpr MINUS numexpr
%{
    std::cout << "Subtracting:" << std::endl;
%}

numexpr := addexpr;
numexpr := subexpr;
numexpr := NUMBER (NUM)
%{
    std::cout << "Number: " << NUM.text << std::endl; 
%}

NUMBER := "\d+";
PLUS   := "\+";
MINUS  := "-";
WS     := "\s+"!;
```

Generate and compile using `make`. Test with the following:

```bash
./calc -s "+ 42 + 55"
./calc -s "42 + 55 -"
./calc -s "42 + 55"
./calc -s "42 + 55 - 5 + 10"
```

The first two will fail, showing the location in the input (row, column) where the expression is invalid. The last two will succeed.

### What changed?

Note the two new tokens, `PLUS` and `MINUS`. They are regular expressions to match the '+' and '-' symbols. The + needs to be escaped because of C++ regular expression syntax.

Note also the two new rules: `addexpr` and `subexpr`. For example, `addexpr` states that a `numexpr` can be followed by a `PLUS` and then another `numexpr`, and that's valid grammar. This means that leading or trailing `PLUS` symbols will cause a syntax error. But what makes it really interesting is that we did not say `NUMBER PLUS NUMBER`. 

To understand why, especially note that the `numexpr` rule is now defined three times! In a Yantra grammar, this is how you indicate alternatives for a rule. Here, we are saying that a numeric expression can be an `addexpr`, a `subexpr` or a `NUMBER`; all three cases are valid grammar. Now, this recursively means that the expression on the left (or right) of a `PLUS` can itself be and `addexpr` or a `subexpr`. So, a long string of addition or subtraction operations, like "42 + 55 - 5 + 10" is valid. 

### Left Right Left

Test our target compiler with the following:

```
./calc -s "42 + 55 - 5 + 10"
```

Observe the output:

```
Adding:
Number: 42
Subtracting:
Number: 55
Adding:
Number: 5
Number: 10
```

This seems to suggest that the parser hit the first `PLUS` and recogized that this was an `addexpr`, with the `NUMBER` 42 as its left expression. It then proceeded to evaluate the part after the `PLUS` as a `numexpr`, encountered the `MINUS`, and recognised that past as a `subexpr`, and so on. The net result is that the rightmost part of the whole expression gets evaluated first: `5+10`, then `55 - result`, then `42 + result`. This is the default behaviour, but it is also wrong. Fortunately, we can change it.

We can specify that if a particular TOKEN is used in a rule, evaluation should be left-to-right instead, which means that any rule on the left of the token should be fully evaluated before any rule to its right (which is the order in which calculations happen in the real world). We do this through a pragma called `%left`, for both `PLUS` and `MINUS`. Please note: this pragma should appear anywhere _before_ the tokens that it lists appear in the grammar file.

So, change our grammar file as follows:

```
%class Calculator;

start := numexpr;

addexpr := numexpr PLUS numexpr
%{
    std::cout << "Adding:" << std::endl;
%}

subexpr := numexpr MINUS numexpr
%{
    std::cout << "Subtracting:" << std::endl;
%}

numexpr := addexpr;
numexpr := subexpr;
numexpr := NUMBER (NUM)
%{
    std::cout << "Number: " << NUM.text << std::endl; 
%}

%left PLUS MINUS;

NUMBER := "\d+";
PLUS   := "\+";
MINUS  := "-";
WS     := "\s+"!;
```

Regenerate and compile with `make`, and test again with `./calc -s "42 + 55 - 5 + 10"`. Observe the changed output:

```
Adding:
Subtracting:
Adding:
Number: 42
Number: 55
Number: 5
Number: 10
```

Now, the outmost `addexpr` takes the `NUMBER` 10 as its right expression, and recognizes the part to its left as a `subexpr`, and so on. So the order of evaluation becomes `42 + 55`, then `result - 5` and then finally `result + 10`, which is correct.

### Member This

That last bit of output was a little hard to read. It would be easier if each expression could be indented one level; that way, we would be able to tell the order of traversal more clearly. Let's add this feature.

Semantic actions, we have learned, are member functions for Walker classes. We can enhance them by adding more members to the Walker class, such as member variables to hold state across semantic actions, and member functions which can be called from any semantic action. Using these, let's enhance our grammar file.

Change **calc.yantra** as follows:

```
%class Calculator;

// Region:Rules

start := numexpr;

addexpr := numexpr PLUS numexpr
%{
    startExpression("Adding:");
%}

subexpr := numexpr MINUS numexpr
%{
    startExpression("Subtracting:");
%}

numexpr := addexpr;
numexpr := subexpr;
numexpr := NUMBER (NUM)
%{
    appendNumber(NUM.text);
%}

// Region:Tokens

%left PLUS MINUS;

NUMBER := "\d+";
PLUS   := "\+";
MINUS  := "-";
WS     := "\s+"!;

// Region:Walkers

%walkers ShowTree;
%default_walker ShowTree;

%members ShowTree 
%{
    int level = 0; // Current level of indentation
    int inExpression = 0; // Position of current NUMBER

    // Return as many spaces as value of `level`,
    // for indenting.
    std::string indentLevel() {
        std::string result(level,' ');
        return result;
    }

    // Start a new level, when a new expression 
    // starts.
    void startExpression(std::string exprName) {
        std::cout << indentLevel() << exprName << std::endl;

        level++;
        inExpression = 0;
    }

    // When a number is output, increase 
    // `inExpression`. If it is even (because our
    // expressions take two numbers), decrease 
    // the level because the expression is done.
    void appendNumber(std::string numText) {
        std::cout << indentLevel() << "Number: " << numText << std::endl; 

        inExpression++;
        if(inExpression % 2 == 0) {
            level--;
            inExpression--;
        }
    }
%}
```

Notice the comments that divide our grammar file into "regions". This is just a convention: rules, tokens, and walkers may appear in pretty much any order in a grammar file. But keeping similar things together makes grammars easier to maintain. Also, a Yantra grammar allows comments using the C++ // syntax. Those comments are just text, and are ignored by Yantra.

Now, look at the Walkers region. Thus far, we have been getting away with the default Walker name generated by Yantra. But once we start adding things like members, we have to formally name our Walkers. The `%walkers` pragma does that: it is followed by a list of Walkers we wish to create for a grammar. In this case, we name only one: `ShowTree`.

Next, the `%default_walker` pragma does two things: one, it ensures that if **ycc** has been called with the option for amalgamated output, the generated `main()` function invokes the named Walker by default. Two, **ycc** ensures that any semantic actions that are not specifically labelled with a Walker name become functions of the default Walker. We will learn how to label semantic actions later. For now, all of them become members of the default walker. Please note, the `%default_walker` pragma _must_ appear after the `%walkers` pragma in a grammar file. Also note that if there is only one Walker defined in a grammar, it is automatically assumed to be the default Walker, even if we don't use the pragma.

Next, the `%members` pragma allows us to add members to the Walker whose name follows it. The members are defined in a C++ code block delimited by %{ and %}, placed immediately after the `%members` pragma. In this case, we add two member variables to keep track of the level and state of indentation, and some member functions which encapsulate them and provide the needed functionality.

Finally, look at the rewritten rules in the Rules region. Notice how the member functions are called in the semantic actions. The semantic actions are unlabelled, and so become members of the default Walker.

Generate and compile with `make`. Test it with `./calc -s "42 + 55 - 5 + 10"`. The output should look like this:

```
Adding:
 Subtracting:
  Adding:
   Number: 42
   Number: 55
  Number: 5
 Number: 10
```

### Exit Stage 2

At this point, we have enhanced our a grammar to recognize addition and subtraction. We have created a named walker, and set it as a default. We have added semantic actions to that walker that print meaningful messages as relevant rules in our grammar are parsed. And we have added members to that walker that provide support functionality for the semantic actions.

## Stage 3: Operator Precedence

### Me first

Let's add multiplication and division, but we need to handle precedence correctly (multiplication before addition).

Change **calc.yantra** as follows. For brevity, I have left out the walkers region, which remains unchanged from the previous exercise.

```
%class Calculator;

// Region:Rules

start := numexpr;

numexpr := addorsubexpr;

addorsubexpr := addorsubexpr PLUS mulordivexpr
%{
    startExpression("Adding:");
%}

addorsubexpr := addorsubexpr MINUS mulordivexpr
%{
    startExpression("Subtracting:");
%}

addorsubexpr := mulordivexpr;

mulordivexpr := mulordivexpr MUL valexpr
%{
    startExpression("Multiplying:");
%}

mulordivexpr := mulordivexpr DIV valexpr
%{
    startExpression("Dividing:");
%}

mulordivexpr := valexpr;

valexpr := NUMBER (NUM)
%{
    appendNumber(NUM.text);
%}

// Region:Tokens

%left PLUS MINUS;
%left MUL DIV;

NUMBER := "\d+";
PLUS   := "\+";
MINUS  := "-";
MUL    := "\*";
DIV    := "/";
WS     := "\s+"!;

// Region:Walkers
// Not shown here, but unchanged from the previous exercise
```

Notice how each rule is defined in terms of another rule. Like numexpr is defined using addorsubexpr, addorsubexpr is defined using mulordivexpr, and mulordivexpr is deined in terms of valexp. This means, for example, that to properly evaluate an addorsubexpr, a mulordivexpr will need to be evaluated first. As a result, multiplication and division will happen before addition and subtraction. This is how you create precedence among rules: by defining a lower-precedence rule (like addorsubexpr) in terms of a higher precedence rule (like mulordivexpr).

Also notice how each 'level' is defined using _itself_ and the next level, like `addorsubexpr := addorsubexpr PLUS mulordivexpr` and `mulordivexpr MUL valexpr`. The _itself_ part ensures that the expression can be repeated, like `1 + 2 + 3 + 4 / 2`.

Generate and compile with `make`. Test it with `./calc -s "2 + 10 / 5 - 3 * 1"`. The output should look like this:

```
Subtracting:
 Adding:
  Number: 2
  Dividing:
   Number: 10
   Number: 5
  Multiplying:
   Number: 3
   Number: 1
```

This shows that the `10 / 5` happens first, then the `3 * 1`, then the result of `10/5` is added with `2`, and finally the result of that gets the result of `3 * 1` subtracted from it.

### But (always)

But what if you wanted, for example, the `5 - 3` to happen first? In arithmetic, as well as most programming or scripting languages, this is managed using parenthesis (()). How do we ensure that parenthesis have precedence before any other case?

As we saw, precedence is achieved by defining a lower-precedence rule in terms of a higher-precedence rule. So `addorsubexpr` is defined usings `mulordivexpr`, and `mulordivexpr` is defined using `valexpr`. The highest precedence is `valexpr`, which is currently defined as just a `NUMBER`. If we also define `valexpr` as an expression with parenthesis, this should give us our desired precedence.

But what can appear inside parenthesis? Complete numeric expressions, right?

Modify **calc.yantra** as follows. As before, the walkers region is omitted here for brevity.

```
%class Calculator;

// Region:Rules

start := numexpr;

numexpr := addorsubexpr;

addorsubexpr := addorsubexpr PLUS mulordivexpr
%{
    startExpression("Adding:");
%}

addorsubexpr := addorsubexpr MINUS mulordivexpr
%{
    startExpression("Subtracting:");
%}

addorsubexpr := mulordivexpr;

addorsubexpr := valexpr;

mulordivexpr := mulordivexpr MUL valexpr
%{
    startExpression("Multiplying:");
%}

mulordivexpr := mulordivexpr DIV valexpr
%{
    startExpression("Dividing:");
%}

mulordivexpr := valexpr;

valexpr := LPAREN numexpr RPAREN;

valexpr := NUMBER (NUM)
%{
    appendNumber(NUM.text);
%}

// Region:Tokens

%left PLUS MINUS;
%left MUL DIV;

NUMBER := "\d+";
PLUS   := "\+";
MINUS  := "-";
MUL    := "\*";
DIV    := "/";
LPAREN := "\(";
RPAREN := "\)";
WS     := "\s+"!;

// Region:Walkers
// Not shown here, but unchanged from the previous exercise
```

Generate and compile with `make`. Test it with `./calc -s "2 + 10 / (5 - 3) * 1"`. The output should look like this:

```
Adding:
 Number: 2
 Multiplying:
  Dividing:
   Number: 10
   Subtracting:
    Number: 5
    Number: 3
   Number: 1
```

Test it again with `./calc -s "2 + (10 / 5 - 3) * 1"`. The results should be as expected (you did expect that, right?).

### Exit Stage 3

At this point, we have enhanced our a grammar to recognize multiplication, division and parenthesis. We have ensured precedence among all operators and parenthesis by defining 'levels' of rules in terms of other 'levels' with higher precedence (sometimes called _subrules_).

## Stage 4: Functions

### Show me the value

So far, the target compiler just shows us the operations is recognises, and gives us an idea about the order in which they will be performed. How about actually performing the calculations that it parses?

We could, with a lot of background work, make that work in the existing walker. The reason why it needs a lot of work is that we have to remember when we hit a number, and then when we hit an operator, and then perform the calculation only when we actually hit another number. But that second number may be part of a higher-precedence operation, and...you get the idea. Luckily, there is a better way in Yantra to take care of such things: _functions_.

Functions, like semantic actions, are C++ code that are attached to rule definitions. Unlike a semantic action, a function can return a single value. So, functions defined for rules with lower precedence can call functions for rules with higher precedence, and use the value (calculated first because of higher precedence) in their own calculations.

The best way to illustrate how functions work is with an example. So, change **calc.yantra** as follows:

```
%class Calculator;

// Region: Walker declarations

%walkers ShowTree Calc;
%default_walker ShowTree;

// Region:Functions

%function numexpr Calc::eval () -> int;
%function addorsubexpr Calc::eval () -> int;
%function mulordivexpr Calc::eval () -> int;
%function valexpr Calc::eval () -> int;

// Region:Rules

start := numexpr (n)
@Calc
%{
    std::cout << "Result:" << eval(n) << std::endl;
%}

numexpr := addorsubexpr (e)
@Calc::eval
%{
    return eval(e);
%}

addorsubexpr := addorsubexpr (lhs) PLUS mulordivexpr (rhs)
%{
    startExpression("Adding:");
%}
@Calc::eval
%{
    return eval(lhs) + eval(rhs);
%}

addorsubexpr := addorsubexpr (lhs) MINUS mulordivexpr (rhs)
%{
    startExpression("Subtracting:");
%}
@Calc::eval
%{
    return eval(lhs) - eval(rhs);
%}

addorsubexpr := mulordivexpr (mde)
@Calc::eval
%{
    return eval(mde);
%}

mulordivexpr := mulordivexpr (lhs) MUL valexpr (rhs)
%{
    startExpression("Multiplying:");
%}
@Calc::eval
%{
    return eval(lhs) * eval(rhs);
%}

mulordivexpr := mulordivexpr (lhs) DIV valexpr (rhs)
%{
    startExpression("Dividing:");
%}
@Calc::eval
%{
    return eval(lhs) / eval(rhs);
%}

mulordivexpr := valexpr (v)
@Calc::eval
%{
    return eval(v);
%}

valexpr := LPAREN numexpr(n) RPAREN
@Calc::eval
%{
    return eval(n);
%}

valexpr := NUMBER (NUM)
%{
    appendNumber(NUM.text);
%}
@Calc::eval
%{
    int result = std::stoi(NUM.text);
    return result;
%}

// Region:Tokens

%left PLUS MINUS;
%left MUL DIV;

NUMBER := "\d+";
PLUS   := "\+";
MINUS  := "-";
MUL    := "\*";
DIV    := "/";
LPAREN := "\(";
RPAREN := "\)";
WS     := "\s+"!;

// Region:Walker Members
%members ShowTree
%{
    int level = 0; // Current level of indentation
    int inExpression = 0; // Position of current NUMBER
    
    // Return as many spaces as value of `level`,
    // for indenting.
    std::string indentLevel() {
        std::string result(level,' ');
        return result;
    }
    
    // Start a new level, when a new expression 
    // starts.
    void startExpression(std::string exprName) {
        std::cout << indentLevel() << exprName << std::endl;
    
        level++;
        inExpression = 0;
    }
    
    // When a number is output, increase 
    // `inExpression`. If it is even (because our
    // expressions take two numbers), decrease 
    // the level because the expression is done.
    void appendNumber(std::string numText) {
        std::cout << indentLevel() << "Number: " << numText << std::endl; 
    
        inExpression++;
        if(inExpression % 2 == 0) {
            level--;
            inExpression--;
        }
    }
%}
```

Lots of new things. To begin with, notice that the %walkers and the %default_walker pragmas have been moved back to the beginning of the file. This is because walkers have to be declared _before_ you declare functions. Members can still be defined after everything else, using the %members pragma, like we have done here.

Next, notice that we have defined a new walker called Calc. For now, our old ShowTree walker remains the default.

Next, look at the functions section. A function is declared using the following syntax:

```
%function RULENAME WALKERNAME::FUNCTIONAME() -> RETURNTYPE; 
```

where RULENAME is the name of a rule (which can be defined later in the file), WALKERNAME is the name of a walker (which has to be declared _before_ the function in the file), FUNCTIONNAME is any name that follows C++ naming conventions, and RETURNTYPE is any valid C++ type.

Functions are defined using _named_ code blocks, as part of rule definitions. If you declare a function for a rule name, you have to define it for every definition of the rule name. In our example, we have defined a function called `Calc::eval` for the rule named `addorsubexpr`. Since `addorsubexpr` is defined thrice in our grammar, the function `Calc::eval` has to be defined for all three rule definitions. And it is.

A function is defined using a named code block. The name of a code block comes on the line immediately before it, using the following syntax:

```
@WALKERNAME::FUNCTIONNAME
```

Of course, the walker and the function have to be declared before the named code block is defined.

Notice, the first two definitions of the `addorsubexpr` rule have both an anonymous code block, and a named one. The anonymous code block defines the semantic action of the default walker.

Next, notice a named code block attached to the `start` rule. This is a semantic action, not a function. You can tell by the name having the syntax:

```
@WALKERNAME
```

Since `Calc` is not the default walker, we have to give a name to semantic action code blocks which are attached to it. This semantic action kicks off all the evaluation.

Finally, notice how all elements in the rule definition have variables or aliases defined for them. As we discussed before, these variables are available in the C++ context of semantic actions and functions. Here you can see them being used to actually evaluate the expression being parsed. The correct `eval` will be called, based on the type of paramter passed to it.

Generate and compile with `make`. Test it with `./calc -s "2 + 10 / 5 - 3 * 1" -w Calc`. The `-w Calc` tells the generated `main()` function to invoke the `Calc` walker instead of the defualt `ShowTree`. The result should look like this:

```
Result:1
```

Try it with any other calculation. If in doubt about how the calculation took place, omit the `-w Calc`, and our `ShowTree` walker would show us the order of evaluation.

Here's an exercise for you: make `Calc` the default walker instead of `ShowTree`. What needs to change? Hint: anonymous code blocks attached to rule definitions are assumed to be semantic actions for the default walker.

### Exit Stage 4

At this point, our grammar can perform complete calculations, and defines two walkers: one that shows the order of evaluation, and one that actually calculates the input expression.

## Conclusion: Your Parser is Ready!

Congratulations! You have successfully completed the Yantra Quick Start Tutorial and built a fully functional **integer arithmetic expression evaluator**.

You started with a simple parser that recognized a single number and progressively enhanced it to handle complex operator precedence and nested parenthetical expressions.

### Key Achievements

During this tutorial, you mastered several fundamental Yantra concepts:

* **Grammar Workflow:** You learned the standard process of defining tokens and rules in a `.yantra` file and compiling it into a runnable C++ binary using `ycc` and a C++ compiler.
* **Tokens and Rules:** You defined fundamental grammar components, including ignorable tokens (like whitespace) and recursive rules to handle repeated operations.
* **Semantic Actions:** You implemented basic, unlabelled semantic actions to observe the parser's traversal order.
* **Operator Precedence:** You effectively managed calculation order using a combination of **rule hierarchy (subrules)** and the **`%left` pragma**.
* **Walkers and Functions:** You defined two distinct walkers—`ShowTree` (for visualization) and `Calc` (for evaluation)—and used **functions** to return calculated values, enabling complete expression evaluation.

### Where to Go Next

The expression evaluator you've built is a powerful foundation. You are now equipped to tackle more complex parsing challenges. Here are a few ideas for extending your new calculator:

* **Support Floating-Point Numbers:** Modify the `NUMBER` token and `Calc::eval` functions to handle decimal values.
* **Implement New Operators:** Add support for exponents (`^`) and the modulo operator (`%`).
* **Add Error Handling:** Explore Yantra's mechanisms for providing more specific and custom error messages during runtime parsing failures.
* **Generate an Abstract Syntax Tree (AST):** Learn how to use Walker functions to build a pure data structure (an AST) that can be passed to other parts of your compiler or application for later processing.

We encourage you to explore the full Yantra documentation to continue your parser development journey!
