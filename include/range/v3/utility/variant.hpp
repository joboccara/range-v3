/// \file
// Range v3 library
//
//  Copyright Casey Carter 2016
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_UTILITY_VARIANT_HPP
#define RANGES_V3_UTILITY_VARIANT_HPP

#include <initializer_list>
#include <limits>
#include <memory>
#include <new>
#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/utility/concepts.hpp>
#include <range/v3/utility/functional.hpp>
#include <range/v3/utility/swap.hpp>

// TODO:
// * allocator garbage

namespace ranges
{
    inline namespace v3
    {
        struct in_place_tag
        {
            in_place_tag() = delete;
        private:
            constexpr in_place_tag(int) noexcept
            {}
        };
        constexpr in_place_tag in_place(in_place_tag arg)
        {
            return arg;
        }
        using in_place_t = in_place_tag(&)(in_place_tag);

        template<typename T>
        constexpr in_place_tag in_place(in_place_tag arg, meta::id<T>)
        {
            return arg;
        }
        template<typename T>
        using in_place_type_t = in_place_tag(&)(in_place_tag, meta::id<T>);

        template<std::size_t I>
        constexpr in_place_tag in_place(in_place_tag arg, meta::size_t<I>)
        {
            return arg;
        }
        template<std::size_t I>
        using in_place_index_t = in_place_tag(&)(in_place_tag, meta::size_t<I>);

        template<typename T, std::size_t I>
        class indexed_element
        {
            CONCEPT_ASSERT(std::is_reference<T>());
            T item_;
        public:
            constexpr indexed_element(T item) noexcept
                : item_(static_cast<T>(item))
            {}
            constexpr T get() const noexcept
            {
                return static_cast<T>(item_);
            }
        };

        template<std::size_t I, typename T>
        constexpr indexed_element<T&&, I> make_indexed_element(T && t) noexcept
        {
            return {(T &&)t};
        }

        namespace detail
        {
            namespace has_adl_swap_detail
            {
                void swap();

                template<typename T, typename = decltype(swap(std::declval<T&>(), std::declval<T&>()))>
                constexpr bool check(int) { return true; }
                template<typename>
                constexpr bool check(...) { return false; }
            }
            template<typename T>
            using has_adl_swap = meta::bool_<has_adl_swap_detail::check<T>(42)>;

        #if defined(__cpp_lib_addressof_constexpr) && __cpp_lib_addressof_constexpr >= 201603

            using std::addressof;

        #else
        #if defined(__clang__)
        #define RANGES_HAS_BUILTIN_ADDRESSOF __has_builtin(__builtin_addressof)
        #elif defined(__GNUC__) && __GNUC__ >= 7
        #define RANGES_HAS_BUILTIN_ADDRESSOF 1
        #else
        #define RANGES_HAS_BUILTIN_ADDRESSOF 0
        #endif

        #if RANGES_HAS_BUILTIN_ADDRESSOF

            template<typename T>
            constexpr T *addressof(T &t) noexcept
            {
                return __builtin_addressof(t);
            }

        #else

            struct MemberAddress
            {
                template<typename T>
                auto requires_(T && t) -> decltype(concepts::valid_expr(
                    (((T &&)t).operator&(), 42)
                ));
            };

            struct NonMemberAddress
            {
                template<typename T>
                auto requires_(T && t) -> decltype(concepts::valid_expr(
                    (operator&((T &&)t), 42)
                ));
            };

            template<typename T>
            using UserDefinedAddressOf = meta::or_<
                concepts::models<MemberAddress, T>,
                concepts::models<NonMemberAddress, T>>;

            template<typename T,
                CONCEPT_REQUIRES_(UserDefinedAddressOf<T>())>
            T *addressof(T &t) noexcept
            {
                return std::addressof(t);
            }

            template<typename T,
                CONCEPT_REQUIRES_(!UserDefinedAddressOf<T>())>
            constexpr T *addressof(T &t) noexcept
            {
                return &t;
            }

        #endif
        #endif // __cpp_lib_addressof_constexpr
        #undef RANGES_HAS_BUILTIN_ADDRESSOF
        } // namespace detail

        namespace smf
        {
            template<bool> struct copy {};
            template<> struct copy<false>
            {
                copy() = default;
                copy(copy const&) = delete;
                copy(copy&&) = default;
                copy& operator=(copy const&) & = default;
                copy& operator=(copy&&) & = default;
            };
            template<bool> struct move {};
            template<> struct move<false>
            {
                move() = default;
                move(move const&) = default;
                move(move&&) = delete;
                move& operator=(move const&) & = default;
                move& operator=(move&&) & = default;
            };
            template<bool> struct copy_assign {};
            template<> struct copy_assign<false>
            {
                copy_assign() = default;
                copy_assign(copy_assign const&) = default;
                copy_assign(copy_assign&&) = default;
                copy_assign& operator=(copy_assign const&) & = delete;
                copy_assign& operator=(copy_assign&&) & = default;
            };
            template<bool> struct move_assign {};
            template<> struct move_assign<false>
            {
                move_assign() = default;
                move_assign(move_assign const&) = default;
                move_assign(move_assign&&) = default;
                move_assign& operator=(move_assign const&) & = default;
                move_assign& operator=(move_assign&&) & = delete;
            };
        } // namespace smf

        // 20.7.2, variant of value types
        template<typename...> class variant;

        // 20.7.3, variant helper classes
        template<typename> struct variant_size {};
        template<typename T>
        struct variant_size<const T> : variant_size<T> {};
        template<typename T>
        struct variant_size<volatile T> : variant_size<T> {};
        template<typename T>
        struct variant_size<const volatile T> : variant_size<T> {};
        template<typename... Ts>
        struct variant_size<variant<Ts...>>
          : meta::size_t<sizeof...(Ts)>
        {};
#if RANGES_CXX_VARIABLE_TEMPLATES
        template<typename T>
        constexpr std::size_t variant_size_v = variant_size<T>::value;
#endif

        template<std::size_t I, typename T> struct variant_alternative {};
        template<std::size_t I, typename T>
        using variant_alternative_t = meta::_t<variant_alternative<I, T>>;
        template<std::size_t I, typename T>
        struct variant_alternative<I, const T>
          : meta::id<meta::_t<std::add_const<variant_alternative_t<I, T>>>>
        {};
        template<std::size_t I, typename T>
        struct variant_alternative<I, volatile T>
          : meta::id<meta::_t<std::add_volatile<variant_alternative_t<I, T>>>>
        {};
        template<std::size_t I, typename T>
        struct variant_alternative<I, const volatile T>
          : meta::id<meta::_t<std::add_cv<variant_alternative_t<I, T>>>>
        {};
        template<std::size_t I, typename... Ts>
        struct variant_alternative<I, variant<Ts...>>
          : meta::id<meta::at_c<meta::list<Ts...>, I>>
        {};

#if RANGES_CXX_INLINE_VARIABLES
        inline
#endif
        constexpr auto variant_npos = static_cast<std::size_t>(-1);

        // 20.7.10, class bad_variant_access
        struct bad_variant_access
          : std::exception
        {
            virtual const char* what() const noexcept
            {
                return "bad variant access";
            }
        };

        namespace detail
        {
            template<typename = void>
            [[noreturn]] bool throw_bad_variant_access()
            {
                throw bad_variant_access{};
            }

            template<std::size_t N>
            using variant_index_t =
                meta::if_c<(N <= std::numeric_limits<signed char>::max()), signed char,
                meta::if_c<(N <= std::numeric_limits<short>::max()), short,
                int>>;

            template<typename T, typename = void>
            class variant_item
            {
                using U = meta::_t<std::remove_cv<T>>;
                U item_;
            public:
                template<typename... Args,
                    CONCEPT_REQUIRES_(Constructible<U, Args &&...>())>
                constexpr variant_item(Args &&... args)
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 5
                  // *sigh* CWG1467 strikes again.
                  : item_((Args &&)args...)
#else
                  : item_{(Args &&)args...}
#endif
                {}

                RANGES_CXX14_CONSTEXPR T &get() & noexcept
                {
                    return item_;
                }
                constexpr T const &get() const& noexcept
                {
                    return item_;
                }
                RANGES_CXX14_CONSTEXPR T &&get() && noexcept
                {
                    return detail::move(item_);
                }
                constexpr T const &&get() const&& noexcept
                {
                    return detail::move(item_);
                }
            };

            template<typename T>
            class variant_item<T, meta::if_<std::is_reference<T>>>
            {
                meta::_t<std::add_pointer<T>> ptr_;
            public:
                template<typename Arg,
                    CONCEPT_REQUIRES_(std::is_constructible<T, Arg>())>
                constexpr variant_item(Arg && arg)
                  : ptr_{detail::addressof(arg)}
                {}

#if defined(__GNUC__) &&  !defined(__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 8
                T &get() & noexcept
                {
                    return *ptr_;
                }
                T get() && noexcept
                {
                    return static_cast<T>(*ptr_);
                }
#endif
                constexpr T &get() const& noexcept
                {
                    return *ptr_;
                }
                constexpr T get() const&& noexcept
                {
                    return static_cast<T>(*ptr_);
                }
            };

            template<typename T>
            class variant_item<T, meta::if_<std::is_void<T>>>
            {};

            template<bool, typename... Ts>
            struct variant_storage_ {
                void clear(std::size_t) noexcept
                {}
            };

            template<typename... Ts>
            using variant_storage = variant_storage_<
                meta::strict_and<std::is_trivially_destructible<variant_item<Ts>>...>::value, Ts...>;

            template<typename First, typename... Rest>
            struct variant_storage_<true, First, Rest...>
            {
                using head_t = variant_item<First>;
                using tail_t = variant_storage<Rest...>;
                static constexpr std::size_t size = sizeof...(Rest) + 1;
                union
                {
                    head_t head_;
                    tail_t tail_;
                };

                variant_storage_() noexcept {}

                template<typename... Args>
                constexpr variant_storage_(meta::size_t<0>, Args &&... args)
                    noexcept(std::is_nothrow_constructible<First, Args...>::value)
                  : head_{(Args &&)args...}
                {}
                template<std::size_t N, typename... Args>
                constexpr variant_storage_(meta::size_t<N>, Args &&... args)
                    noexcept(std::is_nothrow_constructible<
                        tail_t, meta::size_t<N - 1>, Args...>::value)
                  : tail_{meta::size_t<N - 1>{}, (Args &&)args...}
                {}

                void clear(std::size_t) noexcept
                {}

                template<typename... Args>
                void emplace(meta::size_t<0>, Args &&... args)
                    noexcept(std::is_nothrow_constructible<First, Args...>::value)
                {
                    ::new(&head_) head_t{(Args &&)args...};
                }
                template<std::size_t N, typename... Args>
                void emplace(meta::size_t<N>, Args &&... args)
                    noexcept(std::is_nothrow_constructible<
                        tail_t, meta::size_t<N - 1>, Args...>::value)
                {
                    ::new(&tail_) tail_t{
                        meta::size_t<N - 1>{}, (Args &&)args...};
                }
            };

            template<typename First, typename... Rest>
            struct variant_storage_<false, First, Rest...>
            {
                using head_t = variant_item<First>;
                using tail_t = variant_storage<Rest...>;
                static constexpr std::size_t size = sizeof...(Rest) + 1;
                union
                {
                    head_t head_;
                    tail_t tail_;
                };

                ~variant_storage_() {}
                variant_storage_() noexcept {}
                variant_storage_(variant_storage_ const&) = default;
                variant_storage_(variant_storage_ &&) = default;
                variant_storage_& operator=(variant_storage_ const&) & = default;
                variant_storage_& operator=(variant_storage_ &&) & = default;

                template<typename... Args>
                constexpr variant_storage_(meta::size_t<0>, Args &&... args)
                    noexcept(std::is_nothrow_constructible<First, Args...>::value)
                  : head_{(Args &&)args...}
                {}
                template<std::size_t N, typename... Args>
                constexpr variant_storage_(meta::size_t<N>, Args &&... args)
                    noexcept(std::is_nothrow_constructible<
                        tail_t, meta::size_t<N - 1>, Args...>::value)
                  : tail_{meta::size_t<N - 1>{}, (Args &&)args...}
                {}

                void clear(std::size_t i) noexcept
                {
                    if (i == 0)
                    {
                        head_.~head_t();
                    }
                    else
                    {
                        tail_.clear(i - 1);
                        tail_.~tail_t();
                    }
                }

                template<typename... Args>
                void emplace(meta::size_t<0>, Args &&... args)
                    noexcept(std::is_nothrow_constructible<First, Args...>::value)
                {
                    ::new(&head_) head_t{(Args &&)args...};
                }
                template<std::size_t N, typename... Args>
                void emplace(meta::size_t<N>, Args &&... args)
                    noexcept(std::is_nothrow_constructible<
                        tail_t, meta::size_t<N - 1>, Args...>::value)
                {
                    ::new(&tail_) tail_t{
                        meta::size_t<N - 1>{}, (Args &&)args...};
                }
            };

            template<std::size_t N, typename V>
            constexpr auto variant_raw_get_(meta::size_t<0>, V && storage) noexcept
            RANGES_DECLTYPE_AUTO_RETURN
            (
                make_indexed_element<N>(((V &&)storage).head_)
            )
            template<std::size_t N, std::size_t I, typename V>
            constexpr auto variant_raw_get_(meta::size_t<I>, V && storage) noexcept
            RANGES_DECLTYPE_AUTO_RETURN
            (
                variant_raw_get_<N>(meta::size_t<I - 1>{}, ((V &&)storage).tail_)
            )
            template<std::size_t N, typename V>
            constexpr auto variant_raw_get(V && storage) noexcept
            RANGES_DECLTYPE_AUTO_RETURN
            (
                variant_raw_get_<N>(meta::size_t<N>{}, (V &&)storage)
            )

            template<typename... Variants>
            using variant_ordinal_vectors =
                meta::cartesian_product<meta::list<
                    meta::as_list<meta::make_index_sequence<uncvref_t<Variants>::size>>...>>;

            template<typename List>
            using variant_all_same = meta::_t<meta::if_<
                std::is_same<List, meta::push_back<meta::pop_front<List>, meta::front<List>>>,
                meta::id<meta::front<List>>, meta::nil_>>;

            template<typename Projection, typename Visitor, typename Indices, typename... Variants>
            struct variant_single_visit_result_ {};
            template<typename Projection, typename Visitor, std::size_t...Is, typename... Variants>
            struct variant_single_visit_result_<
                Projection, Visitor, meta::list<meta::size_t<Is>...>, Variants...>
            {
                using type = decltype(std::declval<Visitor>()(
                    Projection{}(variant_raw_get<Is>(std::declval<Variants>()))...));
            };
            template<typename Projection, typename Visitor, typename Indices, typename... Variants>
            using variant_single_visit_result =
                meta::_t<variant_single_visit_result_<Projection, Visitor, Indices, Variants...>>;

            template<typename Projection, typename Visitor, typename Indices, typename... Variants>
            struct variant_all_visit_results_ {};
            template<typename Projection, typename Visitor, typename... IndexVectors, typename... Variants>
            struct variant_all_visit_results_<Projection, Visitor, meta::list<IndexVectors...>, Variants...>
            {
                using type = meta::list<
                    variant_single_visit_result<Projection, Visitor, IndexVectors, Variants...>...>;
            };
            template<typename Projection, typename Visitor, typename Indices, typename... Variants>
            using variant_all_visit_results = meta::_t<
                variant_all_visit_results_<Projection, Visitor, Indices, Variants...>>;

            template<typename Projection, typename Visitor, typename... Variants>
            using variant_visit_result = variant_all_same<variant_all_visit_results<
                Projection, Visitor, variant_ordinal_vectors<Variants...>, Variants...>>;

            template<typename Projection, std::size_t...Is, typename Visitor, typename... Variants>
            constexpr variant_visit_result<Projection, Visitor, Variants...>
            variant_dispatch1(meta::list<meta::size_t<Is>...>, Visitor && v, Variants &&... vs)
                noexcept(noexcept(((Visitor &&)v)(Projection{}(detail::variant_raw_get<Is>(
                    (Variants &&)vs))...)))
            {
                return ((Visitor &&)v)(Projection{}(detail::variant_raw_get<Is>(
                    (Variants &&)vs))...);
            }

            template<typename Projection, typename Indices, typename Visitor, typename... Variants>
            constexpr variant_visit_result<Projection, Visitor, Variants...>
            variant_dispatch(Visitor && v, Variants &&... vs)
                noexcept(noexcept(variant_dispatch1<Projection>(Indices{}, (Visitor &&)v, (Variants &&)vs...)))
            {
                return variant_dispatch1<Projection>(Indices{}, (Visitor &&)v, (Variants &&)vs...);
            }

            template<typename Projection, typename Visitor,
                typename IndexVectors, typename... Variants>
            struct variant_dispatch_table_;

            template<typename Projection, typename Visitor,
                typename... IndexVectors, typename... Variants>
            struct variant_dispatch_table_<
                Projection, Visitor, meta::list<IndexVectors...>, Variants...>
            {
                using dispatch_t =
                    variant_visit_result<Projection, Visitor, Variants...> (*)(Visitor &&, Variants &&...);
                static constexpr dispatch_t table[] = {
                    &variant_dispatch<Projection, IndexVectors, Visitor, Variants...>...
                };
            };

            template<typename Projection, typename Visitor,
                typename... IndexVectors, typename... Variants>
            constexpr typename variant_dispatch_table_<
                Projection, Visitor, meta::list<IndexVectors...>, Variants...>::dispatch_t
            variant_dispatch_table_<Projection, Visitor,
                meta::list<IndexVectors...>, Variants...>::table[];

            template<typename Projection, typename Visitor, typename... Variants>
            using variant_dispatch_table = variant_dispatch_table_<
                Projection, Visitor, variant_ordinal_vectors<Variants...>, Variants...>;

            struct raw_indexed_variant
            {
                template<typename T, std::size_t I>
                constexpr auto operator()(indexed_element<T, I> ie) const noexcept
                RANGES_DECLTYPE_AUTO_RETURN
                (
                    ie
                )
            };

            struct cooked_variant
            {
                template<typename T, std::size_t I>
                constexpr auto operator()(indexed_element<T, I> ie) const noexcept
                RANGES_DECLTYPE_AUTO_RETURN
                (
                    ie.get().get()
                )
            };

            struct cooked_indexed_variant
            {
                template<typename T, std::size_t I>
                constexpr auto operator()(indexed_element<T, I> ie) const noexcept
                RANGES_DECLTYPE_AUTO_RETURN
                (
                    make_indexed_element<I>(ie.get().get())
                )
            };

            template<typename Projection = raw_indexed_variant, typename Visitor, typename... Variants,
                typename Dispatch = detail::variant_dispatch_table<Projection, Visitor, Variants...>>
            constexpr detail::variant_visit_result<Projection, Visitor, Variants...>
            variant_raw_visit(Visitor && visitor, std::size_t index, Variants &&... vs)
            {
                return RANGES_EXPECT(index <= sizeof(Dispatch::table) / sizeof(Dispatch::table[0])),
                    Dispatch::table[index]((Visitor &&)visitor, (Variants &&)vs...);
            }

            template<typename...> struct variant_base;

            template<typename... Ts>
            struct variant_emplace_visitor
            {
                using V = variant_base<Ts...>;
                V& source_;
                V& target_;

                template<typename T, std::size_t I>
                RANGES_CXX14_CONSTEXPR void operator()(indexed_element<T, I> e) const
                    noexcept(noexcept(std::declval<V&>().emplace_(
                        meta::size_t<I>{}, e.get().get())))
                {
                    target_.emplace_(meta::size_t<I>{}, e.get().get());
                    source_.clear(meta::size_t<I>{});
                }
            };

            template<typename... Ts>
            struct variant_base
            {
                using index_t = variant_index_t<sizeof...(Ts)>;

                index_t index_ = static_cast<index_t>(-1);
                variant_storage<Ts...> storage_;

                variant_base() = default;
                template<std::size_t I, typename... Args>
                constexpr variant_base(meta::size_t<I>, Args &&... args)
                  : index_{I}
                  , storage_{meta::size_t<I>{}, (Args &&)args...}
                {}

                constexpr bool validate() const noexcept
                {
                    return RANGES_EXPECT(-1 <= index_ && index_ < static_cast<index_t>(sizeof...(Ts))),
                        true;
                }

                constexpr std::size_t index() const noexcept
                {
                    return validate(), static_cast<std::size_t>(index_);
                }
                constexpr bool valueless_by_exception() const noexcept
                {
                    return validate(), index_ < 0;
                }
                void clear() noexcept
                {
                    validate();
                    storage_.clear(index());
                    index_ = static_cast<index_t>(-1);
                }
                template<std::size_t I>
                void clear(meta::size_t<I>) noexcept
                {
                    RANGES_EXPECT(index() == I);
                    storage_.clear(I);
                    index_ = static_cast<index_t>(-1);
                }
                RANGES_CXX14_CONSTEXPR variant_storage<Ts...> &storage() & noexcept
                {
                    return storage_;
                }
                constexpr variant_storage<Ts...> const &storage() const& noexcept
                {
                    return storage_;
                }
                RANGES_CXX14_CONSTEXPR variant_storage<Ts...> && storage() && noexcept
                {
                    return detail::move(storage_);
                }
                constexpr variant_storage<Ts...> const && storage() const&& noexcept
                {
                    return detail::move(storage_);
                }

                template<std::size_t I, typename... Args,
                    typename T = meta::at_c<meta::list<Ts...>, I>,
                    CONCEPT_REQUIRES_(Constructible<T, Args &&...>())>
                void emplace_(meta::size_t<I> i, Args &&... args)
                    noexcept(std::is_nothrow_constructible<T, Args...>::value)
                {
                    clear();
                    storage_.emplace(i, (Args &&)args...);
                    index_ = static_cast<index_t>(I);
                }

                CONCEPT_REQUIRES(meta::strict_and<MoveConstructible<Ts>...>())
                void swap(variant_base& that)
                    noexcept(meta::strict_and<std::is_nothrow_move_constructible<Ts>...>::value)
                {
                    swap_(that, 42);
                }
            private:
                CONCEPT_REQUIRES(detail::is_trivially_copyable<variant_storage<Ts...>>::value
                    && !meta::strict_or<has_adl_swap<Ts>...>::value)
                void swap_(variant_base& that, int) noexcept
                {
                    ranges::swap(*this, that);
                }
                CONCEPT_REQUIRES(meta::strict_and<MoveConstructible<Ts>...>())
                void swap_(variant_base& that, ...)
                    noexcept(meta::strict_and<std::is_nothrow_move_constructible<Ts>...>::value)
                {
                    validate();

                    variant_base tmp;

                    if (!that.valueless_by_exception())
                    {
                        variant_raw_visit(variant_emplace_visitor<Ts...>{that, tmp},
                            that.index(), detail::move(that.storage()));
                    }

                    if (!valueless_by_exception())
                    {
                        variant_raw_visit(variant_emplace_visitor<Ts...>{*this, that},
                            index(), detail::move(storage()));
                    }

                    if (!tmp.valueless_by_exception())
                    {
                        variant_raw_visit(variant_emplace_visitor<Ts...>{tmp, *this},
                            tmp.index(), detail::move(tmp.storage()));
                    }
                }
            };

            template<bool, typename...> struct variant_destruct_base_;

            template<typename... Ts>
            using variant_destruct_base = variant_destruct_base_<meta::strict_and<
                std::is_trivially_destructible<variant_item<Ts>>...>::value, Ts...>;

            template<typename... Ts>
            struct variant_destruct_base_<true, Ts...>
              : variant_base<Ts...>
            {
                using variant_base<Ts...>::variant_base;
            };

            template<typename... Ts>
            struct variant_destruct_base_<false, Ts...>
              : variant_base<Ts...>
            {
                using variant_base<Ts...>::variant_base;

                ~variant_destruct_base_()
                {
                    this->clear();
                }

                variant_destruct_base_() = default;
                variant_destruct_base_(variant_destruct_base_ const&) = default;
                variant_destruct_base_(variant_destruct_base_ &&) = default;
                variant_destruct_base_& operator=(variant_destruct_base_ const&) & = default;
                variant_destruct_base_& operator=(variant_destruct_base_ &&) & = default;
            };

            template<typename... Ts>
            struct variant_construct_visitor
            {
                variant_storage<Ts...>& self_;

                template<typename T, std::size_t I>
                auto operator()(indexed_element<T, I> ie) const
                RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                (
                    self_.emplace(meta::size_t<I>{}, ie.get().get())
                )
            };

            template<typename... Ts>
            struct variant_assign_visitor
            {
                variant_storage<Ts...>& self_;

                template<typename T, std::size_t I>
                auto operator()(indexed_element<T, I> ie) const
                RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                (
                    void(variant_raw_get<I>(self_).get().get() = ie.get().get())
                )
            };

            template<typename... Ts>
            struct variant_two_step_visitor
            {
                variant_base<Ts...>& self_;

                template<std::size_t I, typename T,
                    typename R = uncvref_t<decltype(std::declval<T>().get())>>
                void operator()(indexed_element<T, I> ie) const
                    noexcept(std::is_nothrow_copy_constructible<R>::value &&
                        std::is_nothrow_move_constructible<R>::value)
                {
                    auto tmp = ie.get();
                    self_.clear();
                    self_.storage().emplace(meta::size_t<I>{}, detail::move(tmp).get());
                    self_.index_ = I;
                }
            };

            template<bool, typename...> struct variant_smf_construct_;

            template<typename... Ts>
            using variant_smf_construct = variant_smf_construct_<meta::strict_and<
                detail::is_trivially_copyable<variant_item<Ts>>...>::value, Ts...>;

            template<typename... Ts>
            struct variant_smf_construct_<true, Ts...>
              : variant_destruct_base<Ts...>
            {
                using variant_destruct_base<Ts...>::variant_destruct_base;
            };

            template<typename... Ts>
            struct variant_smf_construct_<false, Ts...>
              : variant_destruct_base<Ts...>
            {
                using base_t = variant_destruct_base<Ts...>;
                using base_t::base_t;

                using base_t::index_;
                using base_t::storage;

                variant_smf_construct_() = default;
                variant_smf_construct_(variant_smf_construct_ const &that)
                    noexcept(meta::strict_and<
                        std::is_nothrow_copy_constructible<Ts>...>::value)
                  : base_t{}
                {
                    if (!that.valueless_by_exception())
                    {
                        auto const i = that.index_;
                        variant_raw_visit(variant_construct_visitor<Ts...>{storage()},
                            static_cast<std::size_t>(i), that.storage());
                        index_ = i;
                    }
                }
                variant_smf_construct_(variant_smf_construct_ && that)
                    noexcept(meta::strict_and<
                        std::is_nothrow_move_constructible<Ts>...>::value)
                  : base_t{}
                {
                    if (!that.valueless_by_exception())
                    {
                        auto const i = that.index_;
                        variant_raw_visit(variant_construct_visitor<Ts...>{storage()},
                            static_cast<std::size_t>(i), detail::move(that.storage()));
                        index_ = i;
                    }
                }
                variant_smf_construct_& operator=(variant_smf_construct_ const &) & = default;
                variant_smf_construct_& operator=(variant_smf_construct_ &&) & = default;
            };

            template<bool, typename...> struct variant_smf_assign_;

            template<typename... Ts>
            using variant_smf_assign = variant_smf_assign_<meta::strict_and<
                detail::is_trivially_copyable<Ts>...>::value, Ts...>;

            template<typename... Ts>
            struct variant_smf_assign_<true, Ts...>
              : variant_smf_construct<Ts...>
            {
                using variant_smf_construct<Ts...>::variant_smf_construct;
            };

            template<typename... Ts>
            struct variant_smf_assign_<false, Ts...>
              : variant_smf_construct<Ts...>
            {
                using base_t = variant_smf_construct<Ts...>;
                using base_t::base_t;

                using base_t::clear;
                using base_t::index_;
                using base_t::storage;
                using base_t::valueless_by_exception;

                variant_smf_assign_() = default;
                variant_smf_assign_(variant_smf_assign_ const &) = default;
                variant_smf_assign_(variant_smf_assign_ &&) = default;
                variant_smf_assign_& operator=(variant_smf_assign_ const &that) &
                    noexcept(meta::strict_and<
                        std::is_nothrow_copy_constructible<Ts>...,
                        std::is_nothrow_move_constructible<Ts>...,
                        std::is_nothrow_copy_assignable<Ts>...>::value)
                {
                    auto const i = that.index_;
                    if (index_ == i)
                    {
                        if (i >= 0)
                        {
                            variant_raw_visit(variant_assign_visitor<Ts...>{storage()},
                                static_cast<std::size_t>(i), that.storage());
                        }
                    }
                    else if (i < 0)
                    {
                        clear();
                    }
                    else
                    {
                        variant_raw_visit(variant_two_step_visitor<Ts...>{*this},
                            static_cast<std::size_t>(i), that.storage());
                    }
                    return *this;
                }
                variant_smf_assign_& operator=(variant_smf_assign_ && that) &
                    noexcept(meta::strict_and<std::is_nothrow_move_constructible<Ts>...,
                        std::is_nothrow_move_assignable<Ts>...>::value)
                {
                    auto const i = that.index_;
                    if (index_ == i)
                    {
                        if (i >= 0)
                        {
                            variant_raw_visit(variant_assign_visitor<Ts...>{storage()},
                                static_cast<std::size_t>(i), detail::move(that.storage()));
                        }
                    }
                    else
                    {
                        clear();
                        if (i >= 0)
                        {
                            variant_raw_visit(variant_construct_visitor<Ts...>{storage()},
                                static_cast<std::size_t>(i), detail::move(that.storage()));
                            index_ = i;
                        }
                    }
                    return *this;
                }
            };

            struct variant_access
            {
                template<typename V,
                    CONCEPT_REQUIRES_(meta::is<uncvref_t<V>, variant>())>
                static constexpr auto storage(V && v) noexcept
                RANGES_DECLTYPE_AUTO_RETURN
                (
                    ((V &&)v).storage()
                )
            };

            template<std::size_t I, typename T, bool = std::is_void<T>::value>
            struct variant_construct_one_overload
            {
                meta::list<meta::size_t<I>, T> fun(T, meta::size_t<I> = {});
            };

            template<std::size_t I, typename T>
            struct variant_construct_one_overload<I, T, true>
            {
                void fun();
            };

            template<std::size_t I, typename...>
            struct variant_construct_overload_set
              : variant_construct_one_overload<I, void>
            {};

            template<std::size_t I, typename First, typename... Rest>
            struct variant_construct_overload_set<I, First, Rest...>
              : variant_construct_one_overload<I, First>
              , variant_construct_overload_set<I + 1, Rest...>
            {
                using variant_construct_one_overload<I, First>::fun;
                using variant_construct_overload_set<I + 1, Rest...>::fun;
            };

            template<typename... Ts>
            struct variant_swap_same_visitor
            {
                variant_storage<Ts...> &self_;

                template<typename T, std::size_t I>
                RANGES_CXX14_CONSTEXPR void operator()(indexed_element<T, I> ie) const
                {
                    ranges::swap(ie.get().get(), variant_raw_get<I>(self_).get().get());
                }
            };

            namespace variant_adl {
                struct swap_hook {};

                template<typename... Ts>
                auto swap(variant<Ts...> &x, variant<Ts...> &y)
                RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                (
                    x.swap(y)
                )
            }
        } // namespace detail

        template<typename... Ts>
        class variant
          : detail::variant_smf_assign<Ts...>
          , detail::variant_adl::swap_hook
          , smf::copy<meta::strict_and<
                std::is_copy_constructible<Ts>...>::value>
          , smf::move<meta::strict_and<
                std::is_move_constructible<Ts>...>::value>
          , smf::copy_assign<meta::strict_and<
                std::is_move_constructible<Ts>...,
                std::is_copy_constructible<Ts>...,
                std::is_copy_assignable<Ts>...>::value>
          , smf::move_assign<meta::strict_and<
                std::is_move_constructible<Ts>...,
                std::is_move_assignable<Ts>...>::value>
        {
            friend detail::variant_access;
            using base_t = detail::variant_smf_assign<Ts...>;
            using base_t::storage;
            using base_t::clear;
            using base_t::index_;
            using base_t::emplace_;

            template<typename T>
            using construct_info = decltype(
                std::declval<detail::variant_construct_overload_set<0, Ts...>>().fun(std::declval<T>()));

            CONCEPT_REQUIRES(meta::strict_and<detail::is_trivially_copyable<Ts>...>::value
                && !meta::strict_or<detail::has_adl_swap<Ts>...>::value)
            void swap_impl(variant& that, int) noexcept
            {
                base_t::swap(that);
            }
            void swap_impl(variant& that, ...)
                noexcept(meta::strict_and<
                    is_nothrow_swappable<
                        meta::_t<std::add_lvalue_reference<Ts>>,
                        meta::_t<std::add_lvalue_reference<Ts>>>...,
                    std::is_nothrow_move_constructible<Ts>...>::value)
            {
                if (index() == that.index())
                {
                    if (!that.valueless_by_exception())
                    {
                        detail::variant_raw_visit(
                            detail::variant_swap_same_visitor<Ts...>{storage()},
                            that.index(), that.storage());
                    }
                }
                else
                {
                    base_t::swap(that);
                }
            }

        public:
            template<typename First = meta::front<meta::list<Ts...>>,
                CONCEPT_REQUIRES_(DefaultConstructible<First>())>
            constexpr variant()
                noexcept(std::is_nothrow_default_constructible<First>::value)
              : base_t{meta::size_t<0>{}}
            {}

            template<typename T, typename Info = construct_info<T>,
                CONCEPT_REQUIRES_(Constructible<meta::second<Info>, T &&>())>
            constexpr variant(T && t)
                noexcept(std::is_nothrow_constructible<meta::second<Info>, T>::value)
              : base_t{meta::first<Info>{}, (T &&)t}
            {}

            template<typename T, typename... Args, typename Tail = meta::find<meta::list<Ts...>, T>,
                CONCEPT_REQUIRES_(Constructible<T, Args &&...>() &&
                    !Same<meta::list<>, Tail>() &&
                    Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>())>
            constexpr explicit variant(in_place_type_t<T>, Args &&... args)
                noexcept(std::is_nothrow_constructible<T, Args...>::value)
              : base_t{meta::size_t<sizeof...(Ts) - meta::size<Tail>::value>{}, (Args &&)args...}
            {}
            template<typename T, typename E, typename... Args,
                typename Tail = meta::find<meta::list<Ts...>, T>,
                CONCEPT_REQUIRES_(Constructible<T, std::initializer_list<E>&, Args &&...>() &&
                    !Same<meta::list<>, Tail>() &&
                    Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>())>
            constexpr explicit variant(in_place_type_t<T>, std::initializer_list<E> il, Args &&... args)
                noexcept(std::is_nothrow_constructible<T, std::initializer_list<E>&, Args...>::value)
              : base_t{meta::size_t<sizeof...(Ts) - meta::size<Tail>::value>{}, il, (Args &&)args...}
            {}

            template<std::size_t I, typename... Args, typename T = meta::at_c<meta::list<Ts...>, I>,
                CONCEPT_REQUIRES_(Constructible<T, Args &&...>())>
            constexpr explicit variant(in_place_index_t<I>, Args &&... args)
                noexcept(std::is_nothrow_constructible<T, Args...>::value)
              : base_t{meta::size_t<I>{}, (Args &&)args...}
            {}
            template<std::size_t I, typename E, typename... Args,
                typename T = meta::at_c<meta::list<Ts...>, I>,
                CONCEPT_REQUIRES_(Constructible<T, std::initializer_list<E>&, Args &&...>())>
            constexpr explicit variant(in_place_index_t<I>, std::initializer_list<E> il, Args &&... args)
                noexcept(std::is_nothrow_constructible<T, std::initializer_list<E>&, Args...>::value)
              : base_t{meta::size_t<I>{}, il, (Args &&)args...}
            {}

            template<typename T, typename Info = construct_info<T>,
                CONCEPT_REQUIRES_(!Same<variant, detail::decay_t<T>>() &&
                    Assignable<meta::second<Info>&, T &&>() &&
                    Constructible<meta::second<Info>, T &&>())>
            variant& operator=(T && t) &
                noexcept(std::is_nothrow_assignable<meta::second<Info>&, T>::value &&
                    std::is_nothrow_constructible<meta::second<Info>, T>::value)
            {
                if (meta::first<Info>::value == index())
                {
                    detail::variant_raw_get<meta::first<Info>::value>(storage()).get().get() = (T &&)t;
                }
                else
                {
                    emplace_(meta::first<Info>{}, (T &&)t);
                }
                return *this;
            }

            template<typename T, typename... Args, typename Tail = meta::find<meta::list<Ts...>, T>,
                CONCEPT_REQUIRES_(Constructible<T, Args &&...>() &&
                    !Same<meta::list<>, Tail>() &&
                    Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>())>
            void emplace(Args &&... args)
                noexcept(std::is_nothrow_constructible<T, Args...>::value)
            {
                emplace_(meta::size_t<sizeof...(Ts) - meta::size<Tail>::value>{}, (Args &&)args...);
            }
            template<typename T, typename E, typename... Args,
                typename Tail = meta::find<meta::list<Ts...>, T>,
                CONCEPT_REQUIRES_(Constructible<T, std::initializer_list<E>&, Args &&...>() &&
                    !Same<meta::list<>, Tail>() &&
                    Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>())>
            void emplace(std::initializer_list<E> il, Args &&... args)
                noexcept(std::is_nothrow_constructible<T, std::initializer_list<E>&, Args...>::value)
            {
                emplace_(meta::size_t<sizeof...(Ts) - meta::size<Tail>::value>{}, il, (Args &&)args...);
            }

            template<std::size_t I, typename... Args, typename T = meta::at_c<meta::list<Ts...>, I>,
                CONCEPT_REQUIRES_(Constructible<T, Args &&...>())>
            void emplace(Args &&... args)
                noexcept(std::is_nothrow_constructible<T, Args...>::value)
            {
                emplace_(meta::size_t<I>{}, (Args &&)args...);
            }
            template<std::size_t I, typename E, typename... Args,
                typename T = meta::at_c<meta::list<Ts...>, I>,
                CONCEPT_REQUIRES_(Constructible<T, std::initializer_list<E>&, Args &&...>())>
            void emplace(std::initializer_list<E> il, Args &&... args)
                noexcept(std::is_nothrow_constructible<T, std::initializer_list<E>&, Args...>::value)
            {
                emplace_(meta::size_t<I>{}, il, (Args &&)args...);
            }

            using base_t::valueless_by_exception;
            using base_t::index;

            CONCEPT_REQUIRES(meta::and_<
                Swappable<meta::_t<std::add_lvalue_reference<Ts>>,
                    meta::_t<std::add_lvalue_reference<Ts>>>...,
                MoveConstructible<Ts>...>())
            void swap(variant& that)
                noexcept(noexcept(std::declval<variant&>().swap_impl(that, 0)))
            {
                swap_impl(that, 0);
            }
        };

        template<>
        class variant<>
        {
            variant() = delete;
        };

        // 20.7.4, value access
        template<typename T, typename... Ts,
            typename Tail = meta::find<meta::list<Ts...>, T>>
        constexpr bool holds_alternative(variant<Ts...> const& v) noexcept
        {
            static_assert(!std::is_same<meta::list<>, Tail>::value,
                "variant has no alternative with type T");
            static_assert(std::is_same<meta::list<>, Tail>::value ||
                std::is_same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>::value,
                "variant has multiple alternatives with type T");
            return sizeof...(Ts) - meta::size<Tail>::value == v.index();
        }

        // Non-standard unchecked access
        template<std::size_t I, typename... Ts, typename T = meta::at_c<meta::list<Ts...>, I>>
        RANGES_CXX14_CONSTEXPR T &get_unchecked(variant<Ts...> &v) noexcept
        {
            return (RANGES_EXPECT(I == v.index()),
                detail::variant_raw_get<I>(detail::variant_access::storage(v)).get().get());
        }
        template<std::size_t I, typename... Ts, typename T = meta::at_c<meta::list<Ts...>, I>>
        constexpr T const &get_unchecked(variant<Ts...> const &v) noexcept
        {
            return RANGES_EXPECT(I == v.index()),
                detail::variant_raw_get<I>(detail::variant_access::storage(v)).get().get();
        }
        template<std::size_t I, typename... Ts, typename T = meta::at_c<meta::list<Ts...>, I>>
        RANGES_CXX14_CONSTEXPR T && get_unchecked(variant<Ts...> && v) noexcept
        {
            return RANGES_EXPECT(I == v.index()),
                detail::variant_raw_get<I>(
                    detail::move(detail::variant_access::storage(v))).get().get();
        }
        template<std::size_t I, typename... Ts, typename T = meta::at_c<meta::list<Ts...>, I>>
        constexpr T const && get_unchecked(variant<Ts...> const && v) noexcept
        {
            return RANGES_EXPECT(I == v.index()),
                detail::variant_raw_get<I>(
                    detail::move(detail::variant_access::storage(v))).get().get();
        }

        template<typename T, typename... Ts, typename Tail = meta::find<meta::list<Ts...>, T>>
        RANGES_CXX14_CONSTEXPR T &get_unchecked(variant<Ts...> &v) noexcept
        {
            static_assert(!Same<meta::list<>, Tail>(),
                "variant has no alternative T");
            static_assert(Same<meta::list<>, Tail>()
                || Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>(),
                "variant has multiple alternatives T");
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return get_unchecked<sizeof...(Ts) - meta::size<Tail>::value>(v);
        }
        template<typename T, typename... Ts, typename Tail = meta::find<meta::list<Ts...>, T>>
        RANGES_CXX14_CONSTEXPR T && get_unchecked(variant<Ts...> && v) noexcept
        {
            static_assert(!Same<meta::list<>, Tail>(),
                "variant has no alternative T");
            static_assert(Same<meta::list<>, Tail>()
                || Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>(),
                "variant has multiple alternatives T");
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return get_unchecked<sizeof...(Ts) - meta::size<Tail>::value>(detail::move(v));
        }
        template<typename T, typename... Ts, typename Tail = meta::find<meta::list<Ts...>, T>>
        constexpr T const &get_unchecked(variant<Ts...> const &v) noexcept
        {
            static_assert(!Same<meta::list<>, Tail>(),
                "variant has no alternative T");
            static_assert(Same<meta::list<>, Tail>()
                || Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>(),
                "variant has multiple alternatives T");
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return get_unchecked<sizeof...(Ts) - meta::size<Tail>::value>(v);
        }
        template<typename T, typename... Ts, typename Tail = meta::find<meta::list<Ts...>, T>>
        constexpr T const && get_unchecked(variant<Ts...> const && v) noexcept
        {
            static_assert(!Same<meta::list<>, Tail>(),
                "variant has no alternative T");
            static_assert(Same<meta::list<>, Tail>()
                || Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>(),
                "variant has multiple alternatives T");
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return get_unchecked<sizeof...(Ts) - meta::size<Tail>::value>(detail::move(v));
        }

        template<std::size_t I, typename... Ts, typename T = meta::at_c<meta::list<Ts...>, I>>
        RANGES_CXX14_CONSTEXPR T &get(variant<Ts...> &v)
        {
            // This is a bit tortured to avoid triggering
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64372
            return void(I != v.index() && detail::throw_bad_variant_access()),
                get_unchecked<I>(v);
        }
        template<std::size_t I, typename... Ts, typename T = meta::at_c<meta::list<Ts...>, I>>
        constexpr T const &get(variant<Ts...> const &v)
        {
            // This is a bit tortured to avoid triggering
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64372
            return void(I != v.index() && detail::throw_bad_variant_access()),
                get_unchecked<I>(v);
        }
        template<std::size_t I, typename... Ts, typename T = meta::at_c<meta::list<Ts...>, I>>
        RANGES_CXX14_CONSTEXPR T && get(variant<Ts...> && v)
        {
            // This is a bit tortured to avoid triggering
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64372
            return void(I != v.index() && detail::throw_bad_variant_access()),
                get_unchecked<I>(detail::move(v));
        }
        template<std::size_t I, typename... Ts, typename T = meta::at_c<meta::list<Ts...>, I>>
        constexpr T const && get(variant<Ts...> const && v)
        {
            // This is a bit tortured to avoid triggering
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64372
            return void(I != v.index() && detail::throw_bad_variant_access()),
                get_unchecked<I>(detail::move(v));
        }

        template<typename T, typename... Ts, typename Tail = meta::find<meta::list<Ts...>, T>>
        RANGES_CXX14_CONSTEXPR T &get(variant<Ts...> &v)
        {
            static_assert(!Same<meta::list<>, Tail>(),
                "variant has no alternative T");
            static_assert(Same<meta::list<>, Tail>()
                || Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>(),
                "variant has multiple alternatives T");
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return get<sizeof...(Ts) - meta::size<Tail>::value>(v);
        }
        template<typename T, typename... Ts, typename Tail = meta::find<meta::list<Ts...>, T>>
        RANGES_CXX14_CONSTEXPR T && get(variant<Ts...> && v)
        {
            static_assert(!Same<meta::list<>, Tail>(),
                "variant has no alternative T");
            static_assert(Same<meta::list<>, Tail>()
                || Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>(),
                "variant has multiple alternatives T");
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return get<sizeof...(Ts) - meta::size<Tail>::value>(detail::move(v));
        }
        template<typename T, typename... Ts, typename Tail = meta::find<meta::list<Ts...>, T>>
        constexpr T const &get(variant<Ts...> const &v)
        {
            static_assert(!Same<meta::list<>, Tail>(),
                "variant has no alternative T");
            static_assert(Same<meta::list<>, Tail>()
                || Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>(),
                "variant has multiple alternatives T");
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return get<sizeof...(Ts) - meta::size<Tail>::value>(v);
        }
        template<typename T, typename... Ts, typename Tail = meta::find<meta::list<Ts...>, T>>
        constexpr T const && get(variant<Ts...> const && v)
        {
            static_assert(!Same<meta::list<>, Tail>(),
                "variant has no alternative T");
            static_assert(Same<meta::list<>, Tail>()
                || Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>(),
                "variant has multiple alternatives T");
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return get<sizeof...(Ts) - meta::size<Tail>::value>(detail::move(v));
        }

        template<std::size_t I, typename... Ts, typename T = variant_alternative_t<I, variant<Ts...>>,
            CONCEPT_REQUIRES_(I < sizeof...(Ts))>
        RANGES_CXX14_CONSTEXPR meta::_t<std::add_pointer<T>>
        get_if(variant<Ts...> *v) noexcept
        {
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return v && v->index() == I ? detail::addressof(get_unchecked<I>(*v)) : nullptr;
        }
        template<std::size_t I, typename... Ts, typename T = variant_alternative_t<I, variant<Ts...>>,
            CONCEPT_REQUIRES_(I < sizeof...(Ts))>
        constexpr meta::_t<std::add_pointer<const T>>
        get_if(variant<Ts...> const *v) noexcept
        {
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return v && v->index() == I ? detail::addressof(get_unchecked<I>(*v)) : nullptr;
        }
        template<typename T, typename... Ts, typename Tail = meta::find<meta::list<Ts...>, T>>
        RANGES_CXX14_CONSTEXPR meta::_t<std::add_pointer<T>>
        get_if(variant<Ts...> *v) noexcept
        {
            static_assert(!Same<meta::list<>, Tail>(),
                "variant has no alternative T");
            static_assert(Same<meta::list<>, Tail>()
                || Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>(),
                "variant has multiple alternatives T");
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return get_if<sizeof...(Ts) - meta::size<Tail>::value>(v);
        }
        template<typename T, typename... Ts, typename Tail = meta::find<meta::list<Ts...>, T>>
        constexpr meta::_t<std::add_pointer<const T>>
        get_if(variant<Ts...> const *v) noexcept
        {
            static_assert(!Same<meta::list<>, Tail>(),
                "variant has no alternative T");
            static_assert(Same<meta::list<>, Tail>()
                || Same<meta::list<>, meta::find<meta::pop_front<Tail>, T>>(),
                "variant has multiple alternatives T");
            static_assert(!std::is_void<T>::value,
                "can't get void alternative");
            return get_if<sizeof...(Ts) - meta::size<Tail>::value>(v);
        }

        // 20.7.5, relational operators
        namespace detail
        {
            template<typename Op, typename... Ts>
            struct variant_relop_visitor
            {
                variant_storage<Ts...> const &self_;

                template<typename T, std::size_t I>
                constexpr auto operator()(indexed_element<T, I> ie) const
                RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                (
                    Op{}(variant_raw_get<I>(self_).get().get(), ie.get().get())
                )
            };
        }
        template<typename... Ts,
            CONCEPT_REQUIRES_(meta::strict_and<EqualityComparable<Ts>...>::value)>
        constexpr auto operator==(variant<Ts...> const &x, variant<Ts...> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            x.index() == y.index()
                && (x.valueless_by_exception()
                || detail::variant_raw_visit(
                    detail::variant_relop_visitor<equal_to, Ts...>{
                        detail::variant_access::storage(x)},
                    x.index(), detail::variant_access::storage(y)))
        )
        template<typename... Ts,
            CONCEPT_REQUIRES_(meta::strict_and<EqualityComparable<Ts>...>::value)>
        constexpr auto operator!=(variant<Ts...> const &x, variant<Ts...> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            x.index() != y.index()
                || (!x.valueless_by_exception()
                && detail::variant_raw_visit(
                    detail::variant_relop_visitor<not_equal_to, Ts...>{
                        detail::variant_access::storage(x)},
                    x.index(), detail::variant_access::storage(y)))
        )
        template<typename... Ts,
            CONCEPT_REQUIRES_(meta::strict_and<TotallyOrdered<Ts>...>::value)>
        constexpr auto operator<(variant<Ts...> const &x, variant<Ts...> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            !y.valueless_by_exception()
                && (x.valueless_by_exception()
                || x.index() < y.index()
                || (!(y.index() < x.index())
                && detail::variant_raw_visit(
                    detail::variant_relop_visitor<ordered_less, Ts...>{
                        detail::variant_access::storage(x)},
                    x.index(), detail::variant_access::storage(y))))
        )
        template<typename... Ts,
            CONCEPT_REQUIRES_(meta::strict_and<TotallyOrdered<Ts>...>::value)>
        constexpr auto operator>(variant<Ts...> const &x, variant<Ts...> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            y < x
        )
        template<typename... Ts,
            CONCEPT_REQUIRES_(meta::strict_and<TotallyOrdered<Ts>...>::value)>
        constexpr auto operator<=(variant<Ts...> const &x, variant<Ts...> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            !(y < x)
        )
        template<typename... Ts,
            CONCEPT_REQUIRES_(meta::strict_and<TotallyOrdered<Ts>...>::value)>
        constexpr auto operator>=(variant<Ts...> const &x, variant<Ts...> const &y)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            !(x < y)
        )

