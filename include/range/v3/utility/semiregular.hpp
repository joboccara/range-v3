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
#include <range/v3/utility/optional.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \addtogroup group-utility
        /// @{
        template<typename Derived, typename T>
        class semiregular_base
        {
        protected:
            optional<T> t_;

            template<typename U>
            void assign(U && u, std::false_type)
                noexcept(std::is_nothrow_constructible<T, U>::value)
            {
                t_.emplace(detail::forward<U>(u));
            }
            template<typename U>
            void assign(U && u, std::true_type)
                noexcept(std::is_nothrow_constructible<T, U>::value &&
                    std::is_nothrow_assignable<T&, U>::value)
            {
                t_ = detail::forward<U>(u);
            }

        public:
            CONCEPT_ASSERT(MoveConstructible<T>());

            semiregular_base() = default;
            constexpr semiregular_base(T && t)
                noexcept(std::is_nothrow_move_constructible<T>::value)
              : t_{in_place, detail::move(t)}
            {}
            CONCEPT_REQUIRES(CopyConstructible<T>())
            constexpr semiregular_base(T const &t)
                noexcept(std::is_nothrow_move_constructible<T>::value)
              : t_{in_place, t}
            {}
            template<typename ...Args,
                CONCEPT_REQUIRES_(Constructible<T, Args &&...>())>
            constexpr semiregular_base(in_place_t, Args &&...args)
                noexcept(std::is_nothrow_constructible<T, Args...>::value)
              : t_{in_place, detail::forward<Args>(args)...}
            {}

            Derived& operator=(T && t) &
                noexcept(noexcept(
                    std::declval<semiregular_base&>().assign(detail::move(t), Movable<T>())))
            {
                assign(detail::move(t), Movable<T>());
                return static_cast<Derived&>(*this);
            }
            CONCEPT_REQUIRES(CopyConstructible<T>())
            Derived& operator=(T const &t) &
                noexcept(noexcept(
                    std::declval<semiregular_base&>().assign(t, Copyable<T>())))
            {
                assign(t, Copyable<T>());
                return static_cast<Derived&>(*this);
            }

            RANGES_CXX14_CONSTEXPR T & get() & noexcept
            {
                return *t_;
            }
            constexpr T const & get() const & noexcept
            {
                return *t_;
            }
            RANGES_CXX14_CONSTEXPR T && get() && noexcept
            {
                return detail::move(*t_);
            }
            constexpr T const && get() const && noexcept
            {
                return detail::move(*t_);
            }
            RANGES_CXX14_CONSTEXPR operator T &() & noexcept
            {
                return get();
            }
            constexpr operator T const &() const & noexcept
            {
                return get();
            }
            RANGES_CXX14_CONSTEXPR operator T &&() && noexcept
            {
                return detail::move(get());
            }
            constexpr operator T const && () const && noexcept
            {
                return detail::move(get());
            }

            template<typename...Args>
            RANGES_CXX14_CONSTEXPR auto operator()(Args &&...args) &
            RANGES_DECLTYPE_NOEXCEPT(
                std::declval<T &>()(detail::forward<Args>(args)...))
            {
                return get()(detail::forward<Args>(args)...);
            }
            template<typename...Args>
            constexpr auto operator()(Args &&...args) const &
            RANGES_DECLTYPE_NOEXCEPT(
                std::declval<T const&>()(detail::forward<Args>(args)...))
            {
                return get()(detail::forward<Args>(args)...);
            }
            template<typename...Args>
            RANGES_CXX14_CONSTEXPR auto operator()(Args &&...args) &&
            RANGES_DECLTYPE_NOEXCEPT(
                std::declval<T &&>()(detail::forward<Args>(args)...))
            {
                return detail::move(get())(detail::forward<Args>(args)...);
            }
            template<typename...Args>
            constexpr auto operator()(Args &&...args) const &&
            RANGES_DECLTYPE_NOEXCEPT(
                std::declval<T const &&>()(detail::forward<Args>(args)...))
            {
                return detail::move(get())(detail::forward<Args>(args)...);
            }
        };

        template<class Derived, class T, bool = Movable<T>()>
        class semiregular_movable_base;

        template<class Derived, class T>
        class semiregular_movable_base<Derived, T, true>
          : public semiregular_base<Derived, T>
        {
            using base_t = semiregular_base<Derived, T>;
        public:
            using base_t::base_t;
            using base_t::operator=;

            // T is already Movable; nothing to do.
        };

        template<typename Derived, typename T>
        class semiregular_movable_base<Derived, T, false>
          : public semiregular_base<Derived, T>
        {
            using base_t = semiregular_base<Derived, T>;
        public:
            using base_t::base_t;
            using base_t::operator=;

            semiregular_movable_base() = default;
            semiregular_movable_base(semiregular_movable_base const&) = default;
            semiregular_movable_base(semiregular_movable_base &&) = default;

            // T is MoveConstructible but not Movable; implement move assignment
            // as destroy-and-construct.
            semiregular_movable_base& operator=(semiregular_movable_base && that) &
                noexcept(std::is_nothrow_move_constructible<T>::value)
            {
                this->t_.reset();
                if (that.t_)
                {
                    this->t_.emplace(detail::move(*that.t_));
                }
                return *this;
            }
        };

        template<typename T, bool = !Copyable<T>() && CopyConstructible<T>()>
        class semiregular;

        template<typename T>
        class semiregular<T, false>
          : public semiregular_movable_base<semiregular<T>, T>
        {
            using base_t = semiregular_movable_base<semiregular<T>, T>;
        public:
            using base_t::base_t;
            using base_t::operator=;
        };

        template<typename T>
        class semiregular<T, true>
          : public semiregular_movable_base<semiregular<T>, T>
        {
            using base_t = semiregular_movable_base<semiregular<T>, T>;
        public:
            using base_t::base_t;
            using base_t::operator=;

            semiregular() = default;
            semiregular(semiregular const&) = default;
            semiregular(semiregular &&) = default;
            semiregular& operator=(semiregular &&) & = default;

            // T is CopyConstructible but not Copyable; implement copy assignment
            // as destroy-and-construct.
            semiregular& operator=(semiregular const &that) &
                noexcept(std::is_nothrow_copy_constructible<T>::value)
            {
                this->t_.reset();
                if (that.t_)
                {
                    this->t_.emplace(*that.t_);
                }
                return *this;
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
        RANGES_CXX14_CONSTEXPR T & get(meta::id_t<semiregular<T>> &t)
        {
            return t.get();
        }

        template<typename T>
        RANGES_CXX14_CONSTEXPR T const & get(meta::id_t<semiregular<T>> const &t)
        {
            return t.get();
        }

        template<typename T>
        RANGES_CXX14_CONSTEXPR T && get(meta::id_t<semiregular<T>> &&t)
        {
            return detail::move(t.get());
        }
        /// @}
    }
}

#endif
