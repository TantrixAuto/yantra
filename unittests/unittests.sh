#!/bin/bash

logger="a.log"
#logger="-"

failfast=0
passcount=0
failcount=0

enabled=1

if [[ -z "${YCC}" ]]; then
  YCC=~/build/yantra/Debug/bin/ycc
fi

while getopts "h?fd" opt; do
  case "$opt" in
    h|\?)
      echo "-f : failfast"
      exit 0
      ;;
    f)
      failfast=1
      ;;
    d)
      enabled=0
      ;;
  esac
done

compile_grammar() {
  if [ $enabled -eq 0 ]; then
    return
  fi

  grammar="$1"
  xerr="$2" #0 - expect success, 1 = expect ycc-error, 2 = expect compile-error

  echo ${BASH_LINENO}: Generating parser
  ${YCC} -c ascii -s "$grammar" -a -n out -g out.md
  if [ $? -ne 0 ]; then
    if [ $xerr -eq 1 ]; then
      return
    fi
    if [ $failfast -ne 0 ]; then
        echo ${BASH_LINENO}: Exiting on failure-ycc failed
        echo $grammar
        exit $?
    fi
    failcount=$((failcount+1))
  else
    passcount=$((passcount+1))
  fi

  FLAGS="-Wall -Werror -Weverything -Wno-padded -Wno-c++98-compat-pedantic -Wno-exit-time-destructors -Wno-global-constructors -Wno-weak-vtables -Wno-switch-default -Wno-switch-enum -Wno-header-hygiene"

  echo ${BASH_LINENO}: Compiling parser
  clang++ -g -std=c++20 $FLAGS -include-pch testpch.hpp.pch out.cpp
  if [ $? -ne 0 ]; then
    if [ $xerr -eq 2 ]; then
      return
    fi
    if [ $failfast -ne 0 ]; then
        echo ${BASH_LINENO}: Exiting on failure-clang failed
        echo $grammar
        exit $?
    fi
    failcount=$((failcount+1))
  else
    passcount=$((passcount+1))
  fi

}

run_passing_test() {
  if [ $enabled -eq 0 ]; then
    return
  fi

  input="$1"

  echo ${BASH_LINENO}: Running passing test ["$input"]
  ./a.out -s "$input" -l "$logger"
  if [ $? -ne 0 ]; then
    if [ $failfast -ne 0 ]; then
        echo ${BASH_LINENO}: Exiting on failure-passing test failed
        exit $?
    fi
    failcount=$((failcount+1))
  else
    passcount=$((passcount+1))
  fi
}

run_failing_test() {
  if [ $enabled -eq 0 ]; then
    return
  fi

  input="$1"

  echo ${BASH_LINENO}: Running failing test ["$input"]
  ./a.out -s "$input" -l "$logger"
  if [ $? -eq 0 ]; then
    if [ $failfast -ne 0 ]; then
        echo ${BASH_LINENO}: Exiting on failure - failing test passed
        exit $?
    fi
    failcount=$((failcount+1))
  else
    passcount=$((passcount+1))
  fi
}

if [ ! -f testpch.hpp.pch ]; then
  clang++ -c -std=c++20 testpch.hpp -o testpch.hpp.pch
fi

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := ID;
stmt := HEX;
ID := "[A-Za-z]";
HEX := "A";
HEX := "a";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'G A a'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := ID;
stmt := HEX;

ID := ".";
HEX := "A";
HEX := "a";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'G A a'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := ID;
ID := "[A-Z][a-z]";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'Ag Aa'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := ID;
ID := "[A-Z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A'
run_passing_test 'AAAA'
run_failing_test 'AAAA1'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := ID;
ID := "[A-K]{2,5}Z";
'

