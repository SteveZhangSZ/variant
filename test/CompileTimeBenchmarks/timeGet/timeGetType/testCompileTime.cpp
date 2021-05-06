#include <cstddef>
#include <utility>
// The define is here! Define TESTMYVARIANT to test szLogVar::variant. Leave
// undefined to test std::variant
//#define TESTMYVARIANT

#include "../../includeForTest.hpp"
template <std::size_t N> struct hasInt {
    static constexpr std::size_t theN = N;
};

template <std::size_t... Idx>
void testCompileTime(std::index_sequence<Idx...>){
    using expander = bool[];
    constexpr
    #ifdef TESTMYVARIANT
    szLogVar::variant<hasInt<Idx>...> theVar;
    static_cast<void>(expander{(szLogVar::get<hasInt<Idx>>(theVar),true)...});
    #else
    std::variant<hasInt<Idx>...> theVar;
    static_cast<void>(expander{(std::get<hasInt<Idx>>(theVar),true)...});
    #endif
}

int main(){
    testCompileTime(std::make_index_sequence<300>{});
}