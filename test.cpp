//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <type_traits>

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <range/v3/utility/variant.hpp>

#define STATIC_ASSERT(...) static_assert((__VA_ARGS__), #__VA_ARGS__)
#define ASSERT_NOEXCEPT(...) STATIC_ASSERT(noexcept(__VA_ARGS__))
#define ASSERT_NOT_NOEXCEPT(...) STATIC_ASSERT(!noexcept(__VA_ARGS__))
#define ASSERT_SAME_TYPE(T, U) STATIC_ASSERT(is_same_v<T, U>)

namespace {
    template<class T>
    constexpr bool is_swappable_v = ::ranges::Swappable<T&, T&>::value;
    template<class T>
    constexpr bool is_nothrow_swappable_v = ranges::is_nothrow_swappable<T&, T&>::value;
    template<class T, class U>
    constexpr bool is_same_v = std::is_same<T, U>::value;
    template<class...>
    using void_t = void;

    constexpr std::nullptr_t native_nullptr{};

    constexpr int PM_TEST_PASS = 0;
    constexpr int PM_TEST_FAIL = 1;

    int test_status = PM_TEST_PASS;

    void fail(const char* file, int line, const char* func, const char* cond)
    {
        std::fprintf(stderr, "%s(%d): in %s: assertion \"%s\" failed.\n", file, line, func, cond);
        test_status = PM_TEST_FAIL;
    }

#define CHECK(...) (!(__VA_ARGS__) ? fail(__FILE__, __LINE__, __FUNCTION__, #__VA_ARGS__) : void())

#ifndef TEST_HAS_NO_EXCEPTIONS
    struct CopyThrows {
        CopyThrows() = default;
        CopyThrows(CopyThrows const&) { throw 42; }
        CopyThrows& operator=(CopyThrows const&) { throw 42; }
    };

    struct MoveThrows {
        static int alive;
        MoveThrows() { ++alive; }
        MoveThrows(MoveThrows const&) {++alive;}
        MoveThrows(MoveThrows&&) {  throw 42; }
        MoveThrows& operator=(MoveThrows const&) { return *this; }
        MoveThrows& operator=(MoveThrows&&) { throw 42; }
        ~MoveThrows() { --alive; }
    };

    int MoveThrows::alive = 0;

    struct MakeEmptyT {
        static int alive;
        MakeEmptyT() { ++alive; }
        MakeEmptyT(MakeEmptyT const&) {
            ++alive;
            // Don't throw from the copy constructor since variant's assignment
            // operator performs a copy before committing to the assignment.
        }
        MakeEmptyT(MakeEmptyT &&) {
            throw 42;
        }
        MakeEmptyT& operator=(MakeEmptyT const&) {
            throw 42;
        }
        MakeEmptyT& operator=(MakeEmptyT&&) {
            throw 42;
        }
        ~MakeEmptyT() { --alive; }
    };
    STATIC_ASSERT(is_swappable_v<MakeEmptyT>); // required for test

    int MakeEmptyT::alive = 0;

    template <class Variant>
    void makeEmpty(Variant& v) {
        Variant v2(ranges::in_place<MakeEmptyT>);
        try {
            v = v2;
            CHECK(false);
        }  catch (...) {
            CHECK(v.valueless_by_exception());
        }
    }
#endif // TEST_HAS_NO_EXCEPTIONS

    // TypeID - Represent a unique identifier for a type. TypeID allows equality
    // comparisons between different types.
    struct TypeID {
        friend bool operator==(TypeID const& LHS, TypeID const& RHS)
        {return LHS.m_id == RHS.m_id; }
#if 0 // UNUSED
        friend bool operator!=(TypeID const& LHS, TypeID const& RHS)
        {return LHS.m_id != RHS.m_id; }
#endif // UNUSED
        private:
        explicit constexpr TypeID(const int* xid) : m_id(xid) {}

        TypeID(const TypeID&) = delete;
        TypeID& operator=(TypeID const&) = delete;

        const int* const m_id;
        template <class T> friend TypeID const& makeTypeID();
    };

    // makeTypeID - Return the TypeID for the specified type 'T'.
    template <class T>
    inline TypeID const& makeTypeID() {
        static int dummy;
        static const TypeID id(&dummy);
        return id;
    }

    template <class ...Args>
    struct ArgumentListID {};

    // makeArgumentID - Create and return a unique identifier for a given set
    // of arguments.
    template <class ...Args>
    inline TypeID const& makeArgumentID() {
        return makeTypeID<ArgumentListID<Args...>>();
    }

    // There are two forms of uses-allocator construction:
    //   (1) UA_AllocArg: 'T(allocator_arg_t, Alloc const&, Args&&...)'
    //   (2) UA_AllocLast: 'T(Args&&..., Alloc const&)'
    // 'UA_None' represents non-uses allocator construction.
    enum class UsesAllocatorType {
        UA_None = 0,
        UA_AllocArg = 2,
        UA_AllocLast = 4
    };
    constexpr UsesAllocatorType UA_None = UsesAllocatorType::UA_None;
    constexpr UsesAllocatorType UA_AllocArg = UsesAllocatorType::UA_AllocArg;
    constexpr UsesAllocatorType UA_AllocLast = UsesAllocatorType::UA_AllocLast;

    template <class Alloc, std::size_t N>
    class UsesAllocatorV1;
        // Implements form (1) of uses-allocator construction from the specified
        // 'Alloc' type and exactly 'N' additional arguments. It also provides
        // non-uses allocator construction from 'N' arguments. This test type
        // blows up when form (2) of uses-allocator is even considered.

    template <class Alloc, std::size_t N>
    class UsesAllocatorV2;
        // Implements form (2) of uses-allocator construction from the specified
        // 'Alloc' type and exactly 'N' additional arguments. It also provides
        // non-uses allocator construction from 'N' arguments.

    template <class Alloc, std::size_t N>
    class UsesAllocatorV3;
        // Implements both form (1) and (2) of uses-allocator construction from
        // the specified 'Alloc' type and exactly 'N' additional arguments. It also
        // provides non-uses allocator construction from 'N' arguments.

    template <class Alloc, std::size_t>
    class NotUsesAllocator;
        // Implements both form (1) and (2) of uses-allocator construction from
        // the specified 'Alloc' type and exactly 'N' additional arguments. It also
        // provides non-uses allocator construction from 'N' arguments. However
        // 'NotUsesAllocator' never provides a 'allocator_type' typedef so it is
        // never automatically uses-allocator constructed.


    template <class ...ArgTypes, class TestType>
    bool checkConstruct(TestType& value, UsesAllocatorType form,
                        typename TestType::CtorAlloc const& alloc)
        // Check that 'value' was constructed using the specified 'form' of
        // construction and with the specified 'ArgTypes...'. Additionally
        // check that 'value' was constructed using the specified 'alloc'.
    {
        if (form == UA_None) {
            return value.template checkConstruct<ArgTypes&&...>(form);
        } else {
            return value.template checkConstruct<ArgTypes&&...>(form, alloc);
        }
    }

    template <class ...ArgTypes, class TestType>
    bool checkConstruct(TestType& value, UsesAllocatorType form) {
        return value.template checkConstruct<ArgTypes&&...>(form);
    }

    template <class TestType>
    bool checkConstructionEquiv(TestType& T, TestType& U)
        // check that 'T' and 'U' where initialized in the exact same manner.
    {
        return T.checkConstructEquiv(U);
    }

    ////////////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <bool IsZero, size_t N, class ArgList, class ...Args>
        struct TakeNImp;

        template <class ArgList, class ...Args>
        struct TakeNImp<true, 0, ArgList, Args...> {
            typedef ArgList type;
        };

        template <size_t N, class ...A1, class F, class ...R>
        struct TakeNImp<false, N, ArgumentListID<A1...>, F, R...>
            : TakeNImp<N-1 == 0, N - 1, ArgumentListID<A1..., F>, R...> {};

        template <size_t N, class ...Args>
        struct TakeNArgs : TakeNImp<N == 0, N, ArgumentListID<>, Args...> {};

        template <class T>
        struct Identity { typedef T type; };

        template <class T>
        using IdentityT = typename Identity<T>::type;

        template <bool Value>
        using EnableIfB = typename std::enable_if<Value, bool>::type;

    } // end namespace detail

    using detail::EnableIfB;

    struct AllocLastTag {};

    template <class Alloc>
    struct UsesAllocatorTestBase {
    public:
        using CtorAlloc = /*typename std::conditional<
            std::is_same<Alloc, std::experimental::erased_type>::value,
            std::experimental::pmr::memory_resource*,*/
            Alloc
        /*>::type*/;

        template <class ...ArgTypes>
        bool checkConstruct(UsesAllocatorType expectType) const {
            return expectType == constructor_called &&
                makeArgumentID<ArgTypes...>() == *args_id;
        }

        template <class ...ArgTypes>
        bool checkConstruct(UsesAllocatorType expectType,
                            CtorAlloc const& expectAlloc) const {
            return expectType == constructor_called &&
                makeArgumentID<ArgTypes...>() == *args_id &&
                expectAlloc == allocator;
        }

        bool checkConstructEquiv(UsesAllocatorTestBase& O) const {
            return constructor_called == O.constructor_called
                && *args_id == *O.args_id
                && allocator == O.allocator;
        }

    protected:
        explicit UsesAllocatorTestBase(const TypeID* aid)
            : args_id(aid), constructor_called(UA_None), allocator()
        {}

        template <class ...Args>
        UsesAllocatorTestBase(std::allocator_arg_t, CtorAlloc const& a, Args&&...)
            : args_id(&makeArgumentID<Args&&...>()),
            constructor_called(UA_AllocArg),
            allocator(a)
        {}

        template <class ...Args, class ArgsIDL = detail::TakeNArgs<sizeof...(Args) - 1, Args&&...>>
        UsesAllocatorTestBase(AllocLastTag, Args&&... args)
            : args_id(&makeTypeID<typename ArgsIDL::type>()),
            constructor_called(UA_AllocLast),
            allocator(getAllocatorFromPack(
                typename ArgsIDL::type{},
                std::forward<Args>(args)...))
        {
        }

    private:
        template <class ...LArgs, class ...Args>
        static CtorAlloc getAllocatorFromPack(ArgumentListID<LArgs...>, Args&&... args) {
            return getAllocatorFromPackImp<LArgs const&...>(args...);
        }

        template <class ...LArgs>
        static CtorAlloc getAllocatorFromPackImp(
            typename detail::Identity<LArgs>::type..., CtorAlloc const& alloc) {
            return alloc;
        }
    public:
        const TypeID* args_id;
        UsesAllocatorType constructor_called = UA_None;
        CtorAlloc allocator;
    };

    template <class Alloc, size_t Arity>
    class UsesAllocatorV1 : public UsesAllocatorTestBase<Alloc>
    {
    public:
        typedef Alloc allocator_type;

        using Base = UsesAllocatorTestBase<Alloc>;
        using CtorAlloc = typename Base::CtorAlloc;

        UsesAllocatorV1() : Base(&makeArgumentID<>()) {}

        UsesAllocatorV1(UsesAllocatorV1 const&)
            : Base(&makeArgumentID<UsesAllocatorV1 const&>()) {}
        UsesAllocatorV1(UsesAllocatorV1 &&)
            : Base(&makeArgumentID<UsesAllocatorV1 &&>()) {}
        // Non-Uses Allocator Ctor
        template <class ...Args, EnableIfB<sizeof...(Args) == Arity> = false>
        UsesAllocatorV1(Args&&...) : Base(&makeArgumentID<Args&&...>()) {}

        // Uses Allocator Arg Ctor
        template <class ...Args>
        UsesAllocatorV1(std::allocator_arg_t tag, CtorAlloc const & a, Args&&... args)
            : Base(tag, a, std::forward<Args>(args)...)
        { }

        // BLOWS UP: Uses Allocator Last Ctor
        template <class First, class ...Args, EnableIfB<sizeof...(Args) == Arity> Dummy = false>
        UsesAllocatorV1(First&&, Args&&...)
        {
            STATIC_ASSERT(!std::is_same<First, First>::value);
        }
    };


    template <class Alloc, size_t Arity>
    class UsesAllocatorV2 : public UsesAllocatorTestBase<Alloc>
    {
    public:
        typedef Alloc allocator_type;

        using Base = UsesAllocatorTestBase<Alloc>;
        using CtorAlloc = typename Base::CtorAlloc;

        UsesAllocatorV2() : Base(&makeArgumentID<>()) {}
        UsesAllocatorV2(UsesAllocatorV2 const&)
            : Base(&makeArgumentID<UsesAllocatorV2 const&>()) {}
        UsesAllocatorV2(UsesAllocatorV2 &&)
            : Base(&makeArgumentID<UsesAllocatorV2 &&>()) {}

        // Non-Uses Allocator Ctor
        template <class ...Args, EnableIfB<sizeof...(Args) == Arity> = false>
        UsesAllocatorV2(Args&&...) : Base(&makeArgumentID<Args&&...>()) {}

        // Uses Allocator Last Ctor
        template <class ...Args, EnableIfB<sizeof...(Args) == Arity + 1> = false>
        UsesAllocatorV2(Args&&... args)
            : Base(AllocLastTag{}, std::forward<Args>(args)...)
        {}
    };

    template <class Alloc, size_t Arity>
    class UsesAllocatorV3 : public UsesAllocatorTestBase<Alloc>
    {
    public:
        typedef Alloc allocator_type;

        using Base = UsesAllocatorTestBase<Alloc>;
        using CtorAlloc = typename Base::CtorAlloc;

        UsesAllocatorV3() : Base(&makeArgumentID<>()) {}
        UsesAllocatorV3(UsesAllocatorV3 const&)
            : Base(&makeArgumentID<UsesAllocatorV3 const&>()) {}
        UsesAllocatorV3(UsesAllocatorV3 &&)
            : Base(&makeArgumentID<UsesAllocatorV3 &&>()) {}

        // Non-Uses Allocator Ctor
        template <class ...Args, EnableIfB<sizeof...(Args) == Arity> = false>
        UsesAllocatorV3(Args&&...) : Base(&makeArgumentID<Args&&...>()) {}

        // Uses Allocator Arg Ctor
        template <class ...Args>
        UsesAllocatorV3(std::allocator_arg_t tag, CtorAlloc const& alloc, Args&&... args)
            : Base(tag, alloc, std::forward<Args>(args)...)
        {}

        // Uses Allocator Last Ctor
        template <class ...Args, EnableIfB<sizeof...(Args) == Arity + 1> = false>
        UsesAllocatorV3(Args&&... args)
            : Base(AllocLastTag{}, std::forward<Args>(args)...)
        {}
    };

    template <class Alloc, size_t Arity>
    class NotUsesAllocator : public UsesAllocatorTestBase<Alloc>
    {
    public:
        // no allocator_type typedef provided

        using Base = UsesAllocatorTestBase<Alloc>;
        using CtorAlloc = typename Base::CtorAlloc;

        NotUsesAllocator() : Base(&makeArgumentID<>()) {}
        NotUsesAllocator(NotUsesAllocator const&)
            : Base(&makeArgumentID<NotUsesAllocator const&>()) {}
        NotUsesAllocator(NotUsesAllocator &&)
            : Base(&makeArgumentID<NotUsesAllocator &&>()) {}
        // Non-Uses Allocator Ctor
        template <class ...Args, EnableIfB<sizeof...(Args) == Arity> = false>
        NotUsesAllocator(Args&&...) : Base(&makeArgumentID<Args&&...>()) {}

        // Uses Allocator Arg Ctor
        template <class ...Args>
        NotUsesAllocator(std::allocator_arg_t tag, CtorAlloc const& alloc, Args&&... args)
            : Base(tag, alloc, std::forward<Args>(args)...)
        {}

        // Uses Allocator Last Ctor
        template <class ...Args, EnableIfB<sizeof...(Args) == Arity + 1> = false>
        NotUsesAllocator(Args&&... args)
            : Base(AllocLastTag{}, std::forward<Args>(args)...)
        {}
    };

    template <class T>
    struct non_allocator
    {
        using value_type = T;
        using propagate_on_container_move_assignment = std::true_type;
        using is_always_equal = std::true_type;

        non_allocator() = default;
        template <class U>
        constexpr non_allocator(non_allocator<U>) noexcept {}

        T* allocate(std::size_t) { throw std::bad_alloc(); }
        void deallocate(T*) noexcept {}
    };

    template <class T, class U>
    constexpr bool operator==(non_allocator<T>, non_allocator<U>) { return true; }
    template <class T, class U>
    constexpr bool operator!=(non_allocator<T>, non_allocator<U>) { return false; }

    namespace detail {
        template <class Tp> void eat_type(Tp);

        template <class Tp, class ...Args>
        constexpr auto test_convertible_imp(int)
            -> decltype(eat_type<Tp>({std::declval<Args>()...}), true)
        { return true; }

        template <class Tp, class ...Args>
        constexpr auto test_convertible_imp(long) -> bool { return false; }
    }

    template <class Tp, class ...Args>
    constexpr bool test_convertible()
    { return detail::test_convertible_imp<Tp, Args...>(0); }

    namespace bad_variant_access {
        void run_test()
        {
            STATIC_ASSERT(std::is_base_of<std::exception, ranges::bad_variant_access>::value);
            STATIC_ASSERT(noexcept(ranges::bad_variant_access{}));
            STATIC_ASSERT(noexcept(ranges::bad_variant_access{}.what()));
            ranges::bad_variant_access ex;
            CHECK(ex.what() != native_nullptr);
        }
    }

    namespace get {
        namespace if_index {
            void test_const_get_if() {
                {
                    using V = ranges::variant<int>;
                    constexpr const V* v = native_nullptr;
                    STATIC_ASSERT(ranges::get_if<0>(v) == native_nullptr);
                    (void)v;
                }
                {
                    using V = ranges::variant<int, long>;
                    constexpr V v(42);
                    ASSERT_NOEXCEPT(ranges::get_if<0>(&v));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<0>(&v)), int const*);
                    STATIC_ASSERT(*ranges::get_if<0>(&v) == 42);
                    STATIC_ASSERT(ranges::get_if<1>(&v) == native_nullptr);
                }
                {
                    using V = ranges::variant<int, long>;
                    constexpr V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<1>(&v)), long const*);
                    STATIC_ASSERT(*ranges::get_if<1>(&v) == 42);
                    STATIC_ASSERT(ranges::get_if<0>(&v) == native_nullptr);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    const V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<0>(std::addressof(v))), int*);
                    CHECK(ranges::get_if<0>(std::addressof(v)) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<0>(std::addressof(v))), int*);
                    CHECK(ranges::get_if<0>(std::addressof(v)) == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<0>(std::addressof(v))), const int*);
                    CHECK(ranges::get_if<0>(std::addressof(v)) == &x);
                }
            }

            void test_get_if()
            {
                {
                    using V = ranges::variant<int>;
                    V* v = native_nullptr;
                    CHECK(ranges::get_if<0>(v) == native_nullptr);
                }
                {
                    using V = ranges::variant<int, long>;
                    V v(42);
                    ASSERT_NOEXCEPT(ranges::get_if<0>(std::addressof(v)));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<0>(std::addressof(v))), int*);
                    CHECK(*ranges::get_if<0>(std::addressof(v)) == 42);
                    CHECK(ranges::get_if<1>(std::addressof(v)) == native_nullptr);
                }
                {
                    using V = ranges::variant<int, long>;
                    V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<1>(std::addressof(v))), long*);
                    CHECK(*ranges::get_if<1>(std::addressof(v)) == 42);
                    CHECK(ranges::get_if<0>(std::addressof(v)) == native_nullptr);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<0>(std::addressof(v))), int*);
                    CHECK(ranges::get_if<0>(std::addressof(v)) == &x);
                }
                {
                    using V = ranges::variant<const int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<0>(std::addressof(v))), const int*);
                    CHECK(ranges::get_if<0>(std::addressof(v)) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<0>(std::addressof(v))), int*);
                    CHECK(ranges::get_if<0>(std::addressof(v)) == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<0>(std::addressof(v))), const int*);
                    CHECK(ranges::get_if<0>(std::addressof(v)) == &x);
                }
            }

            void run_test()
            {
                test_const_get_if();
                test_get_if();
            }
        }

        namespace if_type {
            void test_const_get_if() {
                {
                    using V = ranges::variant<int>;
                    constexpr const V* v = native_nullptr;
                    STATIC_ASSERT(ranges::get_if<int>(v) == native_nullptr);
                    (void)v;
                }
                {
                    using V = ranges::variant<int, long>;
                    constexpr V v(42);
                    ASSERT_NOEXCEPT(ranges::get_if<int>(&v));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<int>(&v)), int const*);
                    STATIC_ASSERT(*ranges::get_if<int>(&v) == 42);
                    STATIC_ASSERT(ranges::get_if<long>(&v) == native_nullptr);
                }
                {
                    using V = ranges::variant<int, long>;
                    constexpr V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<long>(&v)), long const*);
                    STATIC_ASSERT(*ranges::get_if<long>(&v) == 42);
                    STATIC_ASSERT(ranges::get_if<int>(&v) == native_nullptr);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    const V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<int&>(std::addressof(v))), int*);
                    CHECK(ranges::get_if<int&>(std::addressof(v)) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<int&&>(std::addressof(v))), int*);
                    CHECK(ranges::get_if<int&&>(std::addressof(v)) == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<const int&&>(std::addressof(v))), const int*);
                    CHECK(ranges::get_if<const int&&>(std::addressof(v)) == &x);
                }
            }

            void test_get_if()
            {
                {
                    using V = ranges::variant<int>;
                    V* v = native_nullptr;
                    CHECK(ranges::get_if<int>(v) == native_nullptr);
                }
                {
                    using V = ranges::variant<int, long>;
                    V v(42);
                    ASSERT_NOEXCEPT(ranges::get_if<int>(std::addressof(v)));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<int>(std::addressof(v))), int*);
                    CHECK(*ranges::get_if<int>(std::addressof(v)) == 42);
                    CHECK(ranges::get_if<long>(std::addressof(v)) == native_nullptr);
                }
                {
                    using V = ranges::variant<int, long>;
                    V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<long>(std::addressof(v))), long*);
                    CHECK(*ranges::get_if<long>(std::addressof(v)) == 42);
                    CHECK(ranges::get_if<int>(std::addressof(v)) == native_nullptr);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<int&>(std::addressof(v))), int*);
                    CHECK(ranges::get_if<int&>(std::addressof(v)) == &x);
                }
                {
                    using V = ranges::variant<const int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<const int&>(std::addressof(v))), const int*);
                    CHECK(ranges::get_if<const int&>(std::addressof(v)) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<int&&>(std::addressof(v))), int*);
                    CHECK(ranges::get_if<int&&>(std::addressof(v)) == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get_if<const int&&>(std::addressof(v))), const int*);
                    CHECK(ranges::get_if<const int&&>(std::addressof(v)) == &x);
                }
            }

            void run_test()
            {
                test_const_get_if();
                test_get_if();
            }
        }

        namespace index {
            void test_const_lvalue_get() {
                {
                    using V = ranges::variant<int>;
                    constexpr V v(42);
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(v)), int const&);
                    STATIC_ASSERT(ranges::get<0>(v) == 42);
                }
                {
                    using V = ranges::variant<int, long>;
                    constexpr V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get<1>(v)), long const&);
                    STATIC_ASSERT(ranges::get<1>(v) == 42);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    const V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(v)), int&);
                    CHECK(&ranges::get<0>(v) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(v)), int&);
                    CHECK(&ranges::get<0>(v) == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(v)), const int&);
                    CHECK(&ranges::get<0>(v) == &x);
                }
            }

            void test_lvalue_get()
            {
                {
                    using V = ranges::variant<int>;
                    V v(42);
                    ASSERT_NOT_NOEXCEPT(ranges::get<0>(v));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(v)), int&);
                    CHECK(ranges::get<0>(v) == 42);
                }
                {
                    using V = ranges::variant<int, long>;
                    V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get<1>(v)), long&);
                    CHECK(ranges::get<1>(v) == 42);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(v)), int&);
                    CHECK(&ranges::get<0>(v) == &x);
                }
                {
                    using V = ranges::variant<const int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(v)), const int&);
                    CHECK(&ranges::get<0>(v) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(v)), int&);
                    CHECK(&ranges::get<0>(v) == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(v)), const int&);
                    CHECK(&ranges::get<0>(v) == &x);
                }
            }


            void test_rvalue_get()
            {
                {
                    using V = ranges::variant<int>;
                    V v(42);
                    ASSERT_NOT_NOEXCEPT(ranges::get<0>(std::move(v)));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(std::move(v))), int&&);
                    CHECK(ranges::get<0>(std::move(v)) == 42);
                }
                {
                    using V = ranges::variant<int, long>;
                    V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get<1>(std::move(v))), long&&);
                    CHECK(ranges::get<1>(std::move(v)) == 42);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(std::move(v))), int&);
                    CHECK(&ranges::get<0>(std::move(v)) == &x);
                }
                {
                    using V = ranges::variant<const int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(std::move(v))), const int&);
                    CHECK(&ranges::get<0>(std::move(v)) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(std::move(v))), int&&);
                    int&& xref = ranges::get<0>(std::move(v));
                    CHECK(&xref == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(std::move(v))), const int&&);
                    const int&& xref = ranges::get<0>(std::move(v));
                    CHECK(&xref == &x);
                }
            }


            void test_const_rvalue_get()
            {
                {
                    using V = ranges::variant<int>;
                    const V v(42);
                    ASSERT_NOT_NOEXCEPT(ranges::get<0>(std::move(v)));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(std::move(v))), const int&&);
                    CHECK(ranges::get<0>(std::move(v)) == 42);
                }
                {
                    using V = ranges::variant<int, long>;
                    const V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get<1>(std::move(v))), const long&&);
                    CHECK(ranges::get<1>(std::move(v)) == 42);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    const V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(std::move(v))), int&);
                    CHECK(&ranges::get<0>(std::move(v)) == &x);
                }
                {
                    using V = ranges::variant<const int&>;
                    int x = 42;
                    const V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(std::move(v))), const int&);
                    CHECK(&ranges::get<0>(std::move(v)) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(std::move(v))), int&&);
                    int&& xref = ranges::get<0>(std::move(v));
                    CHECK(&xref == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(std::move(v))), const int&&);
                    const int&& xref = ranges::get<0>(std::move(v));
                    CHECK(&xref == &x);
                }
            }

            template <std::size_t I>
            using Idx = std::integral_constant<size_t, I>;

            void test_throws_for_all_value_categories()
            {
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using V = ranges::variant<int, long>;
                V v0(42);
                const V& cv0 = v0;
                CHECK(v0.index() == 0);
                V v1(42l);
                const V& cv1 = v1;
                CHECK(v1.index() == 1);
                std::integral_constant<size_t, 0> zero;
                std::integral_constant<size_t, 1> one;
                auto test = [](auto idx, auto&& v) {
                using Idx = decltype(idx);
                (void)idx;
                try {
                    ranges::get<Idx::value>(std::forward<decltype(v)>(v));
                } catch (ranges::bad_variant_access const&) {
                    return true;
                }
                return false;
                };
                { // lvalue test cases
                    CHECK(test(one, v0));
                    CHECK(test(zero, v1));
                }
                { // const lvalue test cases
                    CHECK(test(one, cv0));
                    CHECK(test(zero, cv1));
                }
                { // rvalue test cases
                    CHECK(test(one, std::move(v0)));
                    CHECK(test(zero, std::move(v1)));
                }
                { // const rvalue test cases
                    CHECK(test(one, std::move(cv0)));
                    CHECK(test(zero, std::move(cv1)));
                }
            #endif // !TEST_HAS_NO_EXCEPTIONS
            }

            void run_test()
            {
                test_const_lvalue_get();
                test_lvalue_get();
                test_rvalue_get();
                test_const_rvalue_get();
                test_throws_for_all_value_categories();
            }
        }

        namespace type {
            void test_const_lvalue_get() {
                {
                    using V = ranges::variant<int>;
                    constexpr V v(42);
                    //ASSERT_NOT_NOEXCEPT(ranges::get<int>(v));
                    ASSERT_SAME_TYPE(decltype(ranges::get<0>(v)), int const&);
                    STATIC_ASSERT(ranges::get<int>(v) == 42);
                }
                {
                    using V = ranges::variant<int, long>;
                    constexpr V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get<long>(v)), long const&);
                    STATIC_ASSERT(ranges::get<long>(v) == 42);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    const V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<int&>(v)), int&);
                    CHECK(&ranges::get<int&>(v) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<int&&>(v)), int&);
                    CHECK(&ranges::get<int&&>(v) == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<const int&&>(v)), const int&);
                    CHECK(&ranges::get<const int&&>(v) == &x);
                }
            }

            void test_lvalue_get()
            {
                {
                    using V = ranges::variant<int>;
                    V v(42);
                    ASSERT_NOT_NOEXCEPT(ranges::get<int>(v));
                    ASSERT_SAME_TYPE(decltype(ranges::get<int>(v)), int&);
                    CHECK(ranges::get<int>(v) == 42);
                }
                {
                    using V = ranges::variant<int, long>;
                    V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get<long>(v)), long&);
                    CHECK(ranges::get<long>(v) == 42);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<int&>(v)), int&);
                    CHECK(&ranges::get<int&>(v) == &x);
                }
                {
                    using V = ranges::variant<const int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<const int&>(v)), const int&);
                    CHECK(&ranges::get<const int&>(v) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<int&&>(v)), int&);
                    CHECK(&ranges::get<int&&>(v) == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<const int&&>(v)), const int&);
                    CHECK(&ranges::get<const int&&>(v) == &x);
                }
            }


            void test_rvalue_get()
            {
                {
                    using V = ranges::variant<int>;
                    V v(42);
                    ASSERT_NOT_NOEXCEPT(ranges::get<int>(std::move(v)));
                    ASSERT_SAME_TYPE(decltype(ranges::get<int>(std::move(v))), int&&);
                    CHECK(ranges::get<int>(std::move(v)) == 42);
                }
                {
                    using V = ranges::variant<int, long>;
                    V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get<long>(std::move(v))), long&&);
                    CHECK(ranges::get<long>(std::move(v)) == 42);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<int&>(std::move(v))), int&);
                    CHECK(&ranges::get<int&>(std::move(v)) == &x);
                }
                {
                    using V = ranges::variant<const int&>;
                    int x = 42;
                    V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<const int&>(std::move(v))), const int&);
                    CHECK(&ranges::get<const int&>(std::move(v)) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<int&&>(std::move(v))), int&&);
                    int&& xref = ranges::get<int&&>(std::move(v));
                    CHECK(&xref == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<const int&&>(std::move(v))), const int&&);
                    const int&& xref = ranges::get<const int&&>(std::move(v));
                    CHECK(&xref == &x);
                }
            }


            void test_const_rvalue_get()
            {
                {
                    using V = ranges::variant<int>;
                    const V v(42);
                    ASSERT_NOT_NOEXCEPT(ranges::get<int>(std::move(v)));
                    ASSERT_SAME_TYPE(decltype(ranges::get<int>(std::move(v))), const int&&);
                    CHECK(ranges::get<int>(std::move(v)) == 42);
                }
                {
                    using V = ranges::variant<int, long>;
                    const V v(42l);
                    ASSERT_SAME_TYPE(decltype(ranges::get<long>(std::move(v))), const long&&);
                    CHECK(ranges::get<long>(std::move(v)) == 42);
                }
                {
                    using V = ranges::variant<int&>;
                    int x = 42;
                    const V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<int&>(std::move(v))), int&);
                    CHECK(&ranges::get<int&>(std::move(v)) == &x);
                }
                {
                    using V = ranges::variant<const int&>;
                    int x = 42;
                    const V v(x);
                    ASSERT_SAME_TYPE(decltype(ranges::get<const int&>(std::move(v))), const int&);
                    CHECK(&ranges::get<const int&>(std::move(v)) == &x);
                }
                {
                    using V = ranges::variant<int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<int&&>(std::move(v))), int&&);
                    int&& xref = ranges::get<int&&>(std::move(v));
                    CHECK(&xref == &x);
                }
                {
                    using V = ranges::variant<const int&&>;
                    int x = 42;
                    const V v(std::move(x));
                    ASSERT_SAME_TYPE(decltype(ranges::get<const int&&>(std::move(v))), const int&&);
                    const int&& xref = ranges::get<const int&&>(std::move(v));
                    CHECK(&xref == &x);
                }
            }

            template <class Tp> struct identity { using type = Tp; };

            void test_throws_for_all_value_categories()
            {
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using V = ranges::variant<int, long>;
                V v0(42);
                const V& cv0 = v0;
                CHECK(v0.index() == 0);
                V v1(42l);
                const V& cv1 = v1;
                CHECK(v1.index() == 1);
                identity<int> zero;
                identity<long> one;
                auto test = [](auto idx, auto&& v) {
                using Idx = decltype(idx);
                (void)idx;
                try {
                    ranges::get<typename Idx::type>(std::forward<decltype(v)>(v));
                } catch (ranges::bad_variant_access const&) {
                    return true;
                } catch (...) { /* ... */ }
                return false;
                };
                { // lvalue test cases
                    CHECK(test(one, v0));
                    CHECK(test(zero, v1));
                }
                { // const lvalue test cases
                    CHECK(test(one, cv0));
                    CHECK(test(zero, cv1));
                }
                { // rvalue test cases
                    CHECK(test(one, std::move(v0)));
                    CHECK(test(zero, std::move(v1)));
                }
                { // const rvalue test cases
                    CHECK(test(one, std::move(cv0)));
                    CHECK(test(zero, std::move(cv1)));
                }
            #endif // !TEST_HAS_NO_EXCEPTIONS
            }

            void run_test()
            {
                test_const_lvalue_get();
                test_lvalue_get();
                test_rvalue_get();
                test_const_rvalue_get();
                test_throws_for_all_value_categories();
            }
        }

        namespace holds_alternative {
            void run_test()
            {
                {
                    using V = ranges::variant<int>;
                    constexpr V v;
                    STATIC_ASSERT(ranges::holds_alternative<int>(v));
                }
                {
                    using V = ranges::variant<int, long>;
                    constexpr V v;
                    STATIC_ASSERT(ranges::holds_alternative<int>(v));
                    STATIC_ASSERT(!ranges::holds_alternative<long>(v));
                }
                { // noexcept test
                    using V = ranges::variant<int>;
                    const V v;
                    STATIC_ASSERT(noexcept(ranges::holds_alternative<int>(v)));
                }
            }
        }
    } // namespace get

