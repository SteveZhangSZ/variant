#include <cstddef>
#include <utility>
// The define is here! Define TESTMYVARIANT to test szLogVar::variant. Leave
// undefined to test std::variant
#define TESTMYVARIANT

#include "../includeForTest.hpp"
template <std::size_t N> struct hasInt {
    static constexpr std::size_t theN = N;
};

template <std::size_t... Idx>
void testCompileTime(std::index_sequence<Idx...>){
    using expander = bool[];
    #ifdef TESTMYVARIANT
    szLogVar::visit([](auto... args){}, szLogVar::variant<hasInt<Idx>...>{hasInt<Idx>{}}...);
    #else
    std::visit([](auto... args){}, std::variant<hasInt<Idx>...>{hasInt<Idx>{}}...);
    #endif
}

int main(){
    testCompileTime(std::make_index_sequence<5>{});
}