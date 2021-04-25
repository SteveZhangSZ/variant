#include <cstddef>
#include <utility>
// The define is here! Define TESTMYVARIANT to test szLogVar::variant. Leave
// undefined to test std::variant
//#define TESTMYVARIANT

#include "../includeForTest.hpp"
template <std::size_t N> struct hasInt {};

template <std::size_t... Idx>
void testCompileTime(std::index_sequence<Idx...>){
    using expander = bool[];
    #ifdef TESTMYVARIANT
    static_cast<void>(expander{(szLogVar::variant<hasInt<Idx>,hasInt<Idx+1>,hasInt<Idx+2>>{
        hasInt<Idx + (Idx % 3)>{}
    },true)...});
    #else
    static_cast<void>(expander{(std::variant<hasInt<Idx>,hasInt<Idx+1>,hasInt<Idx+2>>{
        hasInt<Idx + (Idx % 3)>{}
    },true)...});
    #endif
}

int main(){
    testCompileTime(std::make_index_sequence<1000>{});
}