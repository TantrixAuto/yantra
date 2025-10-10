# Building the project
Yantra is a pure standalone app with no dependencies except the C++ standard libraries.

Hence building it is a very straightforward process.
### 1. Clone the repository
```
git clone git@github.com:TantrixAuto/yantra.git
git checkout develop
```

### 2. Generate the cmake build directory at any convenient location
```
mkdir build
cd build
cmake ..
```

### 3. build the project
```
cmake --build .
```

### 4. use executable `ycc` (or `ycc.exe`) from the bin directory
```
bin/ycc --help
```

### 5. generate a sample parser
```
bin/ycc -f ../samples/basic.y -a
```

### 6. observe that two files were generated:
```
basic.cpp
basic.log
```

### 7. build the generated parser
```
clang++ --std=c++20 basic.cpp
```

### 8. run the parser
```
./a.out --help
```

### 9. Run the parser, for C++ generation
```
./a.out -s "z = a::b::C;" -w CppWalker
```

Observe the following output
```
z = a::b::C
```

### 10. Run the parser, for Java generation
```
./a.out -s "z = a::b::C;" -w JavaWalker
```

Observe the following output
```
z = a.b.C
```

## Finally
Read the [Quickstart](060_quickstart.md) document for a gentle introduction to the content of the basic.y file.
