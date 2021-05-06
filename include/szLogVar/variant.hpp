#ifndef SZVARIANT
#define SZVARIANT

#include <cstdint>          //for std::size_t and Fixed width integer types
#include <exception>        //for std::exception to make bad_variant_access
#include <functional>       //std::hash
#include <initializer_list> //needed for some function overloads
#include <new>              //placement new
#include <type_traits>      //need lots of traits

#ifdef _MSC_VER
#define SZ_VAR_CPPVERSION _MSVC_LANG
#else
#define SZ_VAR_CPPVERSION __cplusplus
#endif

#if __has_include(<compare>) && SZ_VAR_CPPVERSION > 201703L
#include <compare> //For operator <=>
#endif

#ifdef __has_builtin
  #if defined(__clang__) && __has_builtin(__type_pack_element)
  #define SZ_VAR_HASTYPEPACKELEM

  #define SZ_VAR_GETIDXFROMTYPE(THEIDX, THETYPES)                                \
    __type_pack_element<THEIDX, THETYPES>
  #endif
#endif
#ifndef SZ_VAR_HASTYPEPACKELEM

  #if SZ_VAR_CPPVERSION <= 201703L || !defined(__clang__)

  #define SZ_VAR_GETIDXFROMTYPE(THEIDX, THETYPES)                                \
    typename decltype(variantImpl::getBaseClass<THEIDX>(                         \
        static_cast<variantImpl::allIdxAndTypes<THETYPES> *>(0)))::type
  #else
  #define SZ_VAR_GETIDXFROMTYPE(THEIDX, THETYPES) \
  typename decltype(variantImpl::getBaseClass<THEIDX>(static_cast<variantImpl::variantCpp20Impl<variantImpl::theIndexSeq<sizeof...(THETYPES)>,THETYPES> *>(0)))::type
  #endif

#endif

