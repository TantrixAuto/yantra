#!/bin/bash

logger="a.log"
failfast=0
passcount=0
failcount=0
verbose=0
enabled=1

YCC=./bin/ycc

while getopts "h?fdvy:l:" opt; do
  case "$opt" in
    h|\?)
      echo "-f : failfast"
      exit 0
      ;;
    f)
      failfast=1
      ;;
    v)
      verbose=1
      ;;
    y)
      YCC="$OPTARG"
      ;;
    l)
      logger="$OPTARG"
      ;;
    d)
      enabled=0
      ;;
  esac
done

if [[ -n "$MSYSTEM" ]]; then
  MSYS2_ARG_CONV_EXCL=* # set this using export on command line
  CC="cl.exe"
  FLAGS="/EHsc /nologo"
  OUT="./out.exe"
else
  CC="clang++"
  FLAGS="-Wall -Werror -Weverything -Wno-padded -Wno-c++98-compat-pedantic -Wno-exit-time-destructors -Wno-global-constructors -Wno-weak-vtables -Wno-switch-default -Wno-switch-enum -Wno-header-hygiene -Wno-poison-system-directories"
  if [ -f testpch.hpp.pch ]; then
    FLAGS="$FLAGS -include-pch testpch.hpp.pch"
  fi
  OUT="./a.out"
fi

if [ ! -f ${YCC} ]; then
  echo "YCC executable not found at ${YCC}"
  exit 1
fi

if [ ! -f testpch.hpp.pch ]; then
  if [ -f testpch.hpp ]; then
    ${CC} -c -std=c++20 testpch.hpp -o testpch.hpp.pch
  fi
fi

compile_grammar() {
  if [ $enabled -eq 0 ]; then
    return
  fi

  grammar="$1"
  xerr="$2" #0 - expect success, 1 = expect ycc-error, 2 = expect compile-error

  if [ $verbose -eq 1 ]; then
    echo ${BASH_LINENO}: Generating parser
  fi
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

  if [ $verbose -eq 1 ]; then
    echo ${BASH_LINENO}: Compiling parser
  fi

  if [[ -n "$MSYSTEM" ]]; then
    ${CC} /std:c++20 $FLAGS out.cpp
  else
    ${CC} -std=c++20 $FLAGS out.cpp
  fi

  if [ $? -ne 0 ]; then
    if [ $xerr -eq 2 ]; then
      return
    fi
    if [ $failfast -ne 0 ]; then
        echo ${BASH_LINENO}: Exiting on failure: compile failed
        echo $grammar
        exit $?
    fi
    failcount=$((failcount+1))
  else
    passcount=$((passcount+1))
  fi

}

run_passing_test() {
  local OPTIND OPTARG opt input xoutput routput
  if [ $enabled -eq 0 ]; then
    return
  fi

  OPTIND=1
  input=""
  xoutput=""

  while getopts "s:t:" opt "$@"; do
    case "$opt" in
      s)
        input="$OPTARG"
        ;;
      t)
        xoutput="$OPTARG"
        ;;
    esac
  done

  echo -n "${BASH_LINENO}: Running passing test [$input]... "
  routput=$("$OUT" -s "$input" -l "$logger" -t1)
  if [ $? -ne 0 ]; then
    if [ $failfast -ne 0 ]; then
      echo "${BASH_LINENO}: Exiting on failure-passing test failed"
      exit $?
    fi
    failcount=$((failcount+1))
    echo "FAIL"
  else
    if [ "$routput" != "$xoutput" ]; then
      echo "EXPT: [$xoutput]"
      echo "RECV: [$routput]"
      if [ $failfast -ne 0 ]; then
        echo "${BASH_LINENO}: Exiting on failure-unexpected output"
        exit 1
      fi
      failcount=$((failcount+1))
      echo "FAIL"
    else
      passcount=$((passcount+1))
      echo "PASS"
    fi
  fi
}

