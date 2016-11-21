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

#ifndef RANGES_V3_VIEW_TAKE_WHILE_HPP
#define RANGES_V3_VIEW_TAKE_WHILE_HPP

#include <utility>
#include <functional>
#include <type_traits>
#include <meta/meta.hpp>
#include <range/v3/detail/satisfy_boost_range.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/range_concepts.hpp>
#include <range/v3/view_adaptor.hpp>
#include <range/v3/utility/functional.hpp>
#include <range/v3/utility/semiregular.hpp>
#include <range/v3/utility/iterator_concepts.hpp>
#include <range/v3/utility/static_const.hpp>
#include <range/v3/view/view.hpp>

namespace ranges
{
    inline namespace v3
    {
        template<typename F, typename>
        struct function_wrapper
        {
            F f_;

            template<typename... Args>
            RANGES_CXX14_CONSTEXPR auto operator()(Args &&... args) &
                noexcept(noexcept(std::declval<F&>()(std::declval<Args>()...)))
                -> decltype(std::declval<F&>()(std::declval<Args>()...))
            {
                return f_((Args &&)args...);
            }
            template<typename... Args>
            constexpr auto operator()(Args &&... args) const&
                noexcept(noexcept(std::declval<F const&>()(std::declval<Args>()...)))
                -> decltype(std::declval<F const&>()(std::declval<Args>()...))
            {
                return f_((Args &&)args...);
            }
            template<typename... Args>
            RANGES_CXX14_CONSTEXPR auto operator()(Args &&... args) &&
                noexcept(noexcept(std::declval<F&&>()(std::declval<Args>()...)))
                -> decltype(std::declval<F&&>()(std::declval<Args>()...))
            {
                return detail::move(f_)((Args &&)args...);
            }
            template<typename... Args>
            RANGES_CXX14_CONSTEXPR auto operator()(Args &&... args) const&&
                noexcept(noexcept(std::declval<F const&&>()(std::declval<Args>()...)))
                -> decltype(std::declval<F const&&>()(std::declval<Args>()...))
            {
                return detail::move(f_)((Args &&)args...);
            }
        };

        struct regular_function_tag {};
        template<typename F,
            CONCEPT_REQUIRES_(Constructible<detail::decay_t<F>, F&&>())>
        constexpr function_wrapper<detail::decay_t<F>, regular_function_tag>
        regular_function(F && f)
        {
            return {(F &&)f};
        }

        struct non_regular_function_tag {};
        template<typename F,
            CONCEPT_REQUIRES_(Constructible<detail::decay_t<F>, F&&>())>
        function_wrapper<detail::decay_t<F>, non_regular_function_tag>
        non_regular_function(F && f)
        {
            return {(F &&)f};
        }

        template<typename F, typename... Args>
        struct is_regular_function
          : Callable<F const, Args...>
        {};
        template<typename F, typename... Args>
        struct is_regular_function<function_wrapper<F, regular_function_tag>, Args...>
          : std::true_type
        {};
        template<typename F, typename... Args>
        struct is_regular_function<function_wrapper<F, non_regular_function_tag>, Args...>
          : std::false_type
        {};
        template<typename F, typename... Iterators>
        struct is_regular_function<indirected<F>, Iterators...>
          : is_regular_function<F, iterator_reference_t<Iterators>...>
        {};

        /// \addtogroup group-views
        /// @{
        template<typename Rng, typename Pred,
            bool = is_regular_function<Pred, range_iterator_t<Rng>>::value>
        struct iter_take_while_view;

        template<typename Rng, typename Pred>
        struct iter_take_while_view<Rng, Pred, true>
          : view_adaptor<
                iter_take_while_view<Rng, Pred, true>,
                Rng,
                is_finite<Rng>::value ? finite : unknown>
        {
        private:
            CONCEPT_ASSERT(InputRange<Rng>());
            CONCEPT_ASSERT(Predicate<Pred, range_iterator_t<Rng>>());

            friend range_access;
            semiregular_t<function_type<Pred>> pred_;

            template<bool IsConst>
            struct sentinel_adaptor
              : adaptor_base
            {
            private:
                semiregular_ref_or_val_t<function_type<Pred>, IsConst> pred_;
            public:
                sentinel_adaptor() = default;
                sentinel_adaptor(semiregular_ref_or_val_t<function_type<Pred>, IsConst> pred)
                  : pred_(std::move(pred))
                {}
                bool empty(range_iterator_t<Rng> const &it, range_sentinel_t<Rng> const &end) const
                {
                    return it == end || !pred_(it);
                }
            };
            sentinel_adaptor<false> end_adaptor()
            {
                return {pred_};
            }
            CONCEPT_REQUIRES(Callable<Pred const, range_iterator_t<Rng>>())
            sentinel_adaptor<true> end_adaptor() const
            {
                return {pred_};
            }
        public:
            iter_take_while_view() = default;
            iter_take_while_view(Rng rng, Pred pred)
              : iter_take_while_view::view_adaptor{std::move(rng)}
              , pred_(as_function(std::move(pred)))
            {}
        };

        template<typename Rng, typename Pred>
        struct iter_take_while_view<Rng, Pred, false>
          : view_facade<
                iter_take_while_view<Rng, Pred, false>,
                is_finite<Rng>::value ? finite : unknown>
        {
        private:
            CONCEPT_ASSERT(InputRange<Rng>());
            CONCEPT_ASSERT(Predicate<Pred, range_iterator_t<Rng>>());

            friend range_access;

            mutable Rng rng_;
            mutable semiregular_t<function_type<Pred>> pred_;
            mutable range_iterator_t<Rng> it_;
            mutable bool done_ = false;

            struct cursor
            {
            private:
                iter_take_while_view const* rng_ = nullptr;
            public:
                using value_type = range_value_t<Rng>;
                cursor() = default;
                cursor(iter_take_while_view const &rng)
                  : rng_(&rng)
                {}
                range_reference_t<Rng> get() const
                {
                    RANGES_ASSERT(rng_);
                    RANGES_ASSERT(!rng_->done_);
                    return *rng_->it_;
                }
                void next()
                {
                    RANGES_ASSERT(rng_);
                    rng_->next();
                }
                bool done() const
                {
                    RANGES_ASSERT(rng_);
                    return rng_->done_;
                }
            };
            void at_end() const
            {
                done_ = it_ == ranges::end(rng_) || !pred_(it_);
            }
            cursor begin_cursor() const
            {
                return {*this};
            }
            void next() const
            {
                RANGES_ASSERT(!done_);
                ++it_;
                at_end();
            }
        public:
            iter_take_while_view() = default;
            iter_take_while_view(Rng rng, Pred pred)
              : rng_(std::move(rng))
              , pred_(as_function(std::move(pred)))
              , it_(ranges::begin(rng_))
            {
                at_end();
            }
        };

        template<typename Rng, typename Pred>
        struct take_while_view
          : iter_take_while_view<Rng, indirected<Pred>>
        {
            take_while_view() = default;
            take_while_view(Rng rng, Pred pred)
              : iter_take_while_view<Rng, indirected<Pred>>{std::move(rng),
                    indirect(std::move(pred))}
            {}
        };

        namespace view
        {
            struct iter_take_while_fn
            {
            private:
                friend view_access;
                template<typename Pred>
                static auto bind(iter_take_while_fn iter_take_while, Pred pred)
                RANGES_DECLTYPE_AUTO_RETURN
                (
                    make_pipeable(std::bind(iter_take_while, std::placeholders::_1,
                        protect(std::move(pred))))
                )
            public:
                template<typename Rng, typename Pred>
                using Concept = meta::and_<
                    InputRange<Rng>,
                    CallablePredicate<Pred, range_iterator_t<Rng>>>;

                template<typename Rng, typename Pred,
                    CONCEPT_REQUIRES_(Concept<Rng, Pred>())>
                iter_take_while_view<all_t<Rng>, Pred> operator()(Rng && rng, Pred pred) const
                {
                    return {all(std::forward<Rng>(rng)), std::move(pred)};
                }
            #ifndef RANGES_DOXYGEN_INVOKED
                template<typename Rng, typename Pred,
                    CONCEPT_REQUIRES_(!Concept<Rng, Pred>())>
                void operator()(Rng &&, Pred) const
                {
                    CONCEPT_ASSERT_MSG(InputRange<Rng>(),
                        "The object on which view::take_while operates must be a model of the "
                        "InputRange concept.");
                    CONCEPT_ASSERT_MSG(CallablePredicate<Pred, range_iterator_t<Rng>>(),
                        "The function passed to view::take_while must be callable with objects of "
                        "the range's iterator type, and its result type must be convertible to "
                        "bool.");
                }
            #endif
            };

            struct take_while_fn
            {
            private:
                friend view_access;
                template<typename Pred>
                static auto bind(take_while_fn take_while, Pred pred)
                RANGES_DECLTYPE_AUTO_RETURN
                (
                    make_pipeable(std::bind(take_while, std::placeholders::_1,
                        protect(std::move(pred))))
                )
            public:
                template<typename Rng, typename Pred>
                using Concept = meta::and_<
                    InputRange<Rng>,
                    IndirectCallablePredicate<Pred, range_iterator_t<Rng>>>;

                template<typename Rng, typename Pred,
                    CONCEPT_REQUIRES_(Concept<Rng, Pred>())>
                take_while_view<all_t<Rng>, Pred> operator()(Rng && rng, Pred pred) const
                {
                    return {all(std::forward<Rng>(rng)), std::move(pred)};
                }
            #ifndef RANGES_DOXYGEN_INVOKED
                template<typename Rng, typename Pred,
                    CONCEPT_REQUIRES_(!Concept<Rng, Pred>())>
                void operator()(Rng &&, Pred) const
                {
                    CONCEPT_ASSERT_MSG(InputRange<Rng>(),
                        "The object on which view::take_while operates must be a model of the "
                        "InputRange concept.");
                    CONCEPT_ASSERT_MSG(IndirectCallablePredicate<Pred, range_iterator_t<Rng>>(),
                        "The function passed to view::take_while must be callable with objects of "
                        "the range's common reference type, and its result type must be "
                        "convertible to bool.");
                }
            #endif
            };

            /// \relates iter_take_while_fn
            /// \ingroup group-views
            RANGES_INLINE_VARIABLE(view<iter_take_while_fn>, iter_take_while)

            /// \relates take_while_fn
            /// \ingroup group-views
            RANGES_INLINE_VARIABLE(view<take_while_fn>, take_while)
        }
        /// @}
    }
}

RANGES_SATISFY_BOOST_RANGE(::ranges::v3::iter_take_while_view)
RANGES_SATISFY_BOOST_RANGE(::ranges::v3::take_while_view)

#endif
