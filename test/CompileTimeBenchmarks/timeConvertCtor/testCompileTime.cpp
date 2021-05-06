#include <cstddef>
#include <utility>
// The define is here! Define TESTMYVARIANT to test szLogVar::variant. Leave
// undefined to test std::variant
//#define TESTMYVARIANT

#include "../includeForTest.hpp"
template <std::size_t N> struct hasInt {};

template <std::size_t... Idx>
void testCompileTime(std::index_sequence<Idx...>){
    #ifdef TESTMYVARIANT
    (static_cast<void>( szLogVar::variant<hasInt<Idx>...>{hasInt<Idx>{}} ),...);
    #else
    (static_cast<void>( std::variant<hasInt<Idx>...>{hasInt<Idx>{}} ),...);
    #endif
}

int main(){
    testCompileTime(std::make_index_sequence<200>{});
}