run_failing_test() {
  local OPTIND OPTARG opt input xoutput routput
  if [ $enabled -eq 0 ]; then
    return
  fi

  OPTIND=1
  input=""
  xoutput=""

  while getopts "s:e:" opt "$@"; do
    case "$opt" in
      s)
        input="$OPTARG"
        ;;
      e)
        xoutput="$OPTARG"
        ;;
    esac
  done

  echo -n "${BASH_LINENO}: Running failing test [$input]... "
  routput=$("$OUT" -s "$input" -l "$logger")
  if [ $? -eq 0 ]; then
    if [ $failfast -ne 0 ]; then
        echo ${BASH_LINENO}: Exiting on failure - failing test passed
        echo $routput
        exit $?
    fi
    failcount=$((failcount+1))
    echo "FAIL"
  else
    passcount=$((passcount+1))
    echo "PASS"
  fi
}

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
run_passing_test \
  -s 'G A a' \
  -t '0:start_1(1:stmts_1(2:stmts_1(3:stmts_2(4:stmt_1(5:ID(G))) 3:stmt_2(4:HEX(A))) 2:stmt_2(3:HEX(a))) 1:_tEND())'

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
run_passing_test \
  -s 'G A a' \
  -t '0:start_1(1:stmts_1(2:stmts_1(3:stmts_2(4:stmt_1(5:ID(G))) 3:stmt_2(4:HEX(A))) 2:stmt_2(3:HEX(a))) 1:_tEND())'

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
run_passing_test -s 'Ag Aa' -t '0:start_1(1:stmts_1(2:stmts_2(3:stmt_1(4:ID(Ag))) 2:stmt_1(3:ID(Aa))) 1:_tEND())'

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
run_passing_test -s 'A' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(A))) 1:_tEND())'
run_passing_test -s 'AAAA' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(AAAA))) 1:_tEND())'
run_failing_test -s 'AAAA1'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := ID;
ID := "[A-K]{2,5}Z";
'

compile_grammar "$grammar" 0
run_failing_test -s 'AZ'
run_passing_test -s 'ABZ' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(ABZ))) 1:_tEND())'
run_passing_test -s 'ABCZ' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(ABCZ))) 1:_tEND())'
run_passing_test -s 'ABCDZ' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(ABCDZ))) 1:_tEND())'
run_passing_test -s 'ABCDEZ' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(ABCDEZ))) 1:_tEND())'
run_failing_test -s 'ABCDEFZ'
run_failing_test -s 'ABCDEFGHZ'

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
run_passing_test -s 'aBCD' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(aBCD))) 1:_tEND())'
run_failing_test -s 'aBCDe'
run_failing_test -s 'aEFGa'
run_failing_test -s 'aEFG1'

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
run_passing_test -s 'AAAA123A' -t '0:start_1(1:stmts_1(2:stmts_1(3:stmts_2(4:stmt_1(5:ID(AAAA))) 3:stmt_2(4:NUM(123))) 2:stmt_1(3:ID(A))) 1:_tEND())'

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
run_passing_test -s 'A1' -t '0:start_1(1:stmts_2(2:stmt_1(3:IDU(A1))) 1:_tEND())'
run_passing_test -s 'a2' -t '0:start_1(1:stmts_2(2:stmt_2(3:IDL(a2))) 1:_tEND())'
run_failing_test -s 'A2'
run_failing_test -s 'a1'

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
run_passing_test -s 'AAAA1' -t '0:start_1(1:stmts_2(2:stmt_1(3:IDU(AAAA1))) 1:_tEND())'
run_passing_test -s 'A1' -t '0:start_1(1:stmts_2(2:stmt_1(3:IDU(A1))) 1:_tEND())'

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
run_passing_test -s 'AZ' -t '0:start_1(1:stmts_2(2:stmt_1(3:IDU(AZ))) 1:_tEND())'
run_passing_test -s 'ABZ' -t '0:start_1(1:stmts_2(2:stmt_1(3:IDU(ABZ))) 1:_tEND())'
run_passing_test -s 'ABCZ' -t '0:start_1(1:stmts_2(2:stmt_1(3:IDU(ABCZ))) 1:_tEND())'
run_failing_test -s 'ABCDZ'
run_failing_test -s 'ABCDEZ'

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
run_passing_test -s 'G' -t '0:start_1(1:stmts_2(2:stmt_1(3:IDU(G))) 1:_tEND())'
run_failing_test -s 'A'

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
run_passing_test -s 'AAAA1' -t '0:start_1(1:stmts_2(2:stmt_1(3:IDU(AAAA1))) 1:_tEND())'
run_passing_test -s 'A1' -t '0:start_1(1:stmts_2(2:stmt_1(3:IDU(A1))) 1:_tEND())'
run_passing_test -s 'A1' -t '0:start_1(1:stmts_2(2:stmt_1(3:IDU(A1))) 1:_tEND())'
run_passing_test -s 'a2' -t '0:start_1(1:stmts_2(2:stmt_2(3:IDL(a2))) 1:_tEND())'
run_passing_test -s '11' -t '0:start_1(1:stmts_2(2:stmt_2(3:IDL(11))) 1:_tEND())'
run_passing_test -s '2' -t '0:start_1(1:stmts_2(2:stmt_2(3:IDL(2))) 1:_tEND())'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;
stmt := ID;
ID := "[A-F][a-f]*";
'

