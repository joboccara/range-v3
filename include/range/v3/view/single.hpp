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

#ifndef RANGES_V3_VIEW_SINGLE_HPP
#define RANGES_V3_VIEW_SINGLE_HPP

#include <utility>
#include <type_traits>
#include <range/v3/range_fwd.hpp>
#include <range/v3/begin_end.hpp>
#include <range/v3/range_concepts.hpp>
#include <range/v3/range_traits.hpp>
#include <range/v3/view_facade.hpp>
#include <range/v3/utility/iterator_concepts.hpp>
#include <range/v3/utility/iterator_traits.hpp>
#include <range/v3/utility/semiregular.hpp>
#include <range/v3/utility/static_const.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \addtogroup group-views
        /// @{
        template<typename Val>
        struct single_view
          : view_facade<single_view<Val>, (cardinality)1>
        {
        private:
            friend struct range_access;
            mutable semiregular_t<Val> value_;
            struct cursor
            {
            private:
                Val* ptr_ = nullptr;
            public:
                cursor() = default;
                explicit cursor(Val* ptr)
                  : ptr_(ptr)
                {}
                Val& get() const
                {
                    return *ptr_;
                }
                bool equal(cursor const &that) const
                {
                    return ptr_ == that.ptr_;
                }
                void next()
                {
                    ++ptr_;
                }
                void prev()
                {
                    --ptr_;
                }
                void advance(std::ptrdiff_t n)
                {
                    ptr_ += n;
                }
                std::ptrdiff_t distance_to(cursor const &that) const
                {
                    return that.ptr_ - ptr_;
                }
            };
            cursor begin_cursor() const
            {
                return cursor{std::addressof(static_cast<Val&>(value_))};
            }
            cursor end_cursor() const
            {
                return cursor{std::addressof(static_cast<Val&>(value_)) + 1};
            }
        public:
            single_view() = default;
            template<typename Arg,
                CONCEPT_REQUIRES_(Constructible<Val, Arg &&>())>
            constexpr explicit single_view(Arg && arg)
              : value_(std::forward<Arg>(arg))
            {}
            constexpr std::size_t size() const
            {
                return 1;
            }
        };

        namespace view
        {
            struct single_fn
            {
                template<typename Arg, typename Val = detail::decay_t<Arg>,
                    CONCEPT_REQUIRES_(CopyConstructible<Val>() && Constructible<Val, Arg &&>())>
                single_view<Val> operator()(Arg && arg) const
                {
                    return single_view<Val>{std::forward<Arg>(arg)};
                }
            #ifndef RANGES_DOXYGEN_INVOKED
                // For error reporting
                template<typename Arg, typename Val = detail::decay_t<Arg>,
                    CONCEPT_REQUIRES_(!(CopyConstructible<Val>() && Constructible<Val, Arg &&>()))>
                void operator()(Arg &&) const
                {
                    CONCEPT_ASSERT_MSG(CopyConstructible<Val>(),
                        "The object passed to view::single must be a model of the CopyConstructible "
                        "concept; that is, it needs to be copy and move constructible, and destructible.");
                    CONCEPT_ASSERT_MSG(!CopyConstructible<Val>() || Constructible<Val, Arg &&>(),
                        "The object type passed to view::single must be initializable from the "
                        "actual argument expression.");
                }
            #endif
            };

            /// \relates single_fn
            /// \ingroup group-views
            RANGES_INLINE_VARIABLE(single_fn, single)
        }
        /// @}
    }
}

#endif
