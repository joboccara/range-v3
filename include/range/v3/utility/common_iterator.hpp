/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef RANGES_V3_UTILITY_COMMON_ITERATOR_HPP
#define RANGES_V3_UTILITY_COMMON_ITERATOR_HPP

#include <type_traits>
#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/utility/basic_iterator.hpp>
#include <range/v3/utility/concepts.hpp>
#include <range/v3/utility/variant.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \cond
        namespace detail
        {
            template<typename I, bool IsReadable = (bool) Readable<I>()>
            struct common_cursor_types
            {};

            template<typename I>
            struct common_cursor_types<I, true>
            {
                using single_pass = SinglePass<I>;
                using value_type = iterator_value_t<I>;
            };

            template<typename I, typename S>
            struct common_cursor
              : private common_cursor_types<I>
            {
            private:
                friend range_access;
                static_assert(!std::is_same<I, S>::value,
                    "Error: iterator and sentinel types are the same");
                using difference_type = iterator_difference_t<I>;
                struct mixin
                  : basic_mixin<common_cursor>
                {
                    mixin() = default;
                    using basic_mixin<common_cursor>::basic_mixin;
                    constexpr explicit mixin(I it)
                      : mixin(common_cursor{std::move(it)})
                    {}
                    constexpr explicit mixin(S se)
                      : mixin(common_cursor{std::move(se)})
                    {}
                };

                variant<I, S> data_;

                constexpr explicit common_cursor(I it)
                    noexcept(std::is_nothrow_move_constructible<I>::value)
                  : data_(in_place<0>, std::move(it))
                {}
                constexpr explicit common_cursor(S se)
                    noexcept(std::is_nothrow_move_constructible<S>::value)
                  : data_(in_place<1>, std::move(se))
                {}
                constexpr bool is_sentinel() const noexcept
                {
                    return RANGES_EXPECT(data_.index() == 0u || data_.index() == 1u),
                        data_.index() == 1u;
                }
                RANGES_CXX14_CONSTEXPR I & it() noexcept
                {
                    return RANGES_EXPECT(data_.index() == 0u),
                        ranges::get_unchecked<0>(data_);
                }
                constexpr I const & it() const noexcept
                {
                    return RANGES_EXPECT(data_.index() == 0u),
                        ranges::get_unchecked<0>(data_);
                }
                constexpr S const & se() const noexcept
                {
                    return RANGES_EXPECT(data_.index() == 1u),
                        ranges::get_unchecked<1>(data_);
                }
                struct distance_visitor
                {
                    constexpr iterator_difference_t<I>
                    operator()(S const&, S const&) const noexcept
                    {
                        return 0;
                    }
                    template<class Left, class Right>
                    constexpr auto operator()(Left const& left, Right const& right) const
                    RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                    (
                        static_cast<iterator_difference_t<I>>(right - left)
                    )
                };
                CONCEPT_REQUIRES((bool)SizedSentinel<S, I>() && (bool)SizedSentinel<I, I>())
                constexpr iterator_difference_t<I>
                distance_to(common_cursor const &that) const
                {
                    return ranges::visit(distance_visitor{}, data_, that.data_);
                }
                CONCEPT_REQUIRES(Readable<I>())
                constexpr iterator_rvalue_reference_t<I> move() const
                    noexcept(noexcept(iter_move(std::declval<I const &>())))
                {
                    return iter_move(it());
                }
                CONCEPT_REQUIRES(Readable<I>())
                constexpr iterator_reference_t<I> get() const
                    noexcept(noexcept(*std::declval<I const &>()))
                {
                    return *it();
                }
                template<typename T,
                    CONCEPT_REQUIRES_(ExclusivelyWritable_<I, T &&>())>
                RANGES_CXX14_CONSTEXPR void set(T && t) const
                    noexcept(noexcept(*std::declval<I const&>() = std::declval<T>()))
                {
                    *it() = (T &&) t;
                }
                template<typename I2, typename S2>
                struct equality_visitor
                {
                    constexpr bool operator()(S const &, S2 const &) const noexcept
                    {
                        return true;
                    }
                    CONCEPT_REQUIRES(!EqualityComparable<I, I2>())
                    constexpr bool operator()(I const &, I2 const &) const noexcept
                    {
                        return true;
                    }
                    template<typename Left, typename Right>
                    constexpr auto operator()(Left const &l, Right const &r) const
                    RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                    (
                        static_cast<bool>(l == r)
                    )
                };
                template<typename I2, typename S2,
                    CONCEPT_REQUIRES_(Sentinel<S2, I>() && Sentinel<S, I2>())>
                constexpr bool equal(common_cursor<I2, S2> const &that) const
                {
                    return ranges::visit(equality_visitor<I2, S2>{}, data_, that.data_);
                }
                RANGES_CXX14_CONSTEXPR void next()
                    noexcept(noexcept(++std::declval<I&>()))
                {
                    ++it();
                }
                template<typename I2, typename S2>
                struct convert_visitor
                {
                    CONCEPT_ASSERT(ExplicitlyConvertibleTo<I const&, I2>() &&
                                   ExplicitlyConvertibleTo<S const&, S2>());

                    constexpr auto operator()(S const& s) const
                    RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                    (
                        common_cursor<I2, S2>{S2{s}}
                    )
                    constexpr auto operator()(I const& i) const
                    RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                    (
                        common_cursor<I2, S2>{I2{i}}
                    )
                };
                // BUGBUG TODO what about advance??
            public:
                common_cursor() = default;
                template<typename I2, typename S2,
                    CONCEPT_REQUIRES_(ExplicitlyConvertibleTo<I const&, I2>() &&
                                      ExplicitlyConvertibleTo<S const&, S2>())>
                operator common_cursor<I2, S2>() const
                {
                    return ranges::visit(convert_visitor<I2, S2>{}, data_);
                }
            };
        }
        /// \endcond
    }
}

#endif