compile_grammar "$grammar" 0
run_passing_test -s 'Aa' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(Aa))) 1:_tEND())'
run_passing_test -s 'A' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(A))) 1:_tEND())'

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

#compile_grammar "$grammar" 0
#run_passing_test -s 'L1'
#run_passing_test -s 'A2'
#run_passing_test -s 'DEFA2'
#run_passing_test -s 'DEFE3'
#run_failing_test -s '1'
#run_passing_test -s '2'
#run_passing_test -s '3'

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
run_passing_test -s 'AB' -t '0:start_1(1:STRING(AB) 1:_tEND())'
run_failing_test -s 'A'
run_passing_test -s 'AB' -t '0:start_1(1:STRING(AB) 1:_tEND())'
run_failing_test -s 'AC'
run_passing_test -s 'ACDEB' -t '0:start_1(1:STRING(ACDEB) 1:_tEND())'
run_failing_test -s 'ABCDE'

#############################
grammar='
start := IDU;
IDU := "A{1,3}|B{4,7}Z";
'
compile_grammar "$grammar" 0
run_failing_test -s 'AZ'
run_passing_test -s 'A' -t '0:start_1(1:IDU(A) 1:_tEND())'
run_passing_test -s 'AA' -t '0:start_1(1:IDU(AA) 1:_tEND())'
run_passing_test -s 'AAA' -t '0:start_1(1:IDU(AAA) 1:_tEND())'
run_failing_test -s 'AAAA'
run_failing_test -s 'BZ'
run_failing_test -s 'BBZ'
run_failing_test -s 'BBBZ'
run_passing_test -s 'BBBBZ' -t '0:start_1(1:IDU(BBBBZ) 1:_tEND())'
run_passing_test -s 'BBBBBBBZ' -t '0:start_1(1:IDU(BBBBBBBZ) 1:_tEND())'
run_failing_test -s 'BBBBBBBBZ'

#############################
grammar='
start := IDU;
IDU := "A{1,2}Z";
'

compile_grammar "$grammar" 0
run_passing_test -s 'AZ' -t '0:start_1(1:IDU(AZ) 1:_tEND())'

#############################
grammar='
start := IDU;
IDU := "A{1,5}Z";
'

compile_grammar "$grammar" 0
run_passing_test -s 'AZ' -t '0:start_1(1:IDU(AZ) 1:_tEND())'

#############################
grammar='
start := IDU;
IDU := "A{2,3}Z";
'

compile_grammar "$grammar" 0
run_failing_test -s 'AZ'
run_passing_test -s 'AAZ' -t '0:start_1(1:IDU(AAZ) 1:_tEND())'
run_passing_test -s 'AAAZ' -t '0:start_1(1:IDU(AAAZ) 1:_tEND())'
run_failing_test -s 'AAAAZ'

#############################
grammar='
start := IDU;
IDU := "A{2,4}Z";
'

compile_grammar "$grammar" 0
run_failing_test -s 'AZ'
run_passing_test -s 'AAZ' -t '0:start_1(1:IDU(AAZ) 1:_tEND())'
run_passing_test -s 'AAAZ' -t '0:start_1(1:IDU(AAAZ) 1:_tEND())'
run_passing_test -s 'AAAAZ' -t '0:start_1(1:IDU(AAAAZ) 1:_tEND())'
run_failing_test -s 'AAAAAZ'

#############################
grammar='
start := IDU;
IDU := "A{2,7}Z";
'