#ifndef TEST_HAS_NO_EXCEPTIONS
} namespace std {
    template <>
    struct hash< ::MakeEmptyT> {
        size_t operator()(::MakeEmptyT const&) const { CHECK(false); return 0; }
    };
} namespace {
#endif // !TEST_HAS_NO_EXCEPTIONS
    namespace hash {
        void test_hash_variant()
        {
            {
                using V = ranges::variant<int>;
                using H = std::hash<V>;
                const V v(42);
                V v2(100);
                const H h{};
                CHECK(h(v) == h(v));
                CHECK(h(v2) == h(v2));
                {
                    ASSERT_SAME_TYPE(decltype(h(v)), std::size_t);
                    STATIC_ASSERT(std::is_copy_constructible<H>::value);
                }
            }
            {
                using V = ranges::variant<ranges::monostate, int, long, const char*>;
                using H = std::hash<V>;
                const char* str = "hello";
                const V v0;
                const V v1(42);
                V v2(100l);
                V v3(str);
                const H h{};
                CHECK(h(v0) == h(v0));
                CHECK(h(v1) == h(v1));
                CHECK(h(v2) == h(v2));
                CHECK(h(v3) == h(v3));
            }
#ifndef TEST_HAS_NO_EXCEPTIONS
            {
                using V = ranges::variant<int, MakeEmptyT>;
                using H = std::hash<V>;
                V v; makeEmpty(v);
                V v2; makeEmpty(v2);
                const H h{};
                CHECK(h(v) == h(v2));
            }
#endif // TEST_HAS_NO_EXCEPTIONS
        }

        void test_hash_monostate()
        {
            using H = std::hash<ranges::monostate>;
            const H h{};
            ranges::monostate m1{};
            const ranges::monostate m2{};
            CHECK(h(m1) == h(m1));
            CHECK(h(m2) == h(m2));
            CHECK(h(m1) == h(m2));
            {
                ASSERT_SAME_TYPE(decltype(h(m1)), std::size_t);
                STATIC_ASSERT(std::is_copy_constructible<H>::value);
            }
        }

        void run_test()
        {
            test_hash_variant();
            test_hash_monostate();
        }
    } // namespace hash

    namespace helpers {
        namespace variant_alternative {
            template <class V, size_t I, class E>
            void test() {
                STATIC_ASSERT(is_same_v<
                    typename ranges::variant_alternative<I, V>::type, E>);
                STATIC_ASSERT(is_same_v<
                    typename ranges::variant_alternative<I, const V>::type, const E>);
                STATIC_ASSERT(is_same_v<
                    typename ranges::variant_alternative<I, volatile V>::type, volatile E>);
                STATIC_ASSERT(is_same_v<
                    typename ranges::variant_alternative<I, const volatile V>::type, const volatile E>);
                STATIC_ASSERT(is_same_v<
                    ranges::variant_alternative_t<I, V>, E>);
                STATIC_ASSERT(is_same_v<
                    ranges::variant_alternative_t<I, const V>, const E>);
                STATIC_ASSERT(is_same_v<
                    ranges::variant_alternative_t<I, volatile V>, volatile E>);
                STATIC_ASSERT(is_same_v<
                    ranges::variant_alternative_t<I, const volatile V>, const volatile E>);
            }

            void run_test()
            {
                test<ranges::variant<void>, 0, void>();
                using V = ranges::variant<int, int&, int const&, int&&, long double>;
                test<V, 0, int>();
                test<V, 1, int&>();
                test<V, 2, int const&>();
                test<V, 3, int&&>();
                test<V, 4, long double>();
            }
        }

        namespace variant_size {
            template <class V, size_t E>
            void test() {
                STATIC_ASSERT(ranges::variant_size<V>::value == E);
                STATIC_ASSERT(ranges::variant_size<const V>::value == E);
                STATIC_ASSERT(ranges::variant_size<volatile V>::value == E);
                STATIC_ASSERT(ranges::variant_size<const volatile V>::value == E);
                STATIC_ASSERT(ranges::variant_size_v<V> == E);
                STATIC_ASSERT(ranges::variant_size_v<const V> == E);
                STATIC_ASSERT(ranges::variant_size_v<volatile V> == E);
                STATIC_ASSERT(ranges::variant_size_v<const volatile V> == E);
                STATIC_ASSERT(std::is_base_of<std::integral_constant<std::size_t, E>,
                                            ranges::variant_size<V>>::value);
            }

            void run_test()
            {
                test<ranges::variant<>, 0>();
                test<ranges::variant<void>, 1>();
                test<ranges::variant<long, long, void, double>, 4>();
            }
        }
    } // namespace helpers

    namespace monostate {
        void run_test()
        {
            using M = ranges::monostate;
            STATIC_ASSERT(std::is_trivially_default_constructible<M>::value);
            STATIC_ASSERT(std::is_trivially_copy_constructible<M>::value);
            STATIC_ASSERT(std::is_trivially_copy_assignable<M>::value);
            STATIC_ASSERT(std::is_trivially_destructible<M>::value);
            constexpr M m{};
            ((void)m);
        }

        namespace relops {
            void run_test()
            {
                using M = ranges::monostate;
                constexpr M m1{};
                constexpr M m2{};
                {
                    STATIC_ASSERT((m1 < m2) == false);
                    STATIC_ASSERT(noexcept(m1 < m2));
                }
                {
                    STATIC_ASSERT((m1 > m2) == false);
                    STATIC_ASSERT(noexcept(m1 > m2));
                }
                {
                    STATIC_ASSERT((m1 <= m2) == true);
                    STATIC_ASSERT(noexcept(m1 <= m2));
                }
                {
                    STATIC_ASSERT((m1 >= m2) == true);
                    STATIC_ASSERT(noexcept(m1 >= m2));
                }
                {
                    STATIC_ASSERT((m1 == m2) == true);
                    STATIC_ASSERT(noexcept(m1 == m2));
                }
                {
                    STATIC_ASSERT((m1 != m2) == false);
                    STATIC_ASSERT(noexcept(m1 != m2));
                }
            }
        }
    } // namespace monostate

    namespace relops {
        #ifndef TEST_HAS_NO_EXCEPTIONS
        struct MakeEmptyT {
        MakeEmptyT() = default;
        MakeEmptyT(MakeEmptyT&&) {
            throw 42;
        }
        MakeEmptyT& operator=(MakeEmptyT&&) {
            throw 42;
        }
        };
        inline bool operator==(MakeEmptyT const&, MakeEmptyT const&) { CHECK(false); return true; }
        inline bool operator!=(MakeEmptyT const&, MakeEmptyT const&) { CHECK(false); return false; }
        inline bool operator< (MakeEmptyT const&, MakeEmptyT const&) { CHECK(false); return false; }
        inline bool operator<=(MakeEmptyT const&, MakeEmptyT const&) { CHECK(false); return true; }
        inline bool operator> (MakeEmptyT const&, MakeEmptyT const&) { CHECK(false); return false; }
        inline bool operator>=(MakeEmptyT const&, MakeEmptyT const&) { CHECK(false); return true; }

        template <class Variant>
        void makeEmpty(Variant& v) {
            Variant v2(ranges::in_place<MakeEmptyT>);
            try {
                v = std::move(v2);
                CHECK(false);
            } catch (...) {
                CHECK(v.valueless_by_exception());
            }
        }
        #endif // TEST_HAS_NO_EXCEPTIONS

        void test_equality()
        {
            {
                using V = ranges::variant<int, long>;
                constexpr V v1(42);
                constexpr V v2(42);
                STATIC_ASSERT(v1 == v2);
                STATIC_ASSERT(v2 == v1);
                STATIC_ASSERT(!(v1 != v2));
                STATIC_ASSERT(!(v2 != v1));
            }
            {
                using V = ranges::variant<int, long>;
                constexpr V v1(42);
                constexpr V v2(43);
                STATIC_ASSERT(!(v1 == v2));
                STATIC_ASSERT(!(v2 == v1));
                STATIC_ASSERT(v1 != v2);
                STATIC_ASSERT(v2 != v1);
            }
            {
                using V = ranges::variant<int, long>;
                constexpr V v1(42);
                constexpr V v2(42l);
                STATIC_ASSERT(!(v1 == v2));
                STATIC_ASSERT(!(v2 == v1));
                STATIC_ASSERT(v1 != v2);
                STATIC_ASSERT(v2 != v1);
            }
            {
                using V = ranges::variant<int, long>;
                constexpr V v1(42l);
                constexpr V v2(42l);
                STATIC_ASSERT(v1 == v2);
                STATIC_ASSERT(v2 == v1);
                STATIC_ASSERT(!(v1 != v2));
                STATIC_ASSERT(!(v2 != v1));
            }
        #ifndef TEST_HAS_NO_EXCEPTIONS
            {
                using V = ranges::variant<int, MakeEmptyT>;
                V v1;
                V v2; makeEmpty(v2);
                CHECK(!(v1 == v2));
                CHECK(!(v2 == v1));
                CHECK(v1 != v2);
                CHECK(v2 != v1);
            }
            {
                using V = ranges::variant<int, MakeEmptyT>;
                V v1; makeEmpty(v1);
                V v2;
                CHECK(!(v1 == v2));
                CHECK(!(v2 == v1));
                CHECK(v1 != v2);
                CHECK(v2 != v1);
            }
            {
                using V = ranges::variant<int, MakeEmptyT>;
                V v1; makeEmpty(v1);
                V v2; makeEmpty(v2);
                CHECK(v1 == v2);
                CHECK(v2 == v1);
                CHECK(!(v1 != v2));
                CHECK(!(v2 != v1));
            }
        #endif // TEST_HAS_NO_EXCEPTIONS
        }

        template <class Var>
        constexpr bool test_less(Var const& l, Var const& r, bool expect_less,
                                                            bool expect_greater) {
            return ((l < r) == expect_less)
                && (!(l >= r) == expect_less)
                && ((l > r) == expect_greater)
                && (!(l <= r) == expect_greater);
        }

        void test_relational()
        {
            { // same index, same value
                using V = ranges::variant<int, long>;
                constexpr V v1(1);
                constexpr V v2(1);
                STATIC_ASSERT(test_less(v1, v2, false, false));
            }
            { // same index, value < other_value
                using V = ranges::variant<int, long>;
                constexpr V v1(0);
                constexpr V v2(1);
                STATIC_ASSERT(test_less(v1, v2, true, false));
            }
            { // same index, value > other_value
                using V = ranges::variant<int, long>;
                constexpr V v1(1);
                constexpr V v2(0);
                STATIC_ASSERT(test_less(v1, v2, false, true));
            }
            { // LHS.index() < RHS.index()
                using V = ranges::variant<int, long>;
                constexpr V v1(0);
                constexpr V v2(0l);
                STATIC_ASSERT(test_less(v1, v2, true, false));
            }
            { // LHS.index() > RHS.index()
                using V = ranges::variant<int, long>;
                constexpr V v1(0l);
                constexpr V v2(0);
                STATIC_ASSERT(test_less(v1, v2, false, true));
            }
        #ifndef TEST_HAS_NO_EXCEPTIONS
            { // LHS.index() < RHS.index(), RHS is empty
                using V = ranges::variant<int, MakeEmptyT>;
                V v1;
                V v2; makeEmpty(v2);
                CHECK(test_less(v1, v2, false, true));
            }
            { // LHS.index() > RHS.index(), LHS is empty
                using V = ranges::variant<int, MakeEmptyT>;
                V v1; makeEmpty(v1);
                V v2;
                CHECK(test_less(v1, v2, true, false));
            }
            { // LHS.index() == RHS.index(), LHS and RHS are empty
                using V = ranges::variant<int, MakeEmptyT>;
                V v1; makeEmpty(v1);
                V v2; makeEmpty(v2);
                CHECK(test_less(v1, v2, false, false));
            }
        #endif // TEST_HAS_NO_EXCEPTIONS
        }

        void run_test() {
            test_equality();
            test_relational();
        }
    } // namespace relops

    namespace synopsis {
        void run_test()
        {
            STATIC_ASSERT(ranges::variant_npos == static_cast<std::size_t>(-1));
        }
    }

#if 0
    namespace traits {
        void run_test()
        {
            using T = ranges::variant<int, long>;
            STATIC_ASSERT(std::uses_allocator<T, std::allocator<void>>::value);
        }
    }
#endif

    namespace assign {
        namespace copy {
            struct NoCopy {
            NoCopy(NoCopy const&) = delete;
            NoCopy& operator=(NoCopy const&) = default;
            };

            struct NothrowCopy {
            NothrowCopy(NothrowCopy const&) noexcept = default;
            NothrowCopy& operator=(NothrowCopy const&) noexcept = default;
            };

            struct CopyOnly {
            CopyOnly(CopyOnly const&) = default;
            CopyOnly(CopyOnly&&) = delete;
            CopyOnly& operator=(CopyOnly const&) = default;
            CopyOnly& operator=(CopyOnly&&) = delete;
            };

            struct MoveOnly {
            MoveOnly(MoveOnly const&) = delete;
            MoveOnly(MoveOnly&&) = default;
            MoveOnly& operator=(MoveOnly const&) = default;
            };

            struct MoveOnlyNT {
            MoveOnlyNT(MoveOnlyNT const&) = delete;
            MoveOnlyNT(MoveOnlyNT&&) {}
            MoveOnlyNT& operator=(MoveOnlyNT const&) = default;
            };

            struct CopyAssign {
            static int alive;
            static int copy_construct;
            static int copy_assign;
            static int move_construct;
            static int move_assign;
            static void reset() {
                copy_construct = copy_assign = move_construct = move_assign = alive = 0;
            }
            CopyAssign(int v) : value(v) { ++alive; }
            CopyAssign(CopyAssign const& o) : value(o.value) { ++alive; ++copy_construct; }
            CopyAssign(CopyAssign&& o) : value(o.value) { o.value = -1; ++alive; ++move_construct; }
            CopyAssign& operator=(CopyAssign const& o)  { value = o.value; ++copy_assign; return *this; }
            CopyAssign& operator=(CopyAssign&& o)  { value = o.value; o.value = -1; ++move_assign; return *this; }
            ~CopyAssign() { --alive; }
            int value;
            };

            int CopyAssign::alive = 0;
            int CopyAssign::copy_construct = 0;
            int CopyAssign::copy_assign = 0;
            int CopyAssign::move_construct = 0;
            int CopyAssign::move_assign = 0;

            #ifndef TEST_HAS_NO_EXCEPTIONS
            struct CopyThrows {
            CopyThrows() = default;
            CopyThrows(CopyThrows const&) { throw 42; }
            CopyThrows& operator=(CopyThrows const&) { throw 42; }
            };

            struct MoveThrows {
            static int alive;
            MoveThrows() { ++alive; }
            MoveThrows(MoveThrows const&) {++alive;}
            MoveThrows(MoveThrows&&) {  throw 42; }
            MoveThrows& operator=(MoveThrows const&) { return *this; }
            MoveThrows& operator=(MoveThrows&&) { throw 42; }
            ~MoveThrows() { --alive; }
            };

            int MoveThrows::alive = 0;

            struct MakeEmptyT {
            static int alive;
            MakeEmptyT() { ++alive; }
            MakeEmptyT(MakeEmptyT const&) {
                ++alive;
                // Don't throw from the copy constructor since variant's assignment
                // operator performs a copy before committing to the assignment.
            }
            MakeEmptyT(MakeEmptyT &&) {
                throw 42;
            }
            MakeEmptyT& operator=(MakeEmptyT const&) {
                throw 42;
            }
            MakeEmptyT& operator=(MakeEmptyT&&) {
                throw 42;
            }
            ~MakeEmptyT() { --alive; }
            };

            int MakeEmptyT::alive = 0;

            template <class Variant>
            void makeEmpty(Variant& v) {
                Variant v2(ranges::in_place<MakeEmptyT>);
                try {
                    v = v2;
                    CHECK(false);
                }  catch (...) {
                    CHECK(v.valueless_by_exception());
                }
            }
            #endif // TEST_HAS_NO_EXCEPTIONS

            void test_copy_assignment_not_noexcept() {
                {
                    using V = ranges::variant<CopyThrows>;
                    STATIC_ASSERT(!std::is_nothrow_copy_assignable<V>::value);
                }
                {
                    using V = ranges::variant<int, CopyThrows>;
                    STATIC_ASSERT(!std::is_nothrow_copy_assignable<V>::value);
                }
            }

            void test_copy_assignment_sfinae() {
                {
                    using V = ranges::variant<int, long>;
                    STATIC_ASSERT(std::is_copy_assignable<V>::value);
                }
                {
                    // variant only provides copy assignment when both the copy and move
                    // constructors are well formed
                    using V = ranges::variant<int, CopyOnly>;
                    STATIC_ASSERT(!std::is_copy_assignable<V>::value);
                }
                {
                    using V = ranges::variant<int, NoCopy>;
                    STATIC_ASSERT(!std::is_copy_assignable<V>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnly>;
                    STATIC_ASSERT(!std::is_copy_assignable<V>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnlyNT>;
                    STATIC_ASSERT(!std::is_copy_assignable<V>::value);
                }
            }

            void test_copy_assignment_empty_empty()
            {
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using MET = MakeEmptyT;
                {
                    using V = ranges::variant<int, long, MET>;
                    V v1(ranges::in_place<0>); makeEmpty(v1);
                    V v2(ranges::in_place<0>); makeEmpty(v2);
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.valueless_by_exception());
                    CHECK(v1.index() == ranges::variant_npos);
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void test_copy_assignment_non_empty_empty()
            {
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using MET = MakeEmptyT;
                {
                    using V = ranges::variant<int, MET>;
                    V v1(ranges::in_place<0>, 42);
                    V v2(ranges::in_place<0>); makeEmpty(v2);
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.valueless_by_exception());
                    CHECK(v1.index() == ranges::variant_npos);
                }
                {
                    using V = ranges::variant<int, MET, std::string>;
                    V v1(ranges::in_place<2>, "hello");
                    V v2(ranges::in_place<0>); makeEmpty(v2);
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.valueless_by_exception());
                    CHECK(v1.index() == ranges::variant_npos);
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }


            void test_copy_assignment_empty_non_empty()
            {
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using MET = MakeEmptyT;
                {
                    using V = ranges::variant<int, MET>;
                    V v1(ranges::in_place<0>); makeEmpty(v1);
                    V v2(ranges::in_place<0>, 42);
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 0);
                    CHECK(ranges::get<0>(v1) == 42);
                }
                {
                    using V = ranges::variant<int, MET, std::string>;
                    V v1(ranges::in_place<0>); makeEmpty(v1);
                    V v2(ranges::in_place<std::string>, "hello");
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 2);
                    CHECK(ranges::get<2>(v1) == "hello");
                }
            #endif  // TEST_HAS_NO_EXCEPTIONS
            }

            void test_copy_assignment_same_index()
            {
                {
                    using V = ranges::variant<int>;
                    V v1(43);
                    V v2(42);
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 0);
                    CHECK(ranges::get<0>(v1) == 42);
                }
                {
                    using V = ranges::variant<int, long, unsigned>;
                    V v1(43l);
                    V v2(42l);
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 1);
                    CHECK(ranges::get<1>(v1) == 42);
                }
                {
                    using V = ranges::variant<int, CopyAssign, unsigned>;
                    V v1(ranges::in_place<CopyAssign>, 43);
                    V v2(ranges::in_place<CopyAssign>, 42);
                    CopyAssign::reset();
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 1);
                    CHECK(ranges::get<1>(v1).value == 42);
                    CHECK(CopyAssign::copy_construct == 0);
                    CHECK(CopyAssign::move_construct == 0);
                    CHECK(CopyAssign::copy_assign == 1);
                }
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using MET = MakeEmptyT;
                {
                    using V = ranges::variant<int, MET, std::string>;
                    V v1(ranges::in_place<MET>);
                    MET& mref = ranges::get<1>(v1);
                    V v2(ranges::in_place<MET>);
                    try {
                        v1 = v2;
                        CHECK(false);
                    } catch (...) {
                    }
                    CHECK(v1.index() == 1);
                    CHECK(&ranges::get<1>(v1) == &mref);
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void test_copy_assignment_different_index()
            {
                {
                    using V = ranges::variant<int, long, unsigned>;
                    V v1(43);
                    V v2(42l);
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 1);
                    CHECK(ranges::get<1>(v1) == 42);
                }
                {
                    using V = ranges::variant<int, CopyAssign, unsigned>;
                    CopyAssign::reset();
                    V v1(ranges::in_place<unsigned>, 43u);
                    V v2(ranges::in_place<CopyAssign>, 42);
                    CHECK(CopyAssign::copy_construct == 0);
                    CHECK(CopyAssign::move_construct == 0);
                    CHECK(CopyAssign::alive == 1);
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 1);
                    CHECK(ranges::get<1>(v1).value == 42);
                    CHECK(CopyAssign::alive == 2);
                    CHECK(CopyAssign::copy_construct == 1);
                    CHECK(CopyAssign::move_construct == 1);
                    CHECK(CopyAssign::copy_assign == 0);
                }
            #ifndef TEST_HAS_NO_EXCEPTIONS
                {
                    // Test that if copy construction throws then original value is
                    // unchanged.
                    using V = ranges::variant<int, CopyThrows, std::string>;
                    V v1(ranges::in_place<std::string>, "hello");
                    V v2(ranges::in_place<CopyThrows>);
                    try {
                        v1 = v2;
                        CHECK(false);
                    } catch (...) { /* ... */ }
                    CHECK(v1.index() == 2);
                    CHECK(ranges::get<2>(v1) == "hello");
                }
                {
                    // Test that if move construction throws then the variant is left
                    // valueless by exception.
                    using V = ranges::variant<int, MoveThrows, std::string>;
                    V v1(ranges::in_place<std::string>, "hello");
                    V v2(ranges::in_place<MoveThrows>);
                    CHECK(MoveThrows::alive == 1);
                    try {
                        v1 = v2;
                        CHECK(false);
                    } catch (...) { /* ... */ }
                    CHECK(v1.valueless_by_exception());
                    CHECK(v2.index() == 1);
                    CHECK(MoveThrows::alive == 1);
                }
                {
                    using V = ranges::variant<int, CopyThrows, std::string>;
                    V v1(ranges::in_place<CopyThrows>);
                    V v2(ranges::in_place<std::string>, "hello");
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 2);
                    CHECK(ranges::get<2>(v1) == "hello");
                    CHECK(v2.index() == 2);
                    CHECK(ranges::get<2>(v2) == "hello");
                }
                {
                    using V = ranges::variant<int, MoveThrows, std::string>;
                    V v1(ranges::in_place<MoveThrows>);
                    V v2(ranges::in_place<std::string>, "hello");
                    V& vref = (v1 = v2);
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 2);
                    CHECK(ranges::get<2>(v1) == "hello");
                    CHECK(v2.index() == 2);
                    CHECK(ranges::get<2>(v2) == "hello");
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void run_test()
            {
                test_copy_assignment_empty_empty();
                test_copy_assignment_non_empty_empty();
                test_copy_assignment_empty_non_empty();
                test_copy_assignment_same_index();
                test_copy_assignment_different_index();
                test_copy_assignment_sfinae();
                test_copy_assignment_not_noexcept();
            }
        }

        namespace move {
            struct NoCopy {
            NoCopy(NoCopy const&) = delete;
            NoCopy& operator=(NoCopy const&) = default;
            };

            struct CopyOnly {
            CopyOnly(CopyOnly const&) = default;
            CopyOnly(CopyOnly&&) = delete;
            CopyOnly& operator=(CopyOnly const&) = default;
            CopyOnly& operator=(CopyOnly&&) = delete;
            };

            struct MoveOnly {
            MoveOnly(MoveOnly const&) = delete;
            MoveOnly(MoveOnly&&) = default;
            MoveOnly& operator=(MoveOnly const&) = delete;
            MoveOnly& operator=(MoveOnly&&) = default;
            };

            struct MoveOnlyNT {
            MoveOnlyNT(MoveOnlyNT const&) = delete;
            MoveOnlyNT(MoveOnlyNT&&) {}
            MoveOnlyNT& operator=(MoveOnlyNT const&) = delete;
            MoveOnlyNT& operator=(MoveOnlyNT&&) = default;
            };

            struct MoveOnlyOddNothrow {
            MoveOnlyOddNothrow(MoveOnlyOddNothrow&&) noexcept(false) {}
            MoveOnlyOddNothrow(MoveOnlyOddNothrow const&) = delete;
            MoveOnlyOddNothrow& operator=(MoveOnlyOddNothrow&&) noexcept = default;
            MoveOnlyOddNothrow& operator=(MoveOnlyOddNothrow const&) = delete;
            };

            struct MoveAssignOnly {
            MoveAssignOnly(MoveAssignOnly &&) = delete;
            MoveAssignOnly& operator=(MoveAssignOnly&&) = default;
            };

            struct MoveAssign {
            static int move_construct;
            static int move_assign;
            static void reset() {
                move_construct = move_assign = 0;
            }
            MoveAssign(int v) : value(v) {}
            MoveAssign(MoveAssign&& o) : value(o.value) { ++move_construct; o.value = -1; }
            MoveAssign& operator=(MoveAssign&& o)  { value = o.value; ++move_assign; o.value = -1; return *this; }
            int value;
            };

            int MoveAssign::move_construct = 0;
            int MoveAssign::move_assign = 0;


            void test_move_assignment_noexcept() {
                {
                    using V = ranges::variant<int>;
                    STATIC_ASSERT(std::is_nothrow_move_assignable<V>::value);
                }
                {
                    using V = ranges::variant<MoveOnly>;
                    STATIC_ASSERT(std::is_nothrow_move_assignable<V>::value);
                }
                {
                    using V = ranges::variant<int, long>;
                    STATIC_ASSERT(std::is_nothrow_move_assignable<V>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnly>;
                    STATIC_ASSERT(std::is_nothrow_move_assignable<V>::value);
                }
                {
                    using V = ranges::variant<MoveOnlyNT>;
                    STATIC_ASSERT(!std::is_nothrow_move_assignable<V>::value);
                }
                {
                    using V = ranges::variant<MoveOnlyOddNothrow>;
                    STATIC_ASSERT(!std::is_nothrow_move_assignable<V>::value);
                }
            }

            void test_move_assignment_sfinae() {
                {
                    using V = ranges::variant<int, long>;
                    STATIC_ASSERT(std::is_move_assignable<V>::value);
                }
                {
                    // variant only provides move assignment when both the move and move
                    // constructors are well formed
                    using V = ranges::variant<int, CopyOnly>;
                    STATIC_ASSERT(!std::is_move_assignable<V>::value);
                }
                {
                    using V = ranges::variant<int, NoCopy>;
                    STATIC_ASSERT(!std::is_move_assignable<V>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnly>;
                    STATIC_ASSERT(std::is_move_assignable<V>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnlyNT>;
                    STATIC_ASSERT(std::is_move_assignable<V>::value);
                }
                {
                    // variant only provides move assignment when the types also provide
                    // a move constructor.
                    using V = ranges::variant<int, MoveAssignOnly>;
                    STATIC_ASSERT(!std::is_move_assignable<V>::value);
                }
            }

            void test_move_assignment_empty_empty()
            {
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using MET = MakeEmptyT;
                {
                    using V = ranges::variant<int, long, MET>;
                    V v1(ranges::in_place<0>); makeEmpty(v1);
                    V v2(ranges::in_place<0>); makeEmpty(v2);
                    V& vref = (v1 = std::move(v2));
                    CHECK(&vref == &v1);
                    CHECK(v1.valueless_by_exception());
                    CHECK(v1.index() == ranges::variant_npos);
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void test_move_assignment_non_empty_empty()
            {
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using MET = MakeEmptyT;
                {
                    using V = ranges::variant<int, MET>;
                    V v1(ranges::in_place<0>, 42);
                    V v2(ranges::in_place<0>); makeEmpty(v2);
                    V& vref = (v1 = std::move(v2));
                    CHECK(&vref == &v1);
                    CHECK(v1.valueless_by_exception());
                    CHECK(v1.index() == ranges::variant_npos);
                }
                {
                    using V = ranges::variant<int, MET, std::string>;
                    V v1(ranges::in_place<2>, "hello");
                    V v2(ranges::in_place<0>); makeEmpty(v2);
                    V& vref = (v1 = std::move(v2));
                    CHECK(&vref == &v1);
                    CHECK(v1.valueless_by_exception());
                    CHECK(v1.index() == ranges::variant_npos);
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }


            void test_move_assignment_empty_non_empty()
            {
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using MET = MakeEmptyT;
                {
                    using V = ranges::variant<int, MET>;
                    V v1(ranges::in_place<0>); makeEmpty(v1);
                    V v2(ranges::in_place<0>, 42);
                    V& vref = (v1 = std::move(v2));
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 0);
                    CHECK(ranges::get<0>(v1) == 42);
                }
                {
                    using V = ranges::variant<int, MET, std::string>;
                    V v1(ranges::in_place<0>); makeEmpty(v1);
                    V v2(ranges::in_place<std::string>, "hello");
                    V& vref = (v1 = std::move(v2));
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 2);
                    CHECK(ranges::get<2>(v1) == "hello");
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void test_move_assignment_same_index()
            {
                {
                    using V = ranges::variant<int>;
                    V v1(43);
                    V v2(42);
                    V& vref = (v1 = std::move(v2));
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 0);
                    CHECK(ranges::get<0>(v1) == 42);
                }
                {
                    using V = ranges::variant<int, long, unsigned>;
                    V v1(43l);
                    V v2(42l);
                    V& vref = (v1 = std::move(v2));
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 1);
                    CHECK(ranges::get<1>(v1) == 42);
                }
                {
                    using V = ranges::variant<int, MoveAssign, unsigned>;
                    V v1(ranges::in_place<MoveAssign>, 43);
                    V v2(ranges::in_place<MoveAssign>, 42);
                    MoveAssign::reset();
                    V& vref = (v1 = std::move(v2));
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 1);
                    CHECK(ranges::get<1>(v1).value == 42);
                    CHECK(MoveAssign::move_construct == 0);
                    CHECK(MoveAssign::move_assign == 1);
                }
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using MET = MakeEmptyT;
                {
                    using V = ranges::variant<int, MET, std::string>;
                    V v1(ranges::in_place<MET>);
                    MET& mref = ranges::get<1>(v1);
                    V v2(ranges::in_place<MET>);
                    try {
                        v1 = std::move(v2);
                        CHECK(false);
                    } catch (...) {
                    }
                    CHECK(v1.index() == 1);
                    CHECK(&ranges::get<1>(v1) == &mref);
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void test_move_assignment_different_index()
            {
                {
                    using V = ranges::variant<int, long, unsigned>;
                    V v1(43);
                    V v2(42l);
                    V& vref = (v1 = std::move(v2));
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 1);
                    CHECK(ranges::get<1>(v1) == 42);
                }
                {
                    using V = ranges::variant<int, MoveAssign, unsigned>;
                    V v1(ranges::in_place<unsigned>, 43u);
                    V v2(ranges::in_place<MoveAssign>, 42);
                    MoveAssign::reset();
                    V& vref = (v1 = std::move(v2));
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 1);
                    CHECK(ranges::get<1>(v1).value == 42);
                    CHECK(MoveAssign::move_construct == 1);
                    CHECK(MoveAssign::move_assign == 0);
                }
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using MET = MakeEmptyT;
                {
                    using V = ranges::variant<int, MET, std::string>;
                    V v1(ranges::in_place<int>);
                    V v2(ranges::in_place<MET>);
                    try {
                        v1 = std::move(v2);
                        CHECK(false);
                    } catch (...) {
                    }
                    CHECK(v1.valueless_by_exception());
                    CHECK(v1.index() == ranges::variant_npos);
                }
                {
                    using V = ranges::variant<int, MET, std::string>;
                    V v1(ranges::in_place<MET>);
                    V v2(ranges::in_place<std::string>, "hello");
                    V& vref = (v1 = std::move(v2));
                    CHECK(&vref == &v1);
                    CHECK(v1.index() == 2);
                    CHECK(ranges::get<2>(v1) == "hello");
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void run_test()
            {
                test_move_assignment_empty_empty();
                test_move_assignment_non_empty_empty();
                test_move_assignment_empty_non_empty();
                test_move_assignment_same_index();
                test_move_assignment_different_index();
                test_move_assignment_sfinae();
                test_move_assignment_noexcept();
            }
        }

        namespace T {
            namespace MetaHelpers {

            struct Dummy {
            Dummy() = default;
            };

            struct ThrowsCtorT {
            ThrowsCtorT(int) noexcept(false) {}
            ThrowsCtorT& operator=(int) noexcept { return *this; }
            };

            struct ThrowsAssignT {
            ThrowsAssignT(int) noexcept {}
            ThrowsAssignT& operator=(int) noexcept(false) { return *this; }
            };

            struct NoThrowT {
            NoThrowT(int) noexcept {}
            NoThrowT& operator=(int) noexcept { return *this; }
            };

            } // namespace MetaHelpers

            namespace RuntimeHelpers {
            #ifndef TEST_HAS_NO_EXCEPTIONS

            struct ThrowsCtorT {
            int value;
            ThrowsCtorT() : value(0) {}
            ThrowsCtorT(int) noexcept(false) { throw 42; }
            ThrowsCtorT& operator=(int v) noexcept { value = v; return *this; }
            };

            struct ThrowsAssignT {
            int value;
            ThrowsAssignT() : value(0) {}
            ThrowsAssignT(int v) noexcept : value(v) {}
            ThrowsAssignT& operator=(int) noexcept(false) { throw 42; }
            };

            struct NoThrowT {
            int value;
            NoThrowT() : value(0) {}
            NoThrowT(int v) noexcept : value(v) {}
            NoThrowT& operator=(int v) noexcept { value = v; return *this; }
            };

            #endif // TEST_HAS_NO_EXCEPTIONS
            } // namespace RuntimeHelpers

            void test_T_assignment_noexcept() {
                using namespace MetaHelpers;
                {
                    using V = ranges::variant<Dummy, NoThrowT>;
                    STATIC_ASSERT(std::is_nothrow_assignable<V&, int>::value);
                }
                {
                    using V = ranges::variant<Dummy, ThrowsCtorT>;
                    STATIC_ASSERT(std::is_assignable<V&, int>::value);
                    STATIC_ASSERT(!std::is_nothrow_assignable<V&, int>::value);
                }
                {
                    using V = ranges::variant<Dummy, ThrowsAssignT>;
                    STATIC_ASSERT(std::is_assignable<V&, int>::value);
                    STATIC_ASSERT(!std::is_nothrow_assignable<V&, int>::value);
                }
            }

            void test_T_assignment_sfinae() {
                {
                    using V = ranges::variant<int, int&&>;
                    STATIC_ASSERT(!std::is_assignable<V&, int>::value);
                }
                {
                    using V = ranges::variant<int, int const&>;
                    STATIC_ASSERT(!std::is_assignable<V&, int>::value);
                }
                {
                    using V = ranges::variant<long, unsigned>;
                    STATIC_ASSERT(!std::is_assignable<V&, int>::value);
                }
                {
                    using V = ranges::variant<std::string, std::string>;
                    STATIC_ASSERT(!std::is_assignable<V&, const char*>::value);
                }
                {
                    using V = ranges::variant<std::string, void*>;
                    STATIC_ASSERT(!std::is_assignable<V&, int>::value);
                }
            }

            void test_T_assignment_basic()
            {
                {
                    ranges::variant<int> v(43);
                    v = 42;
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v) == 42);
                }
                {
                    ranges::variant<int, long> v(43l);
                    v = 42;
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v) == 42);
                    v = 43l;
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v) == 43);
                }
                {
                    using V = ranges::variant<int &, int&&, long>;
                    int x = 42;
                    V v(43l);
                    v = x;
                    CHECK(v.index() == 0);
                    CHECK(&ranges::get<0>(v) == &x);
                    v = std::move(x);
                    CHECK(v.index() == 1);
                    CHECK(&ranges::get<1>(v) == &x);
                    // 'long' is selected by FUN(int const&) since 'int const&' cannot bind
                    // to 'int&'.
                    int const& cx = x;
                    v = cx;
                    CHECK(v.index() == 2);
                    CHECK(ranges::get<2>(v) == 42);
                }
            }

            void test_T_assignment_performs_construction()
            {
            using namespace RuntimeHelpers;
            #ifndef TEST_HAS_NO_EXCEPTIONS
                {
                    using V = ranges::variant<std::string, ThrowsCtorT>;
                    V v(ranges::in_place<std::string>, "hello");
                    try {
                        v = 42;
                    } catch (...) { /* ... */ }
                    CHECK(v.valueless_by_exception());
                }
                {
                    using V = ranges::variant<ThrowsAssignT, std::string>;
                    V v(ranges::in_place<std::string>, "hello");
                    v = 42;
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v).value == 42);
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void test_T_assignment_performs_assignment()
            {
                using namespace RuntimeHelpers;
            #ifndef TEST_HAS_NO_EXCEPTIONS
                {
                    using V = ranges::variant<ThrowsCtorT>;
                    V v;
                    v = 42;
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v).value == 42);
                }
                {
                    using V = ranges::variant<ThrowsCtorT, std::string>;
                    V v;
                    v = 42;
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v).value == 42);
                }
                {
                    using V = ranges::variant<ThrowsAssignT>;
                    V v(100);
                    try {
                        v = 42;
                        CHECK(false);
                    } catch(...) { /* ... */ }
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v).value == 100);
                }
                {
                    using V = ranges::variant<std::string, ThrowsAssignT>;
                    V v(100);
                    try {
                        v = 42;
                        CHECK(false);
                    } catch(...) { /* ... */ }
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v).value == 100);
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void run_test()
            {
                test_T_assignment_basic();
                test_T_assignment_performs_construction();
                test_T_assignment_performs_assignment();
                test_T_assignment_noexcept();
                test_T_assignment_sfinae();
            }
        }
    } // namespace assign

    namespace ctor {
#if 0
        namespace Alloc_copy {
            struct NonT {
            NonT(int v) : value(v) {}
            NonT(NonT const& o) : value(o.value) {}
            int value;
            };
            STATIC_ASSERT(!std::is_trivially_copy_constructible<NonT>::value);


            struct NoCopy {
            NoCopy(NoCopy const&) = delete;
            };

            struct MoveOnly {
            MoveOnly(MoveOnly const&) = delete;
            MoveOnly(MoveOnly&&) = default;
            };

            struct MoveOnlyNT {
            MoveOnlyNT(MoveOnlyNT const&) = delete;
            MoveOnlyNT(MoveOnlyNT&&) {}
            };

            void test_copy_ctor_sfinae()
            {
                {
                    using V = ranges::variant<int, long>;
                    STATIC_ASSERT(std::is_constructible<V, std::allocator_arg_t const&,
                        std::allocator<void> const&, V const&>::value);
                }
                {
                    using V = ranges::variant<int, NoCopy>;
                    STATIC_ASSERT(!std::is_constructible<V, std::allocator_arg_t const&,
                        std::allocator<void> const&, V const&>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnly>;
                    STATIC_ASSERT(!std::is_constructible<V, std::allocator_arg_t const&,
                        std::allocator<void> const&, V const&>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnlyNT>;
                    STATIC_ASSERT(!std::is_constructible<V, std::allocator_arg_t const&,
                        std::allocator<void> const&, V const&>::value);
                }
            }

            void test_copy_ctor_basic()
            {
                {
                    ranges::variant<int> v(ranges::in_place<0>, 42);
                    ranges::variant<int> v2(std::allocator_arg, std::allocator<void>{}, v);
                    CHECK(v2.index() == 0);
                    CHECK(ranges::get<0>(v2) == 42);
                }
                {
                    ranges::variant<int, long> v(ranges::in_place<1>, 42);
                    ranges::variant<int, long> v2(std::allocator_arg, std::allocator<void>{}, v);
                    CHECK(v2.index() == 1);
                    CHECK(ranges::get<1>(v2) == 42);
                }
                {
                    ranges::variant<NonT> v(ranges::in_place<0>, 42);
                    CHECK(v.index() == 0);
                    ranges::variant<NonT> v2(std::allocator_arg, std::allocator<void>{}, v);
                    CHECK(v2.index() == 0);
                    CHECK(ranges::get<0>(v2).value == 42);
                }
                {
                    ranges::variant<int, NonT> v(ranges::in_place<1>, 42);
                    CHECK(v.index() == 1);
                    ranges::variant<int, NonT> v2(std::allocator_arg, std::allocator<void>{}, v);
                    CHECK(v2.index() == 1);
                    CHECK(ranges::get<1>(v2).value == 42);
                }
            }

            void test_copy_ctor_uses_alloc()
            {
                using A = std::allocator<void>;
                using A2 = non_allocator<int>;
                using UA1 = UsesAllocatorV1<A, 1>;
                using UA2 = UsesAllocatorV2<A, 1>;
                using UA3 = UsesAllocatorV3<A, 1>;
                using NUA = NotUsesAllocator<A, 1>;
                using V = ranges::variant<UA1, UA2, UA3, NUA>;
                const A a{};
                const A2 a2{};
                {
                    V v(ranges::in_place<0>);
                    V v2(std::allocator_arg, a, v);
                    CHECK(v2.index() == 0);
                    CHECK(checkConstruct<UA1 const&>(ranges::get<0>(v2), UA_AllocArg));
                }
                {
                    V v(ranges::in_place<0>);
                    V v2(std::allocator_arg, a2, v);
                    CHECK(v2.index() == 0);
                    CHECK(checkConstruct<UA1 const&>(ranges::get<0>(v2), UA_None));
                }
                {
                    V v(ranges::in_place<1>);
                    V v2(std::allocator_arg, a, v);
                    CHECK(v2.index() == 1);
                    CHECK(checkConstruct<UA2 const&>(ranges::get<1>(v2), UA_AllocLast));
                }
                {
                    V v(ranges::in_place<1>);
                    V v2(std::allocator_arg, a2, v);
                    CHECK(v2.index() == 1);
                    CHECK(checkConstruct<UA2 const&>(ranges::get<1>(v2), UA_None));
                }
                {
                    V v(ranges::in_place<2>);
                    V v2(std::allocator_arg, a, v);
                    CHECK(v2.index() == 2);
                    CHECK(checkConstruct<UA3 const&>(ranges::get<2>(v2), UA_AllocArg));
                }
                {
                    V v(ranges::in_place<2>);
                    V v2(std::allocator_arg, a2, v);
                    CHECK(v2.index() == 2);
                    CHECK(checkConstruct<UA3 const&>(ranges::get<2>(v2), UA_None));
                }
                {
                    V v(ranges::in_place<3>);
                    V v3(std::allocator_arg, a, v);
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<NUA const&>(ranges::get<3>(v3), UA_None));
                }
                {
                    V v(ranges::in_place<3>);
                    V v3(std::allocator_arg, a2, v);
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<NUA const&>(ranges::get<3>(v3), UA_None));
                }
            }

            void run_test()
            {
                test_copy_ctor_basic();
                test_copy_ctor_uses_alloc();
                test_copy_ctor_sfinae();
            }
        }

        namespace Alloc_in_place_index_Args {
            void test_ctor_sfinae() {
                using Tag = std::allocator_arg_t const;
                using A = std::allocator<void> const&;
                {
                    using V = ranges::variant<int>;
                    STATIC_ASSERT(std::is_constructible<V, Tag, A, ranges::in_place_index_t<0>, int>::value);
                    STATIC_ASSERT(test_convertible<V, Tag, A, ranges::in_place_index_t<0>, int>());
                }
                {
                    using V = ranges::variant<int, long, long long>;
                    STATIC_ASSERT(std::is_constructible<V, Tag, A, ranges::in_place_index_t<1>, int>::value);
                    STATIC_ASSERT(test_convertible<V, Tag, A, ranges::in_place_index_t<1>, int>());
                }
                {
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(std::is_constructible<V, Tag, A, ranges::in_place_index_t<2>, int*>::value);
                    STATIC_ASSERT(test_convertible<V, Tag, A, ranges::in_place_index_t<2>, int*>());
                }
                { // args not convertible to type
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_index_t<0>, int*>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_index_t<0>, int*>());
                }
                { // index out of range
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_index_t<5>, int>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_index_t<5>, int>());
                }
            }

            void test_ctor_basic()
            {
                using Tag = std::allocator_arg_t const;
                Tag atag{};
                std::allocator<void> a{};
                {
                    ranges::variant<int> v(atag, a, ranges::in_place<0>, 42);
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v) == 42);
                }
                {
                    ranges::variant<int, long> v(atag, a, ranges::in_place<1>, 42);
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v) == 42);
                }
                {
                    ranges::variant<int, const int, long> v(atag, a, ranges::in_place<1>, 42);
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v) == 42);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(atag, a, ranges::in_place<0>, x);
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v) == x);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(atag, a, ranges::in_place<1>, x);
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v) == x);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(atag, a, ranges::in_place<2>, x);
                    CHECK(v.index() == 2);
                    CHECK(ranges::get<2>(v) == x);
                }
            }

            void test_ctor_uses_alloc()
            {
                using A = std::allocator<void>;
                using A2 = non_allocator<int>;
                using UA1 = UsesAllocatorV1<A, 1>;
                using UA2 = UsesAllocatorV2<A, 1>;
                using UA3 = UsesAllocatorV3<A, 1>;
                using NUA = NotUsesAllocator<A, 1>;
                using V = ranges::variant<UA1, UA2, UA3, NUA>;
                const A a{};
                const A2 a2{};
                {
                    V v(std::allocator_arg, a, ranges::in_place<0>, 42);
                    CHECK(v.index() == 0);
                    CHECK(checkConstruct<int&&>(ranges::get<0>(v), UA_AllocArg));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<0>, 42);
                    CHECK(v.index() == 0);
                    CHECK(checkConstruct<int&&>(ranges::get<0>(v), UA_None));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<1>, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_AllocLast));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<1>, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_None));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<2>, 42);
                    CHECK(v.index() == 2);
                    CHECK(checkConstruct<int&&>(ranges::get<2>(v), UA_AllocArg));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<2>, 42);
                    CHECK(v.index() == 2);
                    CHECK(checkConstruct<int&&>(ranges::get<2>(v), UA_None));
                }
                {
                    V v3(std::allocator_arg, a, ranges::in_place<3>, 42);
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<int &&>(ranges::get<3>(v3), UA_None));
                }
                {
                    V v3(std::allocator_arg, a2, ranges::in_place<3>, 42);
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<int &&>(ranges::get<3>(v3), UA_None));
                }
            }

            void run_test()
            {
                test_ctor_basic();
                test_ctor_uses_alloc();
                test_ctor_sfinae();
            }
        }

        namespace Alloc_in_place_index_init_list_Args {
            struct InitList {
            std::size_t size;
            constexpr InitList(std::initializer_list<int> il) : size(il.size()){}
            };

            struct InitListArg {
            std::size_t size;
            int value;
            constexpr InitListArg(std::initializer_list<int> il, int v) : size(il.size()), value(v) {}
            };

            void test_ctor_sfinae() {
                using Tag = std::allocator_arg_t const&;
                using A = std::allocator<void> const&;
                using IL = std::initializer_list<int>;
                { // just init list
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(std::is_constructible<V, Tag, A, ranges::in_place_index_t<0>, IL>::value);
                    STATIC_ASSERT(test_convertible<V, Tag, A, ranges::in_place_index_t<0>, IL>());
                }
                { // too many arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_index_t<0>, IL, int>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_index_t<0>, IL, int>());
                }
                { // too few arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_index_t<1>, IL>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_index_t<1>, IL>());
                }
                { // init list and arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(std::is_constructible<V, Tag, A, ranges::in_place_index_t<1>, IL, int>::value);
                    STATIC_ASSERT(test_convertible<V, Tag, A, ranges::in_place_index_t<1>, IL, int>());
                }
                { // not constructible from arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_index_t<2>, IL>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_index_t<2>, IL>());
                }
                { // index out of range
                    using V = ranges::variant<InitListArg, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_index_t<4>, IL, int>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_index_t<4>, IL, int>());
                }
            }

            void test_ctor_basic()
            {
                std::allocator_arg_t const atag{};
                std::allocator<void> a{};
                {
                    ranges::variant<InitList, InitListArg> v(atag, a, ranges::in_place<0>, {1, 2, 3});
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v).size == 3);
                }
                {
                    ranges::variant<InitList, InitListArg> v(atag, a, ranges::in_place<1>, {1, 2, 3, 4}, 42);
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v).size == 4);
                    CHECK(ranges::get<1>(v).value == 42);
                }
            }

            void test_no_args_ctor_uses_alloc()
            {
                using IL = std::initializer_list<int>;
                using A = std::allocator<void>;
                using A2 = non_allocator<int>;
                using UA1 = UsesAllocatorV1<A, 1>;
                using UA2 = UsesAllocatorV2<A, 1>;
                using UA3 = UsesAllocatorV3<A, 1>;
                using NUA = NotUsesAllocator<A, 1>;
                using V = ranges::variant<UA1, UA2, UA3, NUA>;
                const A a{};
                const A2 a2{};
                {
                    V v(std::allocator_arg, a, ranges::in_place<0>, {1, 2, 3});
                    CHECK(v.index() == 0);
                    CHECK(checkConstruct<IL&>(ranges::get<0>(v), UA_AllocArg));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<0>, {1, 2, 3});
                    CHECK(v.index() == 0);
                    CHECK(checkConstruct<IL&>(ranges::get<0>(v), UA_None));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<1>, {1, 2, 3});
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<IL&>(ranges::get<1>(v), UA_AllocLast));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<1>, {1, 2, 3});
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<IL&>(ranges::get<1>(v), UA_None));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<2>, {1, 2, 3});
                    CHECK(v.index() == 2);
                    CHECK(checkConstruct<IL&>(ranges::get<2>(v), UA_AllocArg));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<2>, {1, 2, 3});
                    CHECK(v.index() == 2);
                    CHECK(checkConstruct<IL&>(ranges::get<2>(v), UA_None));
                }
                {
                    V v3(std::allocator_arg, a, ranges::in_place<3>, {1, 2, 3});
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<IL&>(ranges::get<3>(v3), UA_None));
                }
                {
                    V v3(std::allocator_arg, a2, ranges::in_place<3>, {1, 2, 3});
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<IL&>(ranges::get<3>(v3), UA_None));
                }
            }

            void test_additional_args_ctor_uses_alloc()
            {
                using IL = std::initializer_list<int>;
                using A = std::allocator<void>;
                using A2 = non_allocator<int>;
                using UA1 = UsesAllocatorV1<A, 3>;
                using UA2 = UsesAllocatorV2<A, 3>;
                using UA3 = UsesAllocatorV3<A, 3>;
                using NUA = NotUsesAllocator<A, 3>;
                using V = ranges::variant<UA1, UA2, UA3, NUA>;
                const A a{};
                const A2 a2{};
                const int x = 42;
                {
                    V v(std::allocator_arg, a, ranges::in_place<0>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 0));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<0>(v), UA_AllocArg)));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<0>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 0));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<0>(v), UA_None)));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<1>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 1));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<1>(v), UA_AllocLast)));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<1>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 1));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<1>(v), UA_None)));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<2>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 2));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<2>(v), UA_AllocArg)));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<2>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 2));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<2>(v), UA_None)));
                }
                {
                    V v3(std::allocator_arg, a, ranges::in_place<3>, {1, 2, 3}, 42, x);
                    CHECK((v3.index() == 3));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<3>(v3), UA_None)));
                }
                {
                    V v3(std::allocator_arg, a2, ranges::in_place<3>, {1, 2, 3}, 42, x);
                    CHECK((v3.index() == 3));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<3>(v3), UA_None)));
                }
            }

            void run_test()
            {
                test_ctor_basic();
                test_no_args_ctor_uses_alloc();
                test_additional_args_ctor_uses_alloc();
                test_ctor_sfinae();
            }
        }

        namespace Alloc_in_place_type_Args {
            void test_ctor_sfinae() {
                using Tag = std::allocator_arg_t const;
                using A = std::allocator<void> const&;
                {
                    using V = ranges::variant<int>;
                    STATIC_ASSERT(std::is_constructible<V, Tag, A, ranges::in_place_type_t<int>, int>::value);
                    STATIC_ASSERT(test_convertible<V, Tag, A, ranges::in_place_type_t<int>, int>());
                }
                {
                    using V = ranges::variant<int, long, long long>;
                    STATIC_ASSERT(std::is_constructible<V, Tag, A, ranges::in_place_type_t<long>, int>::value);
                    STATIC_ASSERT(test_convertible<V, Tag, A, ranges::in_place_type_t<long>, int>());
                }
                {
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(std::is_constructible<V, Tag, A, ranges::in_place_type_t<int*>, int*>::value);
                    STATIC_ASSERT(test_convertible<V, Tag, A, ranges::in_place_type_t<int*>, int*>());
                }
                { // duplicate type
                    using V = ranges::variant<int, long, int>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_type_t<int>, int>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_type_t<int>, int>());
                }
                { // args not convertible to type
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_type_t<int>, int*>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_type_t<int>, int*>());
                }
                { // type not in variant
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_type_t<long long>, int>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_type_t<long long>, int>());
                }
            }

            void test_ctor_basic()
            {
                using Tag = std::allocator_arg_t const;
                Tag atag{};
                std::allocator<void> a{};
                {
                    ranges::variant<int> v(atag, a, ranges::in_place<int>, 42);
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v) == 42);
                }
                {
                    ranges::variant<int, long> v(atag, a, ranges::in_place<long>, 42);
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v) == 42);
                }
                {
                    ranges::variant<int, const int, long> v(atag, a, ranges::in_place<const int>, 42);
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v) == 42);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(atag, a, ranges::in_place<const int>, x);
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v) == x);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(atag, a, ranges::in_place<volatile int>, x);
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v) == x);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(atag, a, ranges::in_place<int>, x);
                    CHECK(v.index() == 2);
                    CHECK(ranges::get<2>(v) == x);
                }
            }


            void test_ctor_uses_alloc()
            {
                using A = std::allocator<void>;
                using A2 = non_allocator<int>;
                using UA1 = UsesAllocatorV1<A, 1>;
                using UA2 = UsesAllocatorV2<A, 1>;
                using UA3 = UsesAllocatorV3<A, 1>;
                using NUA = NotUsesAllocator<A, 1>;
                using V = ranges::variant<UA1, UA2, UA3, NUA>;
                const A a{};
                const A2 a2{};
                {
                    V v(std::allocator_arg, a, ranges::in_place<UA1>, 42);
                    CHECK(v.index() == 0);
                    CHECK(checkConstruct<int&&>(ranges::get<0>(v), UA_AllocArg));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<UA1>, 42);
                    CHECK(v.index() == 0);
                    CHECK(checkConstruct<int&&>(ranges::get<0>(v), UA_None));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<UA2>, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_AllocLast));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<UA2>, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_None));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<UA3>, 42);
                    CHECK(v.index() == 2);
                    CHECK(checkConstruct<int&&>(ranges::get<2>(v), UA_AllocArg));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<UA3>, 42);
                    CHECK(v.index() == 2);
                    CHECK(checkConstruct<int&&>(ranges::get<2>(v), UA_None));
                }
                {
                    V v3(std::allocator_arg, a, ranges::in_place<NUA>, 42);
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<int &&>(ranges::get<3>(v3), UA_None));
                }
                {
                    V v3(std::allocator_arg, a2, ranges::in_place<NUA>, 42);
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<int &&>(ranges::get<3>(v3), UA_None));
                }
            }

            void run_test()
            {
                test_ctor_basic();
                test_ctor_uses_alloc();
                test_ctor_sfinae();
            }
        }

        namespace Alloc_in_place_type_init_list_Args {
            struct InitList {
            std::size_t size;
            constexpr InitList(std::initializer_list<int> il) : size(il.size()){}
            };

            struct InitListArg {
            std::size_t size;
            int value;
            constexpr InitListArg(std::initializer_list<int> il, int v) : size(il.size()), value(v) {}
            };

            void test_ctor_sfinae() {
                using Tag = std::allocator_arg_t const&;
                using A = std::allocator<void> const&;
                using IL = std::initializer_list<int>;
                { // just init list
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(std::is_constructible<V, Tag, A, ranges::in_place_type_t<InitList>, IL>::value);
                    STATIC_ASSERT(test_convertible<V, Tag, A, ranges::in_place_type_t<InitList>, IL>());
                }
                { // too many arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_type_t<InitList>, IL, int>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_type_t<InitList>, IL, int>());
                }
                { // too few arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_type_t<InitListArg>, IL>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_type_t<InitListArg>, IL>());
                }
                { // init list and arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(std::is_constructible<V, Tag, A, ranges::in_place_type_t<InitListArg>, IL, int>::value);
                    STATIC_ASSERT(test_convertible<V, Tag, A, ranges::in_place_type_t<InitListArg>, IL, int>());
                }
                { // not constructible from arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_type_t<int>, IL>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_type_t<int>, IL>());
                }
                { // duplicate types in variant
                    using V = ranges::variant<InitListArg, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, ranges::in_place_type_t<InitListArg>, IL, int>::value);
                    STATIC_ASSERT(!test_convertible<V, Tag, A, ranges::in_place_type_t<InitListArg>, IL, int>());
                }
            }

            void test_ctor_basic()
            {
                std::allocator_arg_t const atag{};
                std::allocator<void> a{};
                {
                    ranges::variant<InitList, InitListArg> v(atag, a, ranges::in_place<InitList>, {1, 2, 3});
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v).size == 3);
                }
                {
                    ranges::variant<InitList, InitListArg> v(atag, a, ranges::in_place<InitListArg>, {1, 2, 3, 4}, 42);
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v).size == 4);
                    CHECK(ranges::get<1>(v).value == 42);
                }
            }

            void test_no_args_ctor_uses_alloc()
            {
                using IL = std::initializer_list<int>;
                using A = std::allocator<void>;
                using A2 = non_allocator<int>;
                using UA1 = UsesAllocatorV1<A, 1>;
                using UA2 = UsesAllocatorV2<A, 1>;
                using UA3 = UsesAllocatorV3<A, 1>;
                using NUA = NotUsesAllocator<A, 1>;
                using V = ranges::variant<UA1, UA2, UA3, NUA>;
                const A a{};
                const A2 a2{};
                {
                    V v(std::allocator_arg, a, ranges::in_place<UA1>, {1, 2, 3});
                    CHECK(v.index() == 0);
                    CHECK(checkConstruct<IL&>(ranges::get<0>(v), UA_AllocArg));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<UA1>, {1, 2, 3});
                    CHECK(v.index() == 0);
                    CHECK(checkConstruct<IL&>(ranges::get<0>(v), UA_None));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<UA2>, {1, 2, 3});
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<IL&>(ranges::get<1>(v), UA_AllocLast));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<UA2>, {1, 2, 3});
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<IL&>(ranges::get<1>(v), UA_None));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<UA3>, {1, 2, 3});
                    CHECK(v.index() == 2);
                    CHECK(checkConstruct<IL&>(ranges::get<2>(v), UA_AllocArg));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<UA3>, {1, 2, 3});
                    CHECK(v.index() == 2);
                    CHECK(checkConstruct<IL&>(ranges::get<2>(v), UA_None));
                }
                {
                    V v3(std::allocator_arg, a, ranges::in_place<NUA>, {1, 2, 3});
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<IL&>(ranges::get<3>(v3), UA_None));
                }
                {
                    V v3(std::allocator_arg, a2, ranges::in_place<NUA>, {1, 2, 3});
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<IL&>(ranges::get<3>(v3), UA_None));
                }
            }

            void test_additional_args_ctor_uses_alloc()
            {
                using IL = std::initializer_list<int>;
                using A = std::allocator<void>;
                using A2 = non_allocator<int>;
                using UA1 = UsesAllocatorV1<A, 3>;
                using UA2 = UsesAllocatorV2<A, 3>;
                using UA3 = UsesAllocatorV3<A, 3>;
                using NUA = NotUsesAllocator<A, 3>;
                using V = ranges::variant<UA1, UA2, UA3, NUA>;
                const A a{};
                const A2 a2{};
                const int x = 42;
                {
                    V v(std::allocator_arg, a, ranges::in_place<UA1>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 0));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<0>(v), UA_AllocArg)));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<UA1>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 0));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<0>(v), UA_None)));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<UA2>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 1));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<1>(v), UA_AllocLast)));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<UA2>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 1));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<1>(v), UA_None)));
                }
                {
                    V v(std::allocator_arg, a, ranges::in_place<UA3>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 2));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<2>(v), UA_AllocArg)));
                }
                {
                    V v(std::allocator_arg, a2, ranges::in_place<UA3>, {1, 2, 3}, 42, x);
                    CHECK((v.index() == 2));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<2>(v), UA_None)));
                }
                {
                    V v3(std::allocator_arg, a, ranges::in_place<NUA>, {1, 2, 3}, 42, x);
                    CHECK((v3.index() == 3));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<3>(v3), UA_None)));
                }
                {
                    V v3(std::allocator_arg, a2, ranges::in_place<NUA>, {1, 2, 3}, 42, x);
                    CHECK((v3.index() == 3));
                    CHECK((checkConstruct<IL&, int&&, int const&>(ranges::get<3>(v3), UA_None)));
                }
            }

            void run_test()
            {
                test_ctor_basic();
                test_no_args_ctor_uses_alloc();
                test_additional_args_ctor_uses_alloc();
                test_ctor_sfinae();
            }
        }

        namespace Alloc_move {
            struct ThrowsMove {
            ThrowsMove(ThrowsMove&&) noexcept(false) {}
            };

            struct NoCopy {
            NoCopy(NoCopy const&) = delete;
            };

            struct MoveOnly {
            int value;
            MoveOnly(int v) : value(v) {}
            MoveOnly(MoveOnly const&) = delete;
            MoveOnly(MoveOnly&&) = default;
            };

            struct MoveOnlyNT {
            int value;
            MoveOnlyNT(int v) : value(v) {}
            MoveOnlyNT(MoveOnlyNT const&) = delete;
            MoveOnlyNT(MoveOnlyNT&& other) : value(other.value) { other.value = -1; }
            };

            void test_move_ctor_sfinae() {
                using ATag = std::allocator_arg_t const&;
                using A = std::allocator<void>;
                {
                    using V = ranges::variant<int, long>;
                    STATIC_ASSERT(std::is_constructible<V, ATag, A const&, V&&>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnly>;
                    STATIC_ASSERT(std::is_constructible<V, ATag, A const&, V&&>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnlyNT>;
                    STATIC_ASSERT(std::is_constructible<V, ATag, A const&, V&&>::value);
                }
                {
                    using V = ranges::variant<int, NoCopy>;
                    STATIC_ASSERT(!std::is_constructible<V, ATag, A const&, V&&>::value);
                }
            }

            void test_move_ctor_basic()
            {
                const auto atag = std::allocator_arg;
                std::allocator<void> a{};
                {
                    ranges::variant<int> v(ranges::in_place<0>, 42);
                    ranges::variant<int> v2(atag, a, std::move(v));
                    CHECK(v2.index() == 0);
                    CHECK(ranges::get<0>(v2) == 42);
                }
                {
                    ranges::variant<int, long> v(ranges::in_place<1>, 42);
                    ranges::variant<int, long> v2(atag, a, std::move(v));
                    CHECK(v2.index() == 1);
                    CHECK(ranges::get<1>(v2) == 42);
                }
                {
                    ranges::variant<MoveOnly> v(ranges::in_place<0>, 42);
                    CHECK(v.index() == 0);
                    ranges::variant<MoveOnly> v2(atag, a, std::move(v));
                    CHECK(v2.index() == 0);
                    CHECK(ranges::get<0>(v2).value == 42);
                }
                {
                    ranges::variant<int, MoveOnly> v(ranges::in_place<1>, 42);
                    CHECK(v.index() == 1);
                    ranges::variant<int, MoveOnly> v2(atag, a, std::move(v));
                    CHECK(v2.index() == 1);
                    CHECK(ranges::get<1>(v2).value == 42);
                }
                {
                    ranges::variant<MoveOnlyNT> v(ranges::in_place<0>, 42);
                    CHECK(v.index() == 0);
                    ranges::variant<MoveOnlyNT> v2(atag, a, std::move(v));
                    CHECK(v2.index() == 0);
                    CHECK(ranges::get<0>(v).value == -1);
                    CHECK(ranges::get<0>(v2).value == 42);
                }
                {
                    ranges::variant<int, MoveOnlyNT> v(ranges::in_place<1>, 42);
                    CHECK(v.index() == 1);
                    ranges::variant<int, MoveOnlyNT> v2(atag, a, std::move(v));
                    CHECK(v2.index() == 1);
                    CHECK(ranges::get<1>(v).value == -1);
                    CHECK(ranges::get<1>(v2).value == 42);
                }
            }

            void test_move_ctor_uses_alloc()
            {
                using A = std::allocator<void>;
                using A2 = non_allocator<int>;
                using UA1 = UsesAllocatorV1<A, 1>;
                using UA2 = UsesAllocatorV2<A, 1>;
                using UA3 = UsesAllocatorV3<A, 1>;
                using NUA = NotUsesAllocator<A, 1>;
                using V = ranges::variant<UA1, UA2, UA3, NUA>;
                const A a{};
                const A2 a2{};
                {
                    V v(ranges::in_place<0>);
                    V v2(std::allocator_arg, a, std::move(v));
                    CHECK(v2.index() == 0);
                    CHECK(checkConstruct<UA1 &&>(ranges::get<0>(v2), UA_AllocArg));
                }
                {
                    V v(ranges::in_place<0>);
                    V v2(std::allocator_arg, a2, std::move(v));
                    CHECK(v2.index() == 0);
                    CHECK(checkConstruct<UA1 &&>(ranges::get<0>(v2), UA_None));
                }
                {
                    V v(ranges::in_place<1>);
                    V v2(std::allocator_arg, a, std::move(v));
                    CHECK(v2.index() == 1);
                    CHECK(checkConstruct<UA2 &&>(ranges::get<1>(v2), UA_AllocLast));
                }
                {
                    V v(ranges::in_place<1>);
                    V v2(std::allocator_arg, a2, std::move(v));
                    CHECK(v2.index() == 1);
                    CHECK(checkConstruct<UA2 &&>(ranges::get<1>(v2), UA_None));
                }
                {
                    V v(ranges::in_place<2>);
                    V v2(std::allocator_arg, a, std::move(v));
                    CHECK(v2.index() == 2);
                    CHECK(checkConstruct<UA3 &&>(ranges::get<2>(v2), UA_AllocArg));
                }
                {
                    V v(ranges::in_place<2>);
                    V v2(std::allocator_arg, a2, std::move(v));
                    CHECK(v2.index() == 2);
                    CHECK(checkConstruct<UA3 &&>(ranges::get<2>(v2), UA_None));
                }
                {
                    V v(ranges::in_place<3>);
                    V v3(std::allocator_arg, a, std::move(v));
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<NUA &&>(ranges::get<3>(v3), UA_None));
                }
                {
                    V v(ranges::in_place<3>);
                    V v3(std::allocator_arg, a2, std::move(v));
                    CHECK(v3.index() == 3);
                    CHECK(checkConstruct<NUA &&>(ranges::get<3>(v3), UA_None));
                }
            }

            void run_test()
            {
                test_move_ctor_basic();
                test_move_ctor_uses_alloc();
                test_move_ctor_sfinae();
            }
        }

        namespace Alloc_T {
            struct Dummy {
            Dummy() = default;
            };

            struct ThrowsT {
            ThrowsT(int) noexcept(false) {}
            };

            struct NoThrowT {
            NoThrowT(int) noexcept(true) {}
            };

            void test_T_ctor_sfinae() {
                using Tag = std::allocator_arg_t const&;
                using A = std::allocator<void> const&;
                {
                    using V = ranges::variant<int, int&&>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, int>::value);
                }
                {
                    using V = ranges::variant<int, int const&>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, int>::value);
                }
                {
                    using V = ranges::variant<long, unsigned>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, int>::value);
                }
                {
                    using V = ranges::variant<std::string, void*>;
                    STATIC_ASSERT(!std::is_constructible<V, Tag, A, int>::value);
                }
            }

            void test_T_ctor_basic()
            {
                using Tag = std::allocator_arg_t const&;
                using A = std::allocator<void> const&;
                const auto atag = std::allocator_arg;
                std::allocator<void> a;
                {
                    using V = ranges::variant<int>;
                    STATIC_ASSERT(test_convertible<V, Tag, A, int>());
                    V v(atag, a, 42);
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v) == 42);
                }
                {
                    using V = ranges::variant<int, long>;
                    V v(atag, a, 42l);
                    STATIC_ASSERT(test_convertible<V, Tag, A, long>());
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v) == 42);
                }
                {
                    using V = ranges::variant<int const&, int&&, long>;
                    STATIC_ASSERT(test_convertible<V, Tag, A, int&>());
                    int x = 42;
                    V v(atag, a, x);
                    CHECK(v.index() == 0);
                    CHECK(&ranges::get<0>(v) == &x);
                }
                {
                    using V = ranges::variant<int const&, int&&, long>;
                    STATIC_ASSERT(test_convertible<V, Tag, A, int>());
                    int x = 42;
                    V v(atag, a, std::move(x));
                    CHECK(v.index() == 1);
                    CHECK(&ranges::get<1>(v) == &x);
                }
            }

            void test_T_ctor_uses_alloc()
            {
                using A = std::allocator<void>;
                using A2 = non_allocator<int>;
                using UA1 = UsesAllocatorV1<A, 1>;
                using UA2 = UsesAllocatorV2<A, 1>;
                using UA3 = UsesAllocatorV3<A, 1>;
                using NUA = NotUsesAllocator<A, 1>;
                const A a{};
                const A2 a2{};
                {
                    using V = ranges::variant<int*, UA1, std::string>;
                    V v(std::allocator_arg, a, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_AllocArg));
                }
                {
                    using V = ranges::variant<int*, UA1, std::string>;
                    V v(std::allocator_arg, a2, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_None));
                }
                {
                    using V = ranges::variant<int*, UA2, std::string>;
                    V v(std::allocator_arg, a, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_AllocLast));
                }
                {
                    using V = ranges::variant<int*, UA2, std::string>;
                    V v(std::allocator_arg, a2, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_None));
                }
                {
                    using V = ranges::variant<int*, UA3, std::string>;
                    V v(std::allocator_arg, a, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_AllocArg));
                }
                {
                    using V = ranges::variant<int*, UA3, std::string>;
                    V v(std::allocator_arg, a2, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_None));
                }
                {
                    using V = ranges::variant<int*, NUA, std::string>;
                    V v(std::allocator_arg, a, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_None));
                }
                {
                    using V = ranges::variant<int*, NUA, std::string>;
                    V v(std::allocator_arg, a2, 42);
                    CHECK(v.index() == 1);
                    CHECK(checkConstruct<int&&>(ranges::get<1>(v), UA_None));
                }
            }

            void run_test()
            {
                test_T_ctor_basic();
                test_T_ctor_uses_alloc();
                test_T_ctor_sfinae();
            }
        }