namespace szLogVar {
struct monostate {};
constexpr bool operator==(monostate, monostate) noexcept { return true; }

#if __has_include(<compare>) && SZ_VAR_CPPVERSION > 201703L
constexpr std::strong_ordering operator<=>(monostate, monostate) noexcept { return std::strong_ordering::equal; }
#else
constexpr bool operator!=(monostate, monostate) noexcept { return false; }
constexpr bool operator<(monostate, monostate) noexcept { return false; }
constexpr bool operator>(monostate, monostate) noexcept { return false; }
constexpr bool operator<=(monostate, monostate) noexcept { return true; }
constexpr bool operator>=(monostate, monostate) noexcept { return true; }
#endif

template <class T> struct in_place_type_t {
  explicit constexpr in_place_type_t() = default;
};
template <class T> inline constexpr in_place_type_t<T> in_place_type{};
template <std::size_t I> struct in_place_index_t {
  static constexpr std::size_t theIdx = I;
  explicit constexpr in_place_index_t() = default;
};
template <std::size_t I> inline constexpr in_place_index_t<I> in_place_index{};

struct bad_variant_access : public std::exception {
  bad_variant_access() = default;
  bad_variant_access(const bad_variant_access &other) = default;
  bad_variant_access &operator=(const bad_variant_access &other) = default;
  virtual const char *what() const noexcept {
    return "Accessed inactive variant member";
  }
};
namespace szHelpMethods {
template <bool> struct hasBool;

template <class T, class>
T getTypeMethod(hasBool<true> *); // Replacement for std::conditional
template <class, class U> U getTypeMethod(hasBool<false> *);

template <class> struct hasType;
template <class T> T removeCVRef_notVoid(hasType<T &> *);

template <class T>
using aliasForCVRef =
    decltype(removeCVRef_notVoid(static_cast<hasType<T &> *>(0)));
// Doesn't work with void or C-style arrays

template <class, class> struct twoTypes;
template <class T> std::size_t isSame(twoTypes<T, T> *);
bool isSame(void *);

template <bool, bool>
struct twoBools; // Used to determine which in_place constructor to use
// template<(Number < middleTypeIdx), (Number > middleTypeIdx)>
//<true,false> go to left node. <false,true>, go to right. <false,false>
// construct theT at that node

template <class T>
using expander = T[]; // Alternative to fold expressions to avoid nesting limit

template <class T> using arrayOfOne = T[1]; // for getRightType's theMethod

template <class TypeInVar, std::size_t N> struct getRightType {
  typedef TypeInVar type;
  constexpr static std::size_t theIdx = N;

  #if SZ_VAR_CPPVERSION <= 201703L
  template <class U>
  constexpr szLogVar::in_place_index_t<N> theMethod(
      TypeInVar, // Makes it so (char const&)[N] better match for const char*
                 // than std::string
      U &&param,
        // Check if U is a bool.
      hasBool<
          
          (sizeof(isSame(
               static_cast<twoTypes<aliasForCVRef<TypeInVar>, bool> *>(0))) ==
           sizeof(std::size_t))> *,
      decltype(sizeof(arrayOfOne<TypeInVar>{
          static_cast<U &&>(param)})) = 0) const;
  #else
  template <class U>
  static
  constexpr szLogVar::in_place_index_t<N> theMethod(TypeInVar, U &&param,
  hasBool<(sizeof(isSame(
               static_cast<twoTypes<aliasForCVRef<TypeInVar>, bool> *>(0))) ==
           sizeof(std::size_t))> *
  ) requires requires{ arrayOfOne<TypeInVar>{static_cast<U &&>(param)}; };

  #endif
};

template <class T> T &&myDeclval(int);
template <class T> T myDeclval(long);

// Used for SFINAE and to stop the (T&&) ctor from being too greedy
template <class T> void isAVariant(twoTypes<T, T> *);
std::size_t isAVariant(...);

//Check if type is void. function type with cv-qualifier or ref-qualifier can't be referenceable either, but this
//is only used for visit(), to check if the Visitor's return type is void.
template<class T, class U = T&> bool isVoid(int);
template<class T> std::size_t isVoid(long);

#ifdef _MSC_VER
template <class T> constexpr void destroyCurrent(const T &param) {
  param.~T(); // Used for variant's destructor, destroys the contained object
}
#endif
} // namespace szHelpMethods

namespace otherIdxSeq { // Inspired by https://stackoverflow.com/a/32223343
template <std::size_t... Nums> struct index_sequence;
#if !(defined(_MSC_VER) || __GNUC__ >= 8)
template <std::size_t... I1, std::size_t... I2>
index_sequence<I1..., (sizeof...(I1) + I2)...> *
myMergeRenumber(index_sequence<I1...> *, index_sequence<I2...> *);

template <std::size_t N> struct hasInt;

index_sequence<> *myMakeIdxSeq(hasInt<0> *, hasInt<0> *);
index_sequence<0> *myMakeIdxSeq(hasInt<1> *, hasInt<0> *);

template <std::size_t N>
decltype(myMergeRenumber(
    myMakeIdxSeq(static_cast<hasInt<N / 2> *>(0),
                 static_cast<hasInt<(N / 2 > 1)> *>(0)),
    myMakeIdxSeq(static_cast<hasInt<N - N / 2> *>(0),
                 static_cast<hasInt<((N - N / 2) > 1)> *>(0))))
myMakeIdxSeq(hasInt<N> *, hasInt<1> *);
#endif
} // namespace otherIdxSeq

namespace variantImpl {
#if defined(_MSC_VER) || __GNUC__ >= 8
template <class = std::size_t, std::size_t... Ns> struct hasNum {};
#endif

template <std::size_t N>
using theIndexSeq =
/*
#if defined(__clang__)
    __make_integer_seq<hasNum, std::size_t, N>;//Bug with clang's
__make_integer_seq
    //https://stackoverflow.com/questions/57133186/g-and-clang-different-behaviour-when-stdmake-index-sequence-and-stdin
#endif
*/

#if __GNUC__ >= 8
    hasNum<std::size_t, __integer_pack(N)...>;
#elif defined(_MSC_VER)
    __make_integer_seq<hasNum, std::size_t, N>;
#else
    decltype(otherIdxSeq::myMakeIdxSeq(
        static_cast<typename otherIdxSeq::hasInt<N> *>(0),
        static_cast<typename otherIdxSeq::hasInt<(N > 1)> *>(0)));
#endif

template <std::size_t... Idx>
using mySeqOfNum =
#if defined(_MSC_VER) || __GNUC__ >= 8
    hasNum<std::size_t, Idx...>;
#else
    otherIdxSeq::index_sequence<Idx...> *;
#endif

constexpr bool
checkAnyFalse(const std::initializer_list<bool> theBoolList) noexcept {
  for (const bool theB : theBoolList) {
    if (!theB)
      return false;
  }
  return true;
}
struct makeValueless {};

#if SZ_VAR_CPPVERSION <= 201703L
// Used for conditionally trivial member functions
template <bool triviallyMoveAssign, class... Ts> struct variantMoveAssign;
#else
template<
#ifdef __clang__
bool trivDestructible,
#endif
class...> class variantCpp20Impl;
#endif
} // namespace variantImpl

template <class... Ts>
using variant = 
#if SZ_VAR_CPPVERSION <= 201703L
variantImpl::variantMoveAssign<
    //(std::is_trivially_move_assignable<Ts>::value && ...),
    variantImpl::checkAnyFalse(
        {std::is_trivially_move_assignable<Ts>::value...}),
    variantImpl::theIndexSeq<sizeof...(Ts)>, Ts...>;
#else
variantImpl::variantCpp20Impl<
#ifdef __clang__
variantImpl::checkAnyFalse({std::is_trivially_destructible<Ts>::value...}),
#endif
variantImpl::theIndexSeq<sizeof...(Ts)>, Ts...>;
#endif


namespace variantImpl {
template <typename...> struct everyIdxAndType; // helper
template <std::size_t... Idx, typename... Ts>
struct everyIdxAndType<variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public szHelpMethods::getRightType<Ts, Idx>... // for the (T&&) variant constructor and assignment
{
  using szHelpMethods::getRightType<Ts, Idx>::theMethod...;
};

template <class... Ts>
using allIdxAndTypes =
    everyIdxAndType<variantImpl::theIndexSeq<sizeof...(Ts)>, Ts...>;
#ifndef SZ_VAR_HASTYPEPACKELEM
template <std::size_t getIdx, class U>
constexpr szHelpMethods::getRightType<U, getIdx>
getBaseClass(szHelpMethods::getRightType<U, getIdx> *);
#endif

template <class U, std::size_t getIdx>
constexpr szHelpMethods::getRightType<U, getIdx>
getBaseClass(szHelpMethods::getRightType<U, getIdx> *);
} // namespace variantImpl

// variant alternative
template <std::size_t Idx, class T> struct variant_alternative {
  typedef T type;
  static constexpr std::size_t theIdx = Idx;
};
template <std::size_t Idx, class T>
struct variant_alternative<Idx, const T>
    : std::add_const<variant_alternative<Idx, T>>::type {};

template <std::size_t Idx, class T>
struct variant_alternative<Idx, volatile T>
    : std::add_volatile<variant_alternative<Idx, T>>::type {};

template <std::size_t Idx, class T>
struct variant_alternative<Idx, const volatile T>
    : std::add_cv<variant_alternative<Idx, T>>::type {};

template <std::size_t I, class... Types>
struct variant_alternative<I, variant<Types...>> {
  typedef SZ_VAR_GETIDXFROMTYPE(I, Types...) type;
  static constexpr std::size_t theIdx = I;
};

template <std::size_t I, class T>
using variant_alternative_t = typename variant_alternative<I, T>::type;
// end variant alternative

template <class T> struct variant_size;

template <class... Types>
struct variant_size<szLogVar::variant<Types...>>
    : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <class T> struct variant_size<const T> : variant_size<T> {};
template <class T> struct variant_size<volatile T> : variant_size<T> {};
template <class T> struct variant_size<const volatile T> : variant_size<T> {};

template <class T>
inline constexpr std::size_t variant_size_v = szLogVar::variant_size<T>::value;

inline constexpr std::size_t variant_npos = static_cast<std::size_t>(-1);

#define SZ_VAR_GETFXN_DECLARATION(MAYBECONST, MAYBEFRIEND, AMPERSANDS)         \
  template <std::size_t I, class... theTypes>                                  \
  MAYBEFRIEND constexpr MAYBECONST SZ_VAR_GETIDXFROMTYPE(I, theTypes...)       \
      AMPERSANDS                                                               \
      get(MAYBECONST szLogVar::variant<theTypes...> AMPERSANDS param)

#define SZ_VAR_IDX_GETUNCHECKED(MAYBECONST, MAYBEFRIEND, AMPERSANDS)           \
  template <std::size_t I, class... theTypes>                                  \
  MAYBEFRIEND constexpr MAYBECONST SZ_VAR_GETIDXFROMTYPE(                      \
      I, theTypes...) AMPERSANDS                                               \
  get_unchecked(MAYBECONST szLogVar::variant<theTypes...> AMPERSANDS param)

SZ_VAR_IDX_GETUNCHECKED(const, , &); // Prototypes of friend functions
SZ_VAR_GETFXN_DECLARATION(const, , &);
template <std::size_t I, class... Types>
constexpr const SZ_VAR_GETIDXFROMTYPE(I, Types...)*
get_if(const szLogVar::variant<Types...> *pv) noexcept;

#if SZ_VAR_CPPVERSION <= 201703L || !defined(__clang__)
#define SZ_VAR_GIVENTYPEGETIDX(TYPELISTNAME)                                   \
  decltype(variantImpl::getBaseClass<T>(                                       \
      static_cast<variantImpl::allIdxAndTypes<TYPELISTNAME> *>(0)))::theIdx
#else
#define SZ_VAR_GIVENTYPEGETIDX(TYPELISTNAME) decltype(variantImpl::getBaseClass<T>(static_cast<variant<TYPELISTNAME> *>(0)))::theIdx
#endif

#ifdef _MSC_VER
/*Needed in MSVC to make a copy of nonconst variant work*/
#define SZ_VAR_FORCONVERTCTOR                                                  \
  , std::size_t = sizeof(szHelpMethods::isAVariant(                            \
        static_cast<szHelpMethods::twoTypes<                                   \
            szLogVar::variant<Ts...>,                                          \
            typename std::remove_cv_t<std::remove_reference_t<T>>> *>(0)))
#define SZ_VAR_THEDESTROYER                                                    \
  szHelpMethods::destroyCurrent(this->template getImpl<Idx>(*this))

#else

#define SZ_VAR_FORCONVERTCTOR
#define SZ_VAR_THEDESTROYER this->template getImpl<Idx>(*this).~Ts()
#endif

#define SZ_VAR_DESTROYCURRENT                                                  \
  if (!szHelpMethods::expander<bool>{                                          \
          std::is_trivially_destructible<Ts>::value...}[this->index()]) {      \
    static_cast<void>(szHelpMethods::expander<bool>{                           \
        (Idx == this->theIndex && (SZ_VAR_THEDESTROYER, true))...});           \
  }
namespace variantImpl {

template <bool triviallyDestructible, std::size_t Low, std::size_t High,
          class... Ts> class theUnionBase;
#if SZ_VAR_CPPVERSION <= 201703L
// Macro to define theUnionBase
#define SZ_VAR_THEUNIONBASE(TREENODEDTOR, UNIONBASEDTOR)                       \
  template <std::size_t otherLow, std::size_t otherHigh> union treeNode {      \
    static constexpr std::size_t middleTypeIdx = (otherLow + otherHigh) / 2;   \
    SZ_VAR_GETIDXFROMTYPE(middleTypeIdx, Ts...) theT;                          \
    /*If there's only 1 type, there are no children nodes*/                    \
    decltype(szHelpMethods::getTypeMethod<                                     \
             makeValueless, treeNode<otherLow, middleTypeIdx - 1>>(            \
        static_cast<szHelpMethods::hasBool<(otherLow == middleTypeIdx)> *>(    \
            0))) theLeftBranch;                                                \
    /*(sizeof...(Ts) < 3) if there are only 1 or 2 types, so no left child     \
     * node.*/                                                                 \
    decltype(szHelpMethods::getTypeMethod<                                     \
             makeValueless, treeNode<middleTypeIdx + 1, otherHigh>>(           \
        static_cast<szHelpMethods::hasBool<(otherHigh == middleTypeIdx)> *>(   \
            0))) theRightBranch;                                               \
    template <std::size_t I, class... Args>                                    \
    constexpr treeNode(szHelpMethods::twoBools<true, false> *,                 \
                       const szLogVar::in_place_index_t<I> theIPT,             \
                       Args &&...args)                                         \
        : theLeftBranch{static_cast<szHelpMethods::twoBools<                   \
                            (I < ((otherLow + middleTypeIdx - 1) / 2)),        \
                            (I > ((otherLow + middleTypeIdx - 1) / 2))> *>(0), \
                        theIPT, static_cast<Args &&>(args)...} {}              \
    template <std::size_t I, class... Args>                                    \
    constexpr treeNode(szHelpMethods::twoBools<false, true> *,                 \
                       const szLogVar::in_place_index_t<I> theIPT,             \
                       Args &&...args)                                         \
        : theRightBranch{                                                      \
              static_cast<szHelpMethods::twoBools<                             \
                  (I < ((middleTypeIdx + 1 + otherHigh) / 2)),                 \
                  (I > ((middleTypeIdx + 1 + otherHigh) / 2))> *>(0),          \
              theIPT, static_cast<Args &&>(args)...} {}                        \
    template <class... Args>                                                   \
    constexpr treeNode(szHelpMethods::twoBools<false, false> *,                \
                       const szLogVar::in_place_index_t<middleTypeIdx>,        \
                       Args &&...args)                                         \
        : theT(static_cast<Args &&>(args)...) {}                               \
    TREENODEDTOR                                                               \
  };                                                                           \
  template <bool Cond, std::size_t otherLow, std::size_t otherHigh>            \
  using allTrivDestructBranchType =                                            \
      decltype(szHelpMethods::getTypeMethod<makeValueless,                     \
                                            treeNode<otherLow, otherHigh>>(    \
          static_cast<szHelpMethods::hasBool<Cond> *>(0)));                    \
                                                                               \
protected:                                                                     \
  typedef decltype(szHelpMethods::getTypeMethod<                               \
                   std::uint_least8_t,                                         \
                   decltype(szHelpMethods::getTypeMethod<                      \
                            std::uint_least16_t,                               \
                            decltype(szHelpMethods::getTypeMethod<             \
                                     std::uint_least32_t, std::size_t>(        \
                                static_cast<szHelpMethods::hasBool<(           \
                                    sizeof...(Ts) < UINT_LEAST64_MAX)> *>(     \
                                    0)))>(                                     \
                       static_cast<szHelpMethods::hasBool<(                    \
                           sizeof...(Ts) < UINT_LEAST32_MAX)> *>(0)))>(        \
      static_cast<szHelpMethods::hasBool<(sizeof...(Ts) < UINT_LEAST16_MAX)>   \
                      *>(0))) size_type;                                       \
  size_type theIndex{0};                                                       \
  static constexpr std::size_t middleTypeIdx = (Low + High) / 2;               \
  union {                                                                      \
    SZ_VAR_GETIDXFROMTYPE(middleTypeIdx, Ts...) theT;                          \
    makeValueless theValueless;                                                \
    allTrivDestructBranchType<(sizeof...(Ts) < 3), Low, middleTypeIdx - 1>     \
        theLeftBranch;                                                         \
    allTrivDestructBranchType<(sizeof...(Ts) == 1), middleTypeIdx + 1, High>   \
        theRightBranch;                                                        \
  };                                                                           \
  /*Constructors*/                                                             \
  constexpr theUnionBase(makeValueless)                                        \
      : theIndex{static_cast<size_type>(-1)}, theValueless{} {}                \
  template <std::size_t I, class... Args>                                      \
  constexpr theUnionBase(szHelpMethods::twoBools<true, false> *,               \
                         const szLogVar::in_place_index_t<I> theIPT,           \
                         Args &&...args)                                       \
      : theIndex{I},                                                           \
        theLeftBranch{static_cast<szHelpMethods::twoBools<                     \
                          (I < ((Low + middleTypeIdx - 1) / 2)),               \
                          (I > ((Low + middleTypeIdx - 1) / 2))> *>(0),        \
                      theIPT, static_cast<Args &&>(args)...} {}                \
  template <std::size_t I, class... Args>                                      \
  constexpr theUnionBase(szHelpMethods::twoBools<false, true> *,               \
                         const szLogVar::in_place_index_t<I> theIPT,           \
                         Args &&...args)                                       \
      : theIndex{I},                                                           \
        theRightBranch{static_cast<szHelpMethods::twoBools<                    \
                           (I < ((middleTypeIdx + 1 + High) / 2)),             \
                           (I > ((middleTypeIdx + 1 + High) / 2))> *>(0),      \
                       theIPT, static_cast<Args &&>(args)...} {}               \
  template <class... Args>                                                     \
  constexpr theUnionBase(szHelpMethods::twoBools<false, false> *,              \
                         const szLogVar::in_place_index_t<middleTypeIdx>,      \
                         Args &&...args)                                       \
      : theIndex{middleTypeIdx}, theT(static_cast<Args &&>(args)...) {}        \
                                                                               \
public:                                                                        \
  constexpr theUnionBase() noexcept(                                           \
      std::is_nothrow_default_constructible_v<SZ_VAR_GETIDXFROMTYPE(0,         \
                                                                    Ts...)>)   \
      : theUnionBase(                                                          \
            static_cast<                                                       \
                szHelpMethods::twoBools<(0 < middleTypeIdx), false> *>(0),     \
            szLogVar::in_place_index_t<0>{}) {}                                \
  template <                                                                   \
      class T                                                                  \
                                                                               \
          SZ_VAR_FORCONVERTCTOR,                                               \
      class U = decltype((static_cast<allIdxAndTypes<Ts...> *>(0)->theMethod(  \
          szHelpMethods::myDeclval<T>(0), szHelpMethods::myDeclval<T>(0),      \
          static_cast<szHelpMethods::hasBool<                                  \
              sizeof(szHelpMethods::isSame(                                    \
                  static_cast<szHelpMethods::twoTypes<                         \
                      typename std::remove_cv_t<std::remove_reference_t<T>>,   \
                      bool> *>(0))) == sizeof(std::size_t)> *>(0))))>          \
  constexpr theUnionBase(T &&t) noexcept(                                      \
      std::is_nothrow_constructible_v<SZ_VAR_GETIDXFROMTYPE(U::theIdx, Ts...), \
                                      T>)                                      \
      : theUnionBase(U{}, static_cast<T &&>(t)) {}                             \
  template <class T, std::size_t typeIdx = SZ_VAR_GIVENTYPEGETIDX(Ts...),      \
            class... Args>                                                     \
  constexpr explicit theUnionBase(const szLogVar::in_place_type_t<T>,          \
                                  Args &&...args)                              \
      : theUnionBase(                                                          \
            static_cast<szHelpMethods::twoBools<(typeIdx < middleTypeIdx),     \
                                                (typeIdx > middleTypeIdx)> *>( \
                0),                                                            \
            szLogVar::in_place_index_t<typeIdx>{},                             \
            static_cast<Args &&>(args)...) {}                                  \
  template <class T, class U,                                                  \
            std::size_t typeIdx = SZ_VAR_GIVENTYPEGETIDX(Ts...),               \
            class... Args>                                                     \
  constexpr explicit theUnionBase(const szLogVar::in_place_type_t<T>,          \
                                  std::initializer_list<U> il, Args &&...args) \
      : theUnionBase(                                                          \
            static_cast<szHelpMethods::twoBools<(typeIdx < middleTypeIdx),     \
                                                (typeIdx > middleTypeIdx)> *>( \
                0),                                                            \
            szLogVar::in_place_index_t<typeIdx>{}, il,                         \
            static_cast<Args &&>(args)...) {}                                  \
  template <std::size_t I, class... Args>                                      \
  constexpr explicit theUnionBase(const szLogVar::in_place_index_t<I> theIPT,  \
                                  Args &&...args)                              \
      : theUnionBase(                                                          \
            static_cast<szHelpMethods::twoBools<(I < middleTypeIdx),           \
                                                (I > middleTypeIdx)> *>(0),    \
            theIPT, static_cast<Args &&>(args)...) {}                          \
  /*Constructors taking an initializer list*/                                  \
  template <std::size_t I, class U, class... Args>                             \
  constexpr explicit theUnionBase(const szLogVar::in_place_index_t<I> theIPT,  \
                                  std::initializer_list<U> il, Args &&...args) \
      : theUnionBase(                                                          \
            static_cast<szHelpMethods::twoBools<(I < middleTypeIdx),           \
                                                (I > middleTypeIdx)> *>(0),    \
            theIPT, static_cast<std::initializer_list<U> &&>(il),              \
            static_cast<Args &&>(args)...) {}                                  \
  UNIONBASEDTOR

template <std::size_t Low, std::size_t High, class... Ts>
class theUnionBase<true, Low, High, Ts...> {
  SZ_VAR_THEUNIONBASE(, )
};

template <std::size_t Low, std::size_t High, class... Ts>
class theUnionBase<false, Low, High, Ts...> {
  SZ_VAR_THEUNIONBASE(~treeNode(){}, ~theUnionBase(){})
};

template <class...> class variantBase;


template <std::size_t... Idx, class... Ts>
class variantBase<variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public theUnionBase<(checkAnyFalse(
                              {std::is_trivially_destructible<Ts>::value...})),
                          0, sizeof...(Ts) - 1, Ts...> {

  void makeVariantValueless() {
    if constexpr (!(checkAnyFalse(
                      {std::is_trivially_destructible<Ts>::value...}))) {
      if (!valueless_by_exception()) {
        SZ_VAR_DESTROYCURRENT
        this->theIndex = static_cast<decltype(this->theIndex)>(-1);
      }
    }
  }

  template <size_t I, class... Args>
      SZ_VAR_GETIDXFROMTYPE(I, Ts...) & theEmplaceHelper(Args &&...args) {
    if constexpr (!(checkAnyFalse(
                      {std::is_trivially_destructible<Ts>::value...}))) {
      if (!valueless_by_exception()) {
        SZ_VAR_DESTROYCURRENT
        this->theIndex = static_cast<decltype(this->theIndex)>(-1);
      }
    }
    // Uses std::addressof possible implementation from
    // https://en.cppreference.com/w/cpp/memory/addressof Variant can't hold
    // function types, so this should always work
    auto &theRef =
        *(::new (reinterpret_cast<SZ_VAR_GETIDXFROMTYPE(I, Ts...) *>(
            &const_cast<char &>(reinterpret_cast<const volatile char &>(
                this->getImpl<I>(*this)))))
              SZ_VAR_GETIDXFROMTYPE(I, Ts...)(static_cast<Args &&>(args)...));
    //::new(this)variantBase{szLogVar::in_place_index_t<I>{},
    //:static_cast<Args&&>(args)...};
    this->theIndex = I;
    return theRef;
  }

protected:
  template <std::size_t I, class U>
      constexpr const SZ_VAR_GETIDXFROMTYPE(I, Ts...) &
      getImpl(const U &theU) const {
    constexpr std::size_t checkThisIdx =
        szHelpMethods::aliasForCVRef<U>::middleTypeIdx;
    if constexpr (I < checkThisIdx)
      return getImpl<I>(theU.theLeftBranch);
    else if constexpr (I > checkThisIdx)
      return getImpl<I>(theU.theRightBranch);
    else
      return theU.theT;
  }

public:
  static constexpr std::size_t numTypes =
      sizeof...(Ts); // Use with visit's binary search and
                     // szHelpMethods::aliasForCVRef<variant>

  using theUnionBase<(checkAnyFalse(
                         {std::is_trivially_destructible<Ts>::value...})),
                     0, sizeof...(Ts) - 1, Ts...>::theUnionBase;
  using theUnionBase<(checkAnyFalse(
                         {std::is_trivially_destructible<Ts>::value...})),
                     0, sizeof...(Ts) - 1, Ts...>::operator=;

  constexpr std::size_t index() const noexcept {
    return valueless_by_exception() ? static_cast<std::size_t>(-1)
                                    : this->theIndex;
  }
  constexpr bool valueless_by_exception() const noexcept {
    return this->theIndex == static_cast<decltype(this->theIndex)>(-1);
  }
  template <size_t I, class... Args>
      SZ_VAR_GETIDXFROMTYPE(I, Ts...) & emplace(Args &&...args) {
    return theEmplaceHelper<I>(static_cast<Args &&>(args)...);
  }
  template <size_t I, class U, class... Args>
      SZ_VAR_GETIDXFROMTYPE(I, Ts...) &
      emplace(std::initializer_list<U> il, Args &&...args) {
    return theEmplaceHelper<I>(static_cast<std::initializer_list<U> &&>(il),
                               static_cast<Args &&>(args)...);
  }
  template <class T, class... Args> T &emplace(Args &&...args) {
    return theEmplaceHelper<SZ_VAR_GIVENTYPEGETIDX(Ts...)>(
        static_cast<Args &&>(args)...);
  }
  template <class T, class U, class... Args>
  T &emplace(std::initializer_list<U> il, Args &&...args) {
    return theEmplaceHelper<SZ_VAR_GIVENTYPEGETIDX(Ts...)>(
        static_cast<std::initializer_list<U> &&>(il),
        static_cast<Args &&>(args)...);
  }
  // Friend fxn prototype

  template <std::size_t I, class... theTypes>
      friend constexpr const SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &
      szLogVar::get(const szLogVar::variant<theTypes...> &param);

  template <std::size_t I, class... theTypes>
      friend constexpr const SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &
      szLogVar::get_unchecked(const szLogVar::variant<theTypes...> &param);

  template <std::size_t I, class... Types>
  friend constexpr const SZ_VAR_GETIDXFROMTYPE(I, Types...)*
  szLogVar::get_if(const szLogVar::variant<Types...> *pv) noexcept;

  void swap(variantBase &rhs) noexcept(
      checkAnyFalse({(std::is_nothrow_move_constructible_v<Ts> &&
                      std::is_nothrow_swappable_v<Ts>)...})) {
    using std::swap;
    if constexpr (checkAnyFalse(
                      {std::is_trivially_move_assignable<Ts>::value...})) {
      return swap(*this, rhs);
    } else if (this->valueless_by_exception()) {
      if (rhs.valueless_by_exception())
        return; // Both variants are valueless

      static_cast<void>(szHelpMethods::expander<bool>{(
          Idx == rhs.index() && (this->theEmplaceHelper<Idx>(static_cast<Ts &&>(
                                     const_cast<Ts &>(rhs.getImpl<Idx>(rhs)))),
                                 true))...});
      return rhs.makeVariantValueless();
    } else if (rhs.valueless_by_exception()) {
      //"this" isn't valueless
      static_cast<void>(szHelpMethods::expander<bool>{
          (Idx == this->index() &&
           (rhs.theEmplaceHelper<Idx>(static_cast<Ts &&>(
                const_cast<Ts &>(this->getImpl<Idx>(*this)))),
            true))...});
      this->makeVariantValueless(); // Now it is
      return;
    }

    if (this->index() == rhs.index()) {
      // static_cast<void>(((Idx == this->index() &&
      // (swap(this->getImpl<Idx>(*this),rhs.getImpl<Idx>(rhs)  ),true )) ||
      // ...));
      static_cast<void>(szHelpMethods::expander<bool>{
          (Idx == this->index() &&
           (swap(const_cast<Ts &>(this->getImpl<Idx>(*this)),
                 const_cast<Ts &>(rhs.getImpl<Idx>(rhs))),
            true))...});
    } else {
      static_cast<void>(szHelpMethods::expander<
                        bool>{(Idx == this->index() && [otherIdx = rhs.index()](
                                                           const auto param,
                                                           auto &refThis,
                                                           auto &refRight) {
        static_cast<void>(szHelpMethods::expander<bool>{(
            Idx == otherIdx &&
            [](auto &theRef, auto &rhsRef,
               auto
                   leftType, // typename
                             // szLogVar::variant_alternative<decltype(param)::theIdx,
                             // variant<Ts...>>::type
               auto rightType // typename szLogVar::variant_alternative<Idx,
                              // variant<Ts...>>::type
// Not sure why MSVC can't use param like gcc and clang. Capturing it like
// [param] doesn't work MSVC needs this theParam parameter
#ifdef _MSC_VER
               ,
               auto theParam
#endif
            ) {
              theRef.template theEmplaceHelper<Idx>(
                  static_cast<decltype(rightType) &&>(rightType));

              rhsRef.template theEmplaceHelper<decltype(
#ifdef _MSC_VER
                  theParam
#else
                      param
#endif
                  )::theIdx>(static_cast<decltype(leftType) &&>(leftType));

              return true;
            }(refThis, refRight,
              static_cast<SZ_VAR_GETIDXFROMTYPE(decltype(param)::theIdx,
                                                Ts...) &&>(
                  const_cast<SZ_VAR_GETIDXFROMTYPE(decltype(param)::theIdx,
                                                   Ts...) &>(
                      refThis.template getImpl<decltype(param)::theIdx>(
                          refThis))),
              static_cast<Ts &&>(
                  const_cast<Ts &>(refRight.template getImpl<Idx>(refRight)))
#ifdef _MSC_VER
                  ,
              param
#endif
              ))...});

        return true;
      }(szHelpMethods::getRightType<Ts, Idx>{}, *this, rhs))...});
    }
  }
  // Comparison operators as hidden friends
  friend constexpr bool operator==(const variantBase &lhs,
                                   const variantBase &rhs) {
    if (lhs.index() != rhs.index())
      return false;
    if (lhs.valueless_by_exception())
      return true;
    bool theBool = false;

    static_cast<void>(szHelpMethods::expander<bool>{
        (Idx == lhs.index() &&
         (theBool = (lhs.getImpl<Idx>(lhs) == rhs.getImpl<Idx>(rhs)),
          true))...});

    // static_cast<void>(((Idx == lhs.index() && (theBool =
    // (lhs.getImpl<Idx>(lhs) == rhs.getImpl<Idx>(rhs)), true )) || ...));
    return theBool;
  }
  friend constexpr bool operator!=(const variantBase &lhs,
                                   const variantBase &rhs) {
    return !(lhs == rhs);
  }
  friend constexpr bool operator<(const variantBase &lhs,
                                  const variantBase &rhs) {
    if (rhs.valueless_by_exception()) {
      return false;
    }
    if (lhs.valueless_by_exception()) {
      return true;
    }
    if (lhs.index() < rhs.index()) {
      return true;
    }
    if (lhs.index() > rhs.index()) {
      return false;
    }

    bool theBool = false;
    static_cast<void>(szHelpMethods::expander<bool>{(
        Idx == lhs.index() &&
        (theBool = (lhs.getImpl<Idx>(lhs) < rhs.getImpl<Idx>(rhs)), true))...});

    return theBool;
  }
  friend constexpr bool operator>(const variantBase &lhs,
                                  const variantBase &rhs) {
    if (lhs.valueless_by_exception())
      return false;
    if (rhs.valueless_by_exception())
      return true;
    if (lhs.index() > rhs.index())
      return true;
    if (lhs.index() < rhs.index())
      return false;

    bool theBool = false;
    static_cast<void>(szHelpMethods::expander<bool>{(
        Idx == lhs.index() &&
        (theBool = (lhs.getImpl<Idx>(lhs) > rhs.getImpl<Idx>(rhs)), true))...});

    return theBool;
  }
  friend constexpr bool operator<=(const variantBase &lhs,
                                   const variantBase &rhs) {
    if (lhs.valueless_by_exception())
      return true;
    if (rhs.valueless_by_exception())
      return false;
    if (lhs.index() < rhs.index())
      return true;
    if (lhs.index() > rhs.index())
      return false;

    bool theBool = false;
    static_cast<void>(szHelpMethods::expander<bool>{
        (Idx == lhs.index() &&
         (theBool = (lhs.getImpl<Idx>(lhs) <= rhs.getImpl<Idx>(rhs)),
          true))...});

    return theBool;
  }
  friend constexpr bool operator>=(const variantBase &lhs,
                                   const variantBase &rhs) {
    if (rhs.valueless_by_exception())
      return true;
    if (lhs.valueless_by_exception())
      return false;
    if (lhs.index() > rhs.index())
      return true;
    if (lhs.index() < rhs.index())
      return false;

    bool theBool = false;
    static_cast<void>(szHelpMethods::expander<bool>{
        (Idx == lhs.index() &&
         (theBool = (lhs.getImpl<Idx>(lhs) >= rhs.getImpl<Idx>(rhs)),
          true))...});
    // static_cast<void>(((Idx == lhs.index() && (theBool =
    // (lhs.getImpl<Idx>(lhs) >= rhs.getImpl<Idx>(rhs)), true )) || ...));
    return theBool;
  }
  // End comparison operators
};

template <bool triviallyDestructible, class...> struct variantDestruct;
template <std::size_t... Idx, class... Ts>
struct variantDestruct<true, variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public variantBase<variantImpl::mySeqOfNum<Idx...>, Ts...> {
  using variantBase<variantImpl::mySeqOfNum<Idx...>, Ts...>::variantBase;
  using variantBase<variantImpl::mySeqOfNum<Idx...>, Ts...>::operator=;
};

template <std::size_t... Idx, class... Ts>
struct variantDestruct<false, variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public variantBase<variantImpl::mySeqOfNum<Idx...>, Ts...> {
  using variantBase<variantImpl::mySeqOfNum<Idx...>, Ts...>::variantBase;
  using variantBase<variantImpl::mySeqOfNum<Idx...>, Ts...>::operator=;
  ~variantDestruct() {
    if (!this->valueless_by_exception()) {
      SZ_VAR_DESTROYCURRENT
    }
  }
};
#define SZ_VAR_COPY_CT_PARENT                                                  \
  variantDestruct<(checkAnyFalse(                                              \
                      {std::is_trivially_destructible<Ts>::value...})),        \
                  variantImpl::mySeqOfNum<Idx...>, Ts...>
template <bool triviallyCopyCtable, class...> struct variantCopyConstruct;
template <std::size_t... Idx, class... Ts>
struct variantCopyConstruct<true, variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public SZ_VAR_COPY_CT_PARENT {
  using SZ_VAR_COPY_CT_PARENT::variantDestruct;
  using SZ_VAR_COPY_CT_PARENT::operator=;
};
template <std::size_t... Idx, class... Ts>
struct variantCopyConstruct<false, variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public SZ_VAR_COPY_CT_PARENT {
  using SZ_VAR_COPY_CT_PARENT::variantDestruct;
  using SZ_VAR_COPY_CT_PARENT::operator=;
  variantCopyConstruct(const variantCopyConstruct &param)
      : SZ_VAR_COPY_CT_PARENT(makeValueless{}) {
    if (!param.valueless_by_exception()) {
      static_cast<void>(szHelpMethods::expander<bool>{
          (Idx == param.index() &&
           (::new (this) variant<Ts...>{szLogVar::in_place_index_t<Idx>{},
                                        param.template getImpl<Idx>(param)},
            false))...});
      // this->theIndex = param.index();
    }
  }
  variantCopyConstruct(variantCopyConstruct &param)
      : variantCopyConstruct(const_cast<const variantCopyConstruct &>(param)) {}
};
#undef SZ_VAR_COPY_CT_PARENT
#define SZ_VAR_MOVE_CT_PARENT                                                  \
  variantCopyConstruct<                                                        \
      (checkAnyFalse({std::is_trivially_copy_constructible<Ts>::value...})),   \
      variantImpl::mySeqOfNum<Idx...>, Ts...>
template <bool triviallyMoveCtable, class...> struct variantMoveConstruct;
template <std::size_t... Idx, class... Ts>
struct variantMoveConstruct<true, variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public SZ_VAR_MOVE_CT_PARENT {
  using SZ_VAR_MOVE_CT_PARENT::variantCopyConstruct;
  using SZ_VAR_MOVE_CT_PARENT::operator=;
};
template <std::size_t... Idx, class... Ts>
struct variantMoveConstruct<false, variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public SZ_VAR_MOVE_CT_PARENT {
  using SZ_VAR_MOVE_CT_PARENT::variantCopyConstruct;
  variantMoveConstruct(const variantMoveConstruct &) = default;
  constexpr variantMoveConstruct(variantMoveConstruct &&param) noexcept(
      checkAnyFalse({std::is_nothrow_move_constructible_v<Ts>...}))
      : SZ_VAR_MOVE_CT_PARENT(makeValueless{}) {
    if (!param.valueless_by_exception()) {
      static_cast<void>(szHelpMethods::expander<bool>{
          (Idx == param.index() &&
           (::new (this)
                variant<Ts...>{szLogVar::in_place_index_t<Idx>{},
                               static_cast<Ts &&>(const_cast<Ts &>(
                                   param.template getImpl<Idx>(param)))}))...});
    }
  }
  using SZ_VAR_MOVE_CT_PARENT::operator=;
};
#undef SZ_VAR_MOVE_CT_PARENT
template <bool triviallyCopyAssign, class... Ts> struct variantCopyAssign;
#define SZ_VAR_COPYASSIGN_PARENT                                               \
  variantMoveConstruct<                                                        \
      (checkAnyFalse({std::is_trivially_move_constructible<Ts>::value...})),   \
      variantImpl::mySeqOfNum<Idx...>, Ts...>
template <std::size_t... Idx, class... Ts>
struct variantCopyAssign<true, variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public SZ_VAR_COPYASSIGN_PARENT {
  using SZ_VAR_COPYASSIGN_PARENT::variantMoveConstruct;
  using SZ_VAR_COPYASSIGN_PARENT::operator=;
};

template <std::size_t... Idx, class... Ts>
struct variantCopyAssign<false, variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public SZ_VAR_COPYASSIGN_PARENT {
  using SZ_VAR_COPYASSIGN_PARENT::variantMoveConstruct;
  using SZ_VAR_COPYASSIGN_PARENT::operator=;
  variantCopyAssign(const variantCopyAssign &) = default;
  variantCopyAssign(variantCopyAssign &&) = default;
  constexpr variantCopyAssign &operator=(const variantCopyAssign &other) {
    if (other.valueless_by_exception()) {
      if (this->valueless_by_exception())
        return *this;

      if constexpr (!(checkAnyFalse(
                        {std::is_trivially_destructible<Ts>::value...}))) {
        SZ_VAR_DESTROYCURRENT
        this->theIndex = static_cast<decltype(this->theIndex)>(-1);
      }
      return *this;
    }
    if (this->index() == other.index()) {
      static_cast<void>(szHelpMethods::expander<bool>{
          (Idx == this->index() &&
           (const_cast<Ts&>(
                this->template getImpl<Idx>(*this)) =
                other.template getImpl<Idx>(other),
            false))...});

      return *this;
    }
    if (szHelpMethods::expander<bool>{(
            std::is_nothrow_copy_constructible<Ts>::value ||
            !std::is_nothrow_move_constructible<Ts>::value)...}[other
                                                                    .index()]) {
      static_cast<void>(szHelpMethods::expander<bool>{
          (Idx == other.index() &&
           ((this->template emplace<Idx>(other.template getImpl<Idx>(other)),
             false)))...});
      return *this;
    }
    return static_cast<variant<Ts...> &>(*this) =
               variant<Ts...>{static_cast<const variant<Ts...> &>(other)};
  }
};
#undef SZ_VAR_COPYASSIGN_PARENT
#define SZ_VAR_MOVEASSIGN_PARENT                                               \
  variantCopyAssign<(checkAnyFalse(                                            \
                        {std::is_trivially_copy_assignable<Ts>::value...})),   \
                    variantImpl::mySeqOfNum<Idx...>, Ts...>
template <std::size_t... Idx, class... Ts>
struct variantMoveAssign<true, variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public SZ_VAR_MOVEASSIGN_PARENT {
  using SZ_VAR_MOVEASSIGN_PARENT::variantCopyAssign;
  using SZ_VAR_MOVEASSIGN_PARENT::operator=;
  template <
      class T,
      std::size_t theTypeIdx =
          decltype(static_cast<allIdxAndTypes<Ts...> *>(0)->theMethod(
              szHelpMethods::myDeclval<T>(0), szHelpMethods::myDeclval<T>(0),
              static_cast<szHelpMethods::hasBool<
                  sizeof(szHelpMethods::isSame(
                      static_cast<szHelpMethods::twoTypes<
                          typename std::remove_cv_t<std::remove_reference_t<T>>,
                          bool> *>(0))) == sizeof(std::size_t)> *>(0)))::theIdx,
      class altType = SZ_VAR_GETIDXFROMTYPE(theTypeIdx, Ts...)>
  variantMoveAssign &operator=(T &&other) noexcept(
      std::is_nothrow_assignable_v<altType &, T>
          &&std::is_nothrow_constructible_v<altType, T>) {
    if (this->index() == theTypeIdx) {
      const_cast<altType &>(this->template getImpl<theTypeIdx>(*this)) =
          static_cast<T &&>(other);
      return *this;
    } else if constexpr (std::is_nothrow_constructible<altType, T>::value ||
                         !std::is_nothrow_move_constructible_v<altType>) {
      this->template emplace<theTypeIdx>(static_cast<T &&>(other));
      return *this;
    } else {
      return *this = variantMoveAssign{szLogVar::in_place_index_t<theTypeIdx>{},
                                       static_cast<T &&>(other)};
    }
  }
};
template <std::size_t... Idx, class... Ts>
struct variantMoveAssign<false, variantImpl::mySeqOfNum<Idx...>, Ts...>
    : public SZ_VAR_MOVEASSIGN_PARENT {
  using SZ_VAR_MOVEASSIGN_PARENT::variantCopyAssign;
  using SZ_VAR_MOVEASSIGN_PARENT::operator=;
  variantMoveAssign(const variantMoveAssign &param) = default;
  variantMoveAssign(variantMoveAssign &&) = default;

  template <
      class T,
      std::size_t theTypeIdx =
          decltype(static_cast<allIdxAndTypes<Ts...> *>(0)->theMethod(
              szHelpMethods::myDeclval<T>(0), szHelpMethods::myDeclval<T>(0),
              static_cast<szHelpMethods::hasBool<
                  sizeof(szHelpMethods::isSame(
                      static_cast<szHelpMethods::twoTypes<
                          typename std::remove_cv_t<std::remove_reference_t<T>>,
                          bool> *>(0))) == sizeof(std::size_t)> *>(0)))::theIdx,
      class altType = SZ_VAR_GETIDXFROMTYPE(theTypeIdx, Ts...)>
  variantMoveAssign &operator=(T &&other) noexcept(
      std::is_nothrow_assignable_v<altType &, T>
          &&std::is_nothrow_constructible_v<altType, T>) {
    if (this->index() == theTypeIdx) {
      const_cast<altType &>(this->template getImpl<theTypeIdx>(*this)) =
          static_cast<T &&>(other);
      return *this;
    } else if constexpr (std::is_nothrow_constructible<altType, T>::value ||
                         !std::is_nothrow_move_constructible_v<altType>) {
      this->template emplace<theTypeIdx>(static_cast<T &&>(other));
      return *this;
    } else {
      return *this = variantMoveAssign{szLogVar::in_place_index_t<theTypeIdx>{},
                                       static_cast<T &&>(other)};
    }
  }

#ifdef _MSC_VER
  // Written this way instead of = default to avoid MSVC's
  //"multiple versions of a defaulted special member functions" error
  constexpr variantMoveAssign &operator=(const variantMoveAssign &param) {
    return static_cast<variantMoveAssign &>(
        static_cast<SZ_VAR_MOVEASSIGN_PARENT &>(*this) =
            static_cast<const SZ_VAR_MOVEASSIGN_PARENT &>(param));
  }
#else
  constexpr variantMoveAssign &operator=(const variantMoveAssign &) = default;
#endif
  constexpr variantMoveAssign &operator=(variantMoveAssign &param) {
    return *this = const_cast<const variantMoveAssign &>(param);
  }
  variantMoveAssign &operator=(variantMoveAssign &&other) noexcept(
      checkAnyFalse({(std::is_nothrow_move_constructible_v<Ts> &&
                      std::is_nothrow_move_assignable_v<Ts>)...})) {
    if (other.valueless_by_exception()) {
      if (this->valueless_by_exception())
        return *this;

      if constexpr (!(checkAnyFalse(
                        {std::is_trivially_destructible<Ts>::value...}))) {
        SZ_VAR_DESTROYCURRENT
        this->theIndex = static_cast<decltype(this->theIndex)>(-1);
      }
      return *this;
    }
    if (this->index() == other.index()) {
      static_cast<void>(szHelpMethods::expander<bool>{
          (Idx == this->index() &&
           (const_cast<Ts &>(this->template getImpl<Idx>(*this)) =
                static_cast<Ts &&>(
                    const_cast<Ts &>(other.template getImpl<Idx>(other))),
            true))...});

      return *this;
    }
    SZ_VAR_DESTROYCURRENT

    this->theIndex = static_cast<decltype(this->theIndex)>(-1);

    static_cast<void>(szHelpMethods::expander<bool>{
        (Idx == other.index() &&
         (::new (reinterpret_cast<Ts *>(
              &const_cast<char &>(reinterpret_cast<const volatile char &>(
                  this->template getImpl<Idx>(*this)))))
              Ts(static_cast<Ts &&>(
                  const_cast<Ts &>(other.template getImpl<Idx>(other)))),
          true))...});

    this->theIndex = other.theIndex;
    return *this;
  }
};
#else

#if defined(__clang__) || defined(_MSC_VER)
//This works in Msvc too
#define SZ_VAR_NOUSEALLIDXANDTYPES : public szHelpMethods::getRightType<Ts, Idx>...
#define SZ_VAR_USINGOWNRIGHTTYPES using szHelpMethods::getRightType<Ts, Idx>::theMethod...;
#define SZ_VAR_USEALLIDXANDTYPES
#else
#define SZ_VAR_NOUSEALLIDXANDTYPES
#define SZ_VAR_USINGOWNRIGHTTYPES
#define SZ_VAR_USEALLIDXANDTYPES static_cast<everyIdxAndType<variantImpl::mySeqOfNum<Idx...>,Ts...> *>(0)->
#endif

#ifdef _MSC_VER
#define SZ_VARCPP20_FORCONVERTCTOR requires requires{\
      sizeof(szHelpMethods::isAVariant(\
          static_cast<szHelpMethods::twoTypes<\
              variantCpp20Impl<Ts...>,\
              typename std::remove_cv_t<std::remove_reference_t<T>>> *>(\
              0)));\
    }
#define SZ_VAR_AUTOPARAM_SWAP ,auto theParam
#define SZ_VAR_USEPARAMORTHEPARAM theParam
#define SZ_VAR_COMMAPARAM ,param
#else
#define SZ_VARCPP20_FORCONVERTCTOR
#define SZ_VAR_AUTOPARAM_SWAP
#define SZ_VAR_USEPARAMORTHEPARAM param
#define SZ_VAR_COMMAPARAM
#endif



