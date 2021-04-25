#include "../include/szLogVar/variant.hpp"

#include <iostream>
#include <string>
struct printsCtor {
  printsCtor() { std::cout << "Default\n"; }
  printsCtor(const std::string &&) { std::cout << "const std::string&&\n"; }
  printsCtor(std::string &&) { std::cout << "std::string&&\n"; }
  printsCtor(const std::string &) { std::cout << "const std::string&\n"; }
  printsCtor(std::string &) { std::cout << "std::string&\n"; }
  ~printsCtor() {
    // std::cout << "Destructor of printsCtor\n";
  }
};

struct Noisy {
  Noisy(int, int) {}
  Noisy() { std::cout << "Noisy default\n"; }
  Noisy(const Noisy &) { std::cout << "Noisy copy ctor\n"; }
  Noisy(Noisy &&) { std::cout << "Noisy move ctor\n"; }
  Noisy &operator=(const Noisy &) {
    std::cout << "Noisy copy assign\n";
    return *this;
  }

  Noisy &operator=(Noisy &&) {
    std::cout << "Noisy move assign\n";
    return *this;
  }
  ~Noisy() { std::cout << "Noisy destructor\n"; }
};

void testReturnType() {
  {
    std::cout << "~~~testReturnType()~~~\n";
    szLogVar::variant<std::string, std::string, std::string> theVar{
        szLogVar::in_place_index_t<2>{}};
    const decltype(theVar) other{szLogVar::in_place_index_t<0>{}};

    printsCtor{szLogVar::get_unchecked<2>(theVar)};
    printsCtor{szLogVar::get_unchecked<0>(other)};
    printsCtor{
        szLogVar::get<0>(decltype(theVar){szLogVar::in_place_index_t<0>{}})};
    printsCtor{szLogVar::get<0>(const_cast<const decltype(theVar) &&>(
        decltype(theVar){szLogVar::in_place_index_t<0>{}}))};
  }
  szLogVar::variant<std::string> otherVar;
  const decltype(otherVar) theConstVar;
  printsCtor{szLogVar::get_unchecked<std::string>(otherVar)};
  printsCtor{szLogVar::get_unchecked<std::string>(theConstVar)};
  printsCtor{szLogVar::get<std::string>(decltype(otherVar){})};
  printsCtor{szLogVar::get<std::string>(
      const_cast<const decltype(otherVar) &&>(decltype(otherVar){}))};

  std::cout << "End testReturnType()\n";
}

constexpr int giveNum() {
  constexpr szLogVar::variant<int, char> simpleVar{11};
  return *szLogVar::get_if<int>(&simpleVar) + szLogVar::get<int>(simpleVar) +
         szLogVar::get_unchecked<0>(simpleVar);
}

void testSwap() {
  std::cout << "~~~testSwap()~~~\n";
  szLogVar::variant<bool, std::string> hasOne("One"),
      hasTwo{std::string{"Two"}}, startsWithBool{true};
  std::cout << szLogVar::get<std::string>(hasOne)
            << szLogVar::get<std::string>(hasTwo) << '\n';
  hasOne.swap(hasTwo);
  std::cout << szLogVar::get_unchecked<std::string>(hasOne)
            << szLogVar::get_unchecked<std::string>(hasTwo) << '\n';

  hasOne.swap(startsWithBool);
  std::cout << szLogVar::get<bool>(hasOne) << ' '
            << szLogVar::get<std::string>(startsWithBool) << '\n';
  std::cout << "End testSwap()\n";
}

struct hasInitList {
  std::size_t successConstruct = 123;
  template <class T> constexpr hasInitList(std::initializer_list<T>) {}
};

void testEmplace() {
  std::cout << "~~~testEmplace()~~~\n";
  szLogVar::variant<bool, std::string, hasInitList> theVar;
  std::cout << theVar.emplace<1>(3, 'D') << ' ' << theVar.emplace<bool>(true)
            << '\n';
  std::cout << theVar.emplace<hasInitList>({1, 2, 3}).successConstruct << '\n';
  std::cout << "End testEmplace()\n";
}