compile_grammar "$grammar" 0
run_failing_test 'AZ'
run_passing_test 'ABZ'
run_passing_test 'ABCZ'
run_passing_test 'ABCDZ'
run_passing_test 'ABCDEZ'
run_failing_test 'ABCDEFZ'
run_failing_test 'ABCDEFGHZ'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := ID;
ID := "a[A-Z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'aBCD'
run_failing_test 'aBCDe'
run_failing_test 'aEFGa'
run_failing_test 'aEFG1'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := ID;
stmt := NUM;
ID := "[A-Za-z]+";
NUM := "[0-9]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'AAAA123A'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := IDU;
stmt := IDL;
IDU := "A*1";
IDL := "a*2";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A1'
run_passing_test 'a2'
run_failing_test 'A2'
run_failing_test 'a1'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := IDU;
stmt := IDL;
IDU := "[A-Z]*1";
IDL := "[a-z]*2";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'AAAA1'
run_passing_test 'A1'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := IDU;
IDU := "[A-F]{3}Z";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'AZ'
run_passing_test 'ABZ'
run_passing_test 'ABCZ'
run_failing_test 'ABCDZ'
run_failing_test 'ABCDEZ'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := IDU;
IDU := "[^A-FH-M]";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'G'
run_failing_test 'A'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := IDU;
stmt := IDL;
IDU := "[A-Z]+1";
IDL := "[a-z]+2";
IDL := "[0-9]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'AAAA1'
run_passing_test 'A1'
run_passing_test 'A1'
run_passing_test 'a2'
run_passing_test '11'
run_passing_test '2'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := ID;
ID := "[A-F][a-f]*";
'

compile_grammar "$grammar" 0
run_passing_test 'Aa'
run_passing_test 'A'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := IDU;
stmt := IDL;
IDU := "[A-Z]*1";
IDL := "[a-z]*1";
WS := "\s"!;
'

compile_grammar "$grammar" 1

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := IDU;
stmt := IDL;

IDU := "[A-Z]*1";
IDL := "[a-z]*1";

WS := "\s"!;
'

compile_grammar "$grammar" 1

#############################
# when one TX is a superset of another.
# Here [A-Z] is a superset of [D-H]
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := IDU;
stmt := IDL;
stmt := IDC;
//IDL := "[A-FL-R]+1";
IDL := "[L-R]+1";
IDU := "[A-Z]*2";
IDC := "[D-H]*3";
WS := "\s"!;
'

enabled=0
compile_grammar "$grammar" 0
run_passing_test 'L1'
run_passing_test 'A2'
run_passing_test 'DEFA2'
run_passing_test 'DEFE3'
run_failing_test '1'
run_passing_test '2'
run_passing_test '3'
enabled=1

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := IDU;
stmt := IDL;
stmt := IDC;
IDL := "[A-FL-R]+1";
IDC := "[D-H]*[3-46-7]";
IDU := "[A-Z]*2";
WS := "\s"!;
'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := IDU;
stmt := IDL;
stmt := IDC;
stmt := IDH;
IDL := "[A-FL-R]+1";
IDC := "[D-H]*2";
IDH := "[H-K]*3";
IDU := "[A-Z]*4";
WS := "\s"!;
'

#############################
grammar='
start := STRING;
STRING := "A[^B]*B";
'

compile_grammar "$grammar" 0
run_passing_test 'AB'
run_failing_test 'A'
run_passing_test 'AB'
run_failing_test 'AC'
run_passing_test 'ACDEB'
run_failing_test 'ABCDE'

#############################
grammar='
start := IDU;
IDU := "A{1,3}|B{4,7}Z";
'
compile_grammar "$grammar" 0
run_failing_test 'AZ'
run_passing_test 'A'
run_passing_test 'AA'
run_passing_test 'AAA'
run_failing_test 'AAAA'
run_failing_test 'BZ'
run_failing_test 'BBZ'
run_failing_test 'BBBZ'
run_passing_test 'BBBBZ'
run_passing_test 'BBBBBBBZ'
run_failing_test 'BBBBBBBBZ'

#############################
grammar='
start := IDU;
IDU := "A{1,2}Z";
'

compile_grammar "$grammar" 0
run_passing_test 'AZ'

#############################
grammar='
start := IDU;
IDU := "A{1,5}Z";
'

