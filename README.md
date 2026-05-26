LLVM Pass Development
=====================

Example LLVM passes - based on **LLVM 19**

**LLVM Pass Development** is a collection of self-contained LLVM analysis and
transformation passes implemented in C++ targeting LLVM 19. Key features:

* **Out-of-tree** - builds against a binary LLVM installation (no need to build LLVM from sources)
* **Complete** - includes `CMake` build scripts, LIT tests, and documentation
* **Modern** - based on the latest version of LLVM

### Overview
LLVM implements a very rich, powerful and popular API. This project demonstrates
a range of self-contained, testable LLVM passes implemented using idiomatic LLVM.

This document explains how to set-up your environment, build and run the
examples, and go about debugging. It contains a high-level overview of the
implemented passes and background information on writing LLVM passes.

### Table of Contents
* [HelloWorld: Your First Pass](#helloworld-your-first-pass)
* Part 1: **LLVM Pass Development** in more detail
  * [Development Environment](#development-environment)
  * [Building & Testing](#building--testing)
  * [Overview of the Passes](#overview-of-the-passes)
  * [Debugging](#debugging)
* Part 2: Passes In LLVM
  * [Analysis vs Transformation Pass](#analysis-vs-transformation-pass)
  * [Dynamic vs Static Plugins](#dynamic-vs-static-plugins)
  * [Optimisation Passes Inside LLVM](#optimisation-passes-inside-llvm)
* [References](#references)


HelloWorld: Your First Pass
===========================
The **HelloWorld** pass is a self-contained *reference example*.

For every function defined in the input module, **HelloWorld** prints its name
and the number of arguments that it takes. You can build it like this:

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
mkdir build
cd build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR <source/dir>/HelloWorld/
make
```

Before you can test it, you need to prepare an input file:

```bash
# Generate an LLVM test file
$LLVM_DIR/bin/clang -O1 -S -emit-llvm <source/dir>/inputs/input_for_hello.c -o input_for_hello.ll
```

Finally, run **HelloWorld** with
[**opt**](http://llvm.org/docs/CommandGuide/opt.html) (use `libHelloWorld.so`
on Linux and `libHelloWorld.dylib` on Mac OS):

```bash
# Run the pass
$LLVM_DIR/bin/opt -load-pass-plugin ./libHelloWorld.{so|dylib} -passes=hello-world -disable-output input_for_hello.ll
# Expected output
(llvm-pass-dev) Hello from: foo
(llvm-pass-dev)   number of arguments: 1
(llvm-pass-dev) Hello from: bar
(llvm-pass-dev)   number of arguments: 2
(llvm-pass-dev) Hello from: fez
(llvm-pass-dev)   number of arguments: 3
(llvm-pass-dev) Hello from: main
(llvm-pass-dev)   number of arguments: 2
```

The **HelloWorld** pass doesn't modify the input module. The `-disable-output`
flag is used to prevent **opt** from printing the output bitcode file.

Development Environment
=======================
## Platform Support And Requirements
This project has been tested on **Ubuntu 22.04** and **Mac OS X 11.7**. In
order to build **LLVM Pass Development** you will need:
  * LLVM 19
  * C++ compiler that supports C++17
  * CMake 3.20 or higher

In order to run the passes, you will need:
  * **clang-19** (to generate input LLVM files)
  * [**opt**](http://llvm.org/docs/CommandGuide/opt.html) (to run the passes)

There are additional requirements for tests (these will be satisfied by
installing LLVM 19):
  * [**lit**](https://llvm.org/docs/CommandGuide/lit.html) (aka **llvm-lit**,
    LLVM tool for executing the tests)
  * [**FileCheck**](https://llvm.org/docs/CommandGuide/FileCheck.html) (LIT
    requirement, it's used to check whether tests generate the expected output)

## Installing LLVM 19 on Mac OS X
On Darwin you can install LLVM 19 with [Homebrew](https://brew.sh/):

```bash
brew install llvm@19
```

If you already have an older version of LLVM installed, you can upgrade it to
LLVM 19 like this:

```bash
brew upgrade llvm
```

Once the installation (or upgrade) is complete, all the required header files,
libraries and tools will be located in `/opt/homebrew/opt/llvm/`.

## Installing LLVM 19 on Ubuntu
On Ubuntu Jammy Jellyfish, you can install modern LLVM from the official
[repository](http://apt.llvm.org/):

```bash
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
sudo apt-add-repository "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-19 main"
sudo apt-get update
sudo apt-get install -y llvm-19 llvm-19-dev llvm-19-tools clang-19
```
This will install all the required header files, libraries and tools in
`/usr/lib/llvm-19/`.

## Building LLVM 19 From Sources
Building from sources can be slow and tricky to debug. It is not necessary, but
might be your preferred way of obtaining LLVM 19. The following steps will work
on Linux and Mac OS X:

```bash
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout release/19.x
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD=host -DLLVM_ENABLE_PROJECTS=clang <llvm-project/root/dir>/llvm/
cmake --build .
```
For more details read the [official documentation](https://llvm.org/docs/CMake.html).

Building & Testing
===================
## Building
You can build **LLVM Pass Development** (and all the provided pass plugins) as follows:

```bash
cd <build/dir>
cmake -DLT_LLVM_INSTALL_DIR=<installation/dir/of/llvm/19> <source/dir>
make
```

The `LT_LLVM_INSTALL_DIR` variable should be set to the root of either the
installation or build directory of LLVM 19. It is used to locate the
corresponding `LLVMConfig.cmake` script that is used to set the include and
library paths.

## Testing
In order to run **LLVM Pass Development** tests, you need to install **llvm-lit** (aka
**lit**). It's not bundled with LLVM 19 packages, but you can install it with
**pip**:

```bash
# Install lit - note that this installs lit globally
pip install lit
```
Running the tests is as simple as:

```bash
$ lit <build_dir>/test
```
Voilà! You should see all tests passing.

## LLVM Plugins as shared objects
In **LLVM Pass Development** every LLVM pass is implemented in a separate shared object.
These shared objects are essentially dynamically loadable plugins for **opt**.
All plugins are built in the `<build/dir>/lib` directory.

Note that the extension of dynamically loaded shared objects differs between
Linux and Mac OS. For example, for the **HelloWorld** pass you will get:

* `libHelloWorld.so` on Linux
* `libHelloWorld.dylib` on MacOS.

For the sake of consistency, in this README.md file all examples use the `*.so`
extension. When working on Mac OS, use `*.dylib` instead.

Overview of The Passes
======================
The available passes are categorised as either Analysis, Transformation or CFG.
The difference between Analysis and Transformation passes is rather
self-explanatory ([here](#analysis-vs-transformation-pass) is a more technical
breakdown). A CFG pass is simply a Transformation pass that modifies the Control
Flow Graph.

In the following table the passes are grouped thematically and ordered by the
level of complexity.

| Name      | Description     | Category |
|-----------|-----------------|------|
|[**HelloWorld**](#helloworld-your-first-pass) | visits all functions and prints their names | Analysis |
|[**OpcodeCounter**](#opcodecounter) | prints a summary of LLVM IR opcodes in the input module | Analysis |
|[**InjectFuncCall**](#injectfunccall) | instruments the input module by inserting calls to `printf` | Transformation |
|[**StaticCallCounter**](#staticcallcounter) | counts direct function calls at compile-time (static analysis) | Analysis |
|[**DynamicCallCounter**](#dynamiccallcounter) | counts direct function calls at run-time (dynamic analysis) | Transformation |
|[**MBASub**](#mbasub) | obfuscate integer `sub` instructions | Transformation |
|[**MBAAdd**](#mbaadd) | obfuscate 8-bit integer `add` instructions | Transformation |
|[**FindFCmpEq**](#findfcmpeq) | finds floating-point equality comparisons | Analysis |
|[**ConvertFCmpEq**](#convertfcmpeq) | converts direct floating-point equality comparisons to difference comparisons | Transformation |
|[**RIV**](#riv) | finds reachable integer values for each basic block | Analysis |
|[**DuplicateBB**](#duplicatebb) | duplicates basic blocks, requires **RIV** analysis results | CFG |
|[**MergeBB**](#mergebb) | merges duplicated basic blocks | CFG |

Once you've [built](#building--testing) this project, you can experiment with
every pass separately. All passes, except for
[**HelloWorld**](#helloworld-your-first-pass), are described in more detail below.

LLVM passes work with LLVM IR files. You can generate one like this:

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
# Textual form
$LLVM_DIR/bin/clang -O1 -emit-llvm input.c -S -o out.ll
# Binary/bit-code form
$LLVM_DIR/bin/clang -O1 -emit-llvm input.c -c -o out.bc
```

Note that `clang` adds the `optnone` [function
attribute](https://llvm.org/docs/LangRef.html#function-attributes) if either

* no optimization level is specified, or
* `-O0` is specified.

If you want to compile at `-O0`, you need to specify `-O0 -Xclang
-disable-O0-optnone` or define a static
[isRequired](https://llvm.org/docs/WritingAnLLVMNewPMPass.html#required-passes)
method in your pass. Alternatively, you can specify `-O1` or higher.

## OpcodeCounter
**OpcodeCounter** is an Analysis pass that prints a summary of the [LLVM IR
opcodes](https://github.com/llvm/llvm-project/blob/release/19.x/llvm/lib/IR/Instruction.cpp#L397-L480)
encountered in every function in the input module.

### Run the pass

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
# Generate an LLVM file to analyze
$LLVM_DIR/bin/clang -emit-llvm -c <source_dir>/inputs/input_for_cc.c -o input_for_cc.bc
# Run the pass through opt
$LLVM_DIR/bin/opt -load-pass-plugin <build_dir>/lib/libOpcodeCounter.so --passes="print<opcode-counter>" -disable-output input_for_cc.bc
```

For `main`, **OpcodeCounter** prints the following summary:

```
=================================================
LLVM-PASS-DEV: OpcodeCounter results for `main`
=================================================
OPCODE               #N TIMES USED
-------------------------------------------------
load                 2
br                   4
icmp                 1
add                  1
ret                  1
alloca               2
store                4
call                 4
-------------------------------------------------
```

### Auto-registration with optimisation pipelines

```bash
$LLVM_DIR/bin/opt -load-pass-plugin <build_dir>/lib/libOpcodeCounter.so --passes='default<O1>' input_for_cc.bc
```

## InjectFuncCall
This pass is a _HelloWorld_ example for _code instrumentation_. For every function
defined in the input module, **InjectFuncCall** will add (_inject_) the following
call to [`printf`](https://en.cppreference.com/w/cpp/io/c/fprintf):

```C
printf("(llvm-pass-dev) Hello from: %s\n(llvm-pass-dev)   number of arguments: %d\n", FuncName, FuncNumArgs)
```

### Run the pass

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -O0 -emit-llvm -c <source_dir>/inputs/input_for_hello.c -o input_for_hello.bc
$LLVM_DIR/bin/opt -load-pass-plugin <build_dir>/lib/libInjectFuncCall.so --passes="inject-func-call" input_for_hello.bc -o instrumented.bin
```

```
$LLVM_DIR/bin/lli instrumented.bin
(llvm-pass-dev) Hello from: main
(llvm-pass-dev)   number of arguments: 2
(llvm-pass-dev) Hello from: foo
(llvm-pass-dev)   number of arguments: 1
(llvm-pass-dev) Hello from: bar
(llvm-pass-dev)   number of arguments: 2
```

## StaticCallCounter
The **StaticCallCounter** pass counts the number of _static_ function calls in
the input LLVM module at compile-time.

### Run the pass through **opt**

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -emit-llvm -c <source_dir>/inputs/input_for_cc.c -o input_for_cc.bc
$LLVM_DIR/bin/opt opt -load-pass-plugin <build_dir>/lib/libStaticCallCounter.so -passes="print<static-cc>" -disable-output input_for_cc.bc
```

```
=================================================
LLVM-PASS-DEV: static analysis results
=================================================
NAME                 #N DIRECT CALLS
-------------------------------------------------
foo                  3
bar                  2
fez                  1
-------------------------------------------------
```

### Run the pass through `static`

```bash
<build_dir>/bin/static input_for_cc.bc
```

## DynamicCallCounter
The **DynamicCallCounter** pass counts the number of _run-time_ function calls
by inserting call-counting instructions that execute every time a function is called.

### Run the pass

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -emit-llvm -c <source_dir>/inputs/input_for_cc.c -o input_for_cc.bc
$LLVM_DIR/bin/opt -load-pass-plugin=<build_dir>/lib/libDynamicCallCounter.so -passes="dynamic-cc" input_for_cc.bc -o instrumented_bin
$LLVM_DIR/bin/lli ./instrumented_bin
```

```
=================================================
LLVM-PASS-DEV: dynamic analysis results
=================================================
NAME                 #N DIRECT CALLS
-------------------------------------------------
foo                  13
bar                  2
fez                  1
main                 1
```

## Mixed Boolean Arithmetic Transformations
These passes implement [mixed boolean arithmetic](https://tel.archives-ouvertes.fr/tel-01623849/document)
transformations often used in code obfuscation.

### MBASub
The **MBASub** pass implements this expression:

```
a - b == (a + ~b) + 1
```

#### Run the pass

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -emit-llvm -S <source_dir>/inputs/input_for_mba_sub.c -o input_for_sub.ll
$LLVM_DIR/bin/opt -load-pass-plugin=<build_dir>/lib/libMBASub.so -passes="mba-sub" -S input_for_sub.ll -o out.ll
```

### MBAAdd
The **MBAAdd** pass implements a formula valid for 8-bit integers:

```
a + b == (((a ^ b) + 2 * (a & b)) * 39 + 23) * 151 + 111
```

#### Run the pass

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -O1 -emit-llvm -S <source_dir>/inputs/input_for_mba.c -o input_for_mba.ll
$LLVM_DIR/bin/opt -load-pass-plugin=<build_dir>/lib/libMBAAdd.so -passes="mba-add" -S input_for_mba.ll -o out.ll
```

## RIV
**RIV** is an analysis pass that for each basic block BB in the input function
computes the set of reachable integer values.

### Run the pass

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_riv.c -o input_for_riv.ll
$LLVM_DIR/bin/opt -load-pass-plugin <build_dir>/lib/libRIV.so -passes="print<riv>" -disable-output input_for_riv.ll
```

## DuplicateBB
This pass duplicates all basic blocks in a module that have reachable integer
values (identified through the **RIV** pass).

### Run the pass

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_duplicate_bb.c -o input_for_duplicate_bb.ll
$LLVM_DIR/bin/opt -load-pass-plugin <build_dir>/lib/libRIV.so -load-pass-plugin <build_dir>/lib/libDuplicateBB.so -passes=duplicate-bb -S input_for_duplicate_bb.ll -o duplicate.ll
```

## MergeBB
**MergeBB** merges qualifying basic blocks that are identical, partially
reverting the transformations introduced by **DuplicateBB**.

### Run the pass

```bash
$LLVM_DIR/bin/opt -load <build_dir>/lib/libMergeBB.so -legacy-merge-bb -S foo.ll -o merge.ll
```

## FindFCmpEq
The **FindFCmpEq** pass finds all floating-point comparison operations that
directly check for equality between two values.

### Run the pass

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -emit-llvm -S -Xclang -disable-O0-optnone -c <source_dir>/inputs/input_for_fcmp_eq.c -o input_for_fcmp_eq.ll
$LLVM_DIR/bin/opt --load-pass-plugin <build_dir>/lib/libFindFCmpEq.so --passes="print<find-fcmp-eq>" -disable-output input_for_fcmp_eq.ll
```

## ConvertFCmpEq
The **ConvertFCmpEq** pass converts direct floating-point equality comparisons
into logically equivalent ones that use a pre-calculated rounding threshold.

### Run the pass

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -emit-llvm -S -Xclang -disable-O0-optnone \
  -c <source_dir>/inputs/input_for_fcmp_eq.c -o input_for_fcmp_eq.ll
$LLVM_DIR/bin/opt --load-pass-plugin <build_dir>/lib/libFindFCmpEq.so \
  --load-pass-plugin <build_dir>/lib/libConvertFCmpEq.so \
  --passes=convert-fcmp-eq -S input_for_fcmp_eq.ll -o fcmp_eq_after_conversion.ll
```

Debugging
==========
Before running a debugger, you may want to analyze the output from
[LLVM_DEBUG](http://llvm.org/docs/ProgrammersManual.html#the-llvm-debug-macro-and-debug-option)
and
[STATISTIC](http://llvm.org/docs/ProgrammersManual.html#the-statistic-class-stats-option)
macros. For example, for **MBAAdd**:

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_mba.c -o input_for_mba.ll
$LLVM_DIR/bin/opt -S -load-pass-plugin <build_dir>/lib/libMBAAdd.so -passes=mba-add input_for_mba.ll -debug-only=mba-add -stats -o out.ll
```

Note that for these macros to work you need a debug build of LLVM (i.e. **opt**)
and **LLVM Pass Development** (i.e. use `-DCMAKE_BUILD_TYPE=Debug` instead of
`-DCMAKE_BUILD_TYPE=Release`).

## Mac OS X

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_mba.c -o input_for_mba.ll
lldb -- $LLVM_DIR/bin/opt -S -load-pass-plugin <build_dir>/lib/libMBAAdd.dylib -passes=mba-add input_for_mba.ll -o out.ll
(lldb) breakpoint set --name MBAAdd::run
(lldb) process launch
```

## Ubuntu

```bash
export LLVM_DIR=<installation/dir/of/llvm/19>
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_mba.c -o input_for_mba.ll
gdb --args $LLVM_DIR/bin/opt -S -load-pass-plugin <build_dir>/lib/libMBAAdd.so -passes=mba-add input_for_mba.ll -o out.ll
(gdb) b MBAAdd.cpp:MBAAdd::run
(gdb) r
```

Analysis vs Transformation Pass
===============================
The implementation of a pass depends on whether it is an Analysis or a
Transformation pass:

* a transformation pass will normally inherit from [PassInfoMixin](https://github.com/llvm/llvm-project/blob/release/19.x/llvm/include/llvm/IR/PassManager.h#L371),
* an analysis pass will inherit from [AnalysisInfoMixin](https://github.com/llvm/llvm-project/blob/release/19.x/llvm/include/llvm/IR/PassManager.h#L394).

Within **LLVM Pass Development** the following passes can be used as reference Analysis
and Transformation examples:

* [**OpcodeCounter**](#opcodecounter) - analysis pass
* [**MBASub**](#mbasub) - transformation pass

### Printing passes for the new pass manager
A printing pass for an Analysis pass is basically a Transformation pass that
requests the results of the analysis and prints them. There's a convention to
register such passes under the `print<analysis-pass-name>` command line option.

Dynamic vs Static Plugins
=========================
By default, all examples in **LLVM Pass Development** are built as dynamic plugins.
LLVM provides infrastructure for both _dynamic_ and _static_ plugins
([documentation](https://llvm.org/docs/WritingAnLLVMPass.html#building-pass-plugins)).

Static plugins are simply libraries linked into your executable (e.g. **opt**)
statically and don't require `-load-pass-plugin` at runtime.

```bash
export LLVM_PASS_DEV_DIR=<source/dir>
export LLVM_PROJECT_DIR=<llvm-project/dir>
export LLVM_BUILD_DIR=<build/dir>

cd $LLVM_PASS_DEV_DIR
bash utils/static_registration.sh --llvm_project_dir $LLVM_PROJECT_DIR
cd $LLVM_BUILD_DIR
cmake -DLLVM_BUILD_EXAMPLES=On -DLLVM_MBASUB_LINK_INTO_TOOLS=On .
cmake --build . --target opt
```

Once **opt** is re-built, you can run **MBASub** without `-load-pass-plugin`:

```bash
$LLVM_BUILD_DIR/bin/opt --passes=mba-sub -S $LLVM_PASS_DEV_DIR/test/MBA_sub.ll
```

Optimisation Passes Inside LLVM
=================================
Apart from writing your own passes, you may want to familiarize yourself with
[the passes available within LLVM](https://llvm.org/docs/Passes.html).

| Name | Description |
|------|-------------|
|[**dce**](https://github.com/llvm/llvm-project/blob/release/19.x/llvm/lib/Transforms/Scalar/DCE.cpp) | Dead Code Elimination |
|[**memcpyopt**](https://github.com/llvm/llvm-project/blob/release/19.x/llvm/lib/Transforms/Scalar/MemCpyOptimizer.cpp) | Optimise calls to `memcpy` |
|[**reassociate**](https://github.com/llvm/llvm-project/blob/release/19.x/llvm/lib/Transforms/Scalar/Reassociate.cpp) | Reassociate expressions to enable further optimisations |
|[**always-inline**](https://github.com/llvm/llvm-project/blob/release/19.x/llvm/lib/Transforms/IPO/AlwaysInliner.cpp) | Always inlines functions decorated with `alwaysinline` |
|[**loop-deletion**](https://github.com/llvm/llvm-project/blob/release/19.x/llvm/lib/Transforms/Scalar/LoopDeletion.cpp) | Delete unused loops |
|[**licm**](https://github.com/llvm/llvm-project/blob/release/19.x/llvm/lib/Transforms/Scalar/LICM.cpp) | Loop-Invariant Code Motion |
|[**slp**](https://github.com/llvm/llvm-project/blob/release/19.x/llvm/lib/Transforms/Vectorize/SLPVectorizer.cpp) | Superword-level parallelism vectorisation |

References
===========
* **LLVM IR**
  * _"LLVM IR Tutorial - Phis, GEPs and other things, oh my!"_, V.Bridgers, F. Piovezan, EuroLLVM ([slides](https://llvm.org/devmtg/2019-04/slides/Tutorial-Bridgers-LLVM_IR_tutorial.pdf), [video](https://www.youtube.com/watch?v=m8G_S5LwlTo&feature=youtu.be))
  * _"Mapping High Level Constructs to LLVM IR"_, M. Rodler ([link](https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/))
* **LLVM Pass Development**
  * _"Writing an LLVM Optimization"_, Jonathan Smith [video](https://www.youtube.com/watch?v=MagR2KY8MQI&t)
  * _"Getting Started With LLVM: Basics"_, J. Paquette, F. Hahn, LLVM Dev Meeting 2019 [video](https://www.youtube.com/watch?v=3QQuhL-dSys&t=826s)
  * _"Writing an LLVM Pass: 101"_, A. Warzyński, LLVM Dev Meeting 2019 [video](https://www.youtube.com/watch?v=ar7cJl2aBuU)
  * _"Writing LLVM Pass in 2018"_, Min-Yih Hsu [blog](https://medium.com/@mshockwave/writing-llvm-pass-in-2018-preface-6b90fa67ae82)
* **LLVM Based Tools Development**
  * _"Introduction to LLVM"_, M. Shah, Fosdem 2018 [link](http://www.mshah.io/fosdem18.html)
  * _"Building an LLVM-based tool. Lessons learned"_, A. Denisov [blog](https://lowlevelbits.org/building-an-llvm-based-tool.-lessons-learned/)

License
========
The MIT License (MIT)

Copyright (c) 2019 Andrzej Warzyński

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
