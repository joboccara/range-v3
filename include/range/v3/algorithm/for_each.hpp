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
#ifndef RANGES_V3_ALGORITHM_FOR_EACH_HPP
#define RANGES_V3_ALGORITHM_FOR_EACH_HPP

#include <range/v3/range_fwd.hpp>
#include <range/v3/begin_end.hpp>
#include <range/v3/range_traits.hpp>
#include <range/v3/range_concepts.hpp>
#include <range/v3/algorithm/tagspec.hpp>
#include <range/v3/utility/functional.hpp>
#include <range/v3/utility/iterator_traits.hpp>
#include <range/v3/utility/static_const.hpp>
#include <range/v3/utility/tagged_pair.hpp>
#include <range/v3/utility/tuple_algorithm.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \addtogroup group-algorithms
        /// @{
        struct for_each_fn
        {
        private:
            template<typename, typename>
            struct pop_back_
            {};
            template<typename... Ts, std::size_t... Is>
            struct pop_back_<meta::list<Ts...>, meta::index_sequence<Is...>>
            {
                using type = meta::list<meta::at_c<meta::list<Ts...>, Is>...>;
            };

            // Complexity: O(meta::size<T>)
            template<typename T>
            using pop_back = meta::_t<pop_back_<T,
                meta::make_index_sequence<
                    meta::dec<meta::if_<meta::empty<T>, meta::size_t<1>, meta::size<T>>>::value>
                >>;

            struct dereference_fn
            {
                template<typename I,
                    CONCEPT_REQUIRES_(Iterator<I>())>
                RANGES_CXX14_CONSTEXPR
                auto operator()(I const &i) const
                RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                (
                    *i
                )
            };
            struct increment_fn
            {
                template<typename I,
                    CONCEPT_REQUIRES_(Iterator<I>())>
                RANGES_CXX14_CONSTEXPR
                auto operator()(I &i) const
                RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                (
                    void(++i)
                )
            };

            template<typename... Rngs>
            struct variadic_helper
            {
                template<typename Fn, typename... Rs>
                using Constraint = meta::and_<
                    CopyConstructible<Fn>,
                    Invocable<Fn&, range_reference_t<Rs>...>>;

                template<typename Fn,
                    CONCEPT_REQUIRES_(
                        meta::and_<
                            meta::strict_and<InputRange<Rngs>...>,
                            Constraint<Fn, Rngs...>>::value)>
                RANGES_CXX14_CONSTEXPR
                tagged_pair<tag::in(std::tuple<range_safe_iterator_t<Rngs>...>), tag::fun(Fn)>
                static impl(Rngs &&... rngs, Fn f)
                {
                    std::tuple<range_iterator_t<Rngs>...> its{begin(rngs)...};
                    std::tuple<range_sentinel_t<Rngs>...> const ends{end(rngs)...};
                    for (; !tuple_any_equal(its, ends); tuple_for_each(its, increment_fn{}))
                    {
                        tuple_apply(f, tuple_transform(its, dereference_fn{}));
                    }
                    return {std::move(its), std::move(f)};
                }
            };
        public:
            template<typename I, typename S, typename F, typename P = ident,
                CONCEPT_REQUIRES_(InputIterator<I>() && Sentinel<S, I>() &&
                    IndirectInvocable<F, projected<I, P>>())>
            RANGES_CXX14_CONSTEXPR
            tagged_pair<tag::in(I), tag::fun(F)>
            operator()(I begin, S end, F fun, P proj = P{}) const
            {
                for(; begin != end; ++begin)
                {
                    invoke(fun, invoke(proj, *begin));
                }
                return {detail::move(begin), detail::move(fun)};
            }

            template<typename Rng, typename F, typename P = ident,
                CONCEPT_REQUIRES_(InputRange<Rng>() &&
                    IndirectInvocable<F, projected<range_iterator_t<Rng>, P>>())>
            RANGES_CXX14_CONSTEXPR
            tagged_pair<tag::in(range_safe_iterator_t<Rng>), tag::fun(F)>
            operator()(Rng &&rng, F fun, P proj = P{}) const
            {
                return {(*this)(begin(rng), end(rng), ref(fun), detail::move(proj)).in(),
                    detail::move(fun)};
            }

            template<typename First, typename... Rest,
                typename Helper = meta::apply<
                    meta::quote<variadic_helper>,
                    pop_back<meta::list<First, Rest...>>>>
            RANGES_CXX14_CONSTEXPR
            auto
            operator()(First && first, Rest &&... rest) const
            RANGES_DECLTYPE_AUTO_RETURN
            (
                Helper::impl((First &&) first, (Rest &&) rest...)
            )
        };

        /// \sa `for_each_fn`
        /// \ingroup group-algorithms
        RANGES_INLINE_VARIABLE(with_braced_init_args<for_each_fn>, for_each)
        /// @}
    } // namespace v3
} // namespace ranges

#endif // include guard
