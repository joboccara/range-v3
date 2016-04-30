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

#ifndef RANGES_V3_UTILITY_BOX_HPP
#define RANGES_V3_UTILITY_BOX_HPP

#include <atomic>
#include <utility>
#include <cstdlib>
#include <type_traits>
#include <range/v3/range_fwd.hpp>
#include <range/v3/utility/get.hpp>
#include <range/v3/utility/concepts.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \cond
        namespace detail
        {
#if RANGES_CXX_LIB_IS_FINAL
            template <class T>
            using is_final = std::is_final<T>;
#else
            template <class>
            using is_final = std::false_type;
#endif
        }
        /// \endcond
      
        /// \addtogroup group-utility Utility
        /// @{
        ///
        template<typename T>
        struct mutable_
        {
            mutable T value;
            CONCEPT_REQUIRES(DefaultConstructible<T>())
            constexpr mutable_()
              : value{}
            {}
            constexpr explicit mutable_(T const &t)
              : value(t)
            {}
            constexpr explicit mutable_(T &&t)
              : value(detail::move(t))
            {}
            mutable_ const &operator=(T const &t) const
            {
                value = t;
                return *this;
            }
            mutable_ const &operator=(T &&t) const
            {
                value = detail::move(t);
                return *this;
            }
            constexpr operator T &() const &
            {
                return value;
            }
        };

        template<typename T>
        struct mutable_<std::atomic<T>>
        {
            mutable std::atomic<T> value;
            mutable_() = default;
            mutable_(mutable_ const &that)
              : value(static_cast<T>(that.value))
            {}
            constexpr explicit mutable_(T &&t)
              : value(detail::move(t))
            {}
            constexpr explicit mutable_(T const &t)
              : value(t)
            {}
            mutable_ const &operator=(mutable_ const &that) const
            {
                value = static_cast<T>(that.value);
                return *this;
            }
            mutable_ const &operator=(T &&t) const
            {
                value = std::move(t);
                return *this;
            }
            mutable_ const &operator=(T const &t) const
            {
                value = t;
                return *this;
            }
            operator T() const
            {
                return value;
            }
            T exchange(T desired)
            {
                return value.exchange(desired);
            }
            operator std::atomic<T> &() const &
            {
                return value;
            }
        };

        template<typename T, T v>
        struct constant
        {
            constant() = default;
            constexpr explicit constant(T const &)
            {}
            constant &operator=(T const &)
            {
                return *this;
            }
            constant const &operator=(T const &) const
            {
                return *this;
            }
            constexpr operator T() const
            {
                return v;
            }
            constexpr T exchange(T const &) const
            {
                return v;
            }
        };

        static_assert(std::is_trivial<constant<int, 0>>::value, "Expected constant to be trivial");

        template<typename Element, typename Tag = Element,
            bool Empty = std::is_empty<Element>::value && !detail::is_final<Element>::value>
        struct box
        {
            Element value;

            template<typename... Args,
                CONCEPT_REQUIRES_(Constructible<Element, Args...>())>
            constexpr explicit box(Args &&... args)
              : value(detail::forward<Args>(args)...)
            {}
            Element &get() & noexcept
            {
                return value;
            }
            Element const &get() const& noexcept
            {
                return value;
            }
            Element &&get() && noexcept
            {
                return value;
            }
        };

        template<typename Element, typename Tag>
        struct box<Element, Tag, true>
          : Element
        {
            template<typename... Args,
                CONCEPT_REQUIRES_(Constructible<Element, Args...>())>
            constexpr explicit box(Args &&... args)
              : Element(detail::forward<Args>(args)...)
            {}
            Element &get() & noexcept
            {
                return static_cast<Element &>(*this);
            }
            Element const &get() const& noexcept
            {
                return static_cast<Element const&>(*this);
            }
            Element &&get() && noexcept
            {
                return static_cast<Element &&>(*this);
            }
        };

        // Get by tag type
        template<typename Tag, typename Element, bool B>
        constexpr Element & get(box<Element, Tag, B> & b) noexcept
        {
            return b.get();
        }

        template<typename Tag, typename Element, bool B>
        constexpr Element const & get(box<Element, Tag, B> const & b) noexcept
        {
            return b.get();
        }

        template<typename Tag, typename Element, bool B>
        constexpr Element && get(box<Element, Tag, B> && b) noexcept
        {
            return detail::move(b).get();
        }

        // Get by index
        template<std::size_t I, typename Element, bool B>
        constexpr Element & get(box<Element, meta::size_t<I>, B> & b) noexcept
        {
            return b.get();
        }

        template<std::size_t I, typename Element, bool B>
        constexpr Element const & get(box<Element, meta::size_t<I>, B> const & b) noexcept
        {
            return b.get();
        }

        template<std::size_t I, typename Element, bool B>
        constexpr Element && get(box<Element, meta::size_t<I>, B> && b) noexcept
        {
            return detail::move(b).get();
        }
        /// @}
    }
}

#endif
