#include <tuple>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/tuple.hpp>

TEST(Tuple, IsTuple) {
    ASSERT_FALSE(entt::is_tuple_v<int>);
    ASSERT_TRUE(entt::is_tuple_v<std::tuple<>>);
    ASSERT_TRUE(entt::is_tuple_v<std::tuple<int>>);
    ASSERT_TRUE((entt::is_tuple_v<std::tuple<int, char>>));
}

TEST(Tuple, UnwrapTuple) {
    auto single = std::make_tuple(42);
    auto multi = std::make_tuple(42, 'c');
    auto ref = std::forward_as_tuple(std::get<0>(single));

    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(single)), int &>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(multi)), std::tuple<int, char> &>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(ref)), int &>();

    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::move(single))), int &&>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::move(multi))), std::tuple<int, char> &&>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::move(ref))), int &>();

    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::as_const(single))), const int &>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::as_const(multi))), const std::tuple<int, char> &>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::as_const(ref))), int &>();

    ASSERT_EQ(entt::unwrap_tuple(single), 42);
    ASSERT_EQ(entt::unwrap_tuple(multi), multi);
    ASSERT_EQ(entt::unwrap_tuple(std::move(ref)), 42);
}

TEST(Tuple, ForwardApply) {
    ASSERT_EQ(entt::forward_apply{[](auto &&...args) { return sizeof...(args); }}(std::make_tuple()), 0u);
    ASSERT_EQ(entt::forward_apply{[](int i) { return i; }}(std::make_tuple(42)), 42);
    ASSERT_EQ(entt::forward_apply{[](auto... args) { return (args + ...); }}(std::make_tuple('a', 1)), 'b');
}