compile_grammar "$grammar" 0
run_failing_test -s 'AZ'
run_passing_test -s 'AAZ' -t '0:start_1(1:IDU(AAZ) 1:_tEND())'
run_passing_test -s 'AAAZ' -t '0:start_1(1:IDU(AAAZ) 1:_tEND())'
run_passing_test -s 'AAAAAAZ' -t '0:start_1(1:IDU(AAAAAAZ) 1:_tEND())'
run_passing_test -s 'AAAAAAAZ' -t '0:start_1(1:IDU(AAAAAAAZ) 1:_tEND())'
run_failing_test -s 'AAAAAAAAZ'

#############################
grammar='
start := IDU;
IDU := "A{3,7}Z";
'

compile_grammar "$grammar" 0
run_failing_test -s 'AZ'
run_failing_test -s 'AAZ'
run_passing_test -s 'AAAZ' -t '0:start_1(1:IDU(AAAZ) 1:_tEND())'
run_passing_test -s 'AAAAAAZ' -t '0:start_1(1:IDU(AAAAAAZ) 1:_tEND())'
run_passing_test -s 'AAAAAAAZ' -t '0:start_1(1:IDU(AAAAAAAZ) 1:_tEND())'
run_failing_test -s 'AAAAAAAAZ'

#############################
grammar='
start := IDU;
IDU := "A{5,6}Z";
'

compile_grammar "$grammar" 0
run_failing_test -s 'AZ'
run_failing_test -s 'AAZ'
run_failing_test -s 'AAAZ'
run_failing_test -s 'AAAAZ'
run_passing_test -s 'AAAAAZ' -t '0:start_1(1:IDU(AAAAAZ) 1:_tEND())'
run_passing_test -s 'AAAAAAZ' -t '0:start_1(1:IDU(AAAAAAZ) 1:_tEND())'
run_failing_test -s 'AAAAAAAZ'

#############################
grammar='
start := IDU;
IDU := "A{5,9}Z";
'

compile_grammar "$grammar" 0
run_failing_test -s 'AZ'
run_failing_test -s 'AAZ'
run_failing_test -s 'AAAZ'
run_failing_test -s 'AAAAZ'
run_passing_test -s 'AAAAAZ' -t '0:start_1(1:IDU(AAAAAZ) 1:_tEND())'
run_passing_test -s 'AAAAAAZ' -t '0:start_1(1:IDU(AAAAAAZ) 1:_tEND())'
run_passing_test -s 'AAAAAAAZ' -t '0:start_1(1:IDU(AAAAAAAZ) 1:_tEND())'
run_passing_test -s 'AAAAAAAAZ' -t '0:start_1(1:IDU(AAAAAAAAZ) 1:_tEND())'
run_passing_test -s 'AAAAAAAAAZ' -t '0:start_1(1:IDU(AAAAAAAAAZ) 1:_tEND())'
run_failing_test -s 'AAAAAAAAAAZ'

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
run_passing_test -s '"xx"' -t '0:start_1(1:STRING(xx) 1:_tEND())'

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
run_passing_test -s '"xx"' -t '0:start_1(1:STRING(xx) 1:_tEND())'

#############################
grammar='
start := ID;
ID := "[A-F][a-f0-9R-V]*";
'

