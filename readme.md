# Small Pascal-like Compiler

A small Pascal-like compiler written in C. The code is first scanned using a lexer, which tokenizes the input. The tokens are then parsed into a symbol table. During parsing the code generator translates the code into GNU assembly code. The generated assembly code is then passed to the assembler and linker to produce an executable. It parses source files, generates x86-64 assembly, and invokes the assembler and linker. It was done as a final project for the course "Compiler Design" at the American University of Armenia.


## Features

- Functions and procedures with parameters.
- Global and function-local variables; 
- `integer` and `string` declarations.
- Integer expressions using literals, variables, `+`, `-`, `*`, `/`, parentheses, and standard precedence.
- Integer-returning function calls inside expressions (arguments can also be expressions).
- Assignments and `write` of expressions.
- Function results are assigned to the function name, for example `sum := a + b`.
- String literals can be assigned to string variables.

Function parameters of the same type are grouped, for example `x, y: integer`. The global `var` section and at least one function parameter group are required by the current parser.

## Build and run

```sh
make
./compiler tests/arithmetic_precedence.pas
./arithmetic_precedence
```

The compiler writes an assembly file and executable named after the input file in the current directory.