  #define SZ_VAR_VARIANTCPP20IMPL(TREENODEDTOR, VARIANTDTOR)\
  SZ_VAR_NOUSEALLIDXANDTYPES {\
   \
     static constexpr bool allTrivDtable = variantImpl::checkAnyFalse({std::is_trivially_destructible<Ts>::value...});\
     \
    SZ_VAR_USINGOWNRIGHTTYPES \
    template <std::size_t otherLow, std::size_t otherHigh> union treeNode {\
    static constexpr std::size_t middleTypeIdx = (otherLow + otherHigh) / 2;\
    SZ_VAR_GETIDXFROMTYPE(middleTypeIdx, Ts...) theT;\
    /*If there's only 1 type, there are no children nodes*/\
    decltype(szHelpMethods::getTypeMethod<\
             makeValueless, treeNode<otherLow, middleTypeIdx - 1>>(\
        static_cast<szHelpMethods::hasBool<(otherLow == middleTypeIdx)> *>(\
            0))) theLeftBranch;\
    /*(sizeof...(Ts) < 3) if there are only 1 or 2 types, so no left child node.*/\
    decltype(szHelpMethods::getTypeMethod<\
             makeValueless, treeNode<middleTypeIdx + 1, otherHigh>>(\
        static_cast<szHelpMethods::hasBool<(otherHigh == middleTypeIdx)> *>(\
            0))) theRightBranch;\
    template <std::size_t I, class... Args>\
    requires (I < middleTypeIdx)\
    constexpr treeNode(const szLogVar::in_place_index_t<I> theIPT,\
                       Args &&...args)\
        : theLeftBranch{theIPT, static_cast<Args &&>(args)...} {}\
    template <std::size_t I, class... Args>\
    requires (I > middleTypeIdx)\
    constexpr treeNode(const szLogVar::in_place_index_t<I> theIPT,\
                       Args &&...args)\
        : theRightBranch{theIPT, static_cast<Args &&>(args)...} {}\
    template <class... Args>\
    constexpr treeNode(const szLogVar::in_place_index_t<middleTypeIdx>, Args &&...args)\
        : theT(static_cast<Args &&>(args)...) {}\
        TREENODEDTOR \
  };\
    static constexpr std::size_t middleTypeIdx = (0 + (sizeof...(Ts) - 1)) / 2;\
    typedef decltype(szHelpMethods::getTypeMethod<                               \
                   std::uint_least8_t,                                         \
                   decltype(szHelpMethods::getTypeMethod<                      \
                            std::uint_least16_t,                               \
                            decltype(szHelpMethods::getTypeMethod<             \
                                     std::uint_least32_t, std::size_t>(        \
                                static_cast<szHelpMethods::hasBool<(           \
                                    sizeof...(Ts) < UINT_LEAST64_MAX)> *>(     \
                                    0)))>(                                     \
                       static_cast<szHelpMethods::hasBool<(                    \
                           sizeof...(Ts) < UINT_LEAST32_MAX)> *>(0)))>(        \
      static_cast<szHelpMethods::hasBool<(sizeof...(Ts) < UINT_LEAST16_MAX)>   \
                      *>(0))) size_type;\
    template <bool Cond, std::size_t otherLow, std::size_t otherHigh>\
  using allTrivDestructBranchType =\
      decltype(szHelpMethods::getTypeMethod<makeValueless,treeNode<otherLow, otherHigh>>(static_cast<szHelpMethods::hasBool<Cond> *>(0)));\
    size_type theIndex;\
    union {\
      SZ_VAR_GETIDXFROMTYPE(middleTypeIdx, Ts...) theT;\
      makeValueless theValueless;\
      allTrivDestructBranchType<(sizeof...(Ts) < 3), 0, middleTypeIdx - 1>\
          theLeftBranch;\
      allTrivDestructBranchType<(sizeof...(Ts) == 1), middleTypeIdx + 1, (sizeof...(Ts) - 1)>\
          theRightBranch;\
    };\
    /*Private methods*/\
      void makeVariantValueless() {\
    if constexpr (!(checkAnyFalse(\
                      {std::is_trivially_destructible<Ts>::value...}))) {\
      if (!valueless_by_exception()) {\
      \
        SZ_VAR_DESTROYCURRENT\
        this->theIndex = static_cast<decltype(this->theIndex)>(-1);\
      }\
    }\
  }\
  template <size_t I, class... Args>\
      SZ_VAR_GETIDXFROMTYPE(I, Ts...) & theEmplaceHelper(Args &&...args) {\
    if constexpr (!(checkAnyFalse({std::is_trivially_destructible<Ts>::value...}))) {\
      if (!valueless_by_exception()) {\
      \
        SZ_VAR_DESTROYCURRENT\
        this->theIndex = static_cast<decltype(this->theIndex)>(-1);\
      }\
    }\
    auto &theRef =\
        *(::new (reinterpret_cast<SZ_VAR_GETIDXFROMTYPE(I, Ts...) *>(\
            &const_cast<char &>(reinterpret_cast<const volatile char &>(\
                this->getImpl<I>(*this)))))\
              SZ_VAR_GETIDXFROMTYPE(I, Ts...)(static_cast<Args &&>(args)...));\
    this->theIndex = I;\
    return theRef;\
  }\
    template <std::size_t I, class U>\
    constexpr const SZ_VAR_GETIDXFROMTYPE(I, Ts...) &\
    getImpl(const U &theU) const{\
      constexpr std::size_t checkThisIdx =\
      szHelpMethods::aliasForCVRef<U>::middleTypeIdx;\
      if constexpr (I < checkThisIdx)\
        return getImpl<I>(theU.theLeftBranch);\
      else if constexpr (I > checkThisIdx)\
        return getImpl<I>(theU.theRightBranch);\
      else\
        return theU.theT;\
      }\
    /*Constructors*/\
    constexpr variantCpp20Impl(makeValueless) : theIndex{static_cast<size_type>(-1)}, theValueless{} {}\
    public:\
    \
    static constexpr std::size_t numTypes = sizeof...(Ts);\
    constexpr variantCpp20Impl() noexcept(\
      std::is_nothrow_default_constructible_v<SZ_VAR_GETIDXFROMTYPE(0, Ts...)>)\
      : variantCpp20Impl(szLogVar::in_place_index_t<0>{}) {}\
    \
      template <                                                                   \
      class T                               \
                                         \
          SZ_VAR_FORCONVERTCTOR,                                               \
      class U = decltype((\
        SZ_VAR_USEALLIDXANDTYPES\
        theMethod(  \
          szHelpMethods::myDeclval<T>(0), szHelpMethods::myDeclval<T>(0),      \
          static_cast<szHelpMethods::hasBool<                                  \
              sizeof(szHelpMethods::isSame(                                    \
                  static_cast<szHelpMethods::twoTypes<                         \
                      typename std::remove_cv_t<std::remove_reference_t<T>>,   \
                      bool> *>(0))) == sizeof(std::size_t)> *>(0))))>\
                      \
    SZ_VARCPP20_FORCONVERTCTOR \
    constexpr variantCpp20Impl(T&& t) : variantCpp20Impl(U{}, static_cast<T &&>(t)) {}\
\
    template <class T, std::size_t typeIdx = SZ_VAR_GIVENTYPEGETIDX(Ts...),\
            class... Args>\
  constexpr explicit variantCpp20Impl(const szLogVar::in_place_type_t<T>,\
                                  Args &&...args) : variantCpp20Impl(szLogVar::in_place_index_t<typeIdx>{},\
            static_cast<Args &&>(args)...) {}\
  template <std::size_t I, class... Args>\
  requires (I < middleTypeIdx)\
  constexpr explicit variantCpp20Impl(const szLogVar::in_place_index_t<I> theIPT,\
                                  Args &&...args) : theIndex{I}, theLeftBranch{theIPT, static_cast<Args&&>(args)...} {}\
  template <std::size_t I, class... Args>\
  requires (I > middleTypeIdx)\
  constexpr explicit variantCpp20Impl(const szLogVar::in_place_index_t<I> theIPT,\
                                  Args &&...args) : theIndex{I},theRightBranch{theIPT, static_cast<Args&&>(args)...} {}\
  template<class... Args>\
  constexpr explicit variantCpp20Impl(const szLogVar::in_place_index_t<middleTypeIdx>,\
  Args &&...args) : theIndex{middleTypeIdx},theT(static_cast<Args&&>(args)...){}\
\
  template <std::size_t I, class U, class... Args>\
  requires (I < middleTypeIdx)\
  constexpr explicit variantCpp20Impl(const szLogVar::in_place_index_t<I> theIPT,\
                                  std::initializer_list<U> il, Args &&...args) :theIndex{I}, theLeftBranch{theIPT, static_cast<std::initializer_list<U> &&>(il),\
            static_cast<Args &&>(args)...}{}\
  template <std::size_t I, class U, class... Args>\
  requires (I > middleTypeIdx)\
  constexpr explicit variantCpp20Impl(const szLogVar::in_place_index_t<I> theIPT,\
                                  std::initializer_list<U> il, Args &&...args) : theIndex{I},theRightBranch{theIPT, static_cast<std::initializer_list<U> &&>(il),\
            static_cast<Args &&>(args)...}{}\
  template <class U, class... Args>\
  constexpr explicit variantCpp20Impl(const szLogVar::in_place_index_t<middleTypeIdx> theIPT,\
                                  std::initializer_list<U> il, Args &&...args) : theIndex{middleTypeIdx}, theT(static_cast<std::initializer_list<U> &&>(il),static_cast<Args &&>(args)...){}\
\
  constexpr variantCpp20Impl(const variantCpp20Impl&) requires (variantImpl::checkAnyFalse({std::is_trivially_copy_constructible<Ts>::value...})) = default;\
\
  variantCpp20Impl(const variantCpp20Impl& param) noexcept(\
      checkAnyFalse({std::is_nothrow_copy_constructible_v<Ts>...})) : variantCpp20Impl(makeValueless{}){\
    if(!param.valueless_by_exception()){\
      static_cast<void>(szHelpMethods::expander<bool>{ ((Idx == param.index()) && (::new(this)variantCpp20Impl{\
        szLogVar::in_place_index_t<Idx>{}, param.template getImpl<Idx>(param)\
      } ) )... });\
    }\
  }\
  constexpr variantCpp20Impl(variantCpp20Impl& param) : variantCpp20Impl(const_cast<const variantCpp20Impl&>(param)){}\
  constexpr variantCpp20Impl(variantCpp20Impl&&) requires (variantImpl::checkAnyFalse({std::is_trivially_move_constructible<Ts>::value...})) = default;\
  variantCpp20Impl(variantCpp20Impl&& param) noexcept(\
      checkAnyFalse({std::is_nothrow_move_constructible_v<Ts>...})) : variantCpp20Impl(makeValueless{}){\
    if(!param.valueless_by_exception()){\
      static_cast<void>(szHelpMethods::expander<bool>{ ((Idx == param.index()) && (::new(this)variantCpp20Impl{\
        szLogVar::in_place_index_t<Idx>{}, static_cast<Ts&&>(const_cast<Ts&>(param.template getImpl<Idx>(param)))\
      } ) )... }); \
    }\
  }\
  constexpr variantCpp20Impl& operator=(const variantCpp20Impl&) requires (variantImpl::checkAnyFalse({std::is_trivially_copy_assignable<Ts>::value...})) = default;\
\
  variantCpp20Impl& operator=(const variantCpp20Impl& other){\
        if (other.valueless_by_exception()) {\
      if (this->valueless_by_exception())\
        return *this;\
\
      if constexpr (!(checkAnyFalse(\
                        {std::is_trivially_destructible<Ts>::value...}))) {\
        SZ_VAR_DESTROYCURRENT \
        this->theIndex = static_cast<decltype(this->theIndex)>(-1);\
      }\
      return *this;\
    }\
    if (this->index() == other.index()) {\
      static_cast<void>(szHelpMethods::expander<bool>{\
          (Idx == this->index() &&\
           (const_cast<SZ_VAR_GETIDXFROMTYPE(Idx, Ts...) &>(\
                this->template getImpl<Idx>(*this)) =\
                other.template getImpl<Idx>(other),\
            false))...});\
\
      return *this;\
    }\
    if (szHelpMethods::expander<bool>{(\
            std::is_nothrow_copy_constructible<Ts>::value ||\
            !std::is_nothrow_move_constructible<Ts>::value)...}[other\
                                                                    .index()]) {\
      static_cast<void>(szHelpMethods::expander<bool>{\
          (Idx == other.index() &&\
           ((this->template theEmplaceHelper<Idx>(other.template getImpl<Idx>(other)),\
             false)))...});\
      return *this;\
    }\
    return *this = variant<Ts...>{other};\
  }\
  constexpr variantCpp20Impl& operator=(variantCpp20Impl&&) requires (variantImpl::checkAnyFalse({std::is_trivially_move_assignable<Ts>::value...})) = default;\
\
  variantCpp20Impl& operator=(variantCpp20Impl&& other) noexcept(\
      checkAnyFalse({(std::is_nothrow_move_constructible_v<Ts> &&\
                      std::is_nothrow_move_assignable_v<Ts>)...})) {\
    if (other.valueless_by_exception()) {\
      if (this->valueless_by_exception())\
        return *this;\
\
      if constexpr (!(checkAnyFalse(\
                        {std::is_trivially_destructible<Ts>::value...}))) {\
        SZ_VAR_DESTROYCURRENT \
        this->theIndex = static_cast<decltype(this->theIndex)>(-1);\
      }\
      return *this;\
    }\
    if (this->index() == other.index()) {\
      static_cast<void>(szHelpMethods::expander<bool>{\
          (Idx == this->index() &&\
           (const_cast<Ts &>(this->template getImpl<Idx>(*this)) =\
                static_cast<Ts &&>(\
                    const_cast<Ts &>(other.template getImpl<Idx>(other))),\
            true))...});\
\
      return *this;\
    }\
    SZ_VAR_DESTROYCURRENT \
\
    this->theIndex = static_cast<decltype(this->theIndex)>(-1);\
\
    static_cast<void>(szHelpMethods::expander<bool>{\
        (Idx == other.index() &&\
         (::new (reinterpret_cast<Ts *>(\
              &const_cast<char &>(reinterpret_cast<const volatile char &>(\
                  this->template getImpl<Idx>(*this)))))\
              Ts(static_cast<Ts &&>(\
                  const_cast<Ts &>(other.template getImpl<Idx>(other)))),\
          true))...});\
\
    this->theIndex = other.theIndex;\
    return *this;\
  }\
\
    template <\
      class T,\
      std::size_t theTypeIdx =\
          decltype(\
            SZ_VAR_USEALLIDXANDTYPES \
            theMethod(\
              szHelpMethods::myDeclval<T>(0), szHelpMethods::myDeclval<T>(0),\
              static_cast<szHelpMethods::hasBool<\
                  sizeof(szHelpMethods::isSame(\
                      static_cast<szHelpMethods::twoTypes<\
                          typename std::remove_cv_t<std::remove_reference_t<T>>,\
                          bool> *>(0))) == sizeof(std::size_t)> *>(0)))::theIdx,\
      class altType = SZ_VAR_GETIDXFROMTYPE(theTypeIdx, Ts...)>\
      \
  variantCpp20Impl &operator=(T &&other) noexcept(\
      std::is_nothrow_assignable_v<altType &, T>\
          &&std::is_nothrow_constructible_v<altType, T>) {\
    if (this->index() == theTypeIdx) {\
      const_cast<altType &>(this->template getImpl<theTypeIdx>(*this)) =\
          static_cast<T &&>(other);\
      return *this;\
    } else if constexpr (std::is_nothrow_constructible<altType, T>::value ||\
                         !std::is_nothrow_move_constructible_v<altType>) {\
      this->template emplace<theTypeIdx>(static_cast<T &&>(other));\
      return *this;\
    } else {\
      return *this = variantCpp20Impl{szLogVar::in_place_index_t<theTypeIdx>{},\
                                       static_cast<T &&>(other)};\
    }\
  }\
\
  /*Public methods*/\
  constexpr std::size_t index() const noexcept {\
    return valueless_by_exception() ? static_cast<std::size_t>(-1)\
                                    : this->theIndex;\
  }\
  constexpr bool valueless_by_exception() const noexcept {\
    return this->theIndex == static_cast<decltype(this->theIndex)>(-1);\
  }\
\
  template <size_t I, class... Args>\
      SZ_VAR_GETIDXFROMTYPE(I, Ts...) & emplace(Args &&...args) {\
    return theEmplaceHelper<I>(static_cast<Args &&>(args)...);\
  }\
  template <size_t I, class U, class... Args>\
      SZ_VAR_GETIDXFROMTYPE(I, Ts...) &\
      emplace(std::initializer_list<U> il, Args &&...args) {\
    return theEmplaceHelper<I>(static_cast<std::initializer_list<U> &&>(il),\
                               static_cast<Args &&>(args)...);\
  }\
  template <class T, class... Args> T &emplace(Args &&...args) {\
    return theEmplaceHelper<SZ_VAR_GIVENTYPEGETIDX(Ts...)>(\
        static_cast<Args &&>(args)...);\
  }\
  template <class T, class U, class... Args>\
  T &emplace(std::initializer_list<U> il, Args &&...args) {\
    return theEmplaceHelper<SZ_VAR_GIVENTYPEGETIDX(Ts...)>(\
        static_cast<std::initializer_list<U> &&>(il),\
        static_cast<Args &&>(args)...);\
  }\
  /*Friend fxn prototype*/\
\
  template <std::size_t I, class... theTypes>\
      friend constexpr const SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &\
      szLogVar::get(const szLogVar::variant<theTypes...> &param);\
\
  template <std::size_t I, class... theTypes>\
      friend constexpr const SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &\
      szLogVar::get_unchecked(const szLogVar::variant<theTypes...> &param);\
\
  template <std::size_t I, class... Types>\
  friend constexpr const SZ_VAR_GETIDXFROMTYPE(I, Types...)*\
  szLogVar::get_if(const szLogVar::variant<Types...> *pv) noexcept;\
\
  void swap(variantCpp20Impl &rhs) noexcept(\
      checkAnyFalse({(std::is_nothrow_move_constructible_v<Ts> &&\
                      std::is_nothrow_swappable_v<Ts>)...})) {\
    using std::swap;\
    if constexpr (checkAnyFalse(\
                      {std::is_trivially_move_assignable<Ts>::value...})) {\
      return swap(*this, rhs);\
    } else if (this->valueless_by_exception()) {\
      if (rhs.valueless_by_exception())\
        return; /*Both variants are valueless*/\
\
      static_cast<void>(szHelpMethods::expander<bool>{(\
          Idx == rhs.index() && (this->theEmplaceHelper<Idx>(static_cast<Ts &&>(\
                                     const_cast<Ts &>(rhs.getImpl<Idx>(rhs)))),\
                                 true))...});\
      return rhs.makeVariantValueless();\
    } else if (rhs.valueless_by_exception()) {\
      /*"this" isn't valueless*/\
      static_cast<void>(szHelpMethods::expander<bool>{\
          (Idx == this->index() &&\
           (rhs.theEmplaceHelper<Idx>(static_cast<Ts &&>(\
                const_cast<Ts &>(this->getImpl<Idx>(*this)))),\
            true))...});\
      this->makeVariantValueless(); /*Now it is*/\
      return;\
    }\
\
    if (this->index() == rhs.index()) {\
      static_cast<void>(szHelpMethods::expander<bool>{\
          (Idx == this->index() &&\
           (swap(const_cast<Ts &>(this->getImpl<Idx>(*this)),\
                 const_cast<Ts &>(rhs.getImpl<Idx>(rhs))),\
            true))...});\
    } else {\
      static_cast<void>(szHelpMethods::expander<\
                        bool>{(Idx == this->index() && [otherIdx = rhs.index()](\
                                                           const auto param,\
                                                           auto &refThis,\
                                                           auto &refRight) {\
        static_cast<void>(szHelpMethods::expander<bool>{(\
            Idx == otherIdx &&\
            [](auto &theRef, auto &rhsRef,\
               auto leftType, auto rightType \
/*Not sure why MSVC can't use param like gcc and clang. Capturing it like*/\
/*[param] doesn't work MSVC needs this theParam parameter*/\
\
SZ_VAR_AUTOPARAM_SWAP \
            ) {\
              theRef.template theEmplaceHelper<Idx>(\
                  static_cast<decltype(rightType) &&>(rightType));\
\
              rhsRef.template theEmplaceHelper<decltype(\
SZ_VAR_USEPARAMORTHEPARAM \
                  )::theIdx>(static_cast<decltype(leftType) &&>(leftType));\
\
              return true;\
            }(refThis, refRight,\
              static_cast<SZ_VAR_GETIDXFROMTYPE(decltype(param)::theIdx,\
                                                Ts...) &&>(\
                  const_cast<SZ_VAR_GETIDXFROMTYPE(decltype(param)::theIdx,\
                                                   Ts...) &>(\
                      refThis.template getImpl<decltype(param)::theIdx>(\
                          refThis))),\
              static_cast<Ts &&>(\
                  const_cast<Ts &>(refRight.template getImpl<Idx>(refRight)))\
SZ_VAR_COMMAPARAM \
              ))...});\
\
        return true;\
      }(szHelpMethods::getRightType<Ts, Idx>{}, *this, rhs))...});\
    }\
  }\
  /*Comparison operators as hidden friends*/\
  friend constexpr bool operator==(const variantCpp20Impl &lhs,\
                                   const variantCpp20Impl &rhs) {\
    if (lhs.index() != rhs.index())\
      return false;\
    if (lhs.valueless_by_exception())\
      return true;\
    bool theBool = false;\
\
    static_cast<void>(szHelpMethods::expander<bool>{\
        (Idx == lhs.index() &&\
         (theBool = (lhs.getImpl<Idx>(lhs) == rhs.getImpl<Idx>(rhs)),\
          true))...});\
    return theBool;\
  }\
  friend constexpr bool operator!=(const variantCpp20Impl &lhs,\
                                   const variantCpp20Impl &rhs) {\
    return !(lhs == rhs);\
  }\
  friend constexpr bool operator<(const variantCpp20Impl &lhs,\
                                  const variantCpp20Impl &rhs) {\
    if (rhs.valueless_by_exception()) {\
      return false;\
    }\
    if (lhs.valueless_by_exception()) {\
      return true;\
    }\
    if (lhs.index() < rhs.index()) {\
      return true;\
    }\
    if (lhs.index() > rhs.index()) {\
      return false;\
    }\
\
    bool theBool = false;\
    static_cast<void>(szHelpMethods::expander<bool>{(\
        Idx == lhs.index() &&\
        (theBool = (lhs.getImpl<Idx>(lhs) < rhs.getImpl<Idx>(rhs)), true))...});\
\
    return theBool;\
  }\
  friend constexpr bool operator>(const variantCpp20Impl &lhs,\
                                  const variantCpp20Impl &rhs) {\
    if (lhs.valueless_by_exception())\
      return false;\
    if (rhs.valueless_by_exception())\
      return true;\
    if (lhs.index() > rhs.index())\
      return true;\
    if (lhs.index() < rhs.index())\
      return false;\
\
    bool theBool = false;\
    static_cast<void>(szHelpMethods::expander<bool>{(\
        Idx == lhs.index() &&\
        (theBool = (lhs.getImpl<Idx>(lhs) > rhs.getImpl<Idx>(rhs)), true))...});\
\
    return theBool;\
  }\
  friend constexpr bool operator<=(const variantCpp20Impl &lhs,\
                                   const variantCpp20Impl &rhs) {\
    if (lhs.valueless_by_exception())\
      return true;\
    if (rhs.valueless_by_exception())\
      return false;\
    if (lhs.index() < rhs.index())\
      return true;\
    if (lhs.index() > rhs.index())\
      return false;\
\
    bool theBool = false;\
    static_cast<void>(szHelpMethods::expander<bool>{\
        (Idx == lhs.index() &&\
         (theBool = (lhs.getImpl<Idx>(lhs) <= rhs.getImpl<Idx>(rhs)),\
          true))...});\
\
    return theBool;\
  }\
  friend constexpr bool operator>=(const variantCpp20Impl &lhs,\
                                   const variantCpp20Impl &rhs) {\
    if (rhs.valueless_by_exception())\
      return true;\
    if (lhs.valueless_by_exception())\
      return false;\
    if (lhs.index() > rhs.index())\
      return true;\
    if (lhs.index() < rhs.index())\
      return false;\
\
    bool theBool = false;\
    static_cast<void>(szHelpMethods::expander<bool>{\
        (Idx == lhs.index() &&\
         (theBool = (lhs.getImpl<Idx>(lhs) >= rhs.getImpl<Idx>(rhs)),\
          true))...});\
    return theBool;\
  }\
  /*End comparison operators*/\
\
  VARIANTDTOR 

  #ifdef __clang__

template<std::size_t...Idx, class... Ts> class variantCpp20Impl<true, variantImpl::mySeqOfNum<Idx...>,Ts...>
SZ_VAR_VARIANTCPP20IMPL(,)
};

template<std::size_t...Idx, class... Ts> class variantCpp20Impl<false, variantImpl::mySeqOfNum<Idx...>,Ts...>
SZ_VAR_VARIANTCPP20IMPL(~treeNode(){},constexpr ~variantCpp20Impl() {\
      if (!this->valueless_by_exception()) {\
        SZ_VAR_DESTROYCURRENT\
      } } 
      )
};
#else
//For non clang, only needs 1 specialization:

template<std::size_t...Idx, class... Ts> class variantCpp20Impl<variantImpl::mySeqOfNum<Idx...>,Ts...>
    SZ_VAR_VARIANTCPP20IMPL(~treeNode() requires (allTrivDtable) = default; constexpr ~treeNode(){}, constexpr ~variantCpp20Impl() requires (allTrivDtable)= default; \
    constexpr ~variantCpp20Impl() {\
      if (!this->valueless_by_exception()) {\
        SZ_VAR_DESTROYCURRENT \
      }\
    }
    )
};

#undef SZ_VAR_VARIANTCPP20IMPL
#undef SZ_VAR_NOUSEALLIDXANDTYPES
#undef SZ_VAR_USINGOWNRIGHTTYPES
#undef SZ_VAR_USEALLIDXANDTYPES
#undef SZ_VARCPP20_FORCONVERTCTOR
#undef SZ_VAR_AUTOPARAM_SWAP
#undef SZ_VAR_USEPARAMORTHEPARAM
#undef SZ_VAR_COMMAPARAM

#endif
   
  //Non friend spaceship operator
  #if __has_include(<compare>) && SZ_VAR_CPPVERSION > 201703L
  template<
  #ifdef __clang__
  bool B,
  #endif
  std::size_t... Idx, class... Types >
  constexpr std::common_comparison_category_t<decltype(szHelpMethods::myDeclval<const Types&>(0) <=> szHelpMethods::myDeclval<const Types&>(0))...> operator<=>(const variantCpp20Impl<
  #ifdef __clang__
  B,
  #endif
  variantImpl::mySeqOfNum<Idx...>,Types...> & v, const variantCpp20Impl<
  #ifdef __clang__
  B,
  #endif
  variantImpl::mySeqOfNum<Idx...>,Types...>& w){
    if(v.valueless_by_exception()){
          if(w.valueless_by_exception()){
            return std::strong_ordering::equal;
          }
          return std::strong_ordering::less;
        } else if(w.valueless_by_exception()){
          return std::strong_ordering::greater;
        }
        if(v.index() != w.index()) return v.index() <=> w.index();
        std::common_comparison_category_t<decltype(szHelpMethods::myDeclval<const Types&>(0) <=> szHelpMethods::myDeclval<const Types&>(0))...> result;
        static_cast<void>(szHelpMethods::expander<bool>{
        (Idx == v.index() && (result = (szLogVar::get_unchecked<Idx>(v) <=> szLogVar::get_unchecked<Idx>(w)),true))...});
        return result;
  }
  #endif