compile_grammar "$grammar" 0
run_passing_test -s 'A' -t '0:start_1(1:ID(A) 1:_tEND())'
run_passing_test -s 'Aa' -t '0:start_1(1:ID(Aa) 1:_tEND())'
run_passing_test -s 'A1' -t '0:start_1(1:ID(A1) 1:_tEND())'
run_passing_test -s 'Aac1efR4T' -t '0:start_1(1:ID(Aac1efR4T) 1:_tEND())'
run_failing_test -s 'Aac1efr4t'
run_failing_test -s 'AZ'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_2(3:stmts_r_1(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_2(3:stmts_r_1(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_2(3:stmts_r_1(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_2(3:stmts_r_1(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_2(3:stmts_r_1(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_2(3:stmts_r_1(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_2(3:stmts_r_1(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_2(3:stmts_r_1(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_1(3:stmts_r_2(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_2(3:stmts_r_1(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_r_2(3:stmts_r_1(4:stmt_r_1(5:stmt_1(6:HEX(A)))) 3:stmt_r_1(4:stmt_1(5:HEX(Z))))) 1:_tEND())'

enabled=1

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
run_passing_test -s 'A Z' -t '0:start_1(1:stmts_1(2:stmts_2(3:stmt_1(4:HEX(A))) 2:stmt_1(3:HEX(Z))) 1:_tEND())'

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
run_passing_test -s 'var v;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(v) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'var vz;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(vz) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'var v1;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(v1) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'var vaz;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(vaz) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'var varz;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(varz) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'var var1;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(var1) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'var val1;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(val1) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'val var1;' -t '0:start_1(1:stmts_2(2:stmt_2(3:VAL(val) 3:ID(var1) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'val val1;' -t '0:start_1(1:stmts_2(2:stmt_2(3:VAL(val) 3:ID(val1) 3:SEMI(;))) 1:_tEND())'

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
run_passing_test -s 'var v;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(v) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'var vz;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(vz) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'var vaz;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(vaz) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'var varz;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(varz) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'var var;' -t '0:start_1(1:stmts_2(2:stmt_1(3:VAR(var) 3:ID(var) 3:SEMI(;))) 1:_tEND())'

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
run_passing_test -s 'var' -t '0:start_1(1:stmt_1(2:ID(var)) 1:_tEND())'

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
run_passing_test -s 'if else' -t '0:start_1(1:stmt_2(2:IF(if) 2:ELSE(else)) 1:_tEND())'
run_passing_test -s 'if' -t '0:start_1(1:stmt_1(2:IF(if)) 1:_tEND())'

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
run_passing_test -s 'if else' -t '0:start_1(1:stmt_1(2:IF(if) 2:ELSE(else)) 1:_tEND())'
run_passing_test -s 'if' -t '0:start_1(1:stmt_2(2:IF(if)) 1:_tEND())'

#############################
grammar='
start := stmts;
stmts := stmts stmt;
stmts := stmt;

stmt := ID(I);

ID := "[A-Z]+";
ENTER_MLCOMMENT := "/\*"! [ML_COMMENT_MODE];
WS := "\s"!;

%lexer_mode ML_COMMENT_MODE;
ENTER_MLCOMMENT := "/\*"! [ML_COMMENT_MODE];
LEAVE_MLCOMMENT := "\*/"! [^];
CMT := ".*"!;
'

compile_grammar "$grammar" 0
run_passing_test -s 'ABC' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(ABC))) 1:_tEND())'
run_passing_test -s 'ABC /*xxx*/' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(ABC))) 1:_tEND())'
run_passing_test -s 'ABC /*xxx*/ DEF' -t '0:start_1(1:stmts_1(2:stmts_2(3:stmt_1(4:ID(ABC))) 2:stmt_1(3:ID(DEF))) 1:_tEND())'
run_passing_test -s 'ABC /*xxx /*yyy*/ zzz*/ DEF' -t '0:start_1(1:stmts_1(2:stmts_2(3:stmt_1(4:ID(ABC))) 2:stmt_1(3:ID(DEF))) 1:_tEND())'
run_passing_test -s 'ABC /*xxx*/ DEF /*yyy*/ GHI' -t '0:start_1(1:stmts_1(2:stmts_1(3:stmts_2(4:stmt_1(5:ID(ABC))) 3:stmt_1(4:ID(DEF))) 2:stmt_1(3:ID(GHI))) 1:_tEND())'

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
  if(v) {}
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
run_passing_test -s 'xx = 11 + 12 + 13;' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(xx) 3:EQ(=) 3:expr_1(4:expr_1(5:expr_3(6:NUM(11)) 5:PLUS(+) 5:expr_3(6:NUM(12))) 4:PLUS(+) 4:expr_3(5:NUM(13))) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'xx = 11 * 12 * 13;' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(xx) 3:EQ(=) 3:expr_2(4:expr_2(5:expr_3(6:NUM(11)) 5:STAR(*) 5:expr_3(6:NUM(12))) 4:STAR(*) 4:expr_3(5:NUM(13))) 3:SEMI(;))) 1:_tEND())'

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
run_passing_test -s 'int aa' -t '0:start_1(1:stmt_1(2:arg_1(3:type_1(4:INT_TYPE(int)) 3:ID(aa))) 1:_tEND())'

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
run_passing_test -s 'fn(){}' -t '0:start_1(1:stmt_1(2:ID(fn) 2:argsx_1(3:LBRACKET(() 3:RBRACKET())) 2:stmt_block_1(3:LCURLY({) 3:RCURLY(}))) 1:_tEND())'

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
run_passing_test -s 'attr1 struct;' -t '0:start_1(1:stmt_1(2:rStructStatement_1(3:rStructDef_1(4:rStructAttrList_1(5:ATTR1(attr1)) 4:STRUCT(struct)) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'attr1 service;' -t '0:start_1(1:stmt_2(2:rServiceStatement_1(3:rServiceDef_1(4:rStructAttrList_1(5:ATTR1(attr1)) 4:SERVICE(service)) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'struct;' -t '0:start_1(1:stmt_1(2:rStructStatement_1(3:rStructDef_1(4:rStructAttrList_0(5:_tEMPTY(rStructAttrList)) 4:STRUCT(struct)) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'service;' -t '0:start_1(1:stmt_2(2:rServiceStatement_1(3:rServiceDef_1(4:rStructAttrList_0(5:_tEMPTY(rStructAttrList)) 4:SERVICE(service)) 3:SEMI(;))) 1:_tEND())'

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
run_passing_test -s 'interface x;' -t '0:start_1(1:rStatements_2(2:rStatement_2(3:INTERFACE(interface) 3:IDENTIFIER(x) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'void service x;' -t '0:start_1(1:rStatements_2(2:rStatement_1(3:rStructAttrList_1(4:rStructAttrItem_1(5:TYPE_VOID(void))) 3:SERVICE(service) 3:IDENTIFIER(x) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'interface x;' -t '0:start_1(1:rStatements_2(2:rStatement_2(3:INTERFACE(interface) 3:IDENTIFIER(x) 3:SEMI(;))) 1:_tEND())'
run_passing_test -s 'service x;' -t '0:start_1(1:rStatements_2(2:rStatement_1(3:rStructAttrList_0(4:_tEMPTY(rStructAttrList)) 3:SERVICE(service) 3:IDENTIFIER(x) 3:SEMI(;))) 1:_tEND())'

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
run_passing_test -s 'string msgID' -t '0:start_1(1:rParameterDef_1(2:rTypeRef_1(3:rTypeRefBase_1(4:rPrimaryTypeRef_1(5:TYPE_STRING(string))) 3:rTypeRefQualifier_0(4:_tEMPTY())) 2:IDENTIFIER(msgID)) 1:_tEND())'
run_passing_test -s 'string* msgID' -t '0:start_1(1:rParameterDef_1(2:rTypeRef_1(3:rTypeRefBase_1(4:rPrimaryTypeRef_1(5:TYPE_STRING(string))) 3:rTypeRefQualifier_1(4:STAR(*))) 2:IDENTIFIER(msgID)) 1:_tEND())'
run_passing_test -s 'string ^msgID' -t '0:start_1(1:rParameterDef_2(2:rTypeRef_1(3:rTypeRefBase_1(4:rPrimaryTypeRef_1(5:TYPE_STRING(string))) 3:rTypeRefQualifier_0(4:_tEMPTY())) 2:CARET(^) 2:IDENTIFIER(msgID)) 1:_tEND())'
run_passing_test -s 'string* ^msgID' -t '0:start_1(1:rParameterDef_2(2:rTypeRef_1(3:rTypeRefBase_1(4:rPrimaryTypeRef_1(5:TYPE_STRING(string))) 3:rTypeRefQualifier_1(4:STAR(*))) 2:CARET(^) 2:IDENTIFIER(msgID)) 1:_tEND())'

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
run_passing_test -s 'namespace name;' -t '0:start_1(1:stmt_1(2:NAMESPACE(namespace) 2:ID(name) 2:SEMI(;)) 1:_tEND())'

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
run_passing_test -s 'ABC' -t '0:start_1(1:stmts_2(2:stmt_1(3:ID(ABC))) 1:_tEND())'
run_passing_test -s 'ABC GO1' -t '0:start_1(1:stmts_1(2:stmts_2(3:stmt_1(4:ID(ABC))) 2:stmt_2(3:GO1(GO1))) 1:_tEND())'
run_passing_test -s 'ABC GO1 DEF' -t '0:start_1(1:stmts_1(2:stmts_1(3:stmts_2(4:stmt_1(5:ID(ABC))) 3:stmt_2(4:GO1(GO1))) 2:stmt_1(3:ID(DEF))) 1:_tEND())'
run_passing_test -s 'ABC GO1 DEF GO2' -t '0:start_1(1:stmts_1(2:stmts_1(3:stmts_1(4:stmts_2(5:stmt_1(6:ID(ABC))) 4:stmt_2(5:GO1(GO1))) 3:stmt_1(4:ID(DEF))) 2:stmt_3(3:GO2(GO2))) 1:_tEND())'
run_passing_test -s 'ABC GO1 DEF GO2 GHI' -t '0:start_1(1:stmts_1(2:stmts_1(3:stmts_1(4:stmts_1(5:stmts_2(6:stmt_1(7:ID(ABC))) 5:stmt_2(6:GO1(GO1))) 4:stmt_1(5:ID(DEF))) 3:stmt_3(4:GO2(GO2))) 2:stmt_1(3:ID(GHI))) 1:_tEND())'
run_failing_test -s 'DEF'
run_failing_test -s 'ABC DEF'
run_failing_test -s 'ABC GO2'
run_failing_test -s 'ABC GO1 ABC'
run_passing_test -s 'ABC GO1 DEF GO2 GHI GOY DEF' -t '0:start_1(1:stmts_1(2:stmts_1(3:stmts_1(4:stmts_1(5:stmts_1(6:stmts_2(7:stmt_1(8:ID(ABC))) 6:stmt_2(7:GO1(GO1))) 5:stmt_1(6:ID(DEF))) 4:stmt_3(5:GO2(GO2))) 3:stmt_1(4:ID(GHI))) 2:stmt_1(3:ID(DEF))) 1:_tEND())'
run_passing_test -s 'ABC GO1 DEF GO2 GHI GOZ ABC' -t '0:start_1(1:stmts_1(2:stmts_1(3:stmts_1(4:stmts_1(5:stmts_1(6:stmts_2(7:stmt_1(8:ID(ABC))) 6:stmt_2(7:GO1(GO1))) 5:stmt_1(6:ID(DEF))) 4:stmt_3(5:GO2(GO2))) 3:stmt_1(4:ID(GHI))) 2:stmt_1(3:ID(ABC))) 1:_tEND())'
run_failing_test -s 'ABC GO1 DEF GO2 GHI GOZ DEF'

#############################
grammar='
%walkers CppServer;
//%walker_output CppServer text_file cpp;
%walker_traversal CppServer top_down;

start := rParameterDef;

rParameterDef := rTypeRef(t) IDENTIFIER
@CppServer %{
    unused(t);
//    std::print("rParameterDef/@CppServer\n");
    str(t, 1);
%}

rParameterDef := rTypeRef CARET IDENTIFIER;

//%type rTypeRef int;
//%type rTypeRef std::string;
%function rTypeRef CppServer::str(int x) -> std::string;
%function rTypeRef CppServer::len(int x) -> size_t;
rTypeRef := rTypeRefBase rTypeRefQualifier
@CppServer %{
//    std::print("rTypeRef/@CppServer ***\n");
%}
@CppServer::str %{
    unused(x);
//    std::print("rTypeRef/CppServer::str ***\n");
    return "";
%}
@CppServer::len %{
    unused(x);
//    std::print("rTypeRef/CppServer::len ***\n");
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
run_passing_test -s 'string msgID' -t '0:start_1(1:rParameterDef_1(2:rTypeRef_1(3:rTypeRefBase_1(4:rPrimaryTypeRef_1(5:TYPE_STRING(string))) 3:rTypeRefQualifier_0(4:_tEMPTY())) 2:IDENTIFIER(msgID)) 1:_tEND())'
run_passing_test -s 'string ^msgID' -t '0:start_1(1:rParameterDef_2(2:rTypeRef_1(3:rTypeRefBase_1(4:rPrimaryTypeRef_1(5:TYPE_STRING(string))) 3:rTypeRefQualifier_0(4:_tEMPTY())) 2:CARET(^) 2:IDENTIFIER(msgID)) 1:_tEND())'

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
run_passing_test -s '1 + 2' -t '0:start_1(1:expr_1(2:expr_5(3:NUMBER(1)) 2:PLUS(+) 2:expr_5(3:NUMBER(2))) 1:_tEND())'
run_passing_test -s '1 - 23' -t '0:start_1(1:expr_2(2:expr_5(3:NUMBER(1)) 2:MINUS(-) 2:expr_5(3:NUMBER(23))) 1:_tEND())'

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
