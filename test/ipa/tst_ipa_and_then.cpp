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
#include <catch2/catch_test_macros.hpp>
#include <ipa/ipa.hpp>

using Result = ipa::expected<int, std::string>;

constexpr Result add(int a, int b, int c)
{
    return Result{.data = a + b + c};
}

constexpr ipa::expected<double, std::string> strDblInt(const std::string&, double d, int i)
{
    return {d + i};
}

struct Adder
{
    constexpr Result operator()(int a, int b) const
    {
        return Result{.data = a + b};
    }
};

TEST_CASE("Forward args (value)")
{
    Result e{.data = 1};

    const auto result = e.and_then(&add, 2, 3);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 6);
}

TEST_CASE("Forward args (Error)")
{
    const std::string err{"err"};

    Result e{.data = Result::data_type(std::unexpect, err)};

    const auto result = e.and_then(&add, 2, 3);

    REQUIRE(result.has_error());
    REQUIRE(result.error() == err);
}


TEST_CASE("Value placeholders count")
{
    STATIC_REQUIRE(ipa::detail::contains_zero_or_one_value_placeholders<int, double>);
    STATIC_REQUIRE(ipa::detail::contains_zero_or_one_value_placeholders<int, double,
                   ipa::placeholders::detail::value_t>);
    STATIC_REQUIRE(ipa::detail::contains_zero_or_one_value_placeholders<
        ipa::placeholders::detail::value_t>);

    STATIC_REQUIRE_FALSE(ipa::detail::contains_zero_or_one_value_placeholders<int, double,
                         ipa::placeholders::detail::value_t, ipa::placeholders::detail::value_t>);

    STATIC_REQUIRE_FALSE(ipa::detail::contains_zero_or_one_value_placeholders<
                         ipa::placeholders::detail::value_t, ipa::placeholders::detail::value_t>);
}

TEST_CASE("Is invocable")
{
    STATIC_REQUIRE(ipa::detail::is_invocable_with_extra_args<decltype(&add), int, int, int>);
    STATIC_REQUIRE_FALSE(ipa::detail::is_invocable_with_extra_args<decltype(&strDblInt), int, int, int>);

    STATIC_REQUIRE(ipa::detail::is_invocable_with_extra_args<decltype(&strDblInt), double,
                   std::string, ipa::placeholders::detail::value_t, int>);
    STATIC_REQUIRE(ipa::detail::is_invocable_with_extra_args<decltype(&strDblInt), std::string,
                   ipa::placeholders::detail::value_t, double, int>);
    STATIC_REQUIRE(ipa::detail::is_invocable_with_extra_args<decltype(&strDblInt), int,
                   std::string, double, ipa::placeholders::detail::value_t>);
    STATIC_REQUIRE_FALSE(ipa::detail::is_invocable_with_extra_args<decltype(&strDblInt), std::string,
                         std::string, double, ipa::placeholders::detail::value_t>);
}

TEST_CASE("Invoke with extra args (double)")
{
    ipa::expected<double, std::string> e{.data = 42.};

    auto result = e.and_then(&strDblInt, std::string{"1"}, ipa::placeholders::value, 1);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 43);
}

TEST_CASE("Invoke with extra args (double) (constexpr)")
{
    constexpr auto valid = []
    {
        ipa::expected<double, std::string> e{.data = 42.};

        auto result = e.and_then(&strDblInt, std::string{"1"}, ipa::placeholders::value, 1);

        return result.has_value() && result.value() == 43;
    }();

    STATIC_REQUIRE(valid);
}

TEST_CASE("Invoke with extra args (int)")
{
    ipa::expected<int, std::string> e{.data = 42};

    auto result = e.and_then(&strDblInt, std::string{"1"}, 1., ipa::placeholders::value);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 43);
}

TEST_CASE("Invoke with extra args (int) (constexpr)")
{
    constexpr auto valid = []
    {
        ipa::expected<int, std::string> e{.data = 42};

        auto result = e.and_then(&strDblInt, std::string{"1"}, 1., ipa::placeholders::value);

        return result.has_value() && result.value() == 43;
    }();

    STATIC_REQUIRE(valid);
}

TEST_CASE("Invoke with extra args (str)")
{
    ipa::expected<std::string, std::string> e{.data = "42"};

    auto result = e.and_then(&strDblInt, ipa::placeholders::value, 1., 1);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 2);
}

TEST_CASE("Invoke with extra args (str) (constexpr)")
{
    constexpr auto valid = []
    {
        ipa::expected<std::string, std::string> e{.data = "42"};

        auto result = e.and_then(&strDblInt, ipa::placeholders::value, 1., 1);

        return result.has_value() && result.value() == 2;
    }();

    STATIC_REQUIRE(valid);
}

TEST_CASE("Invoke a class method (function object)")
{
    ipa::expected<int, std::string> e{.data = 42};

    auto result = e.and_then(Adder{}, 1);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 43);
}

TEST_CASE("Invoke a class method (bind_front-like case)")
{
    ipa::expected<int, std::string> e{.data = 42};

    Adder adder;
    auto result = e.and_then(&Adder::operator(), adder, ipa::placeholders::value, 1);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 43);
}

TEST_CASE("Invoke with extra args (void)")
{
    ipa::expected<void, std::string> e;

    auto result = e.and_then(&add, 1, 1, 1);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 3);
}

TEST_CASE("Invoke with extra args (void) (constexpr)")
{
    constexpr auto valid = []
    {
        ipa::expected<void, std::string> e;

        auto result = e.and_then(&add, 1, 1, 1);

        return result.has_value() && result.value() == 3;
    }();

    STATIC_REQUIRE(valid);
}