void testAssign() {
  std::cout << "~~~testAssign()~~~\n";
  szLogVar::variant<bool, std::string> theVar{"Text\n"},
      toCopy{"toCopy Text\n"};

  std::cout << szLogVar::get<std::string>(
      theVar = szLogVar::variant<bool, std::string>{"ThisString\n"});
  std::cout << szLogVar::get<std::string>(theVar = toCopy);
  std::cout << szLogVar::get<std::string>(theVar = "Something Else\n");
  std::cout << std::boolalpha << szLogVar::get<bool>(theVar = true) << '\n';

  szLogVar::variant<int> theIntVar{1};
  std::cout << szLogVar::get_unchecked<0>(theIntVar =
                                              szLogVar::variant<int>{123})
            << '\n';

  std::cout << "End testAssign()\n";
}

void testVariantIndices() {
  std::cout << "~~~testVariantIndices()~~~\n";
  std::cout << szLogVar::variant<float, long, double>{0}.index() << ' '
            << // Should be 1
      szLogVar::variant<std::string, const char *>{"ABC"}.index() << ' '
            << // Should be 1
      szLogVar::variant<std::string, bool>("ABC").index() << ' '
            <<                                               // Should be 0
      szLogVar::variant<std::string>("test").index() << ' '; // Should be 0

  szLogVar::variant<std::string, bool> assignWithString{true};
  std::cout << assignWithString.index() << ' '; // Should be 1
  assignWithString = "ABC";
  std::cout << assignWithString.index() << '\n'; // Should be 0
  std::cout << "End testVariantIndices()\n";
}

void testCopyMoveCtor() {
  std::cout << "~~~testCopyMoveCtor()~~~\n";
  szLogVar::variant<Noisy> theFirstNoisy;
  szLogVar::variant<Noisy> copied{theFirstNoisy};
  szLogVar::variant<Noisy> theMoved{
      static_cast<szLogVar::variant<Noisy> &&>(copied)};
  theMoved = theFirstNoisy;
  theMoved = static_cast<szLogVar::variant<Noisy> &&>(theFirstNoisy);
}

void testVisit() {
  std::cout << "~~~testVisit()~~~\n";
  constexpr szLogVar::variant<int, int, int, int> theFirst{
      szLogVar::in_place_index_t<0>{}, 1},
      theSecond{szLogVar::in_place_index_t<2>{}, 2},
      theThird{szLogVar::in_place_index_t<3>{}, 3};

  
  std::cout << szLogVar::visit(
      [](auto &&...args) { 
        ((std::cout << args << ' '), ...) << '\n'; 
        return (args + ...);
      },
      theFirst, theSecond, theThird) << " End testVisit()\n";
}

struct testCopyAssign {
  testCopyAssign() = default;
  testCopyAssign(const testCopyAssign &) noexcept(false) {}
  testCopyAssign(testCopyAssign &&) = default;
  testCopyAssign &operator=(const testCopyAssign &) = default;
  testCopyAssign &operator=(testCopyAssign &&) { return *this; }
};

void testHash() {
  std::cout << "~~~testHash()~~~\n";
  szLogVar::variant<bool, std::string> hasString{"QWERTY\n"};
  std::cout << "Hash: "
            << std::hash<szLogVar::variant<bool, std::string>>{}(hasString)
            << '\n';
  std::cout << "End testHash()\n";

  std::cout << std::boolalpha << (hasString == decltype(hasString){"QWERTY\n"})
            << '\n';

  // std::cout << hasString.index() << '\n';

  decltype(hasString) copyOfTheString{
      static_cast<szLogVar::variant<bool, std::string> &&>(hasString)};
  std::cout << szLogVar::get<0>(szLogVar::variant<std::string>("Test "))
            << szLogVar::get<std::string>(copyOfTheString);
}
//Want to see the output to ensure everything's working correctly
int main() {
  static_cast<void>(
      szLogVar::variant<std::string, std::string, std::string, std::string,
                        std::string, std::string, std::string, std::string>{});

  std::cout << "Begin\n";
  testReturnType();
  testSwap();
  testAssign();
  testEmplace();
  testCopyMoveCtor();
  testVisit();
  testVariantIndices();
  testHash();
  std::cout << "giveNum(): " << giveNum() << '\n';

  {
    szLogVar::variant<testCopyAssign, testCopyAssign> theVar{
        szLogVar::in_place_index_t<1>{}},
        otherVar;
    std::cout << "theVar.index(): " << theVar.index()
              << " otherVar.index(): " << otherVar.index() << '\n';
    theVar = otherVar;
    std::cout << "theVar.index(): " << theVar.index()
              << " otherVar.index(): " << otherVar.index() << '\n';
  }
  // std::variant<std::string, std::string> w("abc"); //Ill-formed, as expected
}