#include <gtest/gtest.h>
#include "resp.h"

TEST(RESPTest, ParseInlineCommand) {
    std::string data = "PING\r\n";
    size_t consumed = 0;
    auto args = RESP::parse(data, consumed);
    ASSERT_TRUE(args.has_value());
    ASSERT_EQ(args->size(), 1u);
    EXPECT_EQ((*args)[0], "PING");
    EXPECT_EQ(consumed, data.size());
}

TEST(RESPTest, ParseArrayCommand) {
    std::string data = "*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n";
    size_t consumed = 0;
    auto args = RESP::parse(data, consumed);
    ASSERT_TRUE(args.has_value());
    ASSERT_EQ(args->size(), 3u);
    EXPECT_EQ((*args)[0], "SET");
    EXPECT_EQ((*args)[1], "foo");
    EXPECT_EQ((*args)[2], "bar");
}

TEST(RESPTest, ParseIncomplete) {
    std::string data = "*3\r\n$3\r\nSET\r\n";
    size_t consumed = 0;
    auto args = RESP::parse(data, consumed);
    EXPECT_FALSE(args.has_value());
}

TEST(RESPTest, SimpleString) {
    EXPECT_EQ(RESP::simple_string("OK"), "+OK\r\n");
}

TEST(RESPTest, Error) {
    EXPECT_EQ(RESP::error("oops"), "-ERR oops\r\n");
}

TEST(RESPTest, Integer) {
    EXPECT_EQ(RESP::integer(42), ":42\r\n");
}

TEST(RESPTest, BulkString) {
    EXPECT_EQ(RESP::bulk_string("hello"), "$5\r\nhello\r\n");
}

TEST(RESPTest, NullBulkString) {
    EXPECT_EQ(RESP::null_bulk_string(), "$-1\r\n");
}

TEST(RESPTest, Array) {
    auto r = RESP::array({"a", "b"});
    EXPECT_EQ(r, "*2\r\n$1\r\na\r\n$1\r\nb\r\n");
}