#endif
} // namespace variantImpl
// Friend function definition

// These get functions does not check if I is equal to variant's index.
SZ_VAR_IDX_GETUNCHECKED(const, , &) { return param.template getImpl<I>(param); }
SZ_VAR_IDX_GETUNCHECKED(, , &) {
  return const_cast<SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &>(
      szLogVar::get_unchecked<I>(
          const_cast<const szLogVar::variant<theTypes...> &>(param)));
}
SZ_VAR_IDX_GETUNCHECKED(, , &&) {
  return static_cast<SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &&>(
      const_cast<SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &>(
          szLogVar::get_unchecked<I>(
              const_cast<const szLogVar::variant<theTypes...> &>(param))));
}
SZ_VAR_IDX_GETUNCHECKED(const, , &&) {
  return static_cast<const SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &&>(
      szLogVar::get_unchecked<I>(
          const_cast<const szLogVar::variant<theTypes...> &>(param)));
}
#define SZ_VAR_TYPE_GETUNCHECKED(MAYBECONST, AMPERSANDS)                       \
  template <class T, class... Types>                                           \
  constexpr MAYBECONST T AMPERSANDS get_unchecked(                             \
      MAYBECONST szLogVar::variant<Types...> AMPERSANDS param)

// template<class T, class... Types> constexpr const T& get_unchecked(const
// szLogVar::variant<Types...>& param)
SZ_VAR_TYPE_GETUNCHECKED(const, &) {
  return szLogVar::get_unchecked<SZ_VAR_GIVENTYPEGETIDX(Types...)>(param);
}
SZ_VAR_TYPE_GETUNCHECKED(, &) {
  return const_cast<T &>(
      szLogVar::get_unchecked<SZ_VAR_GIVENTYPEGETIDX(Types...)>(
          const_cast<const szLogVar::variant<Types...> &>(param)));
}
SZ_VAR_TYPE_GETUNCHECKED(, &&) {
  return static_cast<T &&>(
      const_cast<T &>(szLogVar::get_unchecked<SZ_VAR_GIVENTYPEGETIDX(Types...)>(
          const_cast<const szLogVar::variant<Types...> &>(param))));
}
SZ_VAR_TYPE_GETUNCHECKED(const, &&) {
  return static_cast<const T &&>(
      szLogVar::get_unchecked<SZ_VAR_GIVENTYPEGETIDX(Types...)>(
          const_cast<const szLogVar::variant<Types...> &>(param)));
}