        // 20.7.6, visitation
        namespace detail
        {
            template<typename...Variants>
            using variant_total_alternatives = meta::fold<
                meta::list<meta::_t<variant_size<uncvref_t<Variants>>>...>,
                meta::size_t<1>,
                meta::quote<meta::multiplies>>;

            constexpr std::size_t variant_canonical_index()
            {
                return 0;
            }
            template<typename First, typename... Rest>
            constexpr std::size_t
            variant_canonical_index(First const &first, Rest const&... rest)
            {
                return void(first.valueless_by_exception() && detail::throw_bad_variant_access()),
                    first.index() * variant_total_alternatives<Rest...>::value
                        + variant_canonical_index(rest...);
            }
        }

        template<typename Visitor, typename... Variants,
            CONCEPT_REQUIRES_(meta::strict_and<meta::is<uncvref_t<Variants>, variant>...>::value)>
        constexpr auto visit(Visitor && visitor, Variants &&... vs)
        RANGES_DECLTYPE_AUTO_RETURN
        (
            detail::variant_raw_visit<detail::cooked_variant>((Visitor &&)visitor,
                detail::variant_canonical_index(vs...),
                detail::variant_access::storage((Variants &&)vs)...)
        )

        template<typename Visitor, typename... Variants,
            CONCEPT_REQUIRES_(meta::strict_and<meta::is<uncvref_t<Variants>, variant>...>::value)>
        constexpr auto visit_i(Visitor && visitor, Variants &&... vs)
        RANGES_DECLTYPE_AUTO_RETURN
        (
            detail::variant_raw_visit<detail::cooked_indexed_variant>((Visitor &&)visitor,
                detail::variant_canonical_index(vs...),
                detail::variant_access::storage((Variants &&)vs)...)
        )

