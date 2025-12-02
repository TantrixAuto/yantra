#!/bin/bash

./bin/ycc -f ../../recipes/external-separate/grammar.y
clang++ -g -Wall --std=c++20 -I . grammar.cpp ../../recipes/external-separate/main.cpp
echo -n "run external-separate(2 + 1 * 3)..."
routput=$(./a.out "2 + 1 * 3")
if [ "$routput" == "Result(external-separate):5" ]; then
    echo "PASS"
else
    echo "FAIL"
fi

./bin/ycc -f ../../recipes/external-amalgamated/grammar.y -a
clang++ -g -Wall --std=c++20 -I . grammar.cpp ../../recipes/external-amalgamated/compiler_impl.cpp 
echo -n "run external-amalgamated(2 + 1 * 3)..."
routput=$(./a.out -s "2 + 1 * 3")
if [ "$routput" == "Result(external-amalgamated):5" ]; then
    echo "PASS"
else
    echo "FAIL"
fi

./bin/ycc -f ../../recipes/inline-amalgamated/grammar.y -a
clang++ -g -Wall --std=c++20 grammar.cpp
echo -n "run inline-amalgamated(2 * 3 + 1)..."
routput=$(./a.out -s "2 * 3 + 1")
if [ "$routput" == "Result(inline-amalgamated): 7" ]; then
    echo "PASS"
else
    echo "FAIL"
fi

./bin/ycc -f ../../recipes/inline-separate/grammar.y
clang++ -g -Wall --std=c++20 -I . grammar.cpp ../../recipes/inline-separate/main.cpp
echo -n "run inline-separate(2 + 1 * 3)..."
routput=$(./a.out "2 + 1 * 3")
if [ "$routput" == "Result(inline-separate): 5" ]; then
    echo "PASS"
else
    echo "FAIL"
fi

./bin/ycc -f ../../recipes/multiple-walkers/grammar.y -a
clang++ -g -Wall --std=c++20 grammar.cpp
echo -n "run multiple-walkers(a = a::b;)..."
routput=$(./a.out -s "a = a::b;")
if [ "$routput" == "a = a->b" ]; then
    echo "PASS"
else
    echo "FAIL"
fi

echo -n "run multiple-walkers:CppWalker(a = a::b;)..."
routput=$(./a.out -s "a = a::b;" -w CppWalker)
if [ "$routput" == "a = a->b" ]; then
    echo "PASS"
else
    echo "FAIL"
fi

echo -n "run multiple-walkers:JavaWalker(a = a::b;)..."
routput=$(./a.out -s "a = a::b;" -w JavaWalker)
if [ "$routput" == "a = a.b" ]; then
    echo "PASS"
else
    echo "FAIL"
fi