SZ_VAR_GETFXN_DECLARATION(const, , &) {
  if (param.index() != I)
    throw szLogVar::bad_variant_access{};
  return param.template getImpl<I>(param);
}
SZ_VAR_GETFXN_DECLARATION(, , &) {
  return const_cast<SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &>(szLogVar::get<I>(
      const_cast<const szLogVar::variant<theTypes...> &>(param)));
}
SZ_VAR_GETFXN_DECLARATION(, , &&) {
  return static_cast<SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &&>(
      const_cast<SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &>(szLogVar::get<I>(
          const_cast<const szLogVar::variant<theTypes...> &>(param))));
}
SZ_VAR_GETFXN_DECLARATION(const, , &&) {
  return static_cast<const SZ_VAR_GETIDXFROMTYPE(I, theTypes...) &&>(
      szLogVar::get<I>(
          const_cast<const szLogVar::variant<theTypes...> &>(param)));
}

#define SZ_VAR_TYPE_GETFXN(MAYBECONST, AMPERSANDS)                             \
  template <class T, class... Types>                                           \
  constexpr MAYBECONST T AMPERSANDS get(                                       \
      MAYBECONST szLogVar::variant<Types...> AMPERSANDS param)

SZ_VAR_TYPE_GETFXN(, &) {
  return const_cast<T &>(szLogVar::get<SZ_VAR_GIVENTYPEGETIDX(Types...)>(
      const_cast<const szLogVar::variant<Types...> &>(param)));
}
SZ_VAR_TYPE_GETFXN(, &&) {
  return static_cast<T &&>(
      const_cast<T &>(szLogVar::get<SZ_VAR_GIVENTYPEGETIDX(Types...)>(
          const_cast<const szLogVar::variant<Types...> &>(param))));
}
SZ_VAR_TYPE_GETFXN(const, &) {
  return szLogVar::get<SZ_VAR_GIVENTYPEGETIDX(Types...)>(param);
}
SZ_VAR_TYPE_GETFXN(const, &&) {
  return static_cast<const T &&>(
      szLogVar::get<SZ_VAR_GIVENTYPEGETIDX(Types...)>(
          const_cast<const szLogVar::variant<Types...> &>(param)));
}

