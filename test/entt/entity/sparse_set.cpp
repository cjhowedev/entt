#include <algorithm>
#include <functional>
#include <iterator>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/sparse_set.hpp>
#include "../common/config.h"
#include "../common/throwing_allocator.hpp"

struct empty_type {};

struct boxed_int {
    int value;
};

TEST(SparseSet, Functionalities) {
    entt::sparse_set set;

    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] auto alloc = set.get_allocator());
    ASSERT_EQ(set.type(), entt::type_id<void>());

    set.reserve(42);

    ASSERT_EQ(set.capacity(), 42u);
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_TRUE(set.contiguous());
    ASSERT_EQ(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.contains(entt::entity{0}));
    ASSERT_FALSE(set.contains(entt::entity{42}));

    set.reserve(0);

    ASSERT_EQ(set.capacity(), 42u);
    ASSERT_TRUE(set.empty());

    set.push(entt::entity{42});

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1u);
    ASSERT_TRUE(set.contiguous());
    ASSERT_NE(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_NE(set.begin(), set.end());
    ASSERT_FALSE(set.contains(entt::entity{0}));
    ASSERT_TRUE(set.contains(entt::entity{42}));
    ASSERT_EQ(set.index(entt::entity{42}), 0u);
    ASSERT_EQ(set.at(0u), entt::entity{42});
    ASSERT_EQ(set.at(1u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(set[0u], entt::entity{42});
    ASSERT_EQ(set.value(entt::entity{42}), nullptr);

    set.erase(entt::entity{42});

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_TRUE(set.contiguous());
    ASSERT_EQ(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.contains(entt::entity{0}));
    ASSERT_FALSE(set.contains(entt::entity{42}));
    ASSERT_EQ(set.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(set.at(1u), static_cast<entt::entity>(entt::null));

    set.push(entt::entity{42});

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.index(entt::entity{42}), 0u);
    ASSERT_EQ(set.at(0u), entt::entity{42});
    ASSERT_EQ(set.at(1u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(set[0u], entt::entity{42});

    set.clear();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_TRUE(set.contiguous());
    ASSERT_EQ(std::as_const(set).begin(), std::as_const(set).end());
    ASSERT_EQ(set.begin(), set.end());
    ASSERT_FALSE(set.contains(entt::entity{0}));
    ASSERT_FALSE(set.contains(entt::entity{42}));

    ASSERT_NO_FATAL_FAILURE(set.bind(entt::any{}));
}

TEST(SparseSet, Contains) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::in_place};

    set.push(entt::entity{0});
    set.push(entt::entity{3});
    set.push(entt::entity{42});
    set.push(entt::entity{99});
    set.push(traits_type::construct(1, 5));

    ASSERT_FALSE(set.contains(entt::null));
    ASSERT_FALSE(set.contains(entt::tombstone));

    ASSERT_TRUE(set.contains(entt::entity{0}));
    ASSERT_TRUE(set.contains(entt::entity{3}));
    ASSERT_TRUE(set.contains(entt::entity{42}));
    ASSERT_TRUE(set.contains(entt::entity{99}));
    ASSERT_FALSE(set.contains(entt::entity{1}));
    ASSERT_TRUE(set.contains(traits_type::construct(1, 5)));

    ASSERT_TRUE(set.contains(traits_type::construct(3, 0)));
    ASSERT_FALSE(set.contains(traits_type::construct(42, 1)));
    ASSERT_FALSE(set.contains(traits_type::construct(99, traits_type::to_version(entt::tombstone))));

    set.erase(entt::entity{0});
    set.erase(entt::entity{3});

    set.remove(entt::entity{42});
    set.remove(entt::entity{99});

    ASSERT_FALSE(set.contains(entt::null));
    ASSERT_FALSE(set.contains(entt::tombstone));

    ASSERT_FALSE(set.contains(entt::entity{0}));
    ASSERT_FALSE(set.contains(entt::entity{3}));
    ASSERT_FALSE(set.contains(entt::entity{42}));
    ASSERT_FALSE(set.contains(entt::entity{99}));
    ASSERT_FALSE(set.contains(entt::entity{1}));
    ASSERT_TRUE(set.contains(traits_type::construct(1, 5)));

    ASSERT_FALSE(set.contains(traits_type::construct(99, traits_type::to_version(entt::tombstone))));
}

TEST(SparseSet, Current) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::in_place};

    ASSERT_EQ(set.current(traits_type::construct(0, 0)), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(traits_type::construct(3, 3)), traits_type::to_version(entt::tombstone));

    set.push(traits_type::construct(0, 0));
    set.push(traits_type::construct(3, 3));

    ASSERT_NE(set.current(traits_type::construct(0, 0)), traits_type::to_version(entt::tombstone));
    ASSERT_NE(set.current(traits_type::construct(3, 3)), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(traits_type::construct(3, 0)), traits_type::to_version(traits_type::construct(3, 3)));
    ASSERT_EQ(set.current(traits_type::construct(42, 1)), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(traits_type::construct(traits_type::page_size, 1)), traits_type::to_version(entt::tombstone));

    set.remove(entt::entity{0});

    ASSERT_EQ(set.current(traits_type::construct(0, 0)), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(traits_type::construct(3, 0)), traits_type::to_version(traits_type::construct(3, 3)));
}

TEST(SparseSet, Index) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{};

    set.push(traits_type::construct(0, 0));
    set.push(traits_type::construct(3, 3));

    ASSERT_EQ(set.index(traits_type::construct(0, 0)), 0u);
    ASSERT_EQ(set.index(traits_type::construct(3, 3)), 1u);

    set.erase(traits_type::construct(0, 0));

    ASSERT_EQ(set.index(traits_type::construct(3, 3)), 0u);
}

ENTT_DEBUG_TEST(SparseSetDeathTest, Index) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{};

    ASSERT_DEATH(static_cast<void>(set.index(traits_type::construct(3, 0))), "");
    ASSERT_DEATH(static_cast<void>(set.index(entt::null)), "");
}

