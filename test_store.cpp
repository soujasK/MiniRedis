#include <gtest/gtest.h>
#include "store.h"
#include <thread>
#include <chrono>

TEST(StoreTest, SetAndGet) {
    Store s;
    s.set("key", "value");
    EXPECT_EQ(s.get("key"), "value");
}

TEST(StoreTest, GetMissing) {
    Store s;
    EXPECT_EQ(s.get("nope"), std::nullopt);
}

TEST(StoreTest, Del) {
    Store s;
    s.set("k", "v");
    EXPECT_TRUE(s.del("k"));
    EXPECT_EQ(s.get("k"), std::nullopt);
    EXPECT_FALSE(s.del("k"));
}

TEST(StoreTest, TTLExpiry) {
    Store s;
    s.set("k", "v", 1);
    EXPECT_EQ(s.get("k"), "v");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_EQ(s.get("k"), std::nullopt);
}

TEST(StoreTest, IncrDecr) {
    Store s;
    EXPECT_EQ(s.incr("counter"), 1);
    EXPECT_EQ(s.incr("counter"), 2);
    EXPECT_EQ(s.decr("counter"), 1);
    EXPECT_EQ(s.incrby("counter", 10), 11);
}

TEST(StoreTest, ListPushPop) {
    Store s;
    s.rpush("list", {"a", "b", "c"});
    EXPECT_EQ(s.llen("list"), 3);
    EXPECT_EQ(s.lpop("list"), "a");
    EXPECT_EQ(s.rpop("list"), "c");
    EXPECT_EQ(s.llen("list"), 1);
}

TEST(StoreTest, LRange) {
    Store s;
    s.rpush("list", {"a", "b", "c", "d"});
    auto r = s.lrange("list", 1, 2);
    ASSERT_EQ(r.size(), 2u);
    EXPECT_EQ(r[0], "b");
    EXPECT_EQ(r[1], "c");
}

TEST(StoreTest, FlushAll) {
    Store s;
    s.set("a", "1");
    s.set("b", "2");
    s.flushall();
    EXPECT_EQ(s.dbsize(), 0u);
}

TEST(StoreTest, DBSize) {
    Store s;
    s.set("a", "1");
    s.set("b", "2");
    EXPECT_EQ(s.dbsize(), 2u);
}