template <std::size_t I, class... Types>
constexpr const SZ_VAR_GETIDXFROMTYPE(I, Types...)*
get_if(const szLogVar::variant<Types...> *pv) noexcept {
  if (pv->index() != I)
    return 0;
  #if defined(__GNUC__) || defined( __clang__ )
  return __builtin_addressof(pv->template getImpl<I>(*pv));
  #else
  return std::addressof(pv->template getImpl<I>(*pv));
  #endif
}

template <std::size_t I, class... Types>
constexpr SZ_VAR_GETIDXFROMTYPE(I, Types...)*
get_if(szLogVar::variant<Types...> *pv) noexcept {
  return const_cast<SZ_VAR_GETIDXFROMTYPE(I, Types...)*>(
      szLogVar::get_if<I>(const_cast<const szLogVar::variant<Types...> *>(pv)));
}

template <class T, class... Types>
constexpr const T*
get_if(const szLogVar::variant<Types...> *pv) noexcept {
  return szLogVar::get_if<SZ_VAR_GIVENTYPEGETIDX(Types...)>(pv);
}
template <class T, class... Types>
constexpr T*
get_if(szLogVar::variant<Types...> *pv) noexcept {
  return const_cast<T*>(
      szLogVar::get_if<SZ_VAR_GIVENTYPEGETIDX(Types...)>(
          const_cast<const szLogVar::variant<Types...> *>(pv)));
}
template <class T, class... Types>
constexpr bool
holds_alternative(const szLogVar::variant<Types...> &v) noexcept {
  return SZ_VAR_GIVENTYPEGETIDX(Types...) == v.index();
}

template <class... Types>
void swap(szLogVar::variant<Types...> &lhs,
          szLogVar::variant<Types...> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
  return lhs.swap(rhs);
}

