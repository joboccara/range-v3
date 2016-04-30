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

#ifndef RANGES_V3_UTILITY_COMPRESSED_PAIR_HPP
#define RANGES_V3_UTILITY_COMPRESSED_PAIR_HPP

#include <utility>
#include <type_traits>
#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/utility/box.hpp>
#include <range/v3/utility/concepts.hpp>
#include <range/v3/utility/functional.hpp>
#include <range/v3/utility/static_const.hpp>

namespace ranges
{
    inline namespace v3
    {
        template<typename First, typename Second>
        struct compressed_pair
          : box<First, meta::size_t<0>>
          , box<Second, meta::size_t<1>>
        {
        private:
            using first_base = box<First, meta::size_t<0>>;
            using second_base = box<Second, meta::size_t<1>>;
        public:
            using first_type = First;
            using second_type = Second;

            CONCEPT_REQUIRES(DefaultConstructible<First>() &&
                DefaultConstructible<Second>())
            constexpr compressed_pair()
                noexcept(std::is_nothrow_default_constructible<First>() &&
                         std::is_nothrow_default_constructible<Second>())
              : first_base{}
              , second_base{}
            {}

            CONCEPT_REQUIRES(MoveConstructible<First>() &&
                MoveConstructible<Second>())
            constexpr compressed_pair(First f, Second s)
                noexcept(std::is_nothrow_move_constructible<First>() &&
                         std::is_nothrow_move_constructible<Second>())
              : first_base{(First &&) f}
              , second_base{(Second &&) s}
            {}

            template<typename F, typename S,
                CONCEPT_REQUIRES_(Constructible<First, F>() &&
                    Constructible<Second, S>())>
            constexpr compressed_pair(F && f, S && s)
                noexcept(std::is_nothrow_constructible<First, F>() &&
                         std::is_nothrow_constructible<Second, S>())
              : first_base{(F &&) f}
              , second_base{(S &&) s}
            {}

            template<typename F, typename S,
                CONCEPT_REQUIRES_(CopyConstructible<First>() &&
                    CopyConstructible<Second>())>
            constexpr operator std::pair<F, S> () const
            {
                return std::pair<F, S>{first(), second()};
            }

            First &first() & noexcept {
                return ranges::get<0>(*this);
            }
            First const &first() const& noexcept {
                return ranges::get<0>(*this);
            }
            First &&first() && noexcept {
                return ranges::get<0>(detail::move(*this));
            }

            Second &second() & noexcept {
                return ranges::get<1>(*this);
            }
            Second const &second() const& noexcept {
                return ranges::get<1>(*this);
            }
            Second &&second() && noexcept {
                return ranges::get<1>(detail::move(*this));
            }
        };

        struct make_compressed_pair_fn
        {
            using expects_wrapped_references = void;
            template<typename First, typename Second>
            constexpr auto operator()(First && f, Second && s) const ->
                compressed_pair<bind_element_t<First>, bind_element_t<Second>>
            {
                return {detail::forward<First>(f), detail::forward<Second>(s)};
            }
        };

        /// \ingroup group-utility
        /// \sa `make_compressed_pair_fn`
        namespace
        {
            constexpr auto&& make_compressed_pair = static_const<make_compressed_pair_fn>::value;
        }
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wmismatched-tags"

namespace std
{
    template<typename First, typename Second>
    struct tuple_size< ::ranges::v3::compressed_pair<First, Second>>
      : std::integral_constant<size_t, 2>
    {};

    template<typename First, typename Second>
    struct tuple_element<0, ::ranges::v3::compressed_pair<First, Second>>
    {
        using type = First;
    };

    template<typename First, typename Second>
    struct tuple_element<1, ::ranges::v3::compressed_pair<First, Second>>
    {
        using type = Second;
    };
}

#pragma GCC diagnostic pop

#endif
