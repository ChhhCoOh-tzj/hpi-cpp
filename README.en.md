# HPI-cpp

## (I) Introduction

**HPI (High-precision integer library)**
Modern lightweight C++17 high-performance arbitrary-precision integer arithmetic library

This is a purely self-taught and self-developed **lightweight**, **high-precision**, **high-performance**, and memory-saving C++ big integer library. The project is completely self-taught and original; it is an independent development completed by me, **a 14-year-old junior high school student**, in about one month through four generations of updates.
You can even use it as a native C++ data type, because it overloads almost all operators, supports direct input/output with cin and cout, has sufficiently natural construction, and also comes with a large number of practical utility functions!

Development history (only the most prominent updates are listed):
- v1.0: The initial version, pure long multiplication and basic dynamic memory allocation. Operations were extremely slow (computing 2^900000 took about 15 seconds), and long division had a deadlock issue. (At that time it could only be considered a toy program.)
- v2.0: Fully switched to base 10^9 arithmetic, optimized memory allocation and release, and reduced the time to about 10 seconds.
- v3.0: Introduced `__restrict` to remove pointer aliasing restrictions, implemented raw pointer optimization, eliminated redundant branches in core loops, and reduced the time to about 2 seconds.
- v4.0: Implemented Karatsuba multiplication and Knuth Algorithm D division. Applied zero-copy slicing and fast exponentiation with modular reduction at each step; the final 2^900000 computation time stabilized at about 0.35 seconds.
- v4.1: Added a large number of utility functions covering cryptography and number theory, and added bilingual (Doxygen) comments for all functions.

## (II) Software Architecture

The underlying layer uses base 10^9 (`BASE = 1000000000`), with data stored in `std::vector<int>`. `data[0]` is the sign bit (0 positive, 1 negative), and the significant value starts from `data[1]`.
In core operation functions, `__restrict` raw pointers and local-variable register caching are frequently used, removing frequent memory reads/writes when handling carries in inner loops.
Doxygen comments have also been added to some operator overloads with complex mechanisms and to all utility functions.

**Generic Functions**
- Because generic functions are used, an HPI can not only operate with another HPI object; as long as the right-hand side of all operators is an HPI object, an HPI object can operate with native C++ data types and return a new rvalue (depending on whether it is a compound assignment operator) HPI object.

**Multiplication Design (Karatsuba and Long Multiplication)**
- Automatic algorithm selection: set 128 limbs as the threshold. Below the threshold, use long multiplication; above it, automatically enable the Karatsuba algorithm.
- Zero-copy slicing: in Karatsuba recursion, no deep copy of arrays is performed; index ranges are passed directly through pointers.
- Low-level optimization: uses `__restrict` raw pointers and local-variable register caching to remove frequent memory reads/writes when handling carries in inner loops.

**Division/Modulo Design (Knuth Algorithm D)**
- Single-limb specialization: if the divisor is a single limb, directly use a dedicated path with O(n) linear time complexity, which is extremely fast.
- Multi-limb division: uses Knuth Algorithm D with normalization, suppressing the quotient estimate error to <= 2. Directly performs in-place multiply-subtract based on raw pointers, avoiding the creation of temporary objects and secondary memory allocation.

**Fast Exponentiation and Modular Arithmetic**
- Binary fast exponentiation implementation.
- Modular exponentiation takes the modulo immediately after each multiplication (modular reduction while computing), preventing intermediate results from expanding and ensuring that memory usage remains constant when computing giant exponents.

**Memory Optimization**
- Extensively uses `assign`, `reserve`, and `clear` to reuse vector capacity, reducing the number of heap memory requests to the operating system.
- Implements the rvalue reference overload `operator-() &&` for unary minus; combined with move semantics, it achieves zero deep copy when flipping the sign of temporary objects.
- In Karatsuba recursion, no deep copy of arrays is performed; index ranges are passed directly through pointers.

**I/O Design**
- Input/construction/assignment are based on strings, and theoretically infinitely long data can be input.
- Output uses `std::setw()` and `std::setfill()` to pad zeros, so as to maintain correctness in decimal form.

**Utility Functions**
- Comes with a large number of practical utility functions, covering advanced mathematics, number theory, cryptography, arbitrary base conversion, and slice functions, using high-performance algorithms (such as Newton iteration, extended Euclidean algorithm, etc.), and has been deeply optimized.

## (III) Environment Requirements

- C++ Standard: C++17 or higher.
- Compiler: GCC 9+, Clang 10+, MSVC 2019+.
- Operating System: Cross-platform (Windows / Linux / macOS).
- Compilation Optimization: Strongly recommend enabling `-O3 -march=native`. Under GCC, it can automatically vectorize (SIMD) low-level loops, and performance will improve several times.

## (IV) Usage Tutorial

**Installation:** There is only one header file `HPI.h`. You only need to download the `HPI.h` file to your working directory, and then add a line `#include "HPI.h"` at the top of your program to start using it.

### (1) Construction
HPI supports three construction methods:
- `std::string` construction, for example `HPI a("123456");` You can also pass the address of a character array for construction, but an error will be thrown when passing a null pointer or an empty string! Something like `HPI a("00-00123456")` is also legal, and HPI will automatically remove leading zeros for you.
- Native integer construction, for example `HPI a = 987654LL;`
- Default construction is 0, for example `HPI a;`

### (2) Operations
HPI overloads almost all operators; all supported operators are listed below:
- `=` (assignment equal sign)
- `+`, `-`, `*`, `/`, `%`
- `+=`, `-=`, `*=`, `/=`, `%=`
- `-` (negation), `bool()` (so that it can directly participate in logical operations)
- `++` (prefix), `++` (postfix), `--` (prefix), `--` (postfix)
- `>`, `<`, `>=`, `<=`, `==`, `!=`