TEST(SparseSet, Move) {
    entt::sparse_set set;
    set.push(entt::entity{42});

    ASSERT_TRUE(std::is_move_constructible_v<decltype(set)>);
    ASSERT_TRUE(std::is_move_assignable_v<decltype(set)>);

    entt::sparse_set other{std::move(set)};

    ASSERT_TRUE(set.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_EQ(set.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{42});

    set = std::move(other);

    ASSERT_FALSE(set.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_EQ(set.at(0u), entt::entity{42});
    ASSERT_EQ(other.at(0u), static_cast<entt::entity>(entt::null));

    other = entt::sparse_set{};
    other.push(entt::entity{3});
    other = std::move(set);

    ASSERT_TRUE(set.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_EQ(set.at(0u), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(other.at(0u), entt::entity{42});
}

TEST(SparseSet, Swap) {
    entt::sparse_set set;
    entt::sparse_set other{entt::deletion_policy::in_place};

    set.push(entt::entity{42});

    other.push(entt::entity{9});
    other.push(entt::entity{3});
    other.erase(entt::entity{9});

    ASSERT_EQ(set.size(), 1u);
    ASSERT_EQ(other.size(), 2u);

    set.swap(other);

    ASSERT_EQ(set.size(), 2u);
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(set.at(1u), entt::entity{3});
    ASSERT_EQ(other.at(0u), entt::entity{42});
}

TEST(SparseSet, Pagination) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{};

    ASSERT_EQ(set.extent(), 0u);

    set.push(entt::entity{traits_type::page_size - 1u});

    ASSERT_EQ(set.extent(), traits_type::page_size);
    ASSERT_TRUE(set.contains(entt::entity{traits_type::page_size - 1u}));

    set.push(entt::entity{traits_type::page_size});

    ASSERT_EQ(set.extent(), 2 * traits_type::page_size);
    ASSERT_TRUE(set.contains(entt::entity{traits_type::page_size - 1u}));
    ASSERT_TRUE(set.contains(entt::entity{traits_type::page_size}));
    ASSERT_FALSE(set.contains(entt::entity{traits_type::page_size + 1u}));

    set.erase(entt::entity{traits_type::page_size - 1u});

    ASSERT_EQ(set.extent(), 2 * traits_type::page_size);
    ASSERT_FALSE(set.contains(entt::entity{traits_type::page_size - 1u}));
    ASSERT_TRUE(set.contains(entt::entity{traits_type::page_size}));

    set.shrink_to_fit();
    set.erase(entt::entity{traits_type::page_size});

    ASSERT_EQ(set.extent(), 2 * traits_type::page_size);
    ASSERT_FALSE(set.contains(entt::entity{traits_type::page_size - 1u}));
    ASSERT_FALSE(set.contains(entt::entity{traits_type::page_size}));

    set.shrink_to_fit();

    ASSERT_EQ(set.extent(), 2 * traits_type::page_size);
}

TEST(SparseSet, Push) {
    entt::sparse_set set{entt::deletion_policy::in_place};
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    ASSERT_TRUE(set.empty());
    ASSERT_NE(set.push(entity[0u]), set.end());

    set.erase(entity[0u]);

    ASSERT_NE(set.push(entity[1u]), set.end());
    ASSERT_NE(set.push(entity[0u]), set.end());

    ASSERT_EQ(set.at(0u), entity[1u]);
    ASSERT_EQ(set.at(1u), entity[0u]);
    ASSERT_EQ(set.index(entity[0u]), 1u);
    ASSERT_EQ(set.index(entity[1u]), 0u);

    set.erase(std::begin(entity), std::end(entity));

    ASSERT_NE(set.push(entity[1u]), set.end());
    ASSERT_NE(set.push(entity[0u]), set.end());

    ASSERT_EQ(set.at(0u), entity[1u]);
    ASSERT_EQ(set.at(1u), entity[0u]);
    ASSERT_EQ(set.index(entity[0u]), 1u);
    ASSERT_EQ(set.index(entity[1u]), 0u);
}

TEST(SparseSet, PushRange) {
    entt::sparse_set set{entt::deletion_policy::in_place};
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    set.push(entt::entity{12});

    ASSERT_EQ(set.push(std::end(entity), std::end(entity)), set.end());
    ASSERT_NE(set.push(std::begin(entity), std::end(entity)), set.end());

    set.push(entt::entity{24});

    ASSERT_TRUE(set.contains(entity[0u]));
    ASSERT_TRUE(set.contains(entity[1u]));
    ASSERT_FALSE(set.contains(entt::entity{0}));
    ASSERT_FALSE(set.contains(entt::entity{9}));
    ASSERT_TRUE(set.contains(entt::entity{12}));
    ASSERT_TRUE(set.contains(entt::entity{24}));

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 4u);
    ASSERT_EQ(set.index(entt::entity{12}), 0u);
    ASSERT_EQ(set.index(entity[0u]), 1u);
    ASSERT_EQ(set.index(entity[1u]), 2u);
    ASSERT_EQ(set.index(entt::entity{24}), 3u);
    ASSERT_EQ(set.data()[set.index(entt::entity{12})], entt::entity{12});
    ASSERT_EQ(set.data()[set.index(entity[0u])], entity[0u]);
    ASSERT_EQ(set.data()[set.index(entity[1u])], entity[1u]);
    ASSERT_EQ(set.data()[set.index(entt::entity{24})], entt::entity{24});

    set.erase(std::begin(entity), std::end(entity));

    ASSERT_NE(set.push(std::rbegin(entity), std::rend(entity)), set.end());

    ASSERT_EQ(set.size(), 6u);
    ASSERT_EQ(set.at(4u), entity[1u]);
    ASSERT_EQ(set.at(5u), entity[0u]);
    ASSERT_EQ(set.index(entity[0u]), 5u);
    ASSERT_EQ(set.index(entity[1u]), 4u);
}

ENTT_DEBUG_TEST(SparseSetDeathTest, Push) {
    entt::sparse_set set{entt::deletion_policy::in_place};
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};
    set.push(entt::entity{42});

    ASSERT_DEATH(set.push(entt::entity{42}), "");
    ASSERT_DEATH(set.push(std::begin(entity), std::end(entity)), "");
}

TEST(SparseSet, PushOutOfBounds) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::in_place};
    entt::entity entity[2u]{entt::entity{0}, entt::entity{traits_type::page_size}};

    ASSERT_NE(set.push(entity[0u]), set.end());
    ASSERT_EQ(set.extent(), traits_type::page_size);
    ASSERT_EQ(set.index(entity[0u]), 0u);

    set.erase(entity[0u]);

    ASSERT_NE(set.push(entity[1u]), set.end());
    ASSERT_EQ(set.extent(), 2u * traits_type::page_size);
    ASSERT_EQ(set.index(entity[1u]), 0u);
}

TEST(SparseSet, Bump) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set;
    entt::entity entity[3u]{entt::entity{3}, entt::entity{42}, traits_type::construct(9, 3)};
    set.push(std::begin(entity), std::end(entity));

    ASSERT_EQ(set.current(entity[0u]), 0u);
    ASSERT_EQ(set.current(entity[1u]), 0u);
    ASSERT_EQ(set.current(entity[2u]), 3u);

    ASSERT_EQ(set.bump(entity[0u]), 0u);
    ASSERT_EQ(set.bump(traits_type::construct(traits_type::to_entity(entity[1u]), 1)), 1u);
    ASSERT_EQ(set.bump(traits_type::construct(traits_type::to_entity(entity[2u]), 0)), 0u);

    ASSERT_EQ(set.current(entity[0u]), 0u);
    ASSERT_EQ(set.current(entity[1u]), 1u);
    ASSERT_EQ(set.current(entity[2u]), 0u);
}

