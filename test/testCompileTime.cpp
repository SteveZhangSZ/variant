#include <cstddef>
#include <utility>
// The define is here! Define TESTMYVARIANT to test szLogVar::variant. Leave
// undefined to test std::variant
#define TESTMYVARIANT

#ifdef TESTMYVARIANT
#include "../include/szLogVar/variant.hpp"
#else
#include <variant>
#endif
template <std::size_t N> struct hasInt {};

template <std::size_t... Idx>
void testCompileTime(std::index_sequence<Idx...>) {
// using expander = bool[];
#ifdef TESTMYVARIANT
  typedef szLogVar::variant<hasInt<Idx>...> theVar;
  static_cast<void>(theVar{});

  //((static_cast<void>(theVar{hasInt<Idx>{}})),...); //tests efficiency of the
  //(T&& t) constructor

  // static_cast<void>(expander{(
  // (static_cast<void>(theVar{hasInt<Idx>{}})),true )...});

  //[[maybe_unused]] constexpr bool theBools[]{(
  //(static_cast<void>(theVar{hasInt<Idx>{}})),true )...};

#else
  typedef std::variant<hasInt<Idx>...> theVar;
  static_cast<void>(theVar{});
//[[maybe_unused]] constexpr bool theBools[]{(
//(static_cast<void>(theVar{hasInt<Idx>{}})),true )...};
#endif
}
int main() {
  // Was 3000. Feel free to change it
  testCompileTime(std::make_index_sequence<3000>{});
  /*Takes szLogVar::variant about 30 seconds to construct
  szLogVar::variant<hasInt<0>...hasInt<2999>> Implementation defined, but this
  usually exceeds the default max template instantiation limit on other variants
  */
}