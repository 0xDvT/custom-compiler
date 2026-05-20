[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/vjPNwvCX)
# Programming Assignment: Extend the Compiler with Arithmetic Expressions

## Overview

You already have a working compiler for a small Pascal-like language with:

- global variables
- functions
- function parameters
- local variables
- return statements
- assignments
- hopefully `write(...)`

In this assignment, you will extend that compiler so that it supports full arithmetic expressions everywhere integer values are expected.

Your compiler must correctly handle arithmetic expressions involving:

- global integer variables
- local integer variables
- function parameters
- integer literals
- function calls
- parentheses
- operator precedence

---

## Goal

Extend the existing compiler so that expressions such as these work:

```pascal
x := a + b * 2;
y := (x + 10) / 3;
z := add(x, y) * 4;
return localValue + globalValue * parameterValue;
````

Arithmetic expressions must work both inside the main program body and inside functions.

---

## Language Grammar

```ebnf
prg              = prgHeader varDefs { funcDef } "begin" statementSeq "end" "."
prgHeader        = "program" identifier ";"

varDefs          = [ "var" varSeq { varSeq } ]
varSeq           = identifier { "," identifier } ":" type ";"
type             = "integer" | "string"

funcDef          = "function" identifier "(" [ paramSeq ] ")" ":" type ";"
                   varDefs
                   "begin"
                   statementSeq
                   returnStmt
                   "end" ";"

paramSeq         = identifier ":" type { "," identifier ":" type }

statementSeq     = { statement ";" }
statement        = assignment | funcCall | writeCall

assignment       = identifier ":=" expression
writeCall        = "write" "(" expression ")"
returnStmt       = "return" expression ";"

expression       = term { addOp term }
term             = factor { mulOp factor }
factor           = number
                 | identifier
                 | funcCall
                 | "(" expression ")"

funcCall         = identifier "(" [ argumentSeq ] ")"
argumentSeq      = expression { "," expression }

addOp            = "+" | "-"
mulOp            = "*" | "/"

identifier       = letter { letter | digit }
number           = digit { digit }
letter           = "a"..."z" | "A"..."Z"
digit            = "0"..."9"
```

---

## Required Features

### 1. Arithmetic Expressions

Your compiler must support:

```pascal
a + b
a - b
a * b
a / b
(a + b) * c
a + b * c
(a + b) / (c - d)
```

Operator precedence must be correct:

```pascal
a + b * c
```

must be interpreted as:

```pascal
a + (b * c)
```

Parentheses must override precedence.

---

### 2. Global Variables in Expressions

Global variables must be usable in expressions:

```pascal
program Test;
var
  a, b, result: integer;

begin
  a := 10;
  b := 20;
  result := a + b * 2;
  write(result);
end.
```

Expected output:

```text
50
```

---

### 3. Local Variables in Expressions

Local variables inside functions must also work:

```pascal
function compute(x: integer): integer;
var
  temp: integer;
begin
  temp := x * 2;
  return temp + 5;
end;
```

---

### 4. Function Parameters in Expressions

Function parameters must be usable like normal variables:

```pascal
function addScaled(a: integer, b: integer): integer;
begin
  return a + b * 10;
end;
```

---

### 5. Function Calls in Expressions

Function calls must be usable inside arithmetic expressions:

```pascal
result := add(a, b) * multiply(a, b);
```

Function call arguments may also be expressions:

```pascal
result := add(a + 1, b * 2);
```

---

### 6. Return Expressions

Return statements must support full expressions:

```pascal
return (a + b) * 2;
```

not only simple identifiers or numbers.

---

## Example Program

```pascal
program ExpressionFunctionsTest;

var
  a, b, c, result: integer;

function add(x: integer, y: integer): integer;
begin
  return x + y;
end;

function multiply(x: integer, y: integer): integer;
begin
  return x * y;
end;

function calculate(p: integer): integer;
var
  localValue: integer;
begin
  localValue := p + 5;
  return localValue * 2 + a;
end;

begin
  a := 10;
  b := 20;
  c := 3;

  result := add(a, b) * c + calculate(5);

  write(result);
end.
```

Expected result:

```text
120
```

Explanation:

```text
add(a, b) = 30
30 * c = 90
calculate(5) = (5 + 5) * 2 + 10 = 30
result = 90 + 30 = 120
```

---

## Implementation Requirements

You must update:

1. the parser
2. the symbol table, if needed
3. the code generator
4. function call handling
5. return statement handling

Your expression parser should preferably use recursive descent:

```text
parseExpression
parseTerm
parseFactor
```

This structure naturally supports operator precedence.

---

## Type Rules

For this assignment:

* arithmetic expressions are allowed only for `integer`
* `+`, `-`, `*`, `/` work only with integers
* string variables may still be assigned string literals if your compiler already supports this
* string concatenation is not required
* function calls used in arithmetic expressions must return `integer`

Invalid examples:

```pascal
message := a + b;
result := "hello" + "world";
result := stringFunction() + 5;
```

Your compiler should either reject these or clearly report an error.

---

## Submission Instructions

Submit:

* compiler source code
* Makefile
* at least 3 test programs

The Makefile must support:

```sh
make
```

and the compiler must be runnable as:

```sh
./yourcompiler source.pas
```

It should generate:

```sh
source.s
```

Then the assembly should be linked into an executable by your compiler.

---

## Suggested Test Cases

### Test 1: Global expressions

```pascal
program TestGlobal;
var
  a, b, c, result: integer;
begin
  a := 2;
  b := 3;
  c := 4;
  result := a + b * c;
  write(result);
end.
```

Expected output:

```text
14
```

### Test 2: Local variables and parameters

```pascal
program TestLocal;

var
  result: integer;

function calc(x: integer, y: integer): integer;
var
  temp: integer;
begin
  temp := x * y;
  return temp + x;
end;

begin
  result := calc(5, 6);
  write(result);
end.
```

Expected output:

```text
35
```

### Test 3: Nested function calls

```pascal
program TestNested;

var
  result: integer;

function add(a: integer, b: integer): integer;
begin
  return a + b;
end;

function mul(a: integer, b: integer): integer;
begin
  return a * b;
end;

begin
  result := mul(add(2, 3), add(4, 1));
  write(result);
end.
```

Expected output:

```text
25
```

---

## Grading

* Correct parsing of expressions: 25%
* Correct operator precedence: 20%
* Correct handling of globals, locals, and parameters: 20%
* Correct function calls inside expressions: 15%
* Correct return expressions: 10%
* Tests and Makefile: 10%

```

