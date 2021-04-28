# Introduction

This is a single-header C++17 implementation of variant, also known as a sum type, type-safe union, and discriminated union, where the types are stored in a binary tree fashion inside a union. The space required for a variant is the variant's largest contained type plus the size of an integral type, which serves as the index and keeps track of what the variant contains. For example, in a variant with 5 types A, B, C, D, E, type C will be the root node.

| Index  | Class|
| ------------- | ------------- |
| 0 | A |
| 1 | B |
| 2 | C |
| 3 | D |
| 4 | E |

Graphic representiation of how the elements are stored in a union:

    ---Union---	<-This, instead of this-> ---Union---
    /	   \	 		        /	      \
          C (root node)			 E
	    /   \				  \
       A     D				   D
        \     \				    \
	     B     E			             C
            				      \
                                                   B
                                                    \
                                                     A
While most other implementations have O(N) depth, this version, which you may have guessed after viewing the binary tree, only has O(log N) template instantiation depth. Accessing elements, like the `get`, `get_if`, and the `get_unchecked` method shall also have O(log N) depth.
## Documentation
The code emulates much of the standard implementation's behavior, described at  [cppreference](https://en.cppreference.com/w/cpp/utility/variant). Differences are described below.


## Efficiency and Quality of Life Features:
* `get_unchecked`: To use this variant’s get function, use `szLogVar::get<I>(yourVariant)` or `szLogVar::get<T>(yourVariant)`, similar to std’s version `std::get<I>` and `std::get<T>`. Thus, to use the get_unchecked version, use `szLogVar::get_unchecked<I>` or `szLogVar::get_unchecked<T>`, where `I` and `T` are the index and type to look up respectively. `get_unchecked` does **NOT** check if the `variant` actually holds the specified `I` or `T`, so only use this if you know for certain what the variant holds. This function is `noexcept` and, because of the lack of checking, is faster than `get` and `get_if`.

* `visit`: This visit function has a `bool` template parameter called `checkIfValueless`. By default, it is true, so one can use it with the familiar syntax `szLogVar::visit( Visitor, Variants…)`. When it’s true, it checks if any of the variants in `Variants` is `valueless_by_exception`, and if any are, will throw an exception. One can set the `bool` to false by calling the visit function like so: `szLogVar::visit<false>( Visitor, Variants…)`. This proves useful if the user is certain none of the variants they want to visit will be valueless. It also uses `get_unchecked` to retrieve the types held in the variants for `Visitor`'s invocation.

**Note:** This `visit` method isn't strictly standard. When the number of variants passed into the function is 1, the complexity isn't constant time. For any nonzero number of variants, the `visit` method uses a [modified binary search](https://en.wikipedia.org/wiki/Binary_search_algorithm#Alternative_procedure), an algorithm that takes O(log N) time, to reach the correct index. This circumvents the limitations of function pointers that Michael Park describes [here](https://mpark.github.io/programming/2019/01/22/variant-visitation-v2/).

## Example

```
#include "../include/szLogVar/variant.hpp"
#include <iostream>

int main(){
    szLogVar::variant<bool, std::string> myVar{"ABC"};
    std::cout << szLogVar::get<1>(myVar) << '\n'; //prints "ABC"
    myVar = true; //Just assigned a bool to it, so no need for checking. Use szLogVar::get_unchecked
    std::cout << std::boolalpha << szLogVar::get_unchecked<bool>(myVar) << '\n'; //prints "true"
    myVar = "DEF";
    std::cout << szLogVar::get_unchecked<std::string>(myVar) << '\n'; //std::string again. Use get_unchecked<std::string>. prints "DEF"
}
```
## Benchmarks

| Test name  | szLogVar time (seconds)| std time (seconds)| Test description
| ------------- | ------------- |-|-|
[timeConvertCtor](test/CompileTimeBenchmarks/timeConvertCtor/testCompileTime.cpp)|[5.174](test/CompileTimeBenchmarks/timeConvertCtor/szLogVarConvertCtor.png)|[29.164](test/CompileTimeBenchmarks/timeConvertCtor/StdVariantConvertCtor.png)|Measures variant's converting constructor. Variant has 200 types.
[timeDefaultConstruct](test/CompileTimeBenchmarks/timeDefaultConstruct/testCompileTime.cpp)|[3.849](test/CompileTimeBenchmarks/timeDefaultConstruct/szLogVarDefaultConstruct.png)|[3.647](test/CompileTimeBenchmarks/timeDefaultConstruct/StdVariantDefaultConstruct.png)| Measures variant's default constructor. Variant has 200 types.
[timeGetIdx](test/CompileTimeBenchmarks/timeGet/timeGetIdx/testCompileTime.cpp) |[10.379](test/CompileTimeBenchmarks/timeGet/timeGetIdx/szLogVarGetIdx.png)|[43.626](test/CompileTimeBenchmarks/timeGet/timeGetIdx/StdVariantGet.png)| Measures `szLogVar::get` and `std::get`, with a specified index. Variant has 300 types.
[timeGetType](test/CompileTimeBenchmarks/timeGet/timeGetType/testCompileTime.cpp) |[15.155](test/CompileTimeBenchmarks/timeGet/timeGetType/szLogVarGetType.png)|[48.082](test/CompileTimeBenchmarks/timeGet/timeGetType/StdVariantGetType.png)| Measures `szLogVar::get` and `std::get`, with a specified type. Variant has 300 types.
[timeSmallVariants](test/CompileTimeBenchmarks/timeSmallVariants/testCompileTime.cpp) |[11.445](test/CompileTimeBenchmarks/timeSmallVariants/szLogVarSmallCt.png)|[8.138](test/CompileTimeBenchmarks/timeSmallVariants/StdVariantSmallCt.png)| Measures default construction of 1000 variants with 3 types
[timeSmallVariantsConvertCtor](test/CompileTimeBenchmarks/timeSmallVariantsConvertCtor/testCompileTime.cpp) |[12.118](test/CompileTimeBenchmarks/timeSmallVariantsConvertCtor/szLogVarSmallConvertCtor.png)|[11.828](test/CompileTimeBenchmarks/timeSmallVariantsConvertCtor/StdVariantSmallConvertCtor.png)|Measures converting construction of 1000 variants with 3 types
[timeTestVisit](test/CompileTimeBenchmarks/timeTestVisit/testCompileTime.cpp) |[12.136](test/CompileTimeBenchmarks/timeTestVisit/szLogVarVisit.png)|[21.419](test/CompileTimeBenchmarks/timeTestVisit/StdVariantVisit.png)| Measures visit, with 5 variants with 5 types each.

`szLogVar::variant` displays a noticeable improvement in compile time if the variant holds many types, though its performance is very similar to `std::variant` when the types are few.

Flags used were `-c` and `-ftime-trace`. 

Compiler: clang++ version 12.0.0

Target: x86_64-apple-darwin19.6.0

Supported compilers, as tested on [godbolt](https://godbolt.org/z/56Yerrj4f):
* x86-64 clang 5.0.0 and above, with `-std=c++17` flag
* x86-64 gcc 8.3 and above, with `-std=c++17` flag
* x64 msvc 19.24 and above, with `/std:c++17` flag