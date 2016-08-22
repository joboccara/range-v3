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

#ifndef RANGES_V3_UTILITY_SEMIREGULAR_HPP
#define RANGES_V3_UTILITY_SEMIREGULAR_HPP

#include <utility>
#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/utility/get.hpp>
#include <range/v3/detail/optional.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \addtogroup group-utility
        /// @{
        template<typename T>
        struct semiregular
        {
        private:
            optional<T> t_;
        public:
            CONCEPT_REQUIRES(DefaultConstructible<T>())
            semiregular()
              : t_(in_place)
            {}
            CONCEPT_REQUIRES(!DefaultConstructible<T>())
            semiregular() noexcept {}
            semiregular(T f)
              : t_(std::move(f))
            {}
            template<typename ...Args>
            semiregular(in_place_t, Args &&...args)
              : t_(in_place, std::forward<Args>(args)...)
            {}
            T & get() &
            {
                RANGES_ASSERT(!!t_);
                return *t_;
            }
            T const & get() const &
            {
                RANGES_ASSERT(!!t_);
                return *t_;
            }
            T && get() &&
            {
                RANGES_ASSERT(!!t_);
                return std::move(*t_);
            }
            T const && get() const &&
            {
                RANGES_ASSERT(!!t_);
                return std::move(*t_);
            }
            CONCEPT_REQUIRES(Assignable<T&, T const&>())
            semiregular &operator=(T const &t) &
            {
                if (t_)
                    *t_ = t;
                else
                    t_ = t;
                return *this;
            }
            CONCEPT_REQUIRES(!Assignable<T&, T const&>())
            semiregular &operator=(T const &t) &
            {
                t_ = t;
                return *this;
            }
            CONCEPT_REQUIRES(Assignable<T&, T &&>())
            semiregular &operator=(T &&t) &
            {
                if (t_)
                    *t_ = std::move(t);
                else
                    t_ = std::move(t);
                return *this;
            }
            CONCEPT_REQUIRES(!Assignable<T&, T &&>())
            semiregular &operator=(T &&t) &
            {
                t_ = std::move(t);
                return *this;
            }
            operator T &() &
            {
                return get();
            }
            operator T const &() const &
            {
                return get();
            }
            operator T &&() &&
            {
                return std::move(get());
            }
            operator T const &&() const &&
            {
                return std::move(get());
            }
            template<typename...Args>
            auto operator()(Args &&...args) &
                RANGES_DECLTYPE_NOEXCEPT(std::declval<T &>()(std::forward<Args>(args)...))
            {
                return get()(std::forward<Args>(args)...);
            }
            template<typename...Args>
            auto operator()(Args &&...args) const &
                RANGES_DECLTYPE_NOEXCEPT(std::declval<T const &>()(std::forward<Args>(args)...))
            {
                return get()(std::forward<Args>(args)...);
            }
            template<typename...Args>
            auto operator()(Args &&...args) &&
                RANGES_DECLTYPE_NOEXCEPT(std::declval<T &&>()(std::forward<Args>(args)...))
            {
                return std::move(get())(std::forward<Args>(args)...);
            }
            template<typename...Args>
            auto operator()(Args &&...args) const &&
                RANGES_DECLTYPE_NOEXCEPT(std::declval<T const &&>()(std::forward<Args>(args)...))
            {
                return std::move(get())(std::forward<Args>(args)...);
            }
        };

        template<typename T>
        using semiregular_t =
            meta::if_<
                SemiRegular<T>,
                T,
                semiregular<T>>;

        template<typename T, bool IsConst = false>
        using semiregular_ref_or_val_t =
            meta::if_<
                SemiRegular<T>,
                meta::if_c<IsConst, T, reference_wrapper<T> >,
                reference_wrapper<meta::if_c<IsConst, semiregular<T> const, semiregular<T>>>>;

        template<typename T>
        T & get(meta::id_t<semiregular<T>> &t)
        {
            return t.get();
        }

        template<typename T>
        T const & get(meta::id_t<semiregular<T>> const &t)
        {
            return t.get();
        }

        template<typename T>
        T && get(meta::id_t<semiregular<T>> &&t)
        {
            return std::move(t.get());
        }

        template<typename T>
        T const && get(meta::id_t<semiregular<T>> const &&t)
        {
            return std::move(t.get());
        }
        /// @}
    }
}

#endif
