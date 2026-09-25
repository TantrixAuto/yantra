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

## Finally
Follow the [Tutorial](../tutorial/) for a hands-on, step-by-step walkthrough of generating, building and running a parser with `ycc` -- it's the maintained, working equivalent of the sample-parser walkthrough that used to live here. Read the [Quickstart](060_quickstart.md) document first for a gentle introduction to the structure of a grammar file.