compile_grammar "$grammar" 0
run_passing_test 'AZ'

#############################
grammar='
start := IDU;
IDU := "A{2,3}Z";
'

compile_grammar "$grammar" 0
run_failing_test 'AZ'
run_passing_test 'AAZ'
run_passing_test 'AAAZ'
run_failing_test 'AAAAZ'

#############################
grammar='
start := IDU;
IDU := "A{2,4}Z";
'

compile_grammar "$grammar" 0
run_failing_test 'AZ'
run_passing_test 'AAZ'
run_passing_test 'AAAZ'
run_passing_test 'AAAAZ'
run_failing_test 'AAAAAZ'

#############################
grammar='
start := IDU;
IDU := "A{2,7}Z";
'

compile_grammar "$grammar" 0
run_failing_test 'AZ'
run_passing_test 'AAZ'
run_passing_test 'AAAZ'
run_passing_test 'AAAAAAZ'
run_passing_test 'AAAAAAAZ'
run_failing_test 'AAAAAAAAZ'

#############################
grammar='
start := IDU;
IDU := "A{3,7}Z";
'

compile_grammar "$grammar" 0
run_failing_test 'AZ'
run_failing_test 'AAZ'
run_passing_test 'AAAZ'
run_passing_test 'AAAAAAZ'
run_passing_test 'AAAAAAAZ'
run_failing_test 'AAAAAAAAZ'

#############################
grammar='
start := IDU;
IDU := "A{5,6}Z";
'

compile_grammar "$grammar" 0
run_failing_test 'AZ'
run_failing_test 'AAZ'
run_failing_test 'AAAZ'
run_failing_test 'AAAAZ'
run_passing_test 'AAAAAZ'
run_passing_test 'AAAAAAZ'
run_failing_test 'AAAAAAAZ'

#############################
grammar='
start := IDU;
IDU := "A{5,9}Z";
'

compile_grammar "$grammar" 0
run_failing_test 'AZ'
run_failing_test 'AAZ'
run_failing_test 'AAAZ'
run_failing_test 'AAAAZ'
run_passing_test 'AAAAAZ'
run_passing_test 'AAAAAAZ'
run_passing_test 'AAAAAAAZ'
run_passing_test 'AAAAAAAAZ'
run_passing_test 'AAAAAAAAAZ'
run_failing_test 'AAAAAAAAAAZ'

#############################
grammar='
start := STRING(S)
%{
  if(S.text != "xx") {
    throw std::runtime_error("unexpected text:[" + S.text + "]");
  }
%}

STRING := "(!\")xx(!\")";
'

compile_grammar "$grammar" 0
run_passing_test '"xx"'

#############################
grammar='
start := STRING(S) %{
  if(S.text != "xx") {
    throw std::runtime_error("unexpected text:[" + S.text + "]");
  }
%}

STRING := "(!\")[^\"]*(!\")";
'

compile_grammar "$grammar" 0
run_passing_test '"xx"'

#############################
grammar='
start := ID;
ID := "[A-F][a-f0-9R-V]*";
'

compile_grammar "$grammar" 0
run_passing_test 'A'
run_passing_test 'Aa'
run_passing_test 'A1'
run_passing_test 'Aac1efR4T'
run_failing_test 'Aac1efr4t'
run_failing_test 'AZ'