#endif

        namespace copy {
            struct NonT {
            NonT(int v) : value(v) {}
            NonT(NonT const& o) : value(o.value) {}
            int value;
            };
            STATIC_ASSERT(!std::is_trivially_copy_constructible<NonT>::value);


            struct NoCopy {
            NoCopy(NoCopy const&) = delete;
            };

            struct MoveOnly {
            MoveOnly(MoveOnly const&) = delete;
            MoveOnly(MoveOnly&&) = default;
            };


            struct MoveOnlyNT {
            MoveOnlyNT(MoveOnlyNT const&) = delete;
            MoveOnlyNT(MoveOnlyNT&&) {}
            };



            #ifndef TEST_HAS_NO_EXCEPTIONS
            struct MakeEmptyT {
            static int alive;
            MakeEmptyT() { ++alive; }
            MakeEmptyT(MakeEmptyT const&) {
                ++alive;
                // Don't throw from the copy constructor since variant's assignment
                // operator performs a copy before committing to the assignment.
            }
            MakeEmptyT(MakeEmptyT &&) {
                throw 42;
            }
            MakeEmptyT& operator=(MakeEmptyT const&) {
                throw 42;
            }
            MakeEmptyT& operator=(MakeEmptyT&&) {
                throw 42;
            }
            ~MakeEmptyT() { --alive; }
            };

            int MakeEmptyT::alive = 0;

            template <class Variant>
            void makeEmpty(Variant& v) {
                Variant v2(ranges::in_place<MakeEmptyT>);
                try {
                    v = v2;
                    CHECK(false);
                }  catch (...) {
                    CHECK(v.valueless_by_exception());
                }
            }
            #endif // TEST_HAS_NO_EXCEPTIONS

            void test_copy_ctor_sfinae() {
                {
                    using V = ranges::variant<int, long>;
                    STATIC_ASSERT(std::is_copy_constructible<V>::value);
                }
                {
                    using V = ranges::variant<int, NoCopy>;
                    STATIC_ASSERT(!std::is_copy_constructible<V>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnly>;
                    STATIC_ASSERT(!std::is_copy_constructible<V>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnlyNT>;
                    STATIC_ASSERT(!std::is_copy_constructible<V>::value);
                }
            }

            void test_copy_ctor_basic()
            {
                {
                    ranges::variant<int> v(ranges::in_place<0>, 42);
                    ranges::variant<int> v2 = v;
                    CHECK(v2.index() == 0);
                    CHECK(ranges::get<0>(v2) == 42);
                }
                {
                    ranges::variant<int, long> v(ranges::in_place<1>, 42);
                    ranges::variant<int, long> v2 = v;
                    CHECK(v2.index() == 1);
                    CHECK(ranges::get<1>(v2) == 42);
                }
                {
                    ranges::variant<NonT> v(ranges::in_place<0>, 42);
                    CHECK(v.index() == 0);
                    ranges::variant<NonT> v2(v);
                    CHECK(v2.index() == 0);
                    CHECK(ranges::get<0>(v2).value == 42);
                }
                {
                    ranges::variant<int, NonT> v(ranges::in_place<1>, 42);
                    CHECK(v.index() == 1);
                    ranges::variant<int, NonT> v2(v);
                    CHECK(v2.index() == 1);
                    CHECK(ranges::get<1>(v2).value == 42);
                }
            }

            void test_copy_ctor_valueless_by_exception()
            {
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using V = ranges::variant<int, MakeEmptyT>;
                V v1; makeEmpty(v1);
                V const& cv1 = v1;
                V v(cv1);
                CHECK(v.valueless_by_exception());
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void run_test()
            {
                test_copy_ctor_basic();
                test_copy_ctor_valueless_by_exception();
                test_copy_ctor_sfinae();
            }
        }

        namespace default_ {
            struct NonDefaultConstructible {
            NonDefaultConstructible(int) {}
            };

            struct NotNoexcept {
            NotNoexcept() noexcept(false) {}
            };

            #ifndef TEST_HAS_NO_EXCEPTIONS
            struct DefaultCtorThrows {
            DefaultCtorThrows() { throw 42; }
            };
            #endif // TEST_HAS_NO_EXCEPTIONS

            void test_default_ctor_sfinae() {
                {
                    using V = ranges::variant<ranges::monostate, int>;
                    STATIC_ASSERT(std::is_default_constructible<V>::value);
                }
                {
                    using V = ranges::variant<NonDefaultConstructible, int>;
                    STATIC_ASSERT(!std::is_default_constructible<V>::value);
                }
                {
                    using V = ranges::variant<int&, int>;
                    STATIC_ASSERT(!std::is_default_constructible<V>::value);
                }
            }

            void test_default_ctor_noexcept() {
                {
                    using V = ranges::variant<int>;
                    STATIC_ASSERT(std::is_nothrow_default_constructible<V>::value);
                }
                {
                    using V = ranges::variant<NotNoexcept>;
                    STATIC_ASSERT(!std::is_nothrow_default_constructible<V>::value);
                }
            }

            void test_default_ctor_throws()
            {
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using V = ranges::variant<DefaultCtorThrows, int>;
                try {
                    V v;
                    CHECK(false);
                } catch (int const& ex) {
                    CHECK(ex == 42);
                } catch (...) {
                    CHECK(false);
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void test_default_ctor_basic()
            {
                {
                    ranges::variant<int> v;
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v) == 0);
                }
                {
                    ranges::variant<int, long, const void> v;
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v) == 0);
                }
                {
                    using V = ranges::variant<int, void, long>;
                    constexpr V v;
                    STATIC_ASSERT(v.index() == 0);
                    STATIC_ASSERT(ranges::get<0>(v) == 0);
                }
                {
                    using V = ranges::variant<int, long>;
                    constexpr V v;
                    STATIC_ASSERT(v.index() == 0);
                    STATIC_ASSERT(ranges::get<0>(v) == 0);
                }
            }

            void run_test()
            {
                test_default_ctor_basic();
                test_default_ctor_sfinae();
                test_default_ctor_noexcept();
                test_default_ctor_throws();
            }
        }

        namespace in_place_index_Args {
            void test_ctor_sfinae() {
                {
                    using V = ranges::variant<int>;
                    STATIC_ASSERT(std::is_constructible<V, ranges::in_place_index_t<0>, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_index_t<0>, int>());
                }
                {
                    using V = ranges::variant<int, long, long long>;
                    STATIC_ASSERT(std::is_constructible<V, ranges::in_place_index_t<1>, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_index_t<1>, int>());
                }
                {
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(std::is_constructible<V, ranges::in_place_index_t<2>, int*>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_index_t<2>, int*>());
                }
                { // args not convertible to type
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_index_t<0>, int*>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_index_t<0>, int*>());
                }
                { // index not in variant
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_index_t<3>, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_index_t<3>, int>());
                }
            }

            void test_ctor_basic()
            {
                {
                    constexpr ranges::variant<int> v(ranges::in_place<0>, 42);
                    STATIC_ASSERT(v.index() == 0);
                    STATIC_ASSERT(ranges::get<0>(v) == 42);
                }
                {
                    constexpr ranges::variant<int, long, long> v(ranges::in_place<1>, 42);
                    STATIC_ASSERT(v.index() == 1);
                    STATIC_ASSERT(ranges::get<1>(v) == 42);
                }
                {
                    constexpr ranges::variant<int, const int, long> v(ranges::in_place<1>, 42);
                    STATIC_ASSERT(v.index() == 1);
                    STATIC_ASSERT(ranges::get<1>(v) == 42);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(ranges::in_place<0>, x);
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v) == x);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(ranges::in_place<1>, x);
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v) == x);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(ranges::in_place<2>, x);
                    CHECK(v.index() == 2);
                    CHECK(ranges::get<2>(v) == x);
                }
            }

            void run_test()
            {
                test_ctor_basic();
                test_ctor_sfinae();
            }
        }

        namespace in_place_index_init_list_Args {
            struct InitList {
            std::size_t size;
            constexpr InitList(std::initializer_list<int> il) : size(il.size()){}
            };

            struct InitListArg {
            std::size_t size;
            int value;
            constexpr InitListArg(std::initializer_list<int> il, int v) : size(il.size()), value(v) {}
            };

            void test_ctor_sfinae() {
                using IL = std::initializer_list<int>;
                { // just init list
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(std::is_constructible<V, ranges::in_place_index_t<0>, IL>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_index_t<0>, IL>());
                }
                { // too many arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_index_t<0>, IL, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_index_t<0>, IL, int>());
                }
                { // too few arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_index_t<1>, IL>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_index_t<1>, IL>());
                }
                { // init list and arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(std::is_constructible<V, ranges::in_place_index_t<1>, IL, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_index_t<1>, IL, int>());
                }
                { // not constructible from arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_index_t<2>, IL>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_index_t<2>, IL>());
                }
            }

            void test_ctor_basic()
            {
                {
                    constexpr ranges::variant<InitList, InitListArg, InitList> v(ranges::in_place<0>, {1, 2, 3});
                    STATIC_ASSERT(v.index() == 0);
                    STATIC_ASSERT(ranges::get<0>(v).size == 3);
                }
                {
                    constexpr ranges::variant<InitList, InitListArg, InitList> v(ranges::in_place<2>, {1, 2, 3});
                    STATIC_ASSERT(v.index() == 2);
                    STATIC_ASSERT(ranges::get<2>(v).size == 3);
                }
                {
                    constexpr ranges::variant<InitList, InitListArg, InitListArg> v(ranges::in_place<1>, {1, 2, 3, 4}, 42);
                    STATIC_ASSERT(v.index() == 1);
                    STATIC_ASSERT(ranges::get<1>(v).size == 4);
                    STATIC_ASSERT(ranges::get<1>(v).value == 42);
                }
            }

            void run_test()
            {
                test_ctor_basic();
                test_ctor_sfinae();
            }
        }

        namespace in_place_type_Args {
            void test_ctor_sfinae() {
                {
                    using V = ranges::variant<int>;
                    STATIC_ASSERT(std::is_constructible<V, ranges::in_place_type_t<int>, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<int>, int>());
                }
                {
                    using V = ranges::variant<int, long, long long>;
                    STATIC_ASSERT(std::is_constructible<V, ranges::in_place_type_t<long>, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<long>, int>());
                }
                {
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(std::is_constructible<V, ranges::in_place_type_t<int*>, int*>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<int*>, int*>());
                }
                { // duplicate type
                    using V = ranges::variant<int, long, int>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_type_t<int>, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<int>, int>());
                }
                { // args not convertible to type
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_type_t<int>, int*>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<int>, int*>());
                }
                { // type not in variant
                    using V = ranges::variant<int, long, int*>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_type_t<long long>, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<long long>, int>());
                }
            }

            void test_ctor_basic()
            {
                {
                    constexpr ranges::variant<int> v(ranges::in_place<int>, 42);
                    STATIC_ASSERT(v.index() == 0);
                    STATIC_ASSERT(ranges::get<0>(v) == 42);
                }
                {
                    constexpr ranges::variant<int, long> v(ranges::in_place<long>, 42);
                    STATIC_ASSERT(v.index() == 1);
                    STATIC_ASSERT(ranges::get<1>(v) == 42);
                }
                {
                    constexpr ranges::variant<int, const int, long> v(ranges::in_place<const int>, 42);
                    STATIC_ASSERT(v.index() == 1);
                    STATIC_ASSERT(ranges::get<1>(v) == 42);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(ranges::in_place<const int>, x);
                    CHECK(v.index() == 0);
                    CHECK(ranges::get<0>(v) == x);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(ranges::in_place<volatile int>, x);
                    CHECK(v.index() == 1);
                    CHECK(ranges::get<1>(v) == x);
                }
                {
                    using V = ranges::variant<const int, volatile int, int>;
                    int x = 42;
                    V v(ranges::in_place<int>, x);
                    CHECK(v.index() == 2);
                    CHECK(ranges::get<2>(v) == x);
                }
            }

            void run_test()
            {
                test_ctor_basic();
                test_ctor_sfinae();
            }
        }

        namespace in_place_type_init_list_Args {
            struct InitList {
            std::size_t size;
            constexpr InitList(std::initializer_list<int> il) : size(il.size()){}
            };

            struct InitListArg {
            std::size_t size;
            int value;
            constexpr InitListArg(std::initializer_list<int> il, int v) : size(il.size()), value(v) {}
            };

            void test_ctor_sfinae() {
                using IL = std::initializer_list<int>;
                { // just init list
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(std::is_constructible<V, ranges::in_place_type_t<InitList>, IL>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<InitList>, IL>());
                }
                { // too many arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_type_t<InitList>, IL, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<InitList>, IL, int>());
                }
                { // too few arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_type_t<InitListArg>, IL>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<InitListArg>, IL>());
                }
                { // init list and arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(std::is_constructible<V, ranges::in_place_type_t<InitListArg>, IL, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<InitListArg>, IL, int>());
                }
                { // not constructible from arguments
                    using V = ranges::variant<InitList, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_type_t<int>, IL>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<int>, IL>());
                }
                { // duplicate types in variant
                    using V = ranges::variant<InitListArg, InitListArg, int>;
                    STATIC_ASSERT(!std::is_constructible<V, ranges::in_place_type_t<InitListArg>, IL, int>::value);
                    STATIC_ASSERT(!test_convertible<V, ranges::in_place_type_t<InitListArg>, IL, int>());
                }
            }

            void test_ctor_basic()
            {
                {
                    constexpr ranges::variant<InitList, InitListArg> v(ranges::in_place<InitList>, {1, 2, 3});
                    STATIC_ASSERT(v.index() == 0);
                    STATIC_ASSERT(ranges::get<0>(v).size == 3);
                }
                {
                    constexpr ranges::variant<InitList, InitListArg> v(ranges::in_place<InitListArg>, {1, 2, 3, 4}, 42);
                    STATIC_ASSERT(v.index() == 1);
                    STATIC_ASSERT(ranges::get<1>(v).size == 4);
                    STATIC_ASSERT(ranges::get<1>(v).value == 42);
                }
            }

            void run_test()
            {
                test_ctor_basic();
                test_ctor_sfinae();
            }
        }

        namespace move {
            struct ThrowsMove {
            ThrowsMove(ThrowsMove&&) noexcept(false) {}
            };

            struct NoCopy {
            NoCopy(NoCopy const&) = delete;
            };

            struct MoveOnly {
            int value;
            MoveOnly(int v) : value(v) {}
            MoveOnly(MoveOnly const&) = delete;
            MoveOnly(MoveOnly&&) = default;
            };

            struct MoveOnlyNT {
            int value;
            MoveOnlyNT(int v) : value(v) {}
            MoveOnlyNT(MoveOnlyNT const&) = delete;
            MoveOnlyNT(MoveOnlyNT&& other) : value(other.value) { other.value = -1; }
            };


            #ifndef TEST_HAS_NO_EXCEPTIONS
            struct MakeEmptyT {
            static int alive;
            MakeEmptyT() { ++alive; }
            MakeEmptyT(MakeEmptyT const&) {
                ++alive;
                // Don't throw from the copy constructor since variant's assignment
                // operator performs a copy before committing to the assignment.
            }
            MakeEmptyT(MakeEmptyT &&) {
                throw 42;
            }
            MakeEmptyT& operator=(MakeEmptyT const&) {
                throw 42;
            }
            MakeEmptyT& operator=(MakeEmptyT&&) {
                throw 42;
            }
            ~MakeEmptyT() { --alive; }
            };

            int MakeEmptyT::alive = 0;

            template <class Variant>
            void makeEmpty(Variant& v) {
                Variant v2(ranges::in_place<MakeEmptyT>);
                try {
                    v = v2;
                    CHECK(false);
                }  catch (...) {
                    CHECK(v.valueless_by_exception());
                }
            }
            #endif // TEST_HAS_NO_EXCEPTIONS

            void test_move_noexcept() {
                {
                    using V = ranges::variant<int, long>;
                    STATIC_ASSERT(std::is_nothrow_move_constructible<V>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnly>;
                    STATIC_ASSERT(std::is_nothrow_move_constructible<V>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnlyNT>;
                    STATIC_ASSERT(!std::is_nothrow_move_constructible<V>::value);
                }
                {
                    using V = ranges::variant<int, ThrowsMove>;
                    STATIC_ASSERT(!std::is_nothrow_move_constructible<V>::value);
                }
            }

            void test_move_ctor_sfinae() {
                {
                    using V = ranges::variant<int, long>;
                    STATIC_ASSERT(std::is_move_constructible<V>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnly>;
                    STATIC_ASSERT(std::is_move_constructible<V>::value);
                }
                {
                    using V = ranges::variant<int, MoveOnlyNT>;
                    STATIC_ASSERT(std::is_move_constructible<V>::value);
                }
                {
                    using V = ranges::variant<int, NoCopy>;
                    STATIC_ASSERT(!std::is_move_constructible<V>::value);
                }
            }

            void test_move_ctor_basic()
            {
                {
                    ranges::variant<int> v(ranges::in_place<0>, 42);
                    ranges::variant<int> v2 = std::move(v);
                    CHECK(v2.index() == 0);
                    CHECK(ranges::get<0>(v2) == 42);
                }
                {
                    ranges::variant<int, long> v(ranges::in_place<1>, 42);
                    ranges::variant<int, long> v2 = std::move(v);
                    CHECK(v2.index() == 1);
                    CHECK(ranges::get<1>(v2) == 42);
                }
                {
                    ranges::variant<MoveOnly> v(ranges::in_place<0>, 42);
                    CHECK(v.index() == 0);
                    ranges::variant<MoveOnly> v2(std::move(v));
                    CHECK(v2.index() == 0);
                    CHECK(ranges::get<0>(v2).value == 42);
                }
                {
                    ranges::variant<int, MoveOnly> v(ranges::in_place<1>, 42);
                    CHECK(v.index() == 1);
                    ranges::variant<int, MoveOnly> v2(std::move(v));
                    CHECK(v2.index() == 1);
                    CHECK(ranges::get<1>(v2).value == 42);
                }
                {
                    ranges::variant<MoveOnlyNT> v(ranges::in_place<0>, 42);
                    CHECK(v.index() == 0);
                    ranges::variant<MoveOnlyNT> v2(std::move(v));
                    CHECK(v2.index() == 0);
                    CHECK(ranges::get<0>(v).value == -1);
                    CHECK(ranges::get<0>(v2).value == 42);
                }
                {
                    ranges::variant<int, MoveOnlyNT> v(ranges::in_place<1>, 42);
                    CHECK(v.index() == 1);
                    ranges::variant<int, MoveOnlyNT> v2(std::move(v));
                    CHECK(v2.index() == 1);
                    CHECK(ranges::get<1>(v).value == -1);
                    CHECK(ranges::get<1>(v2).value == 42);
                }
            }

            void test_move_ctor_valueless_by_exception()
            {
            #ifndef TEST_HAS_NO_EXCEPTIONS
                using V = ranges::variant<int, MakeEmptyT>;
                V v1; makeEmpty(v1);
                V v(std::move(v1));
                CHECK(v.valueless_by_exception());
            #endif // TEST_HAS_NO_EXCEPTIONS
            }

            void run_test()
            {
                test_move_ctor_basic();
                test_move_ctor_valueless_by_exception();
                test_move_noexcept();
                test_move_ctor_sfinae();
            }
        }

        namespace T {
            struct Dummy {
            Dummy() = default;
            };

            struct ThrowsT {
            ThrowsT(int) noexcept(false) {}
            };

            struct NoThrowT {
            NoThrowT(int) noexcept(true) {}
            };

            void test_T_ctor_noexcept() {
                {
                    using V = ranges::variant<Dummy, NoThrowT>;
                    STATIC_ASSERT(std::is_nothrow_constructible<V, int>::value);
                }
                {
                    using V = ranges::variant<Dummy, ThrowsT>;
                    STATIC_ASSERT(!std::is_nothrow_constructible<V, int>::value);
                }
            }

            void test_T_ctor_sfinae() {
                {
                    using V = ranges::variant<int, int&&>;
                    STATIC_ASSERT(!std::is_constructible<V, int>::value);
                }
                {
                    using V = ranges::variant<int, int const&>;
                    STATIC_ASSERT(!std::is_constructible<V, int>::value);
                }
                {
                    using V = ranges::variant<long, unsigned>;
                    STATIC_ASSERT(!std::is_constructible<V, int>::value);
                }
                {
                    using V = ranges::variant<std::string, std::string>;
                    STATIC_ASSERT(!std::is_constructible<V, const char*>::value);
                }
                {
                    using V = ranges::variant<std::string, void*>;
                    STATIC_ASSERT(!std::is_constructible<V, int>::value);
                }
            }

            void test_T_ctor_basic()
            {
                {
                    constexpr ranges::variant<int> v(42);
                    STATIC_ASSERT(v.index() == 0);
                    STATIC_ASSERT(ranges::get<0>(v) == 42);
                }
                {
                    constexpr ranges::variant<int, long> v(42l);
                    STATIC_ASSERT(v.index() == 1);
                    STATIC_ASSERT(ranges::get<1>(v) == 42);
                }
                {
                    using V = ranges::variant<int const&, int&&, long>;
                    STATIC_ASSERT(std::is_convertible<int&, V>::value);
                    int x = 42;
                    V v(x);
                    CHECK(v.index() == 0);
                    CHECK(&ranges::get<0>(v) == &x);
                }
                {
                    using V = ranges::variant<int const&, int&&, long>;
                    STATIC_ASSERT(std::is_convertible<int, V>::value);
                    int x = 42;
                    V v(std::move(x));
                    CHECK(v.index() == 1);
                    CHECK(&ranges::get<1>(v) == &x);
                }
            }

            void run_test()
            {
                test_T_ctor_basic();
                test_T_ctor_noexcept();
                test_T_ctor_sfinae();
            }
        }
    } // namespace ctor

    namespace dtor {
        struct NonTDtor {
        static int count;
        NonTDtor() = default;
        ~NonTDtor() { ++count; }
        };
        int NonTDtor::count = 0;
        STATIC_ASSERT(!std::is_trivially_destructible<NonTDtor>::value);


        struct NonTDtor1 {
        static int count;
        NonTDtor1() = default;
        ~NonTDtor1() { ++count; }
        };
        int NonTDtor1::count = 0;
        STATIC_ASSERT(!std::is_trivially_destructible<NonTDtor1>::value);

        struct TDtor {
        TDtor(TDtor const&) {} // nontrivial copy
        ~TDtor() = default;
        };
        STATIC_ASSERT(!std::is_trivially_copy_constructible<TDtor>::value);
        STATIC_ASSERT(std::is_trivially_destructible<TDtor>::value);

        void run_test()
        {
            {
                using V = ranges::variant<int, long, TDtor>;
                STATIC_ASSERT(std::is_trivially_destructible<V>::value);
            }
            {
                using V = ranges::variant<void, void const, void const volatile, void volatile>;
                STATIC_ASSERT(std::is_trivially_destructible<V>::value);
            }
            {
                using V = ranges::variant<NonTDtor, int, NonTDtor1>;
                STATIC_ASSERT(!std::is_trivially_destructible<V>::value);
                {
                    V v(ranges::in_place<0>);
                    CHECK(NonTDtor::count == 0);
                    CHECK(NonTDtor1::count == 0);
                }
                CHECK(NonTDtor::count == 1);
                CHECK(NonTDtor1::count == 0);
                NonTDtor::count = 0;
                {
                    V v(ranges::in_place<1>);
                }
                CHECK(NonTDtor::count == 0);
                CHECK(NonTDtor1::count == 0);
                {
                    V v(ranges::in_place<2>);
                    CHECK(NonTDtor::count == 0);
                    CHECK(NonTDtor1::count == 0);
                }
                CHECK(NonTDtor::count == 0);
                CHECK(NonTDtor1::count == 1);
            }
        }
    } // namespace dtor

    namespace emplace {
        namespace index {
            template <class Var, size_t I, class ...Args>
            constexpr auto test_emplace_exists_imp(int) ->
                decltype(std::declval<Var>().template emplace<I>(std::declval<Args>()...), true)
            { return true; }

            template <class, size_t, class...>
            constexpr auto test_emplace_exists_imp(long) -> bool { return false; }

            template <class Var, size_t I, class ...Args>
            constexpr bool emplace_exists() { return test_emplace_exists_imp<Var, I, Args...>(0); }

            void test_emplace_sfinae() {
                using V = ranges::variant<int, int&, int const&, int&&,
                                    void>;
                STATIC_ASSERT(emplace_exists<V, 0>());
                STATIC_ASSERT(emplace_exists<V, 0, int>());
                STATIC_ASSERT(emplace_exists<V, 0, short>());
                STATIC_ASSERT(!emplace_exists<V, 0, int, int>());
                STATIC_ASSERT(emplace_exists<V, 1, int&>());
                STATIC_ASSERT(!emplace_exists<V, 1>());
                STATIC_ASSERT(!emplace_exists<V, 1, int const&>());
                STATIC_ASSERT(!emplace_exists<V, 1, int&&>());
                STATIC_ASSERT(emplace_exists<V, 2, int&>());
                STATIC_ASSERT(emplace_exists<V, 2, const int&>());
                STATIC_ASSERT(emplace_exists<V, 2, int&&>());
                STATIC_ASSERT(!emplace_exists<V, 2, void*>());
                STATIC_ASSERT(emplace_exists<V, 3, int>());
                STATIC_ASSERT(!emplace_exists<V, 3, int&>());
                STATIC_ASSERT(!emplace_exists<V, 3, int const&>());
                STATIC_ASSERT(!emplace_exists<V, 3, int const&&>());
                STATIC_ASSERT(!emplace_exists<V, 4>());
            }

            void test_basic() {
                {
                    using V = ranges::variant<int>;
                    V v(42);
                    v.emplace<0>();
                    CHECK(ranges::get<0>(v) == 0);
                    v.emplace<0>(42);
                    CHECK(ranges::get<0>(v) == 42);
                }
                {
                    using V = ranges::variant<int, long, int const&, int &&, void, std::string>;
                    const int x = 100;
                    int y = 42;
                    int z = 43;
                    V v(ranges::in_place<0>, -1);
                    // default emplace a value
                    v.emplace<1>();
                    CHECK(ranges::get<1>(v) == 0);
                    // emplace a reference
                    v.emplace<2>(x);
                    CHECK(&ranges::get<2>(v) == &x);
                    // emplace an rvalue reference
                    v.emplace<3>(std::move(y));
                    CHECK(&ranges::get<3>(v) == &y);
                    // re-emplace a new reference over the active member
                    v.emplace<3>(std::move(z));
                    CHECK(&ranges::get<3>(v) == &z);
                    // emplace with multiple args
                    v.emplace<5>('a', 'a', 'a');
                    CHECK(ranges::get<5>(v) == "aaa");
                }
            }


            void run_test()
            {
                test_basic();
                test_emplace_sfinae();
            }
        }

        namespace index_init_list {
            struct InitList {
            std::size_t size;
            constexpr InitList(std::initializer_list<int> il) : size(il.size()){}
            };

            struct InitListArg {
            std::size_t size;
            int value;
            constexpr InitListArg(std::initializer_list<int> il, int v) : size(il.size()), value(v) {}
            };

            template <class Var, size_t I, class ...Args>
            constexpr auto test_emplace_exists_imp(int) ->
                decltype(std::declval<Var>().template emplace<I>(std::declval<Args>()...), true)
            { return true; }

            template <class, size_t, class...>
            constexpr auto test_emplace_exists_imp(long) -> bool { return false; }

            template <class Var, size_t I, class ...Args>
            constexpr bool emplace_exists() { return test_emplace_exists_imp<Var, I, Args...>(0); }

            void test_emplace_sfinae() {
                using V = ranges::variant<int, void, InitList, InitListArg, long, long>;
                using IL = std::initializer_list<int>;
                STATIC_ASSERT(emplace_exists<V, 2, IL>());
                STATIC_ASSERT(emplace_exists<V, 2, int>());
                STATIC_ASSERT(!emplace_exists<V, 2, IL, int>());
                STATIC_ASSERT(emplace_exists<V, 3, IL, int>());
                STATIC_ASSERT(!emplace_exists<V, 3, int>());
                STATIC_ASSERT(!emplace_exists<V, 3, IL>());
                STATIC_ASSERT(!emplace_exists<V, 3, IL, int, int>());
            }

            void test_basic() {
                using V = ranges::variant<int, InitList, InitListArg, void>;
                V v;
                v.emplace<1>({1, 2, 3});
                CHECK(ranges::get<1>(v).size == 3);
                v.emplace<2>({1, 2, 3, 4}, 42);
                CHECK(ranges::get<2>(v).size == 4);
                CHECK(ranges::get<2>(v).value == 42);
                v.emplace<1>({1});
                CHECK(ranges::get<1>(v).size == 1);
            }

            void run_test()
            {
                test_basic();
                test_emplace_sfinae();
            }
        }

        namespace type {
            template <class Var, class T, class ...Args>
            constexpr auto test_emplace_exists_imp(int) ->
                decltype(std::declval<Var>().template emplace<T>(std::declval<Args>()...), true)
            { return true; }

            template <class, class, class...>
            constexpr auto test_emplace_exists_imp(long) -> bool { return false; }

            template <class ...Args>
            constexpr bool emplace_exists() { return test_emplace_exists_imp<Args...>(0); }

            void test_emplace_sfinae() {
                using V = ranges::variant<int, int&, int const&, int&&,
                                    long, long,
                                    void>;
                STATIC_ASSERT(emplace_exists<V, int>());
                STATIC_ASSERT(emplace_exists<V, int, int>());
                STATIC_ASSERT(!emplace_exists<V, int, long long>());
                STATIC_ASSERT(!emplace_exists<V, int, int, int>());
                STATIC_ASSERT(emplace_exists<V, int &, int&>());
                STATIC_ASSERT(!emplace_exists<V, int &>());
                STATIC_ASSERT(!emplace_exists<V, int &, int const&>());
                STATIC_ASSERT(!emplace_exists<V, int &, int&&>());
                STATIC_ASSERT(emplace_exists<V, int const&, int&>());
                STATIC_ASSERT(emplace_exists<V, int const&, const int&>());
                STATIC_ASSERT(emplace_exists<V, int const&, int&&>());
                STATIC_ASSERT(!emplace_exists<V, int const&, void*>());
                STATIC_ASSERT(emplace_exists<V, int&&, int>());
                STATIC_ASSERT(!emplace_exists<V, int&&, int&>());
                STATIC_ASSERT(!emplace_exists<V, int&&, int const&>());
                STATIC_ASSERT(!emplace_exists<V, int&&, int const&&>());
                STATIC_ASSERT(!emplace_exists<V, long, long>());
                STATIC_ASSERT(!emplace_exists<V, void>());
            }

            void test_basic() {
                {
                    using V = ranges::variant<int>;
                    V v(42);
                    v.emplace<int>();
                    CHECK(ranges::get<0>(v) == 0);
                    v.emplace<int>(42);
                    CHECK(ranges::get<0>(v) == 42);
                }
                {
                    using V = ranges::variant<int, long, int const&, int &&, void, std::string>;
                    const int x = 100;
                    int y = 42;
                    int z = 43;
                    V v(ranges::in_place<0>, -1);
                    // default emplace a value
                    v.emplace<long>();
                    CHECK(ranges::get<long>(v) == 0);
                    // emplace a reference
                    v.emplace<int const&>(x);
                    CHECK(&ranges::get<int const&>(v) == &x);
                    // emplace an rvalue reference
                    v.emplace<int&&>(std::move(y));
                    CHECK(&ranges::get<int&&>(v) == &y);
                    // re-emplace a new reference over the active member
                    v.emplace<int&&>(std::move(z));
                    CHECK(&ranges::get<int&&>(v) == &z);
                    // emplace with multiple args
                    v.emplace<std::string>('a', 'a', 'a');
                    CHECK(ranges::get<std::string>(v) == "aaa");
                }
            }


            void run_test()
            {
                test_basic();
                test_emplace_sfinae();
            }
        }

        namespace type_init_list {
            struct InitList {
            std::size_t size;
            constexpr InitList(std::initializer_list<int> il) : size(il.size()){}
            };

            struct InitListArg {
            std::size_t size;
            int value;
            constexpr InitListArg(std::initializer_list<int> il, int v) : size(il.size()), value(v) {}
            };

            template <class Var, class T, class ...Args>
            constexpr auto test_emplace_exists_imp(int) ->
                decltype(std::declval<Var>().template emplace<T>(std::declval<Args>()...), true)
            { return true; }

            template <class, class, class...>
            constexpr auto test_emplace_exists_imp(long) -> bool { return false; }

            template <class ...Args>
            constexpr bool emplace_exists() { return test_emplace_exists_imp<Args...>(0); }

            void test_emplace_sfinae() {
                using V = ranges::variant<int, void, InitList, InitListArg, long, long>;
                using IL = std::initializer_list<int>;
                STATIC_ASSERT(emplace_exists<V, InitList, IL>());
                STATIC_ASSERT(emplace_exists<V, InitList, int>());
                STATIC_ASSERT(!emplace_exists<V, InitList, IL, int>());
                STATIC_ASSERT(emplace_exists<V, InitListArg, IL, int>());
                STATIC_ASSERT(!emplace_exists<V, InitListArg, int>());
                STATIC_ASSERT(!emplace_exists<V, InitListArg, IL>());
                STATIC_ASSERT(!emplace_exists<V, InitListArg, IL, int, int>());
            }

            void test_basic() {
                using V = ranges::variant<int, InitList, InitListArg, void>;
                V v;
                v.emplace<InitList>({1, 2, 3});
                CHECK(ranges::get<InitList>(v).size == 3);
                v.emplace<InitListArg>({1, 2, 3, 4}, 42);
                CHECK(ranges::get<InitListArg>(v).size == 4);
                CHECK(ranges::get<InitListArg>(v).value == 42);
                v.emplace<InitList>({1});
                CHECK(ranges::get<InitList>(v).size == 1);
            }

            void run_test()
            {
                test_basic();
                test_emplace_sfinae();
            }
        }
    } // namespace emplace

    namespace status {
        namespace index {
            void run_test()
            {
                {
                    using V = ranges::variant<int, void>;
                    constexpr V v;
                    STATIC_ASSERT(v.index() == 0);
                }
                {
                    using V = ranges::variant<int, long>;
                    constexpr V v(ranges::in_place<1>);
                    STATIC_ASSERT(v.index() == 1);
                }
                {
                    using V = ranges::variant<int, std::string>;
                    V v("abc");
                    CHECK(v.index() == 1);
                    v = 42;
                    CHECK(v.index() == 0);
                }
            #ifndef TEST_HAS_NO_EXCEPTIONS
                {
                    using V = ranges::variant<int, MakeEmptyT>;
                    V v;
                    CHECK(v.index() == 0);
                    makeEmpty(v);
                    CHECK(v.index() == ranges::variant_npos);
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }
        }

        namespace valueless_by_exception {
            void run_test()
            {
                {
                    using V = ranges::variant<int, void>;
                    constexpr V v;
                    STATIC_ASSERT(!v.valueless_by_exception());
                }
                {
                    using V = ranges::variant<int, long, std::string>;
                    const V v("abc");
                    CHECK(!v.valueless_by_exception());
                }
            #ifndef TEST_HAS_NO_EXCEPTIONS
                {
                    using V = ranges::variant<int, MakeEmptyT>;
                    V v;
                    CHECK(!v.valueless_by_exception());
                    makeEmpty(v);
                    CHECK(v.valueless_by_exception());
                }
            #endif // TEST_HAS_NO_EXCEPTIONS
            }
        }
    } // namespace status

    namespace swap_ {
        struct NotSwappable {
        };
        inline void swap(NotSwappable&, NotSwappable&) = delete;

        struct NotCopyable {
        NotCopyable() = default;
        NotCopyable(NotCopyable const&) = delete;
        NotCopyable& operator=(NotCopyable const&) = delete;
        };

        struct NotCopyableWithSwap {
        NotCopyableWithSwap() = default;
        NotCopyableWithSwap(NotCopyableWithSwap const&) = delete;
        NotCopyableWithSwap& operator=(NotCopyableWithSwap const&) = delete;
        };
#if 0 // UNUSED
        inline void swap(NotCopyableWithSwap&, NotCopyableWithSwap) {}
#endif // UNUSED

        struct NotMoveAssignable {
        NotMoveAssignable() = default;
        NotMoveAssignable(NotMoveAssignable&&) = default;
        NotMoveAssignable& operator=(NotMoveAssignable&&) = delete;
        };

        struct NotMoveAssignableWithSwap {
        NotMoveAssignableWithSwap() = default;
        NotMoveAssignableWithSwap(NotMoveAssignableWithSwap&&) = default;
        NotMoveAssignableWithSwap& operator=(NotMoveAssignableWithSwap&&) = delete;
        };
        inline void swap(NotMoveAssignableWithSwap&, NotMoveAssignableWithSwap&) noexcept {}

        template <bool Throws>
        void do_throw() {}

        template <>
        void do_throw<true>() {
        #ifndef TEST_HAS_NO_EXCEPTIONS
            throw 42;
        #else // ^^^ !TEST_HAS_NO_EXCEPTIONS / TEST_HAS_NO_EXCEPTIONS vvv
            std::abort();
        #endif // TEST_HAS_NO_EXCEPTIONS
        }

        template <bool NT_Copy, bool NT_Move, bool NT_CopyAssign, bool NT_MoveAssign,
                bool NT_Swap, bool EnableSwap = true>
        struct NothrowTypeImp {
        static int move_called;
        static int move_assign_called;
        static int swap_called;
        static void reset() { move_called = move_assign_called = swap_called = 0; }
        NothrowTypeImp() = default;
        explicit NothrowTypeImp(int v) : value(v) {}
        NothrowTypeImp(NothrowTypeImp const& o) noexcept(NT_Copy)
            : value(o.value)
        { CHECK(false); } // never called by test
        NothrowTypeImp(NothrowTypeImp&& o) noexcept(NT_Move)
            : value(o.value)
        { ++move_called; do_throw<!NT_Move>(); o.value = -1; }
        NothrowTypeImp& operator=(NothrowTypeImp const&) noexcept(NT_CopyAssign)
        { CHECK(false); return *this;} // never called by the tests
        NothrowTypeImp& operator=(NothrowTypeImp&& o) noexcept(NT_MoveAssign) {
            ++move_assign_called;
            do_throw<!NT_MoveAssign>();
            value = o.value; o.value = -1;
            return *this;
        }
        int value;
        };
        template <bool NT_Copy, bool NT_Move, bool NT_CopyAssign, bool NT_MoveAssign,
                bool NT_Swap, bool EnableSwap>
        int NothrowTypeImp<NT_Copy, NT_Move, NT_CopyAssign, NT_MoveAssign,
                        NT_Swap, EnableSwap>::move_called = 0;
        template <bool NT_Copy, bool NT_Move, bool NT_CopyAssign, bool NT_MoveAssign,
                bool NT_Swap, bool EnableSwap>
        int NothrowTypeImp<NT_Copy, NT_Move, NT_CopyAssign, NT_MoveAssign,
                        NT_Swap, EnableSwap>::move_assign_called = 0;
        template <bool NT_Copy, bool NT_Move, bool NT_CopyAssign, bool NT_MoveAssign,
                bool NT_Swap, bool EnableSwap>
        int NothrowTypeImp<NT_Copy, NT_Move, NT_CopyAssign, NT_MoveAssign,
                        NT_Swap, EnableSwap>::swap_called = 0;

        template <bool NT_Copy, bool NT_Move, bool NT_CopyAssign, bool NT_MoveAssign,
                bool NT_Swap>
        void swap(NothrowTypeImp<NT_Copy, NT_Move, NT_CopyAssign, NT_MoveAssign, NT_Swap, true>& lhs,
                NothrowTypeImp<NT_Copy, NT_Move, NT_CopyAssign, NT_MoveAssign, NT_Swap, true>& rhs)
            noexcept(NT_Swap) {
            lhs.swap_called++;
            do_throw<!NT_Swap>();
            int tmp = lhs.value;
            lhs.value = rhs.value;
            rhs.value = tmp;
        }

        // throwing copy, nothrow move ctor/assign, no swap provided
        using NothrowMoveable = NothrowTypeImp<false, true, false, true, false, false>;
        // throwing copy and move assign, nothrow move ctor, no swap provided
        using NothrowMoveCtor = NothrowTypeImp<false, true, false, false, false, false>;
        // nothrow move ctor, throwing move assignment, swap provided
        using NothrowMoveCtorWithThrowingSwap = NothrowTypeImp<false, true, false, false, false, true>;
        // throwing move ctor, nothrow move assignment, no swap provided
        using ThrowingMoveCtor = NothrowTypeImp<false, false, false, true, false, false>;
        // throwing special members, nothrowing swap
        using ThrowingTypeWithNothrowSwap = NothrowTypeImp<false, false, false, false, true, true>;
        using NothrowTypeWithThrowingSwap = NothrowTypeImp<true, true, true, true, false, true>;
        // throwing move assign with nothrow move and nothrow swap
        using ThrowingMoveAssignNothrowMoveCtorWithSwap = NothrowTypeImp<false, true, false, false, true, true>;
        // throwing move assign with nothrow move but no swap.
        using ThrowingMoveAssignNothrowMoveCtor = NothrowTypeImp<false, true, false, false, false, false>;

        struct NonThrowingNonNoexceptType {
        static int move_called;
        static void reset() { move_called = 0; }
        NonThrowingNonNoexceptType() = default;
        NonThrowingNonNoexceptType(int v) : value(v) {}
        NonThrowingNonNoexceptType(NonThrowingNonNoexceptType&& o) noexcept(false)
            : value(o.value) {
            ++move_called;
            o.value = -1;
        }
        NonThrowingNonNoexceptType& operator=(NonThrowingNonNoexceptType&&) noexcept(false) {
            CHECK(false); // never called by the tests.
            return *this;
        }
        int value;
        };
        int NonThrowingNonNoexceptType::move_called = 0;

        struct ThrowsOnSecondMove {
        int value;
        int move_count;
        ThrowsOnSecondMove(int v) : value(v), move_count(0) {}
        ThrowsOnSecondMove(ThrowsOnSecondMove&& o) noexcept(false)
            : value(o.value), move_count(o.move_count + 1) {
            if (move_count == 2)
                do_throw<true>();
            o.value = -1;
        }
        ThrowsOnSecondMove& operator=(ThrowsOnSecondMove&&) {
            CHECK(false); // not called by test
            return *this;
        }
        };


        void test_swap_valueless_by_exception()
        {
        #ifndef TEST_HAS_NO_EXCEPTIONS
            using V = ranges::variant<int, MakeEmptyT>;
            { // both empty
                V v1; makeEmpty(v1);
                V v2; makeEmpty(v2);
                CHECK(MakeEmptyT::alive == 0);
                { // member swap
                    v1.swap(v2);
                    CHECK(v1.valueless_by_exception());
                    CHECK(v2.valueless_by_exception());
                    CHECK(MakeEmptyT::alive == 0);
                }
                { // non-member swap
                    swap(v1, v2);
                    CHECK(v1.valueless_by_exception());
                    CHECK(v2.valueless_by_exception());
                    CHECK(MakeEmptyT::alive == 0);
                }
            }
            { // only one empty
                V v1(42);
                V v2; makeEmpty(v2);
                { // member swap
                    v1.swap(v2);
                    CHECK(v1.valueless_by_exception());
                    CHECK(ranges::get<0>(v2) == 42);
                    // swap again
                    v2.swap(v1);
                    CHECK(v2.valueless_by_exception());
                    CHECK(ranges::get<0>(v1) == 42);
                }
                { // non-member swap
                    swap(v1, v2);
                    CHECK(v1.valueless_by_exception());
                    CHECK(ranges::get<0>(v2) == 42);
                    // swap again
                    swap(v1, v2);
                    CHECK(v2.valueless_by_exception());
                    CHECK(ranges::get<0>(v1) == 42);
                }
            }
        #endif // TEST_HAS_NO_EXCEPTIONS
        }

        void test_swap_same_alternative()
        {
            {
                using T = ThrowingTypeWithNothrowSwap;
                using V = ranges::variant<T, int>;
                T::reset();
                V v1(ranges::in_place<0>, 42);
                V v2(ranges::in_place<0>, 100);
                v1.swap(v2);
                CHECK(T::swap_called == 1);
                CHECK(ranges::get<0>(v1).value == 100);
                CHECK(ranges::get<0>(v2).value == 42);
                swap(v1, v2);
                CHECK(T::swap_called == 2);
                CHECK(ranges::get<0>(v1).value == 42);
                CHECK(ranges::get<0>(v2).value == 100);
            }
            {
                using T = NothrowMoveable;
                using V = ranges::variant<T, int>;
                T::reset();
                V v1(ranges::in_place<0>, 42);
                V v2(ranges::in_place<0>, 100);
                v1.swap(v2);
                CHECK(T::swap_called == 0);
                CHECK(T::move_called == 1);
                CHECK(T::move_assign_called == 2);
                CHECK(ranges::get<0>(v1).value == 100);
                CHECK(ranges::get<0>(v2).value == 42);
                T::reset();
                swap(v1, v2);
                CHECK(T::swap_called == 0);
                CHECK(T::move_called == 1);
                CHECK(T::move_assign_called == 2);
                CHECK(ranges::get<0>(v1).value == 42);
                CHECK(ranges::get<0>(v2).value == 100);
            }
        #ifndef TEST_HAS_NO_EXCEPTIONS
            {
                using T = NothrowTypeWithThrowingSwap;
                using V = ranges::variant<T, int>;
                T::reset();
                V v1(ranges::in_place<0>, 42);
                V v2(ranges::in_place<0>, 100);
                try {
                    v1.swap(v2);
                    CHECK(false);
                } catch (int) {
                }
                CHECK(T::swap_called == 1);
                CHECK(T::move_called == 0);
                CHECK(T::move_assign_called == 0);
                CHECK(ranges::get<0>(v1).value == 42);
                CHECK(ranges::get<0>(v2).value == 100);
            }
            {
                using T = ThrowingMoveCtor;
                using V = ranges::variant<T, int>;
                T::reset();
                V v1(ranges::in_place<0>, 42);
                V v2(ranges::in_place<0>, 100);
                try {
                    v1.swap(v2);
                    CHECK(false);
                } catch (int) {
                }
                CHECK(T::move_called == 1); // call threw
                CHECK(T::move_assign_called == 0);
                CHECK(ranges::get<0>(v1).value == 42); // throw happend before v1 was moved from
                CHECK(ranges::get<0>(v2).value == 100);
            }
            {
                using T = ThrowingMoveAssignNothrowMoveCtor;
                using V = ranges::variant<T, int>;
                T::reset();
                V v1(ranges::in_place<0>, 42);
                V v2(ranges::in_place<0>, 100);
                try {
                    v1.swap(v2);
                    CHECK(false);
                } catch (int) {
                }
                CHECK(T::move_called == 1);
                CHECK(T::move_assign_called == 1); // call threw and didn't complete
                CHECK(ranges::get<0>(v1).value == -1 || ranges::get<0>(v2).value == -1);
                CHECK(ranges::get<0>(v2).value == 100 || ranges::get<0>(v1).value == 42);
            }
        #endif // TEST_HAS_NO_EXCEPTIONS
        }


        void test_swap_different_alternatives()
        {
            using ranges::swap;
            {
                using T = NothrowMoveCtorWithThrowingSwap;
                using V = ranges::variant<T, int>;
                T::reset();
                V v1(ranges::in_place<0>, 42);
                V v2(ranges::in_place<1>, 100);
                v1.swap(v2);
                CHECK(T::swap_called == 0);
                CHECK(0 < T::move_called && T::move_called <= 2);
                CHECK(T::move_assign_called == 0);
                CHECK(ranges::get<1>(v1) == 100);
                CHECK(ranges::get<0>(v2).value == 42);
                T::reset();
                swap(v1, v2);
                CHECK(T::swap_called == 0);
                CHECK(0 < T::move_called && T::move_called <= 2);
                CHECK(T::move_assign_called == 0);
                CHECK(ranges::get<0>(v1).value == 42);
                CHECK(ranges::get<1>(v2) == 100);
            }
        #ifndef TEST_HAS_NO_EXCEPTIONS
            {
                using T1 = ThrowingTypeWithNothrowSwap;
                using T2 = NonThrowingNonNoexceptType;
                using V = ranges::variant<T1, T2>;
                T1::reset();
                T2::reset();
                V v1(ranges::in_place<0>, 42);
                V v2(ranges::in_place<1>, 100);
                try {
                    v1.swap(v2);
                    CHECK(false);
                } catch (int) {
                }
                CHECK(T1::swap_called == 0);
                CHECK(T1::move_called == 1); // throws
                CHECK(T1::move_assign_called  == 0);
                CHECK(0 <= T2::move_called && T2::move_called <= 1);
                CHECK(ranges::get<0>(v1).value == 42);
                CHECK(v2.valueless_by_exception() || ranges::get<1>(v2).value == 100);
            }
            {
                using T1 = NonThrowingNonNoexceptType;
                using T2 = ThrowingTypeWithNothrowSwap;
                using V = ranges::variant<T1, T2>;
                T1::reset();
                T2::reset();
                V v1(ranges::in_place<0>, 42);
                V v2(ranges::in_place<1>, 100);
                try {
                    v1.swap(v2);
                    CHECK(false);
                } catch (int) {
                }
                CHECK(0 <= T2::move_called && T2::move_called <= 1);
                CHECK(T2::swap_called == 0);
                CHECK(T2::move_called == 1); // throws
                CHECK(T2::move_assign_called  == 0);
                CHECK(v1.valueless_by_exception() || ranges::get<0>(v1).value == 42);
                CHECK(ranges::get<1>(v2).value == 100);
            }
        #endif // !TEST_HAS_NO_EXCEPTIONS
        }

        void test_swap_sfinae()
        {
            {
                // This variant type does not provide either a member or non-member swap
                // but is still swappable via the generic swap algorithm, since the
                // variant is move constructible and move assignable.
                using V = ranges::variant<int, NotSwappable>;
                STATIC_ASSERT(is_swappable_v<V>);
            }
            {
                using V = ranges::variant<int, NotCopyable>;
                STATIC_ASSERT(!is_swappable_v<V>);
            }
            {
                using V = ranges::variant<int, NotCopyableWithSwap>;
                STATIC_ASSERT(!is_swappable_v<V>);
            }
            {
                using V = ranges::variant<int, NotMoveAssignable>;
                STATIC_ASSERT(!is_swappable_v<V>);
            }
        }

        void test_swap_noexcept()
        {
            using ranges::swap;
            {
                using V = ranges::variant<int, NothrowMoveable>;
                STATIC_ASSERT(is_swappable_v<V>);
                STATIC_ASSERT(is_nothrow_swappable_v<V>);
                // instantiate swap
                V v1, v2;
                v1.swap(v2);
                swap(v1, v2);
            }
            {
                using V = ranges::variant<int, NothrowMoveCtor>;
                STATIC_ASSERT(is_swappable_v<V>);
                STATIC_ASSERT(!is_nothrow_swappable_v<V>);
                // instantiate swap
                V v1, v2;
                v1.swap(v2);
                swap(v1, v2);
            }
            {
                using V = ranges::variant<int, ThrowingTypeWithNothrowSwap>;
                STATIC_ASSERT(is_swappable_v<V>);
                STATIC_ASSERT(!is_nothrow_swappable_v<V>);
                // instantiate swap
                V v1, v2;
                v1.swap(v2);
                swap(v1, v2);
            }
            {
                using V = ranges::variant<int, ThrowingMoveAssignNothrowMoveCtor>;
                STATIC_ASSERT(is_swappable_v<V>);
                STATIC_ASSERT(!is_nothrow_swappable_v<V>);
                // instantiate swap
                V v1, v2;
                v1.swap(v2);
                swap(v1, v2);
            }
            {
                using V = ranges::variant<int, ThrowingMoveAssignNothrowMoveCtorWithSwap>;
                STATIC_ASSERT(is_swappable_v<V>);
                STATIC_ASSERT(is_nothrow_swappable_v<V>);
                // instantiate swap
                V v1, v2;
                v1.swap(v2);
                swap(v1, v2);
            }
            {
                using V = ranges::variant<int, NotMoveAssignableWithSwap>;
                STATIC_ASSERT(is_swappable_v<V>);
                STATIC_ASSERT(is_nothrow_swappable_v<V>);
                // instantiate swap
                V v1, v2;
                v1.swap(v2);
                swap(v1, v2);
            }
            {
                // This variant type does not provide either a member or non-member swap
                // but is still swappable via the generic swap algorithm, since the
                // variant is move constructible and move assignable.
                using V = ranges::variant<int, NotSwappable>;
                STATIC_ASSERT(is_swappable_v<V>);
                STATIC_ASSERT(is_nothrow_swappable_v<V>);
                V v1, v2;
                swap(v1, v2);
            }
        }

        void run_test()
        {
            test_swap_valueless_by_exception();
            test_swap_same_alternative();
            test_swap_different_alternatives();
            test_swap_sfinae();
            test_swap_noexcept();
        }
    } // namespace swap_

    namespace visit {
        #ifndef TEST_HAS_NO_EXCEPTIONS
        struct MakeEmptyT {
        MakeEmptyT() = default;
        MakeEmptyT(MakeEmptyT&&) {
            throw 42;
        }
        MakeEmptyT& operator=(MakeEmptyT&&) {
            throw 42;
        }
        };
        template <class Variant>
        void makeEmpty(Variant& v) {
            Variant v2(ranges::in_place<MakeEmptyT>);
            try {
                v = std::move(v2);
                CHECK(false);
            }  catch (...) {
                CHECK(v.valueless_by_exception());
            }
        }
        #endif // TEST_HAS_NO_EXCEPTIONS


        enum CallType : unsigned {
        CT_None,
        CT_NonConst = 1,
        CT_Const = 2,
        CT_LValue = 4,
        CT_RValue = 8
        };

        constexpr CallType operator|(CallType LHS, CallType RHS) {
            return static_cast<CallType>(static_cast<unsigned>(LHS) | static_cast<unsigned>(RHS));
        }

        struct ForwardingCallObject {

        template <class ...Args>
        bool operator()(Args&&...) & {
            set_call<Args&&...>(CT_NonConst | CT_LValue);
            return true;
        }

        template <class ...Args>
        bool operator()(Args&&...) const & {
            set_call<Args&&...>(CT_Const | CT_LValue);
            return true;
        }

        // Don't allow the call operator to be invoked as an rvalue.
        template <class ...Args>
        bool operator()(Args&&...) && {
            set_call<Args&&...>(CT_NonConst | CT_RValue);
            return true;
        }

        template <class ...Args>
        bool operator()(Args&&...) const && {
            set_call<Args&&...>(CT_Const | CT_RValue);
            return true;
        }

        template <class ...Args>
        static void set_call(CallType type) {
            CHECK(last_call_type == CT_None);
            CHECK(last_call_args == native_nullptr);
            last_call_type = type;
            last_call_args = std::addressof(makeArgumentID<Args...>());
        }

        template <class ...Args>
        static bool check_call(CallType type) {
            bool result =
                last_call_type == type
                && last_call_args
                && *last_call_args == makeArgumentID<Args...>();
            last_call_type = CT_None;
            last_call_args = native_nullptr;
            return result;
        }

        static CallType      last_call_type;
        static TypeID const* last_call_args;
        };

        CallType ForwardingCallObject::last_call_type = CT_None;
        TypeID const* ForwardingCallObject::last_call_args = native_nullptr;

        void test_call_operator_forwarding()
        {
            using Fn = ForwardingCallObject;
            Fn obj{};
            Fn const& cobj = obj;
            { // test call operator forwarding - single variant, single arg
                using V = ranges::variant<int>;
                V v(42);
                ranges::visit(obj, v);
                CHECK(Fn::check_call<int&>(CT_NonConst | CT_LValue));
                ranges::visit(cobj, v);
                CHECK(Fn::check_call<int&>(CT_Const | CT_LValue));
                ranges::visit(std::move(obj), v);
                CHECK(Fn::check_call<int&>(CT_NonConst | CT_RValue));
                ranges::visit(std::move(cobj), v);
                CHECK(Fn::check_call<int&>(CT_Const | CT_RValue));
            }
            { // test call operator forwarding - single variant, multi arg
                using V = ranges::variant<int, long, double>;
                V v(42l);
                ranges::visit(obj, v);
                CHECK(Fn::check_call<long&>(CT_NonConst | CT_LValue));
                ranges::visit(cobj, v);
                CHECK(Fn::check_call<long&>(CT_Const | CT_LValue));
                ranges::visit(std::move(obj), v);
                CHECK(Fn::check_call<long&>(CT_NonConst | CT_RValue));
                ranges::visit(std::move(cobj), v);
                CHECK(Fn::check_call<long&>(CT_Const | CT_RValue));
            }
            { // test call operator forwarding - multi variant, multi arg
                using V = ranges::variant<int, long, double>;
                using V2 = ranges::variant<int*, std::string>;
                V v(42l);
                V2 v2("hello");
                ranges::visit(obj, v, v2);
                CHECK((Fn::check_call<long&, std::string&>(CT_NonConst | CT_LValue)));
                ranges::visit(cobj, v, v2);
                CHECK((Fn::check_call<long&, std::string&>(CT_Const | CT_LValue)));
                ranges::visit(std::move(obj), v, v2);
                CHECK((Fn::check_call<long&, std::string&>(CT_NonConst | CT_RValue)));
                ranges::visit(std::move(cobj), v, v2);
                CHECK((Fn::check_call<long&, std::string&>(CT_Const | CT_RValue)));
            }
        }

        void test_argument_forwarding()
        {
            using Fn = ForwardingCallObject;
            Fn obj{};
            const auto Val = CT_LValue | CT_NonConst;
            { // single argument - value type
                using V = ranges::variant<int>;
                V v(42);
                V const& cv = v;
                ranges::visit(obj, v);
                CHECK(Fn::check_call<int &>(Val));
                ranges::visit(obj, cv);
                CHECK(Fn::check_call<int const&>(Val));
                ranges::visit(obj, std::move(v));
                CHECK(Fn::check_call<int &&>(Val));
                ranges::visit(obj, std::move(cv));
                CHECK(Fn::check_call<const int &&>(Val));
            }
            { // single argument - lvalue reference
                using V = ranges::variant<int&>;
                int x = 42;
                V v(x);
                V const& cv = v;
                ranges::visit(obj, v);
                CHECK(Fn::check_call<int &>(Val));
                ranges::visit(obj, cv);
                CHECK(Fn::check_call<int &>(Val));
                ranges::visit(obj, std::move(v));
                CHECK(Fn::check_call<int &>(Val));
                ranges::visit(obj, std::move(cv));
                CHECK(Fn::check_call<int &>(Val));
            }
            { // single argument - rvalue reference
                using V = ranges::variant<int&&>;
                int x = 42;
                V v(std::move(x));
                V const& cv = v;
                ranges::visit(obj, v);
                CHECK(Fn::check_call<int &>(Val));
                ranges::visit(obj, cv);
                CHECK(Fn::check_call<int &>(Val));
                ranges::visit(obj, std::move(v));
                CHECK(Fn::check_call<int &&>(Val));
                ranges::visit(obj, std::move(cv));
                CHECK(Fn::check_call<int &&>(Val));
            }
            { // multi argument - multi variant
                using S = std::string const&;
                using V = ranges::variant<int, S, long&&>;
                std::string const str = "hello";
                long l = 43;
                V v1(42);           V const& cv1 = v1;
                V v2(str);          V const& cv2 = v2;
                V v3(std::move(l)); // V const& cv3 = v3;
                ranges::visit(obj, v1, v2, v3);
                CHECK((Fn::check_call<int&, S, long&>(Val)));
                ranges::visit(obj, cv1, cv2, std::move(v3));
                CHECK((Fn::check_call<const int&, S, long&&>(Val)));
            }
        }

        struct ReturnFirst {
        template <class ...Args>
        constexpr int operator()(int f, Args&&...) const {
            return f;
        }
        };

        struct ReturnArity {
        template <class ...Args>
        constexpr int operator()(Args&&...) const {
            return sizeof...(Args);
        }
        };

        void test_constexpr() {
            constexpr ReturnFirst obj{};
            constexpr ReturnArity aobj{};
            {
                using V = ranges::variant<int>;
                constexpr V v(42);
                STATIC_ASSERT(ranges::visit(obj, v) == 42);
            }
            {
                using V = ranges::variant<short, long, char>;
                constexpr V v(42l);
                STATIC_ASSERT(ranges::visit(obj, v) == 42);
            }
            {
                using V1 = ranges::variant<int>;
                using V2 = ranges::variant<int, char*, long long>;
                using V3 = ranges::variant<bool, int, int>;
                constexpr V1 v1;
                constexpr V2 v2(native_nullptr);
                constexpr V3 v3;
                STATIC_ASSERT(ranges::visit(aobj, v1, v2, v3) == 3);
            }
            {
                using V1 = ranges::variant<int>;
                using V2 = ranges::variant<int, char*, long long>;
                using V3 = ranges::variant<void*, int, int>;
                constexpr V1 v1;
                constexpr V2 v2(native_nullptr);
                constexpr V3 v3;
                STATIC_ASSERT(ranges::visit(aobj, v1, v2, v3) == 3);
            }
        }

        void test_exceptions() {
        #ifndef TEST_HAS_NO_EXCEPTIONS
            ReturnArity obj{};
            auto test = [&](auto&&... args) {
            try {
                ranges::visit(obj, args...);
            } catch (ranges::bad_variant_access const&) {
                return true;
            } catch (...) {}
            return false;
            };
            {
                using V = ranges::variant<int, MakeEmptyT>;
                V v; makeEmpty(v);
                CHECK(test(v));
            }
            {
                using V = ranges::variant<int, MakeEmptyT>;
                using V2 = ranges::variant<long, std::string, void*>;
                V v; makeEmpty(v);
                V2 v2("hello");
                CHECK(test(v, v2));
            }
            {
                using V = ranges::variant<int, MakeEmptyT>;
                using V2 = ranges::variant<long, std::string, void*>;
                V v; makeEmpty(v);
                V2 v2("hello");
                CHECK(test(v2, v));
            }
            {
                using V = ranges::variant<int, MakeEmptyT>;
                using V2 = ranges::variant<long, std::string, void*, MakeEmptyT>;
                V v; makeEmpty(v);
                V2 v2; makeEmpty(v2);
                CHECK(test(v, v2));
            }
        #endif // TEST_HAS_NO_EXCEPTIONS
        }

        void run_test() {
            test_call_operator_forwarding();
            test_argument_forwarding();
            test_constexpr();
            test_exceptions();

            (void)ForwardingCallObject::last_call_type;
            (void)ForwardingCallObject::last_call_args;
        }
    }

    namespace size {
        template <class T>
        using element = std::conditional<
            std::is_reference<T>::value,
            std::reference_wrapper<std::remove_reference_t<T>>,
            T>;

        template <std::size_t N>
        using index_t =
            std::conditional_t<(N < static_cast<std::size_t>(std::numeric_limits<signed char>::max())), signed char,
            std::conditional_t<(N < static_cast<std::size_t>(std::numeric_limits<short>::max())), short, int>>;

        template <class...Ts>
        struct fake_variant {
            std::aligned_union_t<0, typename element<Ts>::type...> data_;
            index_t<sizeof...(Ts)> index_;
        };

        template <class...Ts>
        void check_size() {
            STATIC_ASSERT(sizeof(ranges::variant<Ts...>) == sizeof(fake_variant<Ts...>));
        }

        template <int> struct empty {};

        void run_test() {
            struct not_empty { int i; };
            struct many_bases
                : empty<0>, empty<1>, empty<2>, empty<3> {};

            check_size<bool>();
            check_size<char>();
            check_size<unsigned char>();
            check_size<int>();
            check_size<unsigned int>();
            check_size<long>();
            check_size<long long>();
            check_size<float>();
            check_size<double>();
            check_size<void*>();
            check_size<empty<0>>();
            check_size<not_empty>();
            check_size<many_bases>();

            check_size<bool, char, short, int, long, long long, float, double, long double,
                void*, int&, double&&, empty<0>, empty<1>, not_empty, many_bases>();
        }
    } // namespace size
} // unnamed namespace

int main() {
    bad_variant_access::run_test();

    get::if_index::run_test();
    get::if_type::run_test();
    get::index::run_test();
    get::type::run_test();
    get::holds_alternative::run_test();

    hash::run_test();

    helpers::variant_alternative::run_test();
    helpers::variant_size::run_test();

    monostate::run_test();
    monostate::relops::run_test();

    relops::run_test();

    synopsis::run_test();
#if 0
    traits::run_test();
#endif

    assign::copy::run_test();
    assign::move::run_test();
    assign::T::run_test();

#if 0
    ctor::Alloc_copy::run_test();
    ctor::Alloc_in_place_index_Args::run_test();
    ctor::Alloc_in_place_index_init_list_Args::run_test();
    ctor::Alloc_in_place_type_Args::run_test();
    ctor::Alloc_in_place_type_init_list_Args::run_test();
    ctor::Alloc_move::run_test();
    ctor::Alloc_T::run_test();
#endif

    ctor::copy::run_test();
    ctor::default_::run_test();
    ctor::in_place_index_Args::run_test();
    ctor::in_place_type_Args::run_test();
    ctor::in_place_index_init_list_Args::run_test();
    ctor::in_place_type_init_list_Args::run_test();
    ctor::move::run_test();
    ctor::T::run_test();

    dtor::run_test();

    emplace::index::run_test();
    emplace::index_init_list::run_test();
    emplace::type::run_test();
    emplace::type_init_list::run_test();

    status::index::run_test();
    status::valueless_by_exception::run_test();

    swap_::run_test();

    visit::run_test();

    size::run_test();

    return test_status;
}