        // 20.7.7, typename monostate
        struct monostate {};

        // 20.7.8, monostate relational operators
        constexpr bool operator<(monostate, monostate) noexcept
        {
            return false;
        }
        constexpr bool operator>(monostate, monostate) noexcept
        {
            return false;
        }
        constexpr bool operator<=(monostate, monostate) noexcept
        {
            return true;
        }
        constexpr bool operator>=(monostate, monostate) noexcept
        {
            return true;
        }
        constexpr bool operator==(monostate, monostate) noexcept
        {
            return true;
        }
        constexpr bool operator!=(monostate, monostate) noexcept
        {
            return false;
        }

        // Extension
        template<std::size_t I, typename...Ts, typename... Args>
        auto emplace(variant<Ts...> &v, Args &&... args)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            v.template emplace<I>((Args &&)args...)
        )
        template<std::size_t I, typename...Ts, typename E, typename... Args>
        auto emplace(variant<Ts...> &v, std::initializer_list<E> il, Args &&... args)
        RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
        (
            v.template emplace<I>(il, (Args &&)args...)
        )

        namespace detail
        {
            struct variant_hash_visitor
            {
                template<typename T>
                std::size_t operator()(T const& t) const
                {
                    return std::hash<T>{}(t);
                }
            };
        } // namespace detail
    } // namespace v3
} // namespace ranges

namespace std {
    // 20.7.11, hash support
    template<typename... Ts>
    struct hash< ::ranges::v3::variant<Ts...>>
    {
        size_t operator()(::ranges::v3::variant<Ts...> const& v) const
        {
            size_t seed = v.index();
            if (!v.valueless_by_exception())
            {
                size_t const value = ::ranges::v3::visit(
                    ::ranges::v3::detail::variant_hash_visitor{}, v);
                seed ^= value + static_cast<size_t>(0x9e3779b9) + (seed<<6) + (seed>>2);
            }
            return seed;
        }
    };

    template<>
    struct hash< ::ranges::v3::monostate>
    {
        size_t operator()(::ranges::v3::monostate) const noexcept
        {
            return 42;
        }
    };
} // namespace std

#endif // RANGES_V3_UTILITY_VARIANT_HPP