#############################
grammar='
start := stmts;
stmts := stmts_;        // 1
stmts_ := stmts_ stmt_; // 2
stmts_ := stmt_;        // 3
stmt_ := stmt;          // 4
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts := stmts_;        // 1
stmts_ := stmts_ stmt_; // 2
stmt_ := stmt;          // 4
stmts_ := stmt_;        // 3
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts := stmts_;        // 1
stmts_ := stmt_;        // 3
stmts_ := stmts_ stmt_; // 2
stmt_ := stmt;          // 4
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts := stmts_;        // 1
stmts_ := stmt_;        // 3
stmt_ := stmt;          // 4
stmts_ := stmts_ stmt_; // 2
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts := stmts_;        // 1
stmt_ := stmt;          // 4
stmts_ := stmts_ stmt_; // 2
stmts_ := stmt_;        // 3
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts := stmts_;        // 1
stmt_ := stmt;          // 4
stmts_ := stmt_;        // 3
stmts_ := stmts_ stmt_; // 2
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts_ := stmts_ stmt_; // 2
stmts := stmts_;        // 1
stmts_ := stmt_;        // 3
stmt_ := stmt;          // 4
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts_ := stmts_ stmt_; // 2
stmts := stmts_;        // 1
stmt_ := stmt;          // 4
stmts_ := stmt_;        // 3
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts_ := stmts_ stmt_; // 2
stmts_ := stmt_;        // 3
stmts := stmts_;        // 1
stmt_ := stmt;          // 4
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts_ := stmts_ stmt_; // 2
stmts_ := stmt_;        // 3
stmt_ := stmt;          // 4
stmts := stmts_;        // 1
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts_ := stmts_ stmt_; // 2
stmt_ := stmt;          // 4
stmts := stmts_;        // 1
stmts_ := stmt_;        // 3
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts_ := stmts_ stmt_; // 2
stmt_ := stmt;          // 4
stmts_ := stmt_;        // 3
stmts := stmts_;        // 1
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts_ := stmt_;        // 3
stmts_ := stmts_ stmt_; // 2
stmts := stmts_;        // 1
stmt_ := stmt;          // 4
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts_ := stmt_;        // 3
stmts_ := stmts_ stmt_; // 2
stmt_ := stmt;          // 4
stmts := stmts_;        // 1
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts_ := stmt_;        // 3
stmt_ := stmt;          // 4
stmts := stmts_;        // 1
stmts_ := stmts_ stmt_; // 2
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts_ := stmt_;        // 3
stmt_ := stmt;          // 4
stmts_ := stmts_ stmt_; // 2
stmts := stmts_;        // 1
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmt_ := stmt;          // 4
stmts := stmts_;        // 1
stmts_ := stmts_ stmt_; // 2
stmts_ := stmt_;        // 3
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmt_ := stmt;          // 4
stmts := stmts_;        // 1
stmts_ := stmt_;        // 3
stmts_ := stmts_ stmt_; // 2
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmt_ := stmt;          // 4
stmts_ := stmts_ stmt_; // 2
stmts := stmts_;        // 1
stmts_ := stmt_;        // 3
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmt_ := stmt;          // 4
stmts_ := stmts_ stmt_; // 2
stmts_ := stmt_;        // 3
stmts := stmts_;        // 1
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmt_ := stmt;          // 4
stmts_ := stmt_;        // 3
stmts := stmts_;        // 1
stmts_ := stmts_ stmt_; // 2
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmt_ := stmt;          // 4
stmts_ := stmt_;        // 3
stmts_ := stmts_ stmt_; // 2
stmts := stmts_;        // 1
stmt := HEX;
HEX := "[A-Za-z]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
%walkers Generator;

start := stmts;

stmts := stmts stmt;
stmts := stmt;

%function stmt Generator::len() -> size_t;
stmt := HEX(V)
@Generator::len %{
  return V.text.size();
%}

HEX := "[A-Za-z]+";
HEX := "0x[A-Fa-f0-9]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'A Z'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := VAR ID SEMI;
stmt := VAL ID SEMI;

ID := "[A-Za-z][A-Za-z0-9_]*";
VAR := "var";
VAL := "val";
WS := "\s"!;
SEMI := ";";
'

compile_grammar "$grammar" 0
run_passing_test 'var v;'
run_passing_test 'var vz;'
run_passing_test 'var v1;'
run_passing_test 'var vaz;'
run_passing_test 'var varz;'
run_passing_test 'var var1;'
run_passing_test 'var val1;'
run_passing_test 'val var1;'
run_passing_test 'val val1;'

#############################
grammar='
%fallback ID VAR;

start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := VAR ID SEMI;