ENTT_DEBUG_TEST(SparseSetDeathTest, Bump) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::in_place};
    set.push(entt::entity{3});

    ASSERT_DEATH(set.bump(entt::null), "");
    ASSERT_DEATH(set.bump(entt::tombstone), "");
    ASSERT_DEATH(set.bump(entt::entity{42}), "");
    ASSERT_DEATH(set.bump(traits_type::construct(traits_type::to_entity(entt::entity{3}), traits_type::to_version(entt::tombstone))), "");
}

TEST(SparseSet, Erase) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set;
    entt::entity entity[3u]{entt::entity{3}, entt::entity{42}, traits_type::construct(9, 3)};

    ASSERT_EQ(set.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_TRUE(set.empty());

    set.push(std::begin(entity), std::end(entity));
    set.erase(set.begin(), set.end());

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

    set.push(std::begin(entity), std::end(entity));
    set.erase(entity, entity + 2u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_EQ(*set.begin(), entity[2u]);

    set.erase(entity[2u]);

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

    set.push(std::begin(entity), std::end(entity));
    std::swap(entity[1u], entity[2u]);
    set.erase(entity, entity + 2u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_EQ(*set.begin(), entity[2u]);
}

ENTT_DEBUG_TEST(SparseSetDeathTest, Erase) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set;
    entt::entity entity[2u]{entt::entity{42}, traits_type::construct(9, 3)};

    ASSERT_DEATH(set.erase(std::begin(entity), std::end(entity)), "");
    ASSERT_DEATH(set.erase(entt::null), "");
}

TEST(SparseSet, CrossErase) {
    entt::sparse_set set;
    entt::sparse_set other;
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    set.push(std::begin(entity), std::end(entity));
    other.push(entity[1u]);
    set.erase(other.begin(), other.end());

    ASSERT_TRUE(set.contains(entity[0u]));
    ASSERT_FALSE(set.contains(entity[1u]));
    ASSERT_EQ(set.data()[0u], entity[0u]);
}

TEST(SparseSet, StableErase) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::in_place};
    entt::entity entity[3u]{entt::entity{3}, entt::entity{42}, traits_type::construct(9, 3)};

    ASSERT_EQ(set.policy(), entt::deletion_policy::in_place);
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);

    set.push(entity[0u]);
    set.push(entity[1u]);
    set.push(entity[2u]);

    set.erase(set.begin(), set.end());

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(set.free_list(), 0u);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));
    ASSERT_TRUE(set.at(0u) == entt::tombstone);
    ASSERT_TRUE(set.at(1u) == entt::tombstone);
    ASSERT_TRUE(set.at(2u) == entt::tombstone);

    set.push(entity[0u]);
    set.push(entity[1u]);
    set.push(entity[2u]);

    set.erase(entity, entity + 2u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(set.free_list(), 1u);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_EQ(*set.begin(), entity[2u]);
    ASSERT_TRUE(set.at(0u) == entt::tombstone);
    ASSERT_TRUE(set.at(1u) == entt::tombstone);

    set.erase(entity[2u]);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(set.free_list(), 2u);
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

    set.push(entity[0u]);
    set.push(entity[1u]);
    set.push(entity[2u]);

    std::swap(entity[1u], entity[2u]);
    set.erase(entity, entity + 2u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(set.free_list(), 0u);
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_TRUE(set.at(0u) == entt::tombstone);
    ASSERT_EQ(set.at(1u), entity[2u]);
    ASSERT_TRUE(set.at(2u) == entt::tombstone);
    ASSERT_EQ(*++set.begin(), entity[2u]);

    set.compact();

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1u);
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_TRUE(set.at(0u) == entity[2u]);
    ASSERT_EQ(*set.begin(), entity[2u]);

    set.clear();

    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

    set.push(entity[0u]);
    set.push(entity[1u]);
    set.push(entity[2u]);

    set.erase(entity[2u]);

    ASSERT_NE(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_NE(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

    set.erase(entity[0u]);
    set.erase(entity[1u]);

    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(set.free_list(), 1u);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));
    ASSERT_TRUE(*set.begin() == entt::tombstone);

    set.push(entity[0u]);

    ASSERT_EQ(*++set.begin(), entity[0u]);

    set.push(entity[1u]);
    set.push(entity[2u]);
    set.push(entt::entity{0});

    ASSERT_EQ(set.size(), 4u);
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(*set.begin(), entt::entity{0});
    ASSERT_EQ(set.at(0u), entity[1u]);
    ASSERT_EQ(set.at(1u), entity[0u]);
    ASSERT_EQ(set.at(2u), entity[2u]);

    ASSERT_NE(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_NE(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_NE(set.current(entity[2u]), traits_type::to_version(entt::tombstone));
}

ENTT_DEBUG_TEST(SparseSetDeathTest, StableErase) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::in_place};
    entt::entity entity[2u]{entt::entity{42}, traits_type::construct(9, 3)};

    ASSERT_DEATH(set.erase(std::begin(entity), std::end(entity)), "");
    ASSERT_DEATH(set.erase(entt::null), "");
}

TEST(SparseSet, CrossStableErase) {
    entt::sparse_set set{entt::deletion_policy::in_place};
    entt::sparse_set other{entt::deletion_policy::in_place};
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    set.push(std::begin(entity), std::end(entity));
    other.push(entity[1u]);
    set.erase(other.begin(), other.end());

    ASSERT_TRUE(set.contains(entity[0u]));
    ASSERT_FALSE(set.contains(entity[1u]));
    ASSERT_EQ(set.data()[0u], entity[0u]);
}

TEST(SparseSet, SwapOnlyErase) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::swap_only};
    entt::entity entity[3u]{entt::entity{3}, entt::entity{42}, traits_type::construct(9, 3)};

    ASSERT_EQ(set.policy(), entt::deletion_policy::swap_only);
    ASSERT_EQ(set.free_list(), 0u);
    ASSERT_TRUE(set.empty());

    set.push(std::begin(entity), std::end(entity));
    set.erase(set.begin(), set.end());

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), 0u);

    entity[0u] = traits_type::next(entity[0u]);
    entity[1u] = traits_type::next(entity[1u]);
    entity[2u] = traits_type::next(entity[2u]);

    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entity[0u]));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entity[1u]));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));

    set.push(std::begin(entity), std::end(entity));
    set.erase(entity, entity + 2u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), 1u);

    entity[0u] = traits_type::next(entity[0u]);
    entity[1u] = traits_type::next(entity[1u]);

    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entity[0u]));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entity[1u]));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_EQ(*set.begin(), entity[0u]);

    set.erase(entity[2u]);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), 0u);

    entity[2u] = traits_type::next(entity[2u]);

    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));

    set.push(std::begin(entity), std::end(entity));
    std::swap(entity[1u], entity[2u]);
    set.erase(entity, entity + 2u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), 1);

    entity[0u] = traits_type::next(entity[0u]);
    entity[1u] = traits_type::next(entity[1u]);

    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_EQ(*set.begin(), entity[0u]);
}