An HPI can fully operate with another HPI object, can use these operators freely like native C++ data types, and can also support chained operator calls like native C++ data types.
Moreover, as long as the right-hand side of all operators is an HPI object, HPI objects can also operate with native C++ data types and will return a new rvalue (depending on whether it is a compound assignment operator) HPI object.

Example:

```
    HPI a = 10;
    HPI b = 30;
    a += b;
    a = b * 5;
```

### (3) I/O Stream Overloading
By overloading the input stream `std::istream` and output stream `std::ostream` in `<iostream>`, HPI objects can behave completely like native C++ types (such as `int`, `double`), directly using `std::cin` and `std::cout` or standard input/output streams for I/O. You can directly use `cin >>` to read a big integer and `cout <<` to print a big integer. During input, HPI automatically handles leading zeros, signs, and validates the legality of the data, and supports chained calls.

### (4) Utility Functions
1. Convert to `long long` form: object `.toLL()`, function signature: `long long toLL() const`, obtains the lowest 18 decimal digits of the HPI object and converts them to `long long` type and returns it, for operations with native C++ data types or other purposes; higher digits will be automatically truncated.
2. Convert to `std::string` form in arbitrary base: object `.toBaseStr()`, function signature: `std::string toBaseStr(int base = 10) const`, converts the HPI object to `std::string` type in arbitrary base and returns it, for use by other libraries or GUI display; the user can specify the target base, with range 2 <= base <= 36.
3. Get the number of digits of the HPI object in decimal form: object `.getDigits()`, function signature: `ULL getDigits() const`
4. HPI object integer slice: object `. cutHPI(starting position, truncation length, whether to start from the lower bits) `, function signature: ` HPI cutHPI(ULL pos, ULL size, bool startFromLowDigit = false) const`， The index starts from 0, please refer to the Doxygen code comment for details.
5. Get absolute value: object `.abs()`, function signature: `HPI abs() const`, returns the absolute value.
6. Exponentiation: object `.pow(exponent)`, function signature: `HPI pow(long long exp)`, performs exponentiation based on the fast exponentiation algorithm.
7. Exponentiation with modulo: object `.pow(exponent, modulus)`, function signature: `HPI pow(long long exp, const HPI& mod)`, performs exponentiation + modulo based on the fast exponentiation algorithm.
8. Integer square root: object `.isqrt()`, function signature: `HPI isqrt() const`, calculated based on Newton iteration.
9. Greatest common divisor: object `.gcd(another object)`, function signature: `HPI gcd(HPI other) const`, based on the Euclidean algorithm.
10. Least common multiple: object `.lcm(another object)`, function signature: `HPI lcm(const HPI& other) const`
11. Modular inverse: object `.modInv(modulus)`, function signature: `HPI modInv(const HPI& mod) const`, based on the extended Euclidean algorithm.
12. Check even: object `.isEven()`, function signature: `bool isEven() const`, to prevent users from calling `a % 2 == 0` and causing a full division to run again.
13. Check odd: object `.isOdd()`, function signature: `bool isOdd() const`, to prevent users from calling `a % 2 == 1` and causing a full division to run again.

## (V) Performance Tests

**Test Platform**
- Computer: ASUS ZenBook 14 (2020 thin-and-light laptop)
- Processor: AMD Ryzen 7 4700U (base 2.00 GHz, L1 512KB, L2 4.0MB, L3 8.0MB)
- Memory: 16 GB
- Operating System: Windows 11 Enterprise 21H2
- Environment state: kept plugged in, performance mode enabled, background processes closed, maintained at a relatively low temperature.
- Compilation parameters: `-O3 -march=native -std=c++17`
- The test results below all take the minimum value from multiple runs.

**Addition and Subtraction Tests**
| Operation Type | Scale | Single-run Time |
| :--- | :--- | :--- |
| Addition | 9 digits | 0.167496 us |
| Addition | 1,000 limbs | 1.58479 us |
| Addition | 100,000 limbs | 290.532 us |
| Subtraction | 9 digits | 0.169739 us |
| Subtraction | 1,000 limbs | 1.04228 us |
| Subtraction | 100,000 limbs | 242.621 us |

**Multiplication Tests (based on fast exponentiation)**
- `2^900000`: time 0.1713953s (result **270,927** digits)
- `2^5000000`: time 3.4237692s (result **1,505,150** digits)
- `2^23333333`: time 34.7989968s (result **7,024,034** digits)

**Division and Modulo Tests (random 1000 digits divided by random 500 digits)**
- Division: single-run time approx. 0.0000258s ~ 0.0000376s
- Modulo: single-run time approx. 0.0000259s ~ 0.0000376s

**Exponentiation Modulo Tests**
- `2^900000 % 1000000007`: time 0.0000538s
- `2^2000000 % 1000000007`: time 0.0000273s
- `2^2000000 % random 500-digit large number`: time 0.0004516s

*(Note: All above operation results have been cross-validated against Python and verified absolutely correct)*

## (VI) AI Usage Statement

In the process of developing this project, I always uphold the transparency principle of the open-source community. I hereby solemnly declare:

**1. Originality and Independent Development**
All architecture design, code writing, low-level optimization (from v1.0 to v4.0), test case writing, and performance benchmarking of this project (HPI high-performance big integer library) were completely independently and hand-written by me.

**2. AI Usage Scenarios**
AI tools are only used for theoretical learning (understanding complex low-level computer science concepts), mathematical derivation, and code-assisted debugging, and are absolutely not used to directly generate code, except for some advanced mathematics content (I am only a junior high school student after all).