// Visit function
namespace visitHelper {
#if SZ_VAR_CPPVERSION <= 201703L
template <class T, class U, class... Args>
szLogVar::in_place_index_t<1> overloadInvoke(decltype(static_cast<void>(
    (szHelpMethods::myDeclval<U>(0).*
     szHelpMethods::myDeclval<T>(0))(szHelpMethods::myDeclval<Args>(0)...))) *);

template <class T, class U, class... Args>
szLogVar::in_place_index_t<2> overloadInvoke(decltype(static_cast<void>(
    (szHelpMethods::myDeclval<U>(0).get().*
     szHelpMethods::myDeclval<T>(0))(szHelpMethods::myDeclval<Args>(0)...))) *);

template <class T, class U, class... Args>
szLogVar::in_place_index_t<3> overloadInvoke(decltype(static_cast<void>(
    (*(szHelpMethods::myDeclval<U>(0)).*
     szHelpMethods::myDeclval<T>(0))(szHelpMethods::myDeclval<Args>(0)...))) *);
// template<class T, class U, class... Args> szLogVar::in_place_index_t<3>
// overloadInvoke(...);

template <class T, class U>
szLogVar::in_place_index_t<4>
overloadInvoke(decltype(static_cast<void>(szHelpMethods::myDeclval<U>(0).*
                                          szHelpMethods::myDeclval<T>(0))) *);

template <class T, class U>
szLogVar::in_place_index_t<5>
overloadInvoke(decltype(static_cast<void>(szHelpMethods::myDeclval<U>(0).get().*
                                          szHelpMethods::myDeclval<T>(0))) *);
#else
template <class T, class U, class... Args>
szLogVar::in_place_index_t<1> overloadInvoke(T f, U&& t1, Args&&... args) requires requires{
  (static_cast<U&&>(t1).*f )(static_cast<Args&&>(args)...);
};

template<class T, class U, class... Args> 
szLogVar::in_place_index_t<2> overloadInvoke(T f, U&& t1, Args&&... args) requires requires{
  (static_cast<U&&>(t1).get().*f)(static_cast<Args&&>(args)...);
};

template<class T, class U, class... Args> 
szLogVar::in_place_index_t<3> overloadInvoke(T f, U&& t1, Args&&... args) requires requires{
  ((*static_cast<U&&>(t1)).*f)(static_cast<Args&&>(args)...);
};

template<class T, class U> szLogVar::in_place_index_t<4> overloadInvoke(T f, U&& t1) requires requires{
  static_cast<U&&>(t1).*f;
};

template<class T, class U> szLogVar::in_place_index_t<5> overloadInvoke(T f, U&& t1) requires requires{
  static_cast<U&&>(t1).get().*f;
};
#endif

template <class T, class U> szLogVar::in_place_index_t<6> overloadInvoke(...);

template <class T, class Type, class T1, class... Args>
constexpr decltype(auto) invoke(Type T::*f, T1 &&t1, Args &&...args) {
  constexpr auto chooseFxn =
      decltype(overloadInvoke
      #if SZ_VAR_CPPVERSION <= 201703L
      <decltype(f), T1, Args...>(0)
      #else
      (f, static_cast<T1&&>(t1), static_cast<Args&&>(args)...)
      #endif
      )::theIdx;
  if constexpr (chooseFxn == 1) {
    return (static_cast<T1 &&>(t1).*f)(static_cast<Args &&>(args)...);
  } else if constexpr (chooseFxn == 2) {
    return (t1.get().*f)(static_cast<Args &&>(args)...);
  } else if constexpr (chooseFxn == 3) {
    return ((*static_cast<T1 &&>(t1)).*f)(static_cast<Args &&>(args)...);
  } else if constexpr (chooseFxn == 4) {
    return static_cast<T1 &&>(t1).*f;
  } else if constexpr (chooseFxn == 5) {
    return t1.get().*f;
  } else {
    return (*static_cast<T1 &&>(t1)).*f;
  }
}

template <class F, class... Args>
constexpr decltype(auto) invoke(F &&f, Args &&...args) {
  return static_cast<F &&>(f)(static_cast<Args &&>(args)...);
}

//Used to change the whichVariantIdx'th number into "numToInsert"
template<std::size_t whichVariantIdx, std::size_t numToInsert, std::size_t... Ns, std::size_t... Indices>
variantImpl::mySeqOfNum<(Ns == whichVariantIdx ? numToInsert : Indices)...> 
changedIdxSeq(variantImpl::mySeqOfNum<Ns...>, variantImpl::mySeqOfNum<Indices...>);


// https://en.wikipedia.org/wiki/Binary_search_algorithm#Alternative_procedure
template <std::size_t I, class T> struct containsReference {
  T theRef;
  constexpr containsReference(T &&param) : theRef{static_cast<T &&>(param)} {}
};

template <class...> struct simpleTupleReferences;

template <class F, std::size_t... Ns, class... Ts>
struct simpleTupleReferences<F, variantImpl::mySeqOfNum<Ns...>, Ts...>
    : containsReference<Ns, Ts>... {

  //static constexpr std::size_t theNumTypes[1 + sizeof...(Ts)]{(szHelpMethods::aliasForCVRef<Ts>::numTypes - 1)...};
  std::size_t const *theIdxPtr;
  F theF;

  template <class F_Arg, class... Args>
  constexpr simpleTupleReferences(std::size_t const *ptrParam, F_Arg &&paramF,
                                  Args &&...args)
      : containsReference<Ns, Ts>{static_cast<Args &&>(args)}...,
        theIdxPtr{ptrParam}, theF{static_cast<F_Arg &&>(paramF)} {}

  template <std::size_t whichVariantIdx, std::size_t LowerBound,
            std::size_t... Indices>
  constexpr decltype(invoke(
      static_cast<F &&>(theF),
      szLogVar::get_unchecked<0>(szHelpMethods::myDeclval<Ts>(0))...))
  visitImpl() {
    constexpr std::size_t theUpperBound =
        szHelpMethods::expander<std::size_t>{Indices...}
        [whichVariantIdx];

    if constexpr (LowerBound == theUpperBound) {
      if constexpr (whichVariantIdx == (sizeof...(Ts) - 1)) {
        return invoke(
            static_cast<F &&>(theF),
            szLogVar::get_unchecked<Indices>(static_cast<Ts &&>(
                static_cast<containsReference<Ns, Ts> &>(*this).theRef))...);
      } else {
        ++theIdxPtr;
        return visitImpl<whichVariantIdx + 1, 0,
                         (Ns == whichVariantIdx ? LowerBound : Indices)...>();
      }
    } else {
      constexpr std::size_t ceiling = ((LowerBound + theUpperBound) / 2) +
                                      ((LowerBound & 1) ^ (theUpperBound & 1));
      if (ceiling > *theIdxPtr) {
        return visitImpl<whichVariantIdx, LowerBound,
                         (Ns == whichVariantIdx ? ceiling - 1 : Indices)...>();
      } else {
        return visitImpl<whichVariantIdx, ceiling,
                         (Ns == whichVariantIdx ? theUpperBound
                                                : Indices)...>();
      }
    }
  }

  //Linear search for elements when visit's return type is void. Allows for less template instantiation recursion depth
  template<std::size_t whichVariantIdx, std::size_t... varAltIdx, std::size_t... Indices>
  constexpr void visitDiscardReturn(variantImpl::mySeqOfNum<varAltIdx...>,otherIdxSeq::index_sequence<Indices...>*){
    if constexpr(whichVariantIdx == sizeof...(Ts)){ //Pretty sure it's not sizeof...(Ts) - 1
      return invoke(
            static_cast<F &&>(theF),
            szLogVar::get_unchecked<Indices>(static_cast<Ts &&>(
                static_cast<containsReference<Ns, Ts> &>(*this).theRef))...);
    } else {
      static_cast<void>(szHelpMethods::expander<bool>{
        ((varAltIdx == *theIdxPtr) && ( ++theIdxPtr,visitDiscardReturn<whichVariantIdx + 1>(
          variantImpl::theIndexSeq<szHelpMethods::expander<std::size_t>{Indices...,0}[whichVariantIdx + 1]>{},
          decltype(changedIdxSeq<whichVariantIdx,varAltIdx>(variantImpl::mySeqOfNum<Ns...>{},static_cast<otherIdxSeq::index_sequence<Indices...>*>(0) ) ){}
        ),true) )
      ...});
    }
  }

};

} // namespace visitHelper
// Set checkIfValueless to false if user knows variants will never be valueless
template <bool checkIfValueless = true, class Visitor, class... Variants>
constexpr decltype(auto) visit(Visitor &&vis,
                               Variants &&...vars) noexcept(!checkIfValueless) {
  #define SZ_VAR_INVOKEBODY(RETURNTYPE) if constexpr (sizeof...(vars) != 0) {\
    if constexpr (checkIfValueless) {\
      if (!variantImpl::checkAnyFalse({!vars.valueless_by_exception()...}))\
        throw szLogVar::bad_variant_access{};\
    }\
    const std::size_t theVariantIndices[]{vars.index()...};\
    if constexpr(sizeof(szHelpMethods::isVoid<RETURNTYPE>(0)) == sizeof(bool)){\
      /*Not void*/\
      return visitHelper::simpleTupleReferences<\
               Visitor &&, variantImpl::theIndexSeq<sizeof...(Variants)>,\
               Variants &&...>{theVariantIndices, static_cast<Visitor &&>(vis),\
                               static_cast<Variants &&>(vars)...}\
        .template visitImpl<\
            0, 0, (szHelpMethods::aliasForCVRef<Variants>::numTypes - 1)...>();\
    } else { /*Return type is void*/\
      visitHelper::simpleTupleReferences<\
               Visitor &&, variantImpl::theIndexSeq<sizeof...(Variants)>,\
               Variants &&...>{theVariantIndices, static_cast<Visitor &&>(vis),\
                               static_cast<Variants &&>(vars)...}\
        .template visitDiscardReturn<0>(\
          variantImpl::theIndexSeq<szHelpMethods::expander<std::size_t>{szHelpMethods::aliasForCVRef<Variants>::numTypes...}[0]>{},\
          variantImpl::mySeqOfNum<szHelpMethods::aliasForCVRef<Variants>::numTypes...>{}\
          );\
    }\
  } else {\
    return static_cast<Visitor &&>(vis)();\
  }

  SZ_VAR_INVOKEBODY(decltype(visitHelper::invoke(static_cast<Visitor&&>(vis), szLogVar::get_unchecked<0>(static_cast<Variants&&>(vars))...)))
}

#if SZ_VAR_CPPVERSION > 201703L
template <class R, bool checkIfValueless = true, class Visitor, class... Variants>
constexpr R visit( Visitor&& vis, Variants&&... vars ){
  SZ_VAR_INVOKEBODY(R)
}
#endif
#undef SZ_VAR_INVOKEBODY

namespace hashHelper {
template <class T> std::size_t isHashable(decltype(std::hash<T>{}) *);
template <class T> bool isHashable(...);

template <class... Ts> struct hasHash;

#if SZ_VAR_CPPVERSION <= 201703L
template <bool B, std::size_t... Is, class... Ts>
struct hasHash<szLogVar::variantImpl::variantMoveAssign<
    B, szLogVar::variantImpl::mySeqOfNum<Is...>, Ts...>> {
  std::size_t
  operator()(const szLogVar::variantImpl::variantMoveAssign<
             B, szLogVar::variantImpl::mySeqOfNum<Is...>, Ts...> &param) const {
    return (param.valueless_by_exception()
                ? 87654321 // Arbitrary magic number for valueless
                : szLogVar::szHelpMethods::expander<std::size_t>{(
                      Is == param.index()
                          ? std::hash<Ts>{}(szLogVar::get_unchecked<Is>(param))
                          : 0)...}[param.index()] +
                      param.index());
  }  
};
#else
template <
#ifdef __clang__
bool B,
#endif
std::size_t... Is, class... Ts>
struct hasHash<szLogVar::variantImpl::variantCpp20Impl<
#ifdef __clang__
B,
#endif
    szLogVar::variantImpl::mySeqOfNum<Is...>, Ts...>> {
  std::size_t
  operator()(const szLogVar::variantImpl::variantCpp20Impl<
        #ifdef __clang__
      B,
      #endif
             szLogVar::variantImpl::mySeqOfNum<Is...>, Ts...> &param) const {
    return (param.valueless_by_exception()
                ? 87654321 // Arbitrary magic number for valueless
                : szLogVar::szHelpMethods::expander<std::size_t>{(
                      Is == param.index()
                          ? std::hash<Ts>{}(szLogVar::get_unchecked<Is>(param))
                          : 0)...}[param.index()] +
                      param.index());
  }  
};
#endif

struct noHash {
  noHash() = delete;
  noHash(const noHash &) = delete;
  noHash(noHash &&) = delete;
  noHash &operator=(const noHash &) = delete;
  noHash &operator=(noHash &&) = delete;
};
} // namespace hashHelper

#undef SZ_VAR_GIVENTYPEGETIDX
#undef SZ_VAR_GETFXN_DECLARATION
#undef SZ_VAR_HASTYPEPACKELEM
#undef SZ_VAR_IDX_GETUNCHECKED
#undef SZ_VAR_TYPE_GETUNCHECKED
#undef SZ_VAR_TYPE_GETFXN
#undef SZ_VAR_GETIDXFROMTYPE
#undef SZ_VAR_THEUNIONBASE
#undef SZ_VAR_FORCONVERTCTOR
#undef SZ_VAR_DESTROYCURRENT
#undef SZ_VAR_CPPVERSION
} // namespace szLogVar

namespace std {
template <> struct hash<szLogVar::monostate> {
  std::size_t operator()(szLogVar::monostate) const {
    return 12345678; // Arbitrary magic number
  }
};

template<class... Ts>
struct hash<szLogVar::variant<Ts...>> : decltype(szLogVar::szHelpMethods::getTypeMethod<
               szLogVar::hashHelper::hasHash<szLogVar::variant<Ts...>>,
               szLogVar::hashHelper::noHash>(
          static_cast<szLogVar::szHelpMethods::hasBool<
              szLogVar::variantImpl::checkAnyFalse(
                  {(sizeof(szLogVar::hashHelper::isHashable<Ts>(0)) ==
                    sizeof(std::size_t))...})> *>(0))) {};

//variantCpp20Impl<variantImpl::mySeqOfNum<Idx...>,Ts...>
} // namespace std
#endif