ENTT_DEBUG_TEST(SparseSetDeathTest, SwapOnlyErase) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::swap_only};
    entt::entity entity[2u]{entt::entity{42}, traits_type::construct(9, 3)};

    ASSERT_DEATH(set.erase(std::begin(entity), std::end(entity)), "");
    ASSERT_DEATH(set.erase(entt::null), "");
}

TEST(SparseSet, CrossSwapOnlyErase) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::swap_only};
    entt::sparse_set other{entt::deletion_policy::swap_only};
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    set.push(std::begin(entity), std::end(entity));
    other.push(entity[1u]);
    set.erase(other.begin(), other.end());
    entity[1u] = traits_type::next(entity[1u]);

    ASSERT_TRUE(set.contains(entity[0u]));
    ASSERT_TRUE(set.contains(entity[1u]));
    ASSERT_EQ(set.data()[0u], entity[0u]);
    ASSERT_EQ(set.data()[1u], entity[1u]);
}

TEST(SparseSet, Remove) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set;
    entt::entity entity[3u]{entt::entity{3}, entt::entity{42}, traits_type::construct(9, 3)};

    ASSERT_EQ(set.policy(), entt::deletion_policy::swap_and_pop);
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_TRUE(set.empty());

    ASSERT_EQ(set.remove(std::begin(entity), std::end(entity)), 0u);
    ASSERT_FALSE(set.remove(entity[1u]));

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);

    set.push(std::begin(entity), std::end(entity));

    ASSERT_EQ(set.remove(set.begin(), set.end()), 3u);
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

    set.push(std::begin(entity), std::end(entity));

    ASSERT_EQ(set.remove(entity, entity + 2u), 2u);
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_EQ(*set.begin(), entity[2u]);

    ASSERT_TRUE(set.remove(entity[2u]));
    ASSERT_FALSE(set.remove(entity[2u]));
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

    set.push(entity, entity + 2u);

    ASSERT_EQ(set.remove(std::begin(entity), std::end(entity)), 2u);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);

    set.push(std::begin(entity), std::end(entity));
    std::swap(entity[1u], entity[2u]);

    ASSERT_EQ(set.remove(entity, entity + 2u), 2u);
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(*set.begin(), entity[2u]);

    ASSERT_FALSE(set.remove(traits_type::construct(9, 0)));
    ASSERT_FALSE(set.remove(entt::tombstone));
    ASSERT_FALSE(set.remove(entt::null));
}

TEST(SparseSet, CrossRemove) {
    entt::sparse_set set;
    entt::sparse_set other;
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    set.push(std::begin(entity), std::end(entity));
    other.push(entity[1u]);
    set.remove(other.begin(), other.end());

    ASSERT_TRUE(set.contains(entity[0u]));
    ASSERT_FALSE(set.contains(entity[1u]));
    ASSERT_EQ(set.data()[0u], entity[0u]);
}

TEST(SparseSet, StableRemove) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::in_place};
    entt::entity entity[3u]{entt::entity{3}, entt::entity{42}, traits_type::construct(9, 3)};

    ASSERT_EQ(set.policy(), entt::deletion_policy::in_place);
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);

    ASSERT_EQ(set.remove(std::begin(entity), std::end(entity)), 0u);
    ASSERT_FALSE(set.remove(entity[1u]));

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);

    set.push(entity[0u]);
    set.push(entity[1u]);
    set.push(entity[2u]);

    ASSERT_EQ(set.remove(set.begin(), set.end()), 3u);
    ASSERT_EQ(set.remove(set.begin(), set.end()), 0u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(set.free_list(), 0u);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));
    ASSERT_TRUE(set.at(0u) == entt::tombstone);
    ASSERT_TRUE(set.at(1u) == entt::tombstone);
    ASSERT_TRUE(set.at(2u) == entt::tombstone);

    set.push(entity[0u]);
    set.push(entity[1u]);
    set.push(entity[2u]);

    ASSERT_EQ(set.remove(entity, entity + 2u), 2u);
    ASSERT_EQ(set.remove(entity, entity + 2u), 0u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(set.free_list(), 1u);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_EQ(*set.begin(), entity[2u]);
    ASSERT_TRUE(set.at(0u) == entt::tombstone);
    ASSERT_TRUE(set.at(1u) == entt::tombstone);

    ASSERT_TRUE(set.remove(entity[2u]));
    ASSERT_FALSE(set.remove(entity[2u]));

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(set.free_list(), 2u);
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

    set.push(entity[0u]);
    set.push(entity[1u]);
    set.push(entity[2u]);

    std::swap(entity[1u], entity[2u]);

    ASSERT_EQ(set.remove(entity, entity + 2u), 2u);
    ASSERT_EQ(set.remove(entity, entity + 2u), 0u);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(set.free_list(), 0u);
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_TRUE(set.at(0u) == entt::tombstone);
    ASSERT_EQ(set.at(1u), entity[2u]);
    ASSERT_TRUE(set.at(2u) == entt::tombstone);
    ASSERT_EQ(*++set.begin(), entity[2u]);

    set.compact();

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1u);
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_TRUE(set.at(0u) == entity[2u]);
    ASSERT_EQ(*set.begin(), entity[2u]);

    set.clear();

    ASSERT_EQ(set.size(), 0u);
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

    set.push(entity[0u]);
    set.push(entity[1u]);
    set.push(entity[2u]);

    ASSERT_TRUE(set.remove(entity[2u]));
    ASSERT_FALSE(set.remove(entity[2u]));

    ASSERT_NE(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_NE(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

    ASSERT_TRUE(set.remove(entity[0u]));
    ASSERT_TRUE(set.remove(entity[1u]));
    ASSERT_EQ(set.remove(entity, entity + 2u), 0u);

    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(set.free_list(), 1u);
    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));
    ASSERT_TRUE(*set.begin() == entt::tombstone);

    set.push(entity[0u]);

    ASSERT_EQ(*++set.begin(), entity[0u]);

    set.push(entity[1u]);
    set.push(entity[2u]);
    set.push(entt::entity{0});

    ASSERT_EQ(set.size(), 4u);
    ASSERT_EQ(set.free_list(), traits_type::entity_mask);
    ASSERT_EQ(*set.begin(), entt::entity{0});
    ASSERT_EQ(set.at(0u), entity[1u]);
    ASSERT_EQ(set.at(1u), entity[0u]);
    ASSERT_EQ(set.at(2u), entity[2u]);

    ASSERT_NE(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
    ASSERT_NE(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
    ASSERT_NE(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

    ASSERT_FALSE(set.remove(traits_type::construct(9, 0)));
    ASSERT_FALSE(set.remove(entt::null));
}

TEST(SparseSet, CrossStableRemove) {
    entt::sparse_set set{entt::deletion_policy::in_place};
    entt::sparse_set other{entt::deletion_policy::in_place};
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    set.push(std::begin(entity), std::end(entity));
    other.push(entity[1u]);
    set.remove(other.begin(), other.end());

    ASSERT_TRUE(set.contains(entity[0u]));
    ASSERT_FALSE(set.contains(entity[1u]));
    ASSERT_EQ(set.data()[0u], entity[0u]);
}

TEST(SparseSet, SwapOnlyRemove) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::swap_only};
    entt::entity entity[3u]{entt::entity{3}, entt::entity{42}, traits_type::construct(9, 3)};

    ASSERT_EQ(set.policy(), entt::deletion_policy::swap_only);
    ASSERT_EQ(set.free_list(), 0u);
    ASSERT_TRUE(set.empty());

    ASSERT_EQ(set.remove(std::begin(entity), std::end(entity)), 0u);
    ASSERT_FALSE(set.remove(entity[1u]));

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.free_list(), 0u);

    set.push(std::begin(entity), std::end(entity));

    ASSERT_EQ(set.remove(set.begin(), set.end()), 3u);
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), 0u);

    entity[0u] = traits_type::next(entity[0u]);
    entity[1u] = traits_type::next(entity[1u]);
    entity[2u] = traits_type::next(entity[2u]);

    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entity[0u]));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entity[1u]));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));

    set.push(std::begin(entity), std::end(entity));

    ASSERT_EQ(set.remove(entity, entity + 2u), 2u);
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), 1u);

    entity[0u] = traits_type::next(entity[0u]);
    entity[1u] = traits_type::next(entity[1u]);

    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entity[0u]));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entity[1u]));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_EQ(*set.begin(), entity[0u]);

    ASSERT_TRUE(set.remove(entity[2u]));
    ASSERT_FALSE(set.remove(entity[2u]));

    entity[2u] = traits_type::next(entity[2u]);

    ASSERT_TRUE(set.remove(entity[2u]));
    ASSERT_FALSE(set.remove(entity[2u]));
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), 0u);
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(traits_type::next(entity[2u])));

    set.push(entity, entity + 2u);

    ASSERT_EQ(set.remove(std::begin(entity), std::end(entity)), 2u);

    entity[0u] = traits_type::next(entity[0u]);
    entity[1u] = traits_type::next(entity[1u]);
    entity[2u] = traits_type::next(entity[2u]);

    ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entity[0u]));
    ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entity[1u]));
    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), 0u);

    set.push(std::begin(entity), std::end(entity));
    std::swap(entity[1u], entity[2u]);

    ASSERT_EQ(set.remove(entity, entity + 2u), 2u);

    entity[0u] = traits_type::next(entity[0u]);
    entity[1u] = traits_type::next(entity[1u]);

    ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entity[2u]));
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.free_list(), 1u);
    ASSERT_EQ(*set.begin(), entity[0u]);

    ASSERT_FALSE(set.remove(traits_type::construct(9, 0)));
    ASSERT_FALSE(set.remove(entt::tombstone));
    ASSERT_FALSE(set.remove(entt::null));
}

