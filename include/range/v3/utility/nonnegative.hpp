/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-2014
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_UTILITY_NONNEGATIVE_HPP
#define RANGES_V3_UTILITY_NONNEGATIVE_HPP

#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/detail/config.hpp>
#include <range/v3/utility/concepts.hpp>
#include <range/v3/utility/swap.hpp>
#include <functional>
#include <iosfwd>

namespace ranges
{
    inline namespace v3
    {
        template<typename> class nonnegative;

        /// \cond
        namespace nonnegative_detail
        {
            struct tag {};

            template<typename T, CONCEPT_REQUIRES_(SignedIntegral<T>())>
            static constexpr T check(T t) noexcept
            {
                return RANGES_EXPECT(T{0} <= t), t;
            }
            template<typename T, CONCEPT_REQUIRES_(!SignedIntegral<T>())>
            static constexpr T check(T t) noexcept
            {
                return t;
            }
            template<typename T, CONCEPT_REQUIRES_(SignedIntegral<T>())>
            static constexpr T assume(T t) noexcept
            {
                return RANGES_ASSUME(T{0} <= t), t;
            }
            template<typename T, CONCEPT_REQUIRES_(!SignedIntegral<T>())>
            static constexpr T assume(T t) noexcept
            {
                return t;
            }

            template<typename T>
            RANGES_CXX14_CONSTEXPR void swap(nonnegative<T> &x, T &y) noexcept
            {
                x.swap(y);
            }
            template<typename T>
            RANGES_CXX14_CONSTEXPR void swap(T &x, nonnegative<T> &y) noexcept
            {
                y.swap(x);
            }
            template<typename T>
            RANGES_CXX14_CONSTEXPR void swap(nonnegative<T> &x, nonnegative<T> &y) noexcept
            {
                x.swap(y);
            }

            struct access
            {
                template<typename T>
                static constexpr nonnegative<T> create_unchecked(T value) noexcept
                {
                    return nonnegative<T>{tag{}, value};
                }
            };
        } // namespace nonnegative_detail
        /// \endcond

        template<typename T>
        class nonnegative
          : private nonnegative_detail::tag
        {
        public:
            CONCEPT_ASSERT(Integral<T>());

            nonnegative() = default;

            constexpr nonnegative(T value) noexcept
            : value_(nonnegative_detail::check(value))
            {}

            RANGES_CXX14_CONSTEXPR nonnegative &operator=(T value) & noexcept
            {
                value_ = nonnegative_detail::check(value);
                return *this;
            }
            constexpr operator T() const noexcept
            {
                return nonnegative_detail::assume(value_);
            }
            constexpr nonnegative operator+() const noexcept
            {
                return *this;
            }

            RANGES_CXX14_CONSTEXPR void swap(T &t) noexcept
            {
                nonnegative_detail::check(t);
                ranges::swap(value_, t);
            }
            RANGES_CXX14_CONSTEXPR void swap(nonnegative &that) noexcept
            {
                ranges::swap(value_, that.value_);
            }

        #define RANGES_NONNEGATIVE_OP(OP)                            \
            template<typename U>                                     \
            constexpr friend nonnegative<                            \
                decltype(std::declval<T>() OP std::declval<U>())>    \
            operator OP (nonnegative t, nonnegative<U> u) noexcept   \
            {                                                        \
                return nonnegative_detail::access::create_unchecked( \
                    static_cast<T>(t) OP static_cast<U>(u));         \
            }

            RANGES_NONNEGATIVE_OP(+)
            RANGES_NONNEGATIVE_OP(*)
            RANGES_NONNEGATIVE_OP(/)
            RANGES_NONNEGATIVE_OP(%)
            RANGES_NONNEGATIVE_OP(&)
            RANGES_NONNEGATIVE_OP(|)
            RANGES_NONNEGATIVE_OP(^)

        #undef RANGES_NONNEGATIVE_OP

            constexpr friend nonnegative operator & (nonnegative x, T y) noexcept
            {
                return {nonnegative_detail::tag{}, static_cast<T>(x) & y};
            }
            constexpr friend nonnegative operator & (T x, nonnegative y) noexcept
            {
                return {nonnegative_detail::tag{}, x & static_cast<T>(y)};
            }

        private:
            friend nonnegative_detail::access;

            constexpr nonnegative(nonnegative_detail::tag, T value) noexcept
            : value_(nonnegative_detail::check(value))
            {}

            T value_;
        };

        template<typename T>
        inline auto operator<<(std::ostream &os, nonnegative<T> x)
        RANGES_DECLTYPE_AUTO_RETURN
        (
            os << static_cast<T>(x)
        )
    } // namespace v3
} // namespace ranges

namespace std
{
    template<typename T>
        // requires requires(T const &t) { { hash<T>{}(t) } -> size_t; }
    struct hash<::ranges::v3::nonnegative<T>> {
        size_t operator()(::ranges::v3::nonnegative<T> const &t) const {
            return hash<T>{}(static_cast<T const&>(t));
        }
    };
} // namespace std

#endif // RANGES_V3_UTILITY_NONNEGATIVE_HPP