VAR := "var";
ID := "[a-z]+";
SEMI := ";";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'var v;'
run_passing_test 'var vz;'
run_passing_test 'var vaz;'
run_passing_test 'var varz;'
run_passing_test 'var var;'

#############################
# test ruletype with deleted copy-ctors
grammar='
%walkers Generator;
start := stmt;

%function stmt Generator::len() -> std::unique_ptr<std::string>;
stmt := ID
@Generator::len %{
  return std::make_unique<std::string>("xyz");
%}

ID := "[a-z]+";
'

compile_grammar "$grammar" 0
run_passing_test 'var'

#############################
grammar='
start := ID;
//ID := "(A*|B*)Z";
//ID := "(A|B)Z";
//ID := "(A+|B+)Z";
//ID := "(A+|B+)";
ID := "A*Z";
'

#############################
grammar='
start := IDU;
IDU := "A+|B+";
//IDU := "A|BZ";
//IDU := "(A|B)Z";
//IDU := "(A{1,3}|B{4,7})Z";
//IDU := "[A-F]{1,3}Z";
//IDU := "(A{1,3})|(B{4,7})Z";
'

#############################
grammar='
start := stmt;

stmt := IF;
stmt := IF ELSE;

IF := "if";
ELSE := "else";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'if else'
run_passing_test 'if'

#############################
grammar='
start := stmt;

stmt := IF ELSE;
stmt := IF;

IF := "if";
ELSE := "else";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'if else'
run_passing_test 'if'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;

stmt := ID(I)
%{
  std::print("ID:{}\n", I.text);
%}

ID := "[A-Z]+";
ENTER_MLCOMMENT := "/\*"! [ML_COMMENT_MODE];
WS := "\s"!;

%lexer_mode ML_COMMENT_MODE;
ENTER_MLCOMMENT := "/\*"! [ML_COMMENT_MODE];
LEAVE_MLCOMMENT := "\*/"! [^];
CMT := ".*"!;
'

compile_grammar "$grammar" 0
run_passing_test 'ABC'
run_passing_test 'ABC /*xxx*/'
run_passing_test 'ABC /*xxx*/ DEF'
run_passing_test 'ABC /*xxx /*yyy*/ zzz*/ DEF'
run_passing_test 'ABC /*xxx*/ DEF /*yyy*/ GHI'

#############################
grammar='
%walkers Generator;

start := stmts;
stmts := stmts stmt;
stmts := stmt;

//%function stmt Generator::eval() -> int;
stmt := ID EQ expr(e) SEMI
@Generator %{
  auto v = eval(e);
  std::print("e={}\n", v);
%}

//%type expr int;
%function expr Generator::eval() -> int;
expr := expr(l) PLUS expr(r)
@Generator::eval %{
  return eval(l) + eval(r);
%}

expr := expr(l) STAR expr(r)
@Generator::eval %{
  return eval(l) * eval(r);
%}

expr := NUM(N)
@Generator::eval %{
  return std::atoi(N.text.c_str());
%}

SEMI := ";";
STAR := "\*";
PLUS :=> "\+";

EQ := "=";
ID :== "[A-Za-z]+";
NUM := "[0-9]+";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'xx = 11 + 12 + 13;'
run_passing_test 'xx = 11 * 12 * 13;'

#############################
grammar='
start := stmt;

stmt := arg %{
//    return 0;
%}

//%type arg size_t;
arg := type ID %{
//    return 0;//t->n.text.size();
%}

type := INT_TYPE;

INT_TYPE := "int";

ID := "[A-Za-z][A-Za-z0-9_]*";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'int aa'

#############################
grammar='
start := stmt;
stmt := ID argsx stmt_block;

argsx := LBRACKET RBRACKET;
stmt_block := LCURLY RCURLY;

LBRACKET := "\(";
RBRACKET := "\)";
LCURLY := "\{";
RCURLY := "\}";

ID := "[A-Za-z][A-Za-z0-9_]*";
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'fn(){}'

