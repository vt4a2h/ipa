/*
* IPA -- Inline partial application for monadic operations.
 *
 * Copyright Vitaly Fanaskov 2026-present.
 *
 * Use, modification and distribution are subject to the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or a copy at http://www.boost.org/LICENSE_1_0.txt.
 *
 * Project home: https://github.com/vt4a2h/ipa.
 */
#pragma once

#include <iostream>

#include <expected>
#include <functional>
#include <utility>

namespace ipa
{
    template <class T, class E>
    struct expected;

    namespace detail
    {
        struct unwrapped_t
        {
            struct secret
            {
            };

            constexpr unwrapped_t(secret, secret)
            {
            }
        };
    }

    inline constexpr detail::unwrapped_t unwrapped(detail::unwrapped_t::secret{}, detail::unwrapped_t::secret{});

    namespace detail
    {
        template <class U>
        inline constexpr bool is_expected = false;

        template <class T, class E>
        inline constexpr bool is_expected<expected<T, E>> = true;

        template <class T>
        inline constexpr bool is_unwrapped = std::same_as<std::remove_cvref_t<T>, detail::unwrapped_t>;

        template <class... Args>
        constexpr inline std::size_t unwrapped_count =
            (std::size_t{} + ... + static_cast<std::size_t>(is_unwrapped<Args>));

        template <class... Args>
        constexpr inline bool single_arg_is_unwrapped = sizeof...(Args) == 1 && unwrapped_count<Args...> == 1;

        template <class... Args>
        constexpr inline bool contains_zero_or_one_unwrapped = unwrapped_count<Args...> <= 1;

        template <class... Args>
        inline constexpr bool contains_unwrapped = (... || is_unwrapped<Args>);

        template <class F, class T, class... Args>
        constexpr inline bool is_invocable_with_extra_args = std::is_invocable_v<F, T, Args...>;

        template <class F, class T, class... Args>
            requires(contains_unwrapped<Args...>)
        constexpr inline bool is_invocable_with_extra_args<F, T, Args...> =
            std::is_invocable_v<F, std::conditional_t<is_unwrapped<Args>, T, Args>...>;

        template <class F, class T, class... Args>
        constexpr inline bool is_nothrow_invocable_with_extra_args = std::is_nothrow_invocable_v<F, T, Args...>;

        template <class F, class T, class... Args>
            requires(contains_unwrapped<Args...>)
        constexpr inline bool is_nothrow_invocable_with_extra_args<F, T, Args...> =
            std::is_nothrow_invocable_v<F, std::conditional_t<is_unwrapped<Args>, T, Args>...>;

        template <class F, class T, class... Args>
        struct invoke_result
        {
            using type = std::invoke_result_t<F, T, Args...>;
        };

        template <class F, class T, class... Args>
            requires(contains_unwrapped<Args...>)
        struct invoke_result<F, T, Args...>
        {
            static_assert(is_invocable_with_extra_args<F, T, Args...>);

            using type = std::invoke_result_t<F, std::conditional_t<is_unwrapped<Args>, T, Args>...>;
        };

        template <class F, class T, class... Args>
        using invoke_result_t = invoke_result<F, T, Args...>::type;

        template <class F, class T, class... Args>
        [[nodiscard]] constexpr auto invoke_with_extra_args(F&& f, T&& t,
                                                            Args&&... args) noexcept(
            is_nothrow_invocable_with_extra_args<F, T, Args...>
        )
        {
            return std::invoke(f, std::forward<T>(t), std::forward<Args>(args)...);
        }

        template <class T, class Arg>
        constexpr auto&& forward(T&& t, Arg&& arg) noexcept
        {
            // We can have only 0 or 1 unwrapped argument
            if constexpr (is_unwrapped<Arg>)
            {
                return std::forward<T>(t);
            }
            else
            {
                return std::forward<Arg>(arg);
            }
        }

