# tTek

A compiled, statically typed language built on LLVM. The main idea is replacing polling loops with declarative rules — you describe *when* something should happen, and the runtime handles the rest.

Built this as a personal project to experiment with LLVM and language design. Works on Windows (MSYS2/MinGW) and Linux.

## What it looks like

```ttek
init {
    mem temperature: i32 = 22;
    mem ac_on: i32 = 0;
    print("System started\n");
}

rule too_hot when temperature > 25 && ac_on == 0 {
    ac_on = 1;
    print("AC turned on\n");
}

rule cooled_down when temperature <= 22 && ac_on == 1 {
    ac_on = 0;
    print("AC turned off\n");
}

rule shutdown when temperature > 40 {
    print("Critical temp, shutting down\n");
    halt;
}
```

## Building

**Windows (MSYS2 UCRT64):**

```bash
pacman -S mingw-w64-ucrt-x86_64-llvm mingw-w64-ucrt-x86_64-gcc cmake make
```

```bash
git clone https://github.com/RobEst0906/tTek-language.git
cd tTek-language
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

**Linux (Ubuntu/Debian):**

```bash
sudo apt install build-essential cmake llvm-dev git
git clone https://github.com/RobEst0906/tTek-language.git
cd tTek-language
mkdir build && cd build
cmake ..
make
```

## Compiling a tTek file

On Windows there's a batch script:
```bat
.\compile.bat examples\smart_home.ttek
```

Or manually:
```bash
./build/ttek_compiler examples/hello.ttek
./examples/hello
```

## Language basics

**Variables** — `mem` for regular, `reg` for register-hint:
```ttek
mem counter: i32 = 0;
reg status: u8;
```

Types: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f32`, `f64`, `ptr`

**Rules:**
```ttek
rule name every "1s" when condition {
    // body
}
```
`every` and `priority` are optional. Rules run in a loop; when the condition is true, the body executes.

**Functions:**
```ttek
fn add(a: i32, b: i32): i32 {
    return a + b;
}
```

**Control flow:** `if`/`else`, `while`, `return`, `halt`, `sleep(ms)`

**Built-ins:** `print(...)` maps to `printf`, `sleep(ms)` maps to `Sleep` on Windows

## Project layout

```
src/
  lexer/       — tokenizer
  parser/      — AST and grammar
  codegen/     — LLVM IR generation and linking
examples/      — sample .ttek programs
CMakeLists.txt
compile.bat
```

## Status

v0.1 — works, but rough around the edges. Core features (rules, functions, if/while, print, sleep, halt) are stable. Some things in the spec aren't fully implemented yet (inline asm, atomic rules, trigger). Contributions welcome.