TEST(SparseSet, CrossSwapOnlyRemove) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set{entt::deletion_policy::swap_only};
    entt::sparse_set other{entt::deletion_policy::swap_only};
    entt::entity entity[2u]{entt::entity{3}, entt::entity{42}};

    set.push(std::begin(entity), std::end(entity));
    other.push(entity[1u]);
    set.remove(other.begin(), other.end());
    entity[1u] = traits_type::next(entity[1u]);

    ASSERT_TRUE(set.contains(entity[0u]));
    ASSERT_TRUE(set.contains(entity[1u]));
    ASSERT_EQ(set.data()[0u], entity[0u]);
    ASSERT_EQ(set.data()[1u], entity[1u]);
}

TEST(SparseSet, Compact) {
    entt::sparse_set set{entt::deletion_policy::in_place};

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);

    set.compact();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);

    set.push(entt::entity{0});
    set.compact();

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1u);

    set.push(entt::entity{42});
    set.erase(entt::entity{0});

    ASSERT_EQ(set.size(), 2u);
    ASSERT_EQ(set.index(entt::entity{42}), 1u);

    set.compact();

    ASSERT_EQ(set.size(), 1u);
    ASSERT_EQ(set.index(entt::entity{42}), 0u);

    set.push(entt::entity{0});
    set.compact();

    ASSERT_EQ(set.size(), 2u);
    ASSERT_EQ(set.index(entt::entity{42}), 0u);
    ASSERT_EQ(set.index(entt::entity{0}), 1u);

    set.erase(entt::entity{0});
    set.erase(entt::entity{42});
    set.compact();

    ASSERT_TRUE(set.empty());
}

TEST(SparseSet, SwapEntity) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set;

    set.push(traits_type::construct(3, 5));
    set.push(traits_type::construct(42, 99));

    ASSERT_EQ(set.index(traits_type::construct(3, 5)), 0u);
    ASSERT_EQ(set.index(traits_type::construct(42, 99)), 1u);

    set.swap_elements(traits_type::construct(3, 5), traits_type::construct(42, 99));

    ASSERT_EQ(set.index(traits_type::construct(3, 5)), 1u);
    ASSERT_EQ(set.index(traits_type::construct(42, 99)), 0u);

    set.swap_elements(traits_type::construct(3, 5), traits_type::construct(42, 99));

    ASSERT_EQ(set.index(traits_type::construct(3, 5)), 0u);
    ASSERT_EQ(set.index(traits_type::construct(42, 99)), 1u);
}

ENTT_DEBUG_TEST(SparseSetDeathTest, SwapEntity) {
    entt::sparse_set set;

    ASSERT_TRUE(set.empty());
    ASSERT_DEATH(set.swap_elements(entt::entity{0}, entt::entity{1}), "");
}

TEST(SparseSet, Clear) {
    entt::sparse_set set{entt::deletion_policy::in_place};

    set.push(entt::entity{3});
    set.push(entt::entity{42});
    set.push(entt::entity{9});
    set.erase(entt::entity{42});

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(*set.begin(), entt::entity{9});

    set.clear();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);

    ASSERT_EQ(set.find(entt::entity{3}), set.end());
    ASSERT_EQ(set.find(entt::entity{9}), set.end());

    set.push(entt::entity{3});
    set.push(entt::entity{42});
    set.push(entt::entity{9});

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 3u);
    ASSERT_EQ(*set.begin(), entt::entity{9});

    set.clear();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0u);

    ASSERT_EQ(set.find(entt::entity{3}), set.end());
    ASSERT_EQ(set.find(entt::entity{42}), set.end());
    ASSERT_EQ(set.find(entt::entity{9}), set.end());
}