#############################
# rStructAttrList is nullable, followed by a token STRUCT
grammar='
start := stmt;
stmt := rStructStatement;
stmt := rServiceStatement;

rStructStatement := rStructDef SEMI;
rServiceStatement := rServiceDef SEMI;

rStructDef := rStructAttrList STRUCT;
rServiceDef := rStructAttrList SERVICE;

rStructAttrList := ATTR1;
rStructAttrList := ;

SEMI := ";";

STRUCT := "struct";
SERVICE := "service";
ATTR1 := "attr1";

WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'attr1 struct;'
run_passing_test 'attr1 service;'
run_passing_test 'struct;'
run_passing_test 'service;'

#############################
# rStructAttrList is nullable, followed by a token STRUCT
grammar='
start := rStatements;

rStatements := rStatements rStatement;
rStatements := rStatement;

////////////////////////////////////////////////////////////////////////////////
rStatement := rStructAttrList SERVICE IDENTIFIER SEMI;
rStatement := INTERFACE IDENTIFIER SEMI;

////////////////////////////////////////////////////////////////////////////////
rStructAttrList := rStructAttrItem;
rStructAttrList := ;

rStructAttrItem := TYPE_VOID;

////////////////////////////////////////////////////////////
INTERFACE := "interface";
SERVICE := "service";

IDENTIFIER := "[\l][\l\d_]*";
TYPE_VOID := "void";

SEMI  := ";";

WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'interface x;'
run_passing_test 'void service x;'
run_passing_test 'interface x;'
run_passing_test 'service x;'

#############################
# rStructAttrList is nullable, followed by a token STRUCT
grammar='
start := rParameterDef;

rParameterDef := rTypeRef IDENTIFIER;
rParameterDef := rTypeRef CARET IDENTIFIER;

rTypeRef := rTypeRefBase rTypeRefQualifier;
rTypeRefBase := rPrimaryTypeRef;

rPrimaryTypeRef := TYPE_STRING;

rTypeRefQualifier := STAR;
rTypeRefQualifier := ;

IDENTIFIER := "[\l][\l\d_]*";
TYPE_STRING := "string";
STAR := "\*";
CARET  := "^";

WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'string msgID'
run_passing_test 'string* msgID'
run_passing_test 'string ^msgID'
run_passing_test 'string* ^msgID'

#############################
grammar='
start := stmt;
stmt := NAMESPACE ID SEMI;

NAMESPACE := "namespace";
ID := "[\l][\l\d]*";
SEMI := ";";

WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'namespace name;'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;

stmt := ID;
stmt := GO1;
stmt := GO2;

ID := "ABC";
GO1 := "GO1"! [alt1];
WS := "\s"!;

%lexer_mode alt1;
ID := "DEF";
GOS := "GOS"! [alt1];
GOB := "GOB"! [^];
GO2 := "GO2"! [alt2];
GOX := "GOX"! [];
WS := "\s"!;

%lexer_mode alt2;
ID := "GHI";
GOY := "GOY"! [^];
GOZ := "GOZ"! [];
WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'ABC'
run_passing_test 'ABC GO1'
run_passing_test 'ABC GO1 DEF'
run_passing_test 'ABC GO1 DEF GO2'
run_passing_test 'ABC GO1 DEF GO2 GHI'
run_failing_test 'DEF'
run_failing_test 'ABC DEF'
run_failing_test 'ABC GO2'
run_failing_test 'ABC GO1 ABC'
run_passing_test 'ABC GO1 DEF GO2 GHI GOY DEF'
run_passing_test 'ABC GO1 DEF GO2 GHI GOZ ABC'
run_failing_test 'ABC GO1 DEF GO2 GHI GOZ DEF'

#############################
grammar='
%walkers CppServer;
//%walker_output CppServer text_file cpp;
%walker_traversal CppServer top_down;

start := rParameterDef;

rParameterDef := rTypeRef(t) IDENTIFIER
@CppServer %{
    unused(t);
    std::print("rParameterDef/@CppServer\n");
    str(t, 1);
%}