        template <class F, class T, class... Args>
            requires(contains_unwrapped<Args...>)
        [[nodiscard]] constexpr auto invoke_with_extra_args(F&& f, T&& t,
                                                            Args&&... args) noexcept(
            is_nothrow_invocable_with_extra_args<F, T, Args...>
        )
        {
            return std::invoke(f, forward(std::forward<T>(t), std::forward<Args>(args))...);
        }

        template <class U1, class U2>
        constexpr inline bool same_error_type = std::same_as<
            typename std::remove_cvref_t<U1>::error_type, typename std::remove_cvref_t<U2>::error_type>;
    }

    template <class T, class E>
    struct expected
    {
        using data_type = std::expected<T, E>;
        using value_type = data_type::value_type;
        using error_type = data_type::error_type;

        template <class Self, class F, class... Args>
        [[nodiscard]] constexpr auto and_then(
            this Self&& self, F&& f,
            Args&&... args) noexcept(detail::is_nothrow_invocable_with_extra_args<F, T, Args...>)
        {
            using FwdT = decltype(std::forward_like<Self>(std::declval<T>()));

            static_assert(!detail::single_arg_is_unwrapped<Args...>,
                          "The trailing argument pack of a single argument must not contain std::unwrapped.")
                ;
            static_assert(detail::contains_zero_or_one_unwrapped<Args...>,
                          "The trailing argument pack must contain zero or one std::unwrapped.");
            static_assert(detail::is_invocable_with_extra_args<F, FwdT, Args...>,
                          "The function must be invocable with a value type and all extra arguments. "
                          "Consider using std::unwrapped if an argument of a value type shouldn't go first.");

            using Ret = detail::invoke_result_t<F, FwdT, Args...>;

            static_assert(detail::is_expected<Ret>, "The function must return an expected");
            static_assert(detail::same_error_type<expected, Ret>,
                          "The function must return an expected with the same error type");

            if (self.data.has_value())
            {
                return detail::invoke_with_extra_args(f, std::forward_like<Self>(self.data).value(),
                                                      std::forward<Args>(args)...);
            }
            else
            {
                return Ret{.data = typename Ret::data_type(std::unexpect, std::forward_like<Self>(self.data).error())};
            }
        }

        [[nodiscard]] constexpr bool has_value() const noexcept { return data.has_value(); }
        [[nodiscard]] constexpr bool has_error() const noexcept { return !has_value(); }

        template <class Self>
        [[nodiscard]] constexpr auto&& value(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.data).value();
        }

        template <class Self>
        [[nodiscard]] constexpr auto&& error(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.data).error();
        }

        data_type data;
    };

    template <class E>
    struct expected<void, E>
    {
        using data_type = std::expected<void, E>;
        using value_type = data_type::value_type;
        using error_type = data_type::error_type;

        template <class Self, class F, class... Args>
        [[nodiscard]] constexpr auto and_then(
            this Self&& self, F&& f,
            Args&&... args) noexcept(std::is_nothrow_invocable_v<F, Args...>)
        {
            static_assert(!detail::contains_unwrapped<Args...>,
                          "The trailing argument pack must not contain the std::unwrapped "
                          "if value_type is void");
            static_assert(std::is_invocable_v<F, Args...>, "The function must be invocable with all extra arguments.");

            using Ret = std::invoke_result_t<F, Args...>;

            static_assert(detail::is_expected<Ret>, "The function must return an expected");
            static_assert(detail::same_error_type<expected, Ret>,
                          "The function must return an expected with the same error type");

            if (self.data.has_value())
            {
                return std::invoke(f, std::forward<Args>(args)...);
            }
            else
            {
                return Ret{.data = typename Ret::data_type(std::unexpect, std::forward_like<Self>(self.data).error())};
            }
        }

        [[nodiscard]] constexpr bool has_value() const noexcept { return data.has_value(); }
        [[nodiscard]] constexpr bool has_error() const noexcept { return !has_value(); }

        template <class Self>
        [[nodiscard]] constexpr auto&& error(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.data).error();
        }

        data_type data;
    };
} // namespace ipa