TEST(SparseSet, Contiguous) {
    entt::sparse_set swap_and_pop{entt::deletion_policy::swap_and_pop};
    entt::sparse_set in_place{entt::deletion_policy::in_place};

    const entt::entity entity{42};
    const entt::entity other{3};

    ASSERT_TRUE(swap_and_pop.contiguous());
    ASSERT_TRUE(in_place.contiguous());

    swap_and_pop.push(entity);
    in_place.push(entity);

    swap_and_pop.push(other);
    in_place.push(other);

    ASSERT_TRUE(swap_and_pop.contiguous());
    ASSERT_TRUE(in_place.contiguous());

    swap_and_pop.erase(entity);
    in_place.erase(entity);

    ASSERT_TRUE(swap_and_pop.contiguous());
    ASSERT_FALSE(in_place.contiguous());

    swap_and_pop.push(entity);
    in_place.push(entity);

    ASSERT_TRUE(swap_and_pop.contiguous());
    ASSERT_TRUE(in_place.contiguous());

    swap_and_pop.erase(other);
    in_place.erase(other);

    ASSERT_TRUE(swap_and_pop.contiguous());
    ASSERT_FALSE(in_place.contiguous());

    in_place.compact();

    ASSERT_TRUE(swap_and_pop.contiguous());
    ASSERT_TRUE(in_place.contiguous());

    swap_and_pop.push(other);
    in_place.push(other);

    swap_and_pop.erase(entity);
    in_place.erase(entity);

    ASSERT_TRUE(swap_and_pop.contiguous());
    ASSERT_FALSE(in_place.contiguous());

    swap_and_pop.clear();
    in_place.clear();

    ASSERT_TRUE(swap_and_pop.contiguous());
    ASSERT_TRUE(in_place.contiguous());
}

TEST(SparseSet, Iterator) {
    using iterator = typename entt::sparse_set::iterator;

    testing::StaticAssertTypeEq<iterator::value_type, entt::entity>();
    testing::StaticAssertTypeEq<iterator::pointer, const entt::entity *>();
    testing::StaticAssertTypeEq<iterator::reference, const entt::entity &>();

    entt::sparse_set set;
    set.push(entt::entity{3});

    iterator end{set.begin()};
    iterator begin{};

    ASSERT_EQ(end.data(), set.data());
    ASSERT_EQ(begin.data(), nullptr);

    begin = set.end();
    std::swap(begin, end);

    ASSERT_EQ(end.data(), set.data());
    ASSERT_EQ(begin.data(), set.data());

    ASSERT_EQ(begin, set.cbegin());
    ASSERT_EQ(end, set.cend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.index(), 0);
    ASSERT_EQ(end.index(), -1);

    ASSERT_EQ(begin++, set.begin());
    ASSERT_EQ(begin--, set.end());

    ASSERT_EQ(begin + 1, set.end());
    ASSERT_EQ(end - 1, set.begin());

    ASSERT_EQ(++begin, set.end());
    ASSERT_EQ(--begin, set.begin());

    ASSERT_EQ(begin += 1, set.end());
    ASSERT_EQ(begin -= 1, set.begin());

    ASSERT_EQ(begin + (end - begin), set.end());
    ASSERT_EQ(begin - (begin - end), set.end());

    ASSERT_EQ(end - (end - begin), set.begin());
    ASSERT_EQ(end + (begin - end), set.begin());

    ASSERT_EQ(begin[0u], *set.begin());

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, set.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, set.end());

    ASSERT_EQ(*begin, entt::entity{3});
    ASSERT_EQ(*begin.operator->(), entt::entity{3});

    ASSERT_EQ(begin.index(), 0);
    ASSERT_EQ(end.index(), -1);

    set.push(entt::entity{42});
    begin = set.begin();

    ASSERT_EQ(begin.index(), 1);
    ASSERT_EQ(end.index(), -1);

    ASSERT_EQ(begin[0u], entt::entity{42});
    ASSERT_EQ(begin[1u], entt::entity{3});
}

TEST(SparseSet, ReverseIterator) {
    using reverse_iterator = typename entt::sparse_set::reverse_iterator;

    testing::StaticAssertTypeEq<reverse_iterator::value_type, entt::entity>();
    testing::StaticAssertTypeEq<reverse_iterator::pointer, const entt::entity *>();
    testing::StaticAssertTypeEq<reverse_iterator::reference, const entt::entity &>();

    entt::sparse_set set;
    set.push(entt::entity{3});

    reverse_iterator end{set.rbegin()};
    reverse_iterator begin{};
    begin = set.rend();
    std::swap(begin, end);

    ASSERT_EQ(begin, set.crbegin());
    ASSERT_EQ(end, set.crend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base().index(), -1);
    ASSERT_EQ(end.base().index(), 0);

    ASSERT_EQ(begin++, set.rbegin());
    ASSERT_EQ(begin--, set.rend());

    ASSERT_EQ(begin + 1, set.rend());
    ASSERT_EQ(end - 1, set.rbegin());

    ASSERT_EQ(++begin, set.rend());
    ASSERT_EQ(--begin, set.rbegin());

    ASSERT_EQ(begin += 1, set.rend());
    ASSERT_EQ(begin -= 1, set.rbegin());

    ASSERT_EQ(begin + (end - begin), set.rend());
    ASSERT_EQ(begin - (begin - end), set.rend());

    ASSERT_EQ(end - (end - begin), set.rbegin());
    ASSERT_EQ(end + (begin - end), set.rbegin());

    ASSERT_EQ(begin[0u], *set.rbegin());

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, set.rbegin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, set.rend());

    ASSERT_EQ(*begin, entt::entity{3});
    ASSERT_EQ(*begin.operator->(), entt::entity{3});

    ASSERT_EQ(begin.base().index(), -1);
    ASSERT_EQ(end.base().index(), 0);

    set.push(entt::entity{42});
    end = set.rend();

    ASSERT_EQ(begin.base().index(), -1);
    ASSERT_EQ(end.base().index(), 1);

    ASSERT_EQ(begin[0u], entt::entity{3});
    ASSERT_EQ(begin[1u], entt::entity{42});
}

TEST(SparseSet, Find) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set set;
    set.push(entt::entity{3});
    set.push(entt::entity{42});
    set.push(traits_type::construct(99, 1));

    ASSERT_NE(set.find(entt::entity{3}), set.end());
    ASSERT_NE(set.find(entt::entity{42}), set.end());
    ASSERT_NE(set.find(traits_type::construct(99, 1)), set.end());
    ASSERT_EQ(set.find(traits_type::construct(99, 5)), set.end());
    ASSERT_EQ(set.find(entt::entity{0}), set.end());
    ASSERT_EQ(set.find(entt::tombstone), set.end());
    ASSERT_EQ(set.find(entt::null), set.end());

    auto it = set.find(traits_type::construct(99, 1));

    ASSERT_EQ(*it, traits_type::construct(99, 1));
    ASSERT_EQ(*(++it), entt::entity{42});
    ASSERT_EQ(*(++it), entt::entity{3});
    ASSERT_EQ(++it, set.end());
    ASSERT_EQ(++set.find(entt::entity{3}), set.end());
}

