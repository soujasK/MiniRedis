#include <gtest/gtest.h>
#include "resp.h"
#include "command_handler.h"

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

TEST(RESPTest, RejectMalformedNumericLengthsWithoutThrowing) {
    const std::vector<std::string> malformed = {
        "*-1\r\n",
        "*oops\r\n",
        "*184467440737095516161\r\n",
        "*1\r\n$-1\r\n",
        "*1\r\n$x\r\n",
        "*1\r\n$184467440737095516161\r\n",
    };
    for (const auto& input : malformed) {
        size_t consumed = 123;
        EXPECT_FALSE(RESP::parse(input, consumed).has_value());
        EXPECT_EQ(consumed, 123u);
    }
}

TEST(RESPTest, RejectIncorrectBulkStringTerminator) {
    size_t consumed = 0;
    EXPECT_FALSE(RESP::parse("*1\r\n$3\r\nfooXX", consumed).has_value());
    EXPECT_FALSE(RESP::parse("*1\r\n$3\r\nfoo\r", consumed).has_value());
}

TEST(RESPTest, AllowCRLFWithinBulkPayload) {
    size_t consumed = 0;
    const auto parsed = RESP::parse("*1\r\n$4\r\na\r\nb\r\n", consumed);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_EQ(parsed->size(), 1u);
    EXPECT_EQ((*parsed)[0], "a\r\nb");
    EXPECT_EQ(consumed, 14u);
}

TEST(CommandHandlerTest, RejectMalformedNumericArgumentsWithoutThrowing) {
    Store store;
    CommandHandler handler(store);
    store.set("counter", "3");
    const std::vector<std::vector<std::string>> invalid = {
        {"INCRBY", "counter", "1junk"},
        {"INCRBY", "counter", "999999999999999999999"},
        {"EXPIRE", "counter", "oops"},
        {"LRANGE", "list", "invalid", "1"},
        {"SET", "key", "value", "EX", "nope"},
        {"SET", "key", "value", "EX", "0"},
        {"SET", "key", "value", "UNKNOWN", "1"},
        {"SET", "key", "value", "EX", "10", "EX", "20"}
    };
    for (const auto& command : invalid) {
        std::string response;
        EXPECT_NO_THROW(response = handler.handle(command));
        EXPECT_EQ(response.rfind("-ERR ", 0), 0u);
    }
    EXPECT_EQ(store.get("counter"), "3");
    EXPECT_FALSE(store.get("key").has_value());
}

TEST(CommandHandlerTest, PingRejectsExtraArguments) {
    Store store;
    CommandHandler handler(store);
    EXPECT_EQ(handler.handle({"PING"}), "+PONG\r\n");
    EXPECT_EQ(handler.handle({"PING", "hello"}), "$5\r\nhello\r\n");
    EXPECT_EQ(handler.handle({"PING", "hello", "ignored"}), "-ERR wrong number of arguments for PING\r\n");
}

TEST(CommandHandlerTest, CommandNamesAreCaseInsensitive) {
    Store store;
    CommandHandler handler(store);
    EXPECT_EQ(handler.handle({"pInG"}), "+PONG\r\n");
    EXPECT_EQ(handler.handle({"sEt", "key", "value", "eX", "10"}), "+OK\r\n");
    EXPECT_EQ(handler.handle({"gEt", "key"}), "$5\r\nvalue\r\n");
}