rParameterDef := rTypeRef CARET IDENTIFIER;

//%type rTypeRef int;
//%type rTypeRef std::string;
%function rTypeRef CppServer::str(int x) -> std::string;
%function rTypeRef CppServer::len(int x) -> size_t;
rTypeRef := rTypeRefBase rTypeRefQualifier
@CppServer %{
    std::print("rTypeRef/@CppServer ***\n");
%}
@CppServer::str %{
    unused(x);
    std::print("rTypeRef/CppServer::str ***\n");
    return "";
%}
@CppServer::len %{
    unused(x);
    std::print("rTypeRef/CppServer::len ***\n");
    return 0;
%}

rTypeRefBase := rPrimaryTypeRef;

rPrimaryTypeRef := TYPE_STRING;

rTypeRefQualifier := STAR;
rTypeRefQualifier := ;

IDENTIFIER := "[\l][\l\d_]*";
TYPE_STRING := "string";
STAR := "\*";
CARET  := "^";

WS := "\s"!;
'

compile_grammar "$grammar" 0
run_passing_test 'string msgID'
run_passing_test 'string ^msgID'

#############################
grammar='
%class Calculator;
%left PLUS MINUS;
%left STAR SLASH;

start := expr;

expr := expr PLUS expr;
expr := expr MINUS expr;
expr := expr STAR expr;
expr := expr SLASH expr;
expr := NUMBER;

NUMBER := "\d+";
PLUS := "\+";
MINUS := "-";
STAR := "\*";
SLASH := "/";
WS := "\s+"!;
'

compile_grammar "$grammar" 0
run_passing_test '1 + 2'
run_passing_test '1 - 23'

#############################
# this infinite loop
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;

stmt := IF l_expr LCURLY RCURLY;
stmt := IF l_expr LCURLY stmts RCURLY ELSE;

l_expr := HEX;

IF := "if";
ELSE := "else";
LCURLY := "\{";
RCURLY := "\}";

HEX := "[A-Za-z]+";
HEX := "0x[A-Fa-f0-9]+";
WS := "\s"!;
'

#############################
# test EPSILON
grammar='
%members %{
    using DataType = std::variant<bool, int64_t, std::string>;
    std::unordered_map<std::string, DataType> vars;
    static inline std::string str(const DataType& dt) {
        return std::visit([](const auto& v){
            return std::format("{}", v);
        }, dt);
    }
%}

start := stmt;

stmt := LBRACKET params RBRACKET;

params := params COMMA param;
params := param;

param := expr;
//param := ;

expr := ID;

LBRACKET := "\(";
RBRACKET := "\)";
COMMA := ",";
ID := "[A-Za-z]+";
WS := "\s"!;
'

#############################
# this gives SR conflict
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;

stmt := IF l_expr LCURLY stmts RCURLY;
stmt := IF l_expr LCURLY stmts RCURLY ELSE;

l_expr := HEX;

IF := "if";
ELSE := "else";
LCURLY := "\{";
RCURLY := "\}";

HEX := "[A-Za-z]+";
HEX := "0x[A-Fa-f0-9]+";
WS := "\s"!;
'

#############################
# basic RXes for new Lexer
grammar='
start := stmt;
stmt := RX1;
stmt := RX2;
stmt := RX3;

//RX1 := "AB";
//RX1 := "A|B";
//RX1 := "[^A-Za-z0-9]";
//RX1 := "(ABC)";
//RX1 := "(![^A-Za-z0-9])";
RX1 := "A{3,7}";
RX2 := "[^A-Za-z]*";
RX3 := "[A-Z]";

//RX1 := "[^;]+";
'

#############################
# basic RX superset tests for new Lexer
grammar='
%fallback ID VAR;

start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := VAR ID SEMI;
//stmt := HEX SEMI;

VAR := "var";
ID := "[a-z]+";
//HEX := "[a-z][a-f][p-t]";
SEMI := ";";
WS := "\s"!;
'

#############################
echo All tests done
echo PASSED $passcount
echo FAILED $failcount
