calc: calc.cpp
	clang++ --std=c++20  -o $@ $<

calc.cpp: calc.yantra
	ycc -f $< -a

.PHONY: clean
clean:
	rm -f calc.log calc.cpp calc