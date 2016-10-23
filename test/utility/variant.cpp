// Range v3 library
//
//  Copyright Eric Niebler 2015
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#include <vector>
#include <sstream>
#include <iostream>
#include <range/v3/utility/variant.hpp>
#include "../simple_test.hpp"
#include "../test_utils.hpp"

#define STATIC_ASSERT(...) static_assert((__VA_ARGS__), #__VA_ARGS__)

int main()
{
    using namespace ranges;

    // Simple variant and access.
    {
        variant<int, short> v;
        CHECK(v.index() == 0u);
        auto v2 = v;
        CHECK(v2.index() == 0u);
        v.emplace<1>((short)2);
        CHECK(v.index() == 1u);
        CHECK(get<1>(v) == (short)2);
        try
        {
            get<0>(v);
            CHECK(false);
        }
        catch(bad_variant_access)
        {}
        catch(...)
        {
            CHECK(!(bool)"unknown exception");
        }
        v = v2;
        CHECK(v.index() == 0u);
    }

#if 0
    // variant of void
    {
        variant<void, void> v;
        CHECK(v.index() == 0u);
        v.emplace<0>();
        CHECK(v.index() == 0u);
        try
        {
            // Will only compile if get returns void
            v.index() == 0 ? void() : get<0>(v);
        }
        catch(...)
        {
            CHECK(false);
        }
        v.emplace<1>();
        CHECK(v.index() == 1u);
        try
        {
            get<0>(v);
            CHECK(false);
        }
        catch(bad_variant_access)
        {}
        catch(...)
        {
            CHECK(!(bool)"unknown exception");
        }
    }
#endif

    // variant of references
    {
        int i = 42;
        std::string s = "hello world";
        variant<int&, std::string&> v{in_place<0>, i};
        CONCEPT_ASSERT(!DefaultConstructible<variant<int&, std::string&>>());
        CHECK(v.index() == 0u);
        CHECK(get<0>(v) == 42);
        CHECK(&get<0>(v) == &i);
        auto const & cv = v;
        get<0>(cv) = 24;
        CHECK(i == 24);
        v.emplace<1>(s);
        CHECK(v.index() == 1u);
        CHECK(get<1>(v) == "hello world");
        CHECK(&get<1>(v) == &s);
        get<1>(cv) = "goodbye";
        CHECK(s == "goodbye");
    }

    // Move test 1
    {
        variant<int, MoveOnlyString> v{in_place<1>, "hello world"};
        CHECK(get<1>(v) == "hello world");
        MoveOnlyString s = get<1>(std::move(v));
        CHECK(s == "hello world");
        CHECK(get<1>(v) == "");
        v.emplace<1>("goodbye");
        CHECK(get<1>(v) == "goodbye");
        auto v2 = std::move(v);
        CHECK(get<1>(v2) == "goodbye");
        CHECK(get<1>(v) == "");
        v = std::move(v2);
        CHECK(get<1>(v) == "goodbye");
        CHECK(get<1>(v2) == "");
    }

    // Move test 2
    {
        MoveOnlyString s = "hello world";
        variant<MoveOnlyString&> v{in_place<0>, s};
        CHECK(get<0>(v) == "hello world");
        MoveOnlyString &s2 = get<0>(std::move(v));
        CHECK(&s2 == &s);
    }

    // Apply test 1
    {
        using V = ranges::variant<int, std::string>;
        enum { is_int, is_string };
        auto fun = overload(
            [&](int && i) { CHECK(i == 42); return is_int; },
            [&](std::string && s) { CHECK(s == "hello"); return is_string; });
        CHECK(ranges::visit(fun, V{in_place<1>, "hello"}) == is_string);
        CHECK(ranges::visit(fun, V{in_place<0>, 42}) == is_int);
    }

    // Apply test 2
    {
        using V = ranges::variant<int, std::string>;
        enum { is_int, is_string };
        auto fun = overload(
            [&](int const &i) { CHECK(i == 42); return is_int; },
            [&](std::string const &s) { CHECK(s == "hello"); return is_string; });
        CHECK(ranges::visit(fun, V{in_place<1>, "hello"}) == is_string);
        CHECK(ranges::visit(fun, V{in_place<0>, 42}) == is_int);
        { const V v{in_place<1>, "hello"}; CHECK(ranges::visit(fun, v) == is_string); }
        { const V v{in_place<0>, 42}; CHECK(ranges::visit(fun, v) == is_int); }
    }

    // constexpr variant
    {
        constexpr variant<int, short> v{in_place<1>, (short)2};
        STATIC_ASSERT(!v.valueless_by_exception());
        STATIC_ASSERT(v.index() == 1);
    }

#if 0
    // Variant and arrays
    {
        variant<int[5], std::vector<int>> v{in_place<0>, {1,2,3,4,5}};
        int (&rgi)[5] = get<0>(v);
        check_equal(rgi, {1,2,3,4,5});

        variant<int[5], std::vector<int>> v2{in_place<0>, {}};
        int (&rgi2)[5] = get<0>(v2);
        check_equal(rgi2, {0,0,0,0,0});

        v2 = v;
        check_equal(rgi2, {1,2,3,4,5});

        struct T
        {
            T() = delete;
            T(int) {}
            T(T const &) = default;
            T &operator=(T const &) = default;
        };

        // Should compile and not assert at runtime.
        variant<T[5]> vrgt{in_place<0>, {T{42},T{42},T{42},T{42},T{42}}};
        (void) vrgt;
    }
#endif

    return ::test_result();
}
