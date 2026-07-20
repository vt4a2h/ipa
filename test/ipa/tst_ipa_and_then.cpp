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


TEST_CASE("Check placeholders count")
{
    STATIC_REQUIRE(ipa::detail::contains_zero_or_one_unwrapped<int, double>);
    STATIC_REQUIRE(ipa::detail::contains_zero_or_one_unwrapped<int, double,
                   ipa::detail::unwrapped_t>);
    STATIC_REQUIRE(ipa::detail::contains_zero_or_one_unwrapped<
        ipa::detail::unwrapped_t>);

    STATIC_REQUIRE_FALSE(ipa::detail::contains_zero_or_one_unwrapped<int, double,
                         ipa::detail::unwrapped_t, ipa::detail::unwrapped_t>);

    STATIC_REQUIRE_FALSE(ipa::detail::contains_zero_or_one_unwrapped<
                         ipa::detail::unwrapped_t, ipa::detail::unwrapped_t>);
}

TEST_CASE("Check placeholders count (single arg pack")
{
    STATIC_REQUIRE(ipa::detail::single_arg_is_unwrapped<ipa::detail::unwrapped_t>);
    STATIC_REQUIRE_FALSE(ipa::detail::single_arg_is_unwrapped<int>);
}

TEST_CASE("Placeholders count")
{
    STATIC_REQUIRE(ipa::detail::unwrapped_count<int, double> == 0);
    STATIC_REQUIRE(ipa::detail::unwrapped_count<int, double, ipa::detail::unwrapped_t> == 1);
    STATIC_REQUIRE(ipa::detail::unwrapped_count<int, ipa::detail::unwrapped_t, double,
                   ipa::detail::unwrapped_t> == 2);
    STATIC_REQUIRE(ipa::detail::unwrapped_count<ipa::detail::unwrapped_t> == 1);
}

TEST_CASE("Is invocable")
{
    STATIC_REQUIRE(ipa::detail::is_invocable_with_extra_args<decltype(&add), int, int, int>);
    STATIC_REQUIRE_FALSE(ipa::detail::is_invocable_with_extra_args<decltype(&strDblInt), int, int, int>);

    STATIC_REQUIRE(ipa::detail::is_invocable_with_extra_args<decltype(&strDblInt), double,
                   std::string, ipa::detail::unwrapped_t, int>);
    STATIC_REQUIRE(ipa::detail::is_invocable_with_extra_args<decltype(&strDblInt), std::string,
                   ipa::detail::unwrapped_t, double, int>);
    STATIC_REQUIRE(ipa::detail::is_invocable_with_extra_args<decltype(&strDblInt), int,
                   std::string, double, ipa::detail::unwrapped_t>);
    STATIC_REQUIRE_FALSE(ipa::detail::is_invocable_with_extra_args<decltype(&strDblInt), std::string,
                         std::string, double, ipa::detail::unwrapped_t>);
}

TEST_CASE("Invoke with extra args (double)")
{
    ipa::expected<double, std::string> e{.data = 42.};

    auto result = e.and_then(&strDblInt, std::string{"1"}, ipa::unwrapped, 1);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 43);
}

TEST_CASE("Invoke with extra args (double) (constexpr)")
{
    constexpr auto valid = []
    {
        ipa::expected<double, std::string> e{.data = 42.};

        auto result = e.and_then(&strDblInt, std::string{"1"}, ipa::unwrapped, 1);

        return result.has_value() && result.value() == 43;
    }();

    STATIC_REQUIRE(valid);
}

TEST_CASE("Invoke with extra args (int)")
{
    ipa::expected<int, std::string> e{.data = 42};

    auto result = e.and_then(&strDblInt, std::string{"1"}, 1., ipa::unwrapped);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 43);
}

TEST_CASE("Invoke with extra args (int) (constexpr)")
{
    constexpr auto valid = []
    {
        ipa::expected<int, std::string> e{.data = 42};

        auto result = e.and_then(&strDblInt, std::string{"1"}, 1., ipa::unwrapped);

        return result.has_value() && result.value() == 43;
    }();

    STATIC_REQUIRE(valid);
}

TEST_CASE("Invoke with extra args (str)")
{
    ipa::expected<std::string, std::string> e{.data = "42"};

    auto result = e.and_then(&strDblInt, ipa::unwrapped, 1., 1);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 2);
}

TEST_CASE("Invoke with extra args (str) (constexpr)")
{
    constexpr auto valid = []
    {
        ipa::expected<std::string, std::string> e{.data = "42"};

        auto result = e.and_then(&strDblInt, ipa::unwrapped, 1., 1);

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
    auto result = e.and_then(&Adder::operator(), adder, ipa::unwrapped, 1);

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

TEST_CASE("Can modify in-place (don't do this! it's a value category test)")
{
    ipa::expected<int, std::string> e{.data = 42};

    const auto modifyInPlace = [](int& v) -> ipa::expected<int, std::string>
    {
        ++v;
        return {};
    };

    std::ignore = e.and_then(modifyInPlace);

    constexpr int expectedResult = 43;

    const int actualResult = e.value();

    REQUIRE(expectedResult == actualResult);
}

TEST_CASE("Can modify in-place (don't do this! it's a value category test) (constexpr)")
{
    constexpr auto valid = []
    {
        ipa::expected<int, std::string> e{.data = 42};

        const auto modifyInPlace = [](int& v) -> ipa::expected<int, std::string>
        {
            ++v;
            return {};
        };

        std::ignore = e.and_then(modifyInPlace);

        return e.value() == 43;
    }();

    STATIC_REQUIRE(valid);
}
