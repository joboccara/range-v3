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

#ifndef RANGES_V3_UTILITY_OPTIONAL_HPP
#define RANGES_V3_UTILITY_OPTIONAL_HPP

#include <initializer_list>
#include <stdexcept>
#include <range/v3/detail/config.hpp>
#include <range/v3/utility/static_const.hpp>
#include <range/v3/utility/variant.hpp>

namespace ranges
{
    inline namespace v3
    {
        template<typename> class optional;

        class bad_optional_access
          : public std::logic_error
        {
        public:
            bad_optional_access()
              : std::logic_error("bad optional access")
            {}
        };

        struct nullopt_t
        {
            struct tag {};

            nullopt_t() = delete;
            constexpr nullopt_t(tag) noexcept {}
        };
#if RANGES_CXX_INLINE_VARIABLES >= RANGES_CXX_INLINE_VARIABLES_17
        inline constexpr nullopt_t nullopt{nullopt_t::tag{}};
#else
        namespace detail
        {
            template<typename>
            struct nullopt_holder {
                static constexpr nullopt_t nullopt{nullopt_t::tag{}};
            };
            template<typename T>
            constexpr nullopt_t nullopt_holder<T>::nullopt;
        }
        inline namespace {
            constexpr auto& nullopt = detail::nullopt_holder<void>::nullopt;
        }
#endif

        namespace detail
        {
            template<typename = void>
            [[noreturn]] bool throw_bad_optional_access()
            {
                throw bad_optional_access{};
            }

            template<typename T>
            class optional_cleanup_guard
            {
                optional<T>* ptr_;
            public:
                ~optional_cleanup_guard()
                {
                    if (ptr_)
                    {
                        ptr_->reset();
                    }
                }
                constexpr optional_cleanup_guard(optional<T>& opt) noexcept
                  : ptr_{&opt}
                {}

                void release() noexcept
                {
                    ptr_ = nullptr;
                }
            };

            struct optional_access
            {
                template<typename O,
                    CONCEPT_REQUIRES_(meta::is<uncvref_t<O>, optional>())>
                static constexpr auto data(O && o) noexcept
                RANGES_DECLTYPE_AUTO_RETURN
                (
                    ((O &&)o).v_
                )
            };

            namespace optional_adl
            {
                struct swap_hook {};

                template<typename T>
                auto swap(optional<T> &x, optional<T> &y)
                RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                (
                    x.swap(y)
                )
            }
        }

        template<typename T>
        class optional
          : detail::optional_adl::swap_hook
        {
            friend detail::optional_access;
            using guard_t = detail::optional_cleanup_guard<T>;
            variant<monostate, T> v_;
        public:
            CONCEPT_ASSERT(std::is_object<T>());
            CONCEPT_ASSERT(Destructible<T>());
            CONCEPT_ASSERT(!Same<nullopt_t, T>());
            using value_type = T;

            constexpr optional() noexcept {}
            constexpr optional(nullopt_t) noexcept {}

            CONCEPT_REQUIRES(CopyConstructible<T>())
            constexpr optional(T const &t)
                noexcept(std::is_nothrow_copy_constructible<T>::value)
              : v_{in_place<1>, t}
            {}
            CONCEPT_REQUIRES(MoveConstructible<T>())
            constexpr optional(T && t)
                noexcept(std::is_nothrow_move_constructible<T>::value)
              : v_{in_place<1>, detail::move(t)}
            {}

            template<typename... Args,
                CONCEPT_REQUIRES_(Constructible<T, Args &&...>())>
            constexpr explicit optional(in_place_t, Args &&... args)
                noexcept(std::is_nothrow_constructible<T, Args...>::value)
              : v_{in_place<1>, (Args &&)args...}
            {}
            template<typename E, typename... Args,
                CONCEPT_REQUIRES_(Constructible<T, std::initializer_list<E>&, Args &&...>())>
            RANGES_CXX14_CONSTEXPR explicit
            optional(in_place_t, std::initializer_list<E> &il, Args &&... args)
                noexcept(std::is_nothrow_constructible<
                    T, std::initializer_list<E>&, Args...>::value)
              : v_{in_place<1>, il, (Args &&)args...}
            {}

            optional& operator=(nullopt_t) noexcept
            {
                reset();
                return *this;
            }
            template<typename U,
                CONCEPT_REQUIRES_(Constructible<T, U &&>() && Assignable<T&, U &&>())>
            optional& operator=(U && u)
                noexcept(std::is_nothrow_constructible<T, U>::value &&
                    std::is_nothrow_assignable<T&, U>::value)
            {
                guard_t guard{*this};
                v_ = (U &&)u;
                guard.release();
                return *this;
            }

            template<typename... Args,
                CONCEPT_REQUIRES_(Constructible<T, Args &&...>())>
            void emplace(Args &&... args)
                noexcept(std::is_nothrow_constructible<T, Args...>::value)
            {
                guard_t guard{*this};
                v_.template emplace<1>((Args &&)args...);
                guard.release();
            }
            template<typename E, typename... Args,
                CONCEPT_REQUIRES_(Constructible<T, std::initializer_list<E>&, Args &&...>())>
            void emplace(std::initializer_list<E> il, Args &&... args)
                noexcept(std::is_nothrow_constructible<
                    T, std::initializer_list<E>&, Args...>::value)
            {
                guard_t guard{*this};
                v_.template emplace<1>(il, (Args &&)args...);
                guard.release();
            }

            CONCEPT_REQUIRES(Swappable<T&, T&>() && MoveConstructible<T>())
            void swap(optional &that)
                noexcept(std::is_nothrow_move_constructible<T>::value &&
                    is_nothrow_swappable<T&, T&>::value)
            {
                guard_t guard{*this};
                ranges::swap(v_, that.v_);
                guard.release();
            }

            constexpr T const *operator->() const noexcept
            {
                return detail::addressof(**this);
            }
            RANGES_CXX14_CONSTEXPR T *operator->() noexcept
            {
                return detail::addressof(**this);
            }

            RANGES_CXX14_CONSTEXPR T &operator*() & noexcept
            {
                return RANGES_EXPECT(has_value()),
                    ranges::get_unchecked<1>(v_);
            }
            constexpr T const &operator*() const& noexcept
            {
                return RANGES_EXPECT(has_value()),
                    ranges::get_unchecked<1>(v_);
            }
            RANGES_CXX14_CONSTEXPR T && operator*() && noexcept
            {
                return detail::move(**this);
            }
            constexpr T const && operator*() const&& noexcept
            {
                return detail::move(**this);
            }

            constexpr explicit operator bool() const noexcept
            {
                return has_value();
            }
            constexpr bool has_value() const noexcept
            {
                return RANGES_EXPECT(v_.index() == 0 || v_.index() == 1),
                    v_.index() != 0;
            }

            constexpr T const &value() const&
            {
                return (has_value() || detail::throw_bad_optional_access()),
                    **this;
            }
            RANGES_CXX14_CONSTEXPR T &value() &
            {
                return (has_value() || detail::throw_bad_optional_access()),
                    **this;
            }
            constexpr T const && value() const&&
            {
                return (has_value() || detail::throw_bad_optional_access()),
                    detail::move(**this);
            }
            RANGES_CXX14_CONSTEXPR T && value() &&
            {
                return (has_value() || detail::throw_bad_optional_access()),
                    detail::move(**this);
            }

            template<typename U,
                CONCEPT_REQUIRES_(CopyConstructible<T>() && ConvertibleTo<U, T>())>
            constexpr T value_or(U && u) const&
            {
                return has_value() ? **this : static_cast<T>((U &&)u);
            }
            template<typename U,
                CONCEPT_REQUIRES_(MoveConstructible<T>() && ConvertibleTo<U, T>())>
            T value_or(U && u) &&
            {
                return has_value() ? detail::move(**this) : static_cast<T>((U &&)u);
            }

            void reset() noexcept
            {
                v_.template emplace<0>();
            }
        };

        template<typename T>
        constexpr auto operator==(optional<T> const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) == detail::optional_access::data(y)
        )
        template<typename T>
        constexpr auto operator==(optional<T> const &x, T const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) == y
        )
        template<typename T>
        constexpr auto operator==(T const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            x == detail::optional_access::data(y)
        )
        template<typename T>
        constexpr bool operator==(optional<T> const &x, nullopt_t) noexcept
        {
            return !x;
        }
        template<typename T>
        constexpr bool operator==(nullopt_t, optional<T> const &x) noexcept
        {
            return !x;
        }

        template<typename T>
        constexpr auto operator!=(optional<T> const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) != detail::optional_access::data(y)
        )
        template<typename T>
        constexpr auto operator!=(optional<T> const &x, T const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) != y
        )
        template<typename T>
        constexpr auto operator!=(T const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            x != detail::optional_access::data(y)
        )
        template<typename T>
        constexpr bool operator!=(optional<T> const &x, nullopt_t) noexcept
        {
            return !!x;
        }
        template<typename T>
        constexpr bool operator!=(nullopt_t, optional<T> const &x) noexcept
        {
            return !!x;
        }

        template<typename T>
        constexpr auto operator<(optional<T> const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) < detail::optional_access::data(y)
        )
        template<typename T>
        constexpr auto operator<(optional<T> const &x, T const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) < y
        )
        template<typename T>
        constexpr auto operator<(T const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            x < detail::optional_access::data(y)
        )
        template<typename T>
        constexpr bool operator<(optional<T> const &, nullopt_t) noexcept
        {
            return false;
        }
        template<typename T>
        constexpr bool operator<(nullopt_t, optional<T> const &x) noexcept
        {
            return !!x;
        }

        template<typename T>
        constexpr auto operator>(optional<T> const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) > detail::optional_access::data(y)
        )
        template<typename T>
        constexpr auto operator>(optional<T> const &x, T const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) > y
        )
        template<typename T>
        constexpr auto operator>(T const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            x > detail::optional_access::data(y)
        )
        template<typename T>
        constexpr bool operator>(optional<T> const &x, nullopt_t) noexcept
        {
            return !!x;
        }
        template<typename T>
        constexpr bool operator>(nullopt_t, optional<T> const &) noexcept
        {
            return false;
        }


        template<typename T>
        constexpr auto operator<=(optional<T> const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) <= detail::optional_access::data(y)
        )
        template<typename T>
        constexpr auto operator<=(optional<T> const &x, T const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) <= y
        )
        template<typename T>
        constexpr auto operator<=(T const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            x <= detail::optional_access::data(y)
        )
        template<typename T>
        constexpr bool operator<=(optional<T> const &x, nullopt_t) noexcept
        {
            return !x;
        }
        template<typename T>
        constexpr bool operator<=(nullopt_t, optional<T> const &) noexcept
        {
            return true;
        }

        template<typename T>
        constexpr auto operator>=(optional<T> const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) >= detail::optional_access::data(y)
        )
        template<typename T>
        constexpr auto operator>=(optional<T> const &x, T const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            detail::optional_access::data(x) >= y
        )
        template<typename T>
        constexpr auto operator>=(T const &x, optional<T> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            x >= detail::optional_access::data(y)
        )
        template<typename T>
        constexpr bool operator>=(optional<T> const &, nullopt_t) noexcept
        {
            return true;
        }
        template<typename T>
        constexpr bool operator>=(nullopt_t, optional<T> const &x) noexcept
        {
            return !x;
        }

        template<typename T>
        constexpr auto make_optional(T && t)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            optional<detail::decay_t<T>>{(T &&)t}
        )
        template<typename T, typename... Args>
        constexpr auto make_optional(Args &&... args)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            optional<T>{in_place, (Args &&)args...}
        )
        template<typename T, typename U, typename... Args>
        constexpr auto make_optional(std::initializer_list<U> il, Args &&... args)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            optional<T>{in_place, il, (Args &&)args...}
        )

        namespace detail
        {
            template<typename T, typename Tag = void, bool Enable = true>
            struct non_propagating_cache
              : optional<T>
            {
                using optional<T>::optional;
                using optional<T>::operator=;

                non_propagating_cache() = default;
                constexpr
                non_propagating_cache(non_propagating_cache const &) noexcept
                  : optional<T>{}
                {}
                RANGES_CXX14_CONSTEXPR
                non_propagating_cache(non_propagating_cache && that) noexcept
                  : optional<T>{}
                {
                    that.optional<T>::reset();
                }
                RANGES_CXX14_CONSTEXPR
                non_propagating_cache& operator=(non_propagating_cache const &) noexcept
                {
                    this->optional<T>::reset();
                    return *this;
                }
                RANGES_CXX14_CONSTEXPR
                non_propagating_cache& operator=(non_propagating_cache && that) noexcept
                {
                    that.optional<T>::reset();
                    this->optional<T>::reset();
                    return *this;
                }
            };

            template<typename T, typename Tag>
            struct non_propagating_cache<T, Tag, false>
            {};
        }
    }
}

#endif