TEST(SparseSet, Data) {
    entt::sparse_set set;

    set.push(entt::entity{3});
    set.push(entt::entity{12});
    set.push(entt::entity{42});

    ASSERT_EQ(set.index(entt::entity{3}), 0u);
    ASSERT_EQ(set.index(entt::entity{12}), 1u);
    ASSERT_EQ(set.index(entt::entity{42}), 2u);

    ASSERT_EQ(set.data()[0u], entt::entity{3});
    ASSERT_EQ(set.data()[1u], entt::entity{12});
    ASSERT_EQ(set.data()[2u], entt::entity{42});
}

TEST(SparseSet, SortOrdered) {
    entt::sparse_set set;
    entt::entity entity[5u]{entt::entity{42}, entt::entity{12}, entt::entity{9}, entt::entity{7}, entt::entity{3}};

    set.push(std::begin(entity), std::end(entity));
    set.sort(std::less{});

    ASSERT_TRUE(std::equal(std::rbegin(entity), std::rend(entity), set.begin(), set.end()));
}

TEST(SparseSet, SortReverse) {
    entt::sparse_set set;
    entt::entity entity[5u]{entt::entity{3}, entt::entity{7}, entt::entity{9}, entt::entity{12}, entt::entity{42}};

    set.push(std::begin(entity), std::end(entity));
    set.sort(std::less{});

    ASSERT_TRUE(std::equal(std::begin(entity), std::end(entity), set.begin(), set.end()));
}

TEST(SparseSet, SortUnordered) {
    entt::sparse_set set;
    entt::entity entity[5u]{entt::entity{9}, entt::entity{7}, entt::entity{3}, entt::entity{12}, entt::entity{42}};

    set.push(std::begin(entity), std::end(entity));
    set.sort(std::less{});

    auto begin = set.begin();
    auto end = set.end();

    ASSERT_EQ(*(begin++), entity[2u]);
    ASSERT_EQ(*(begin++), entity[1u]);
    ASSERT_EQ(*(begin++), entity[0u]);
    ASSERT_EQ(*(begin++), entity[3u]);
    ASSERT_EQ(*(begin++), entity[4u]);
    ASSERT_EQ(begin, end);
}

TEST(SparseSet, SortRange) {
    entt::sparse_set set{entt::deletion_policy::in_place};
    entt::entity entity[5u]{entt::entity{7}, entt::entity{9}, entt::entity{3}, entt::entity{12}, entt::entity{42}};

    set.push(std::begin(entity), std::end(entity));
    set.erase(entity[0u]);

    ASSERT_EQ(set.size(), 5u);

    set.sort(std::less{});

    ASSERT_EQ(set.size(), 4u);
    ASSERT_EQ(set[0u], entity[4u]);
    ASSERT_EQ(set[1u], entity[3u]);
    ASSERT_EQ(set[2u], entity[1u]);
    ASSERT_EQ(set[3u], entity[2u]);

    set.clear();
    set.compact();
    set.push(std::begin(entity), std::end(entity));
    set.sort_n(0u, std::less{});

    ASSERT_TRUE(std::equal(std::rbegin(entity), std::rend(entity), set.begin(), set.end()));

    set.sort_n(2u, std::less{});

    ASSERT_EQ(set.data()[0u], entity[1u]);
    ASSERT_EQ(set.data()[1u], entity[0u]);
    ASSERT_EQ(set.data()[2u], entity[2u]);

    set.sort_n(5u, std::less{});

    auto begin = set.begin();
    auto end = set.end();

    ASSERT_EQ(*(begin++), entity[2u]);
    ASSERT_EQ(*(begin++), entity[0u]);
    ASSERT_EQ(*(begin++), entity[1u]);
    ASSERT_EQ(*(begin++), entity[3u]);
    ASSERT_EQ(*(begin++), entity[4u]);
    ASSERT_EQ(begin, end);
}

ENTT_DEBUG_TEST(SparseSetDeathTest, SortRange) {
    entt::sparse_set set{entt::deletion_policy::in_place};
    entt::entity entity{42};

    set.push(entity);
    set.erase(entity);

    ASSERT_DEATH(set.sort_n(0u, std::less{});, "");
    ASSERT_DEATH(set.sort_n(3u, std::less{});, "");
}

TEST(SparseSet, RespectDisjoint) {
    entt::sparse_set lhs;
    entt::sparse_set rhs;

    entt::entity lhs_entity[3u]{entt::entity{3}, entt::entity{12}, entt::entity{42}};
    lhs.push(std::begin(lhs_entity), std::end(lhs_entity));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));

    lhs.sort_as(rhs);

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));
}

TEST(SparseSet, RespectOverlap) {
    entt::sparse_set lhs;
    entt::sparse_set rhs;

    entt::entity lhs_entity[3u]{entt::entity{3}, entt::entity{12}, entt::entity{42}};
    lhs.push(std::begin(lhs_entity), std::end(lhs_entity));

    entt::entity rhs_entity[1u]{entt::entity{12}};
    rhs.push(std::begin(rhs_entity), std::end(rhs_entity));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.begin(), rhs.end()));

    lhs.sort_as(rhs);

    auto begin = lhs.begin();
    auto end = lhs.end();

    ASSERT_EQ(*(begin++), lhs_entity[1u]);
    ASSERT_EQ(*(begin++), lhs_entity[2u]);
    ASSERT_EQ(*(begin++), lhs_entity[0u]);
    ASSERT_EQ(begin, end);
}

TEST(SparseSet, RespectOrdered) {
    entt::sparse_set lhs;
    entt::sparse_set rhs;

    entt::entity lhs_entity[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    lhs.push(std::begin(lhs_entity), std::end(lhs_entity));

    entt::entity rhs_entity[6u]{entt::entity{6}, entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    rhs.push(std::begin(rhs_entity), std::end(rhs_entity));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs);

    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.begin(), rhs.end()));
}

TEST(SparseSet, RespectReverse) {
    entt::sparse_set lhs;
    entt::sparse_set rhs;

    entt::entity lhs_entity[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    lhs.push(std::begin(lhs_entity), std::end(lhs_entity));

    entt::entity rhs_entity[6u]{entt::entity{5}, entt::entity{4}, entt::entity{3}, entt::entity{2}, entt::entity{1}, entt::entity{6}};
    rhs.push(std::begin(rhs_entity), std::end(rhs_entity));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs);

    auto begin = rhs.begin();
    auto end = rhs.end();

    ASSERT_EQ(*(begin++), rhs_entity[0u]);
    ASSERT_EQ(*(begin++), rhs_entity[1u]);
    ASSERT_EQ(*(begin++), rhs_entity[2u]);
    ASSERT_EQ(*(begin++), rhs_entity[3u]);
    ASSERT_EQ(*(begin++), rhs_entity[4u]);
    ASSERT_EQ(*(begin++), rhs_entity[5u]);
    ASSERT_EQ(begin, end);
}

