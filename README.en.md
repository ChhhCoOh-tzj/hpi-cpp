# HPI-cpp

## (1) Introduction

**HPI (High-precision integer library)**
A modern, lightweight C++17 high-performance arbitrary-precision integer arithmetic library

This is a purely self-taught and self-developed **lightweight**, **high-precision**, **high-performance**, and memory-efficient C++ big integer library. The project is entirely original and self-taught, independently developed by me, **a 14-year-old junior high school student**, over the course of about a month through four generations of updates.
You can even use it just like a native C++ data type because it overloads almost all operators, supports direct input/output via cin and cout, has natural construction methods, and comes with many practical utility functions!

Development History (only listing the most prominent updates):
- v1.0: The initial version, with pure schoolbook multiplication and basic dynamic memory allocation. Extremely slow (~15 seconds for 2^900000) and had deadlock issues during long division. (Could only be considered a toy program at that time)
- v2.0: Fully switched to base 10^9 arithmetic, optimized memory allocation and deallocation, reducing the time to ~10 seconds.
- v3.0: Introduced `__restrict` to remove pointer aliasing restrictions, implemented bare pointer optimizations, and eliminated redundant branches in core loops, reducing the time to ~2 seconds.
- v4.0 (Current Version): Implemented Karatsuba multiplication and Knuth Algorithm D division. Applied zero-copy slicing and fast exponentiation with continuous modular reduction, finally stabilizing the computation time for 2^900000 at around 0.35 seconds.

## (2) Software Architecture

The underlying implementation uses base 10^9 (`BASE = 1000000000`), with data stored in a `std::vector<int>`. `data[0]` holds the sign bit (0 for positive, 1 for negative), and the valid numerical limbs start from `data[1]`.
Core arithmetic functions frequently utilize `__restrict` bare pointers and register caching for local variables, eliminating frequent memory reads/writes during carry propagation in inner loops.
Doxygen comments have also been added to functions or overloads with complex mechanisms.

**Generic Functions**
- Because generic functions are adopted, an HPI can not only operate with another HPI object, but as long as the right side of all operators is ensured to be an HPI object, an HPI object can operate with native C++ data types and return a new rvalue (depending on whether it is a compound assignment operator) HPI object.

**Multiplication Design (Karatsuba & Schoolbook)**
- Automatic algorithm dispatch: A threshold of 128 limbs is set. Below the threshold, it uses schoolbook multiplication; above, it automatically enables the Karatsuba algorithm.
- Zero-copy slicing: Karatsuba recursion avoids deep copying of arrays, directly passing index ranges via pointers.
- Low-level optimization: Utilizes `__restrict` bare pointers and register caching for local variables, eliminating frequent memory reads/writes during carry propagation in inner loops.

**Division/Modulo Design (Knuth Algorithm D)**
- Single-limb specialization: If the divisor is a single limb, it uses a specialized channel with O(n) linear time complexity, which is extremely fast.
- Multi-limb division: Uses Knuth's Algorithm D with normalization, compressing the quotient estimation error to <= 2. Performs in-place multiply-subtract directly on bare pointers, avoiding the creation of temporary objects and secondary memory allocation.

**Fast Exponentiation & Modular Arithmetic**
- Binary fast exponentiation implementation.
- Modular exponentiation takes the modulo immediately after each multiplication (continuous modular reduction), preventing intermediate results from expanding and ensuring constant memory usage when computing massive exponents.

**Memory Optimization**
- Extensive use of `assign`, `reserve`, and `clear` to reuse vector capacities, reducing the number of heap memory allocation calls to the operating system.
- Implemented an rvalue reference overload `operator-() &&` for the unary minus. Combined with move semantics, it achieves zero deep copy when flipping the sign of temporary objects.
- Karatsuba recursion avoids deep copying of arrays, directly passing index ranges via pointers.

**I/O Design**
- Input/construction/assignment are based on strings, allowing theoretically infinitely long data input.
- Output uses `std::setw()` and `std::setfill()` functions to pad with zeros, maintaining the correct decimal format.

## (3) Requirements

- C++ Standard: C++17 or higher.
- Compilers: GCC 9+, Clang 10+, MSVC 2019+.
- Operating Systems: Cross-platform (Windows / Linux / macOS).
- Compiler Optimization: Strongly recommend enabling `-O3 -march=native`. Under GCC, it automatically vectorizes (SIMD) core loops, providing a several-fold performance boost.

## (4) Usage Tutorial

**Installation:** There is only one header file, `HPI.h`. You just need to download the `HPI.h` file to your working directory, and then add `#include "HPI.h"` at the top of your program to start using it.

