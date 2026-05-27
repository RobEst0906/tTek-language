# tTek Language (v0.1)

tTek is a low-level reactive programming language that compiles directly to native machine code via LLVM.
It replaces traditional polling loops with declarative rules (`rule ... when ...`), making it suitable for embedded systems, real-time control, drivers, and performance-critical applications.

## Repository

[https://github.com/RobEst0906/tTek-language](https://github.com/RobEst0906/tTek-language)

## Why tTek?

- Reactive rules – describe *when* something should happen, not how to poll for it.
- Low-level control – manual memory management, pointers, bitwise operations.
- No intermediate C – the LLVM backend generates native object files, linked directly into an executable.
- Static typing with explicit sizes (`i32`, `u64`, `f32`, `ptr`).
- Minimal, unique syntax – familiar keywords but a fresh, declarative flow.

## Is tTek an esoteric language?

No. tTek is a practical, compiled language designed for systems programming. Its rule-based model comes from real-world paradigms like event-driven and reactive systems (similar to Esterel or synchronous languages), but applied to low-level code. The syntax may look unusual compared to C, but it is not a joke or a puzzle – it compiles to efficient native code through LLVM and gives the programmer full control over memory and execution.

## Features

- Global and local variables (`mem`, `reg`)
- `init` block – runs once at startup
- Rules with `when`, `every`, `priority`, `atomic`
- Functions (`fn`) with parameters and return types
- Expressions: arithmetic, comparisons, logical (`and`/`or`/`not`), function calls
- Statements: `if`/`else`, `while`, `return`, `halt`, `nop`, `trigger`, inline assembly (`asm`)
- Automatic mapping of `print` to `printf`
- Cross-platform compilation (via LLVM)

## Dependencies (to build the compiler)

- A C++17 compiler (GCC 9+, Clang 7+, or MSVC 2019+)
- LLVM 21 (including development headers and libraries)
- CMake 3.16 or newer
- (Optional) MinGW-w64 or MSYS2 UCRT64 environment on Windows

On MSYS2 UCRT64 you can install everything with:

```bash
pacman -S mingw-w64-ucrt-x86_64-llvm mingw-w64-ucrt-x86_64-gcc cmake make
Building the tTek compiler
Clone the repository.

Create a build directory and run CMake:

bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
(On Linux/macOS use cmake .. && make)

The executable ttek_compiler (or ttek_compiler.exe) will be placed in the build folder.

Compiling and running a tTek program
Use the compiler directly:

bash
./build/ttek_compiler examples/hello.ttek
This produces an object file (.o) and automatically links it into an executable (.exe on Windows). Then run the executable:

bash
./examples/hello.exe
Alternatively, use the provided compile.bat (Windows) or a simple shell script to compile and run in one step:

bash
compile.bat examples/smart_home.ttek
Syntax overview (v0.1)
Hello World
 
init {
    print("Hello World\n");
}
Variables and types
 
mem counter: i32 = 0;
reg status: u8;
ptr buffer: u8 = null;
Types: u8, u16, u32, u64, i8, i16, i32, i64, f32, f64, ptr, arrays (type[size]), void.

Rules

rule tick every "1s" when counter < 100 {
    counter = counter + 1;
    print("Tick: %d\n", counter);
}

rule alarm when counter >= 100 {
    print("Alarm!\n");
    halt;
}
Rules support optional priority, atomic, and every (time interval). Conditions may use any boolean expression. The runtime evaluates rules continuously; when a condition becomes true, the body executes.

Functions
 
fn add(a: i32, b: i32): i32 {
    return a + b;
}
Control flow
if/else, while, return, halt, nop, trigger, inline assembly (asm { ... }).

Full example: Smart Home simulation
See examples/smart_home.ttek for a realistic reactive system with multiple rules, timers, and state management.

Project structure
 
tTek/
├── CMakeLists.txt          # Build system configuration
├── compile.bat             # Convenience script (Windows)
├── README.md
├── examples/               # tTek source files
│   ├── hello.ttek
│   ├── smart_home.ttek
│   └── etc
└── src/                    # Compiler source code
    ├── main.cpp            # Entry point
    ├── lexer/              # Lexer (Token.hpp, Lexer.hpp, Lexer.cpp)
    ├── parser/             # Parser and AST (AST.hpp, Parser.hpp, Parser.cpp)
    └── codegen/            # LLVM code generator (CodeGenLLVM.hpp, CodeGenLLVM.cpp)
lexer/ – Breaks input   into tokens.

parser/ – Builds an abstract syntax tree (AST) according to the tTek grammar.

codegen/ – Walks the AST and generates LLVM IR, then compiles it to an object file and links it.

Status
Version 0.1 – fully working compiler, stable syntax. The language is capable of expressing reactive, low-level programs with direct memory access and real-time constraints. Contributions and feedback are welcome.

## Installation & Usage

### Windows (MinGW-w64 / MSYS2)

1. Install MSYS2 from https://www.msys2.org/ and open **MSYS2 UCRT64** terminal.
2. Install required packages:

pacman -S mingw-w64-ucrt-x86_64-llvm mingw-w64-ucrt-x86_64-gcc cmake make git

Clone the repository:
    
git clone https://github.com/RobEst0906/tTek-language.git
cd tTek-language
Build and run a tTek program (one command):

powershell:
.\compile.bat examples\smart_home.ttek

The batch file will create a build folder, compile the compiler (if needed), translate your .ttek to an executable, and run it immediately.

To compile manually without the script:

powershell:
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
cd ..
.\build\ttek_compiler.exe examples\hello.ttek


Linux
Install dependencies (example for Ubuntu/Debian):

sudo apt update
sudo apt install build-essential cmake llvm-21-dev clang git
(On other distributions, use the appropriate package manager and ensure LLVM 21 development libraries are present.)

Clone and build:

git clone https://github.com/RobEst0906/tTek-language.git
cd tTek-language
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
cd ..
Compile and run a tTek program:

./build/ttek_compiler examples/hello.ttek
./examples/hello
(The compiler generates an object file and automatically links it with gcc. If you prefer clang, edit the link command in src/codegen/CodeGenLLVM.cpp.)