TEST(SparseSet, RespectUnordered) {
    entt::sparse_set lhs;
    entt::sparse_set rhs;

    entt::entity lhs_entity[5u]{entt::entity{1}, entt::entity{2}, entt::entity{3}, entt::entity{4}, entt::entity{5}};
    lhs.push(std::begin(lhs_entity), std::end(lhs_entity));

    entt::entity rhs_entity[6u]{entt::entity{3}, entt::entity{2}, entt::entity{6}, entt::entity{1}, entt::entity{4}, entt::entity{5}};
    rhs.push(std::begin(rhs_entity), std::end(rhs_entity));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs);

    auto begin = rhs.begin();
    auto end = rhs.end();

    ASSERT_EQ(*(begin++), rhs_entity[5u]);
    ASSERT_EQ(*(begin++), rhs_entity[4u]);
    ASSERT_EQ(*(begin++), rhs_entity[0u]);
    ASSERT_EQ(*(begin++), rhs_entity[1u]);
    ASSERT_EQ(*(begin++), rhs_entity[3u]);
    ASSERT_EQ(*(begin++), rhs_entity[2u]);
    ASSERT_EQ(begin, end);
}

TEST(SparseSet, RespectInvalid) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::sparse_set lhs;
    entt::sparse_set rhs;

    entt::entity lhs_entity[3u]{entt::entity{1}, entt::entity{2}, traits_type::construct(3, 1)};
    lhs.push(std::begin(lhs_entity), std::end(lhs_entity));

    entt::entity rhs_entity[3u]{entt::entity{2}, entt::entity{1}, traits_type::construct(3, 2)};
    rhs.push(std::begin(rhs_entity), std::end(rhs_entity));

    ASSERT_TRUE(std::equal(std::rbegin(lhs_entity), std::rend(lhs_entity), lhs.begin(), lhs.end()));
    ASSERT_TRUE(std::equal(std::rbegin(rhs_entity), std::rend(rhs_entity), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs);

    auto begin = rhs.begin();
    auto end = rhs.end();

    ASSERT_EQ(*(begin++), rhs_entity[0u]);
    ASSERT_EQ(*(begin++), rhs_entity[1u]);
    ASSERT_EQ(*(begin++), rhs_entity[2u]);
    ASSERT_EQ(rhs.current(rhs_entity[0u]), 0u);
    ASSERT_EQ(rhs.current(rhs_entity[1u]), 0u);
    ASSERT_EQ(rhs.current(rhs_entity[2u]), 2u);
    ASSERT_EQ(begin, end);
}

TEST(SparseSet, CanModifyDuringIteration) {
    entt::sparse_set set;
    set.push(entt::entity{0});

    ASSERT_EQ(set.capacity(), 1u);

    const auto it = set.begin();
    set.reserve(2u);

    ASSERT_EQ(set.capacity(), 2u);

    // this should crash with asan enabled if we break the constraint
    [[maybe_unused]] const auto entity = *it;
}

TEST(SparseSet, CustomAllocator) {
    test::throwing_allocator<entt::entity> allocator{};
    entt::basic_sparse_set<entt::entity, test::throwing_allocator<entt::entity>> set{allocator};

    ASSERT_EQ(set.get_allocator(), allocator);

    set.reserve(1u);

    ASSERT_EQ(set.capacity(), 1u);

    set.push(entt::entity{0});
    set.push(entt::entity{1});

    entt::basic_sparse_set<entt::entity, test::throwing_allocator<entt::entity>> other{std::move(set), allocator};

    ASSERT_TRUE(set.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_EQ(set.capacity(), 0u);
    ASSERT_EQ(other.capacity(), 2u);
    ASSERT_EQ(other.size(), 2u);

    set = std::move(other);

    ASSERT_FALSE(set.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_EQ(other.capacity(), 0u);
    ASSERT_EQ(set.capacity(), 2u);
    ASSERT_EQ(set.size(), 2u);

    set.swap(other);
    set = std::move(other);

    ASSERT_FALSE(set.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_EQ(other.capacity(), 0u);
    ASSERT_EQ(set.capacity(), 2u);
    ASSERT_EQ(set.size(), 2u);

    set.clear();

    ASSERT_EQ(set.capacity(), 2u);
    ASSERT_EQ(set.size(), 0u);

    set.shrink_to_fit();

    ASSERT_EQ(set.capacity(), 0u);
}

TEST(SparseSet, ThrowingAllocator) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::basic_sparse_set<entt::entity, test::throwing_allocator<entt::entity>> set{};

    test::throwing_allocator<entt::entity>::trigger_on_allocate = true;

    ASSERT_THROW(set.reserve(1u), test::throwing_allocator<entt::entity>::exception_type);
    ASSERT_EQ(set.capacity(), 0u);
    ASSERT_EQ(set.extent(), 0u);

    test::throwing_allocator<entt::entity>::trigger_on_allocate = true;

    ASSERT_THROW(set.push(entt::entity{0}), test::throwing_allocator<entt::entity>::exception_type);
    ASSERT_EQ(set.extent(), traits_type::page_size);
    ASSERT_EQ(set.capacity(), 0u);

    set.push(entt::entity{0});
    test::throwing_allocator<entt::entity>::trigger_on_allocate = true;

    ASSERT_THROW(set.reserve(2u), test::throwing_allocator<entt::entity>::exception_type);
    ASSERT_EQ(set.extent(), traits_type::page_size);
    ASSERT_TRUE(set.contains(entt::entity{0}));
    ASSERT_EQ(set.capacity(), 1u);

    test::throwing_allocator<entt::entity>::trigger_on_allocate = true;

    ASSERT_THROW(set.push(entt::entity{1}), test::throwing_allocator<entt::entity>::exception_type);
    ASSERT_EQ(set.extent(), traits_type::page_size);
    ASSERT_TRUE(set.contains(entt::entity{0}));
    ASSERT_FALSE(set.contains(entt::entity{1}));
    ASSERT_EQ(set.capacity(), 1u);

    entt::entity entity[2u]{entt::entity{1}, entt::entity{traits_type::page_size}};
    test::throwing_allocator<entt::entity>::trigger_after_allocate = true;

    ASSERT_THROW(set.push(std::begin(entity), std::end(entity)), test::throwing_allocator<entt::entity>::exception_type);
    ASSERT_EQ(set.extent(), 2 * traits_type::page_size);
    ASSERT_TRUE(set.contains(entt::entity{0}));
    ASSERT_TRUE(set.contains(entt::entity{1}));
    ASSERT_FALSE(set.contains(entt::entity{traits_type::page_size}));
    ASSERT_EQ(set.capacity(), 2u);
    ASSERT_EQ(set.size(), 2u);

    set.push(entity[1u]);

    ASSERT_TRUE(set.contains(entt::entity{traits_type::page_size}));
}