### (1) Construction
HPI supports three construction methods:
- Construction from `std::string`, e.g., `HPI a("123456");`. You can also pass the address of a character array for construction, but an error will be thrown if a null pointer or an empty string is passed! `HPI a("00-00123456")` is also valid; HPI will automatically remove leading zeros for you.
- Construction from native integers, e.g., `HPI a = 987654LL;`.
- Default construction to 0, e.g., `HPI a;`.

### (2) Operations
HPI overloads almost all operators. The supported operators are listed below:
- `=` (Assignment)
- `+`, `-`, `*`, `/`, `%`
- `+=`, `-=`, `*=`, `/=`, `%=`
- `-` (Unary minus), `bool()` (Allows it to participate directly in logical operations)
- `++` (Prefix), `++` (Postfix), `--` (Prefix), `--` (Postfix)
- `>`, `<`, `>=`, `<=`, `==`, `!=`

An HPI can fully operate with another HPI object, freely using these operators just like native C++ data types, and it also supports chained operator calls just like native C++ data types.
Not only that, as long as the right side of all operators is ensured to be an HPI object, an HPI object can also operate with native C++ data types and return a new rvalue (depending on whether it is a compound assignment operator) HPI object.

Example:

```
    HPI a = 10;
    HPI b = 30;
    a += b;
    a = b * 5;
```

### (3) I/O Stream Overloading
By overloading the input stream `std::istream` and output stream `std::ostream` from `<iostream>`, HPI objects can be input/output directly using `std::cin` and `std::cout` or standard input/output streams just like native C++ types (e.g., `int`, `double`). You can directly use `cin >>` to read big integers and `cout <<` to print big integers. During input, HPI automatically handles leading zeros, signs, and validates data legality, and supports chained calls.

### (4) Utility Functions
1. Get `long long` form: object `.getLL()`, function signature: `long long getLL()`. Gets the lowest 18 decimal digits of the HPI object, converts them to `long long` and returns it, for operations with native C++ data types or other uses. Higher digits will be automatically truncated.
2. Get `std::string` form: object `.getStr()`, function signature: `std::string getStr()`. Converts the HPI object to `std::string` and returns it, for use with other libraries or GUI display.
3. Exponentiation: object `.pow(exponent)`, function signature: `HPI pow(long long exp)`, performs exponentiation based on the fast exponentiation algorithm.
4. Modular exponentiation: object `.pow(exponent, modulus)`, function signature: `HPI pow(long long exp, const HPI& mod)`, performs exponentiation + modulo based on the fast exponentiation algorithm.
5. Absolute value: object `.abs()`, function signature: `HPI abs()`, returns the absolute value.

## (5) Performance Benchmark

**Test Platform**
- Computer: ASUS ZenBook 14 (2020 lightweight laptop)
- Processor: AMD Ryzen 7 4700U (Base 2.00 GHz, L1 512KB, L2 4.0MB, L3 8.0MB)
- Memory: 16 GB
- OS: Windows 11 Enterprise 21H2
- Environment State: Plugged in, performance mode enabled, background processes closed, kept at a low temperature.
- Compilation Flags: `-O3 -march=native -std=c++17`
- The following test results are the minimum values from multiple runs.

**Addition & Subtraction Test**
| Operation Type | Size | Time per operation |
| :--- | :--- | :--- |
| Addition | 9 digits | 0.167496 us |
| Addition | 1,000 limbs | 1.58479 us |
| Addition | 100,000 limbs | 290.532 us |
| Subtraction | 9 digits | 0.169739 us |
| Subtraction | 1,000 limbs | 1.04228 us |
| Subtraction | 100,000 limbs | 242.621 us |

**Multiplication Test (based on fast exponentiation)**
- `2^900000`: 0.1713953s (Result: **270,927** digits)
- `2^5000000`: 3.4237692s (Result: **1,505,150** digits)
- `2^23333333`: 34.7989968s (Result: **7,024,034** digits)

**Division & Modulo Test (random 1000 digits divided by random 500 digits)**
- Division: ~ 0.0000258s ~ 0.0000376s per operation
- Modulo: ~ 0.0000259s ~ 0.0000376s per operation

**Modular Exponentiation Test**
- `2^900000 % 1000000007`: 0.0000538s
- `2^2000000 % 1000000007`: 0.0000273s
- `2^2000000 % random 500-digit number`: 0.0004516s

*(Note: All the above computation results have been cross-verified against Python and confirmed absolutely correct)*

## (6) AI Usage Declaration

Throughout the development of this project, I consistently uphold the transparency principles of the open-source community. I hereby declare:

**1. Originality and Independent Development**
All architectural design, coding, low-level optimization (from v1.0 to v4.0), test case writing, and performance benchmarking of this project (HPI high-performance big integer library) were **completely independently and hand-written by myself**.

**2. Use Cases of AI**
AI tools were only used for theoretical learning (understanding complex low-level computer science concepts), mathematical derivation, and assisted code debugging, and were **strictly not used to directly generate code**.