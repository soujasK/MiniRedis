# MiniRedis

A Redis-compatible in-memory key-value store written in C++17. Implements the RESP (Redis Serialization Protocol) wire protocol — connect with `redis-cli` or any standard Redis client.

---

## Supported Commands

| Category    | Commands                                             |
|-------------|------------------------------------------------------|
| Strings     | `SET`, `GET`, `DEL`, `EXISTS`                        |
| Expiry      | `SET key val EX secs`, `EXPIRE`, `TTL`               |
| Counters    | `INCR`, `DECR`, `INCRBY`                             |
| Lists       | `LPUSH`, `RPUSH`, `LPOP`, `RPOP`, `LRANGE`, `LLEN`  |
| Server      | `PING`, `KEYS`, `DBSIZE`, `FLUSHALL`                 |

---

## Architecture

```
Client TCP connection
        │
        ▼
  server.cpp          — multi-client TCP server (one thread per client, detached)
        │
        ▼
  resp.cpp            — RESP protocol parser and serializer
        │
        ▼
  command_handler.cpp — command dispatch and validation
        │
        ▼
  store.cpp           — thread-safe KV store (shared_mutex, LRU eviction, TTL expiry)
```

**Design decisions:**
- `shared_mutex` for concurrent reads, exclusive writes
- Background thread polls for expired keys (lazy TTL cleanup)
- LRU eviction kicks in when max key limit is exceeded
- One detached thread per client — simple, no event loop overhead
- RESP parser handles inline and bulk string commands

---

## Build

Requires: `cmake >= 3.10`, `g++ with C++17 support`

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## Run

```bash
./miniredis           # starts on port 6379
./miniredis 6380      # custom port
```

---

## Connect via redis-cli

```bash
redis-cli -p 6379
```

```
127.0.0.1:6379> PING
PONG

127.0.0.1:6379> SET name "soujas"
OK
127.0.0.1:6379> GET name
"soujas"

127.0.0.1:6379> SET counter 0
OK
127.0.0.1:6379> INCR counter
(integer) 1
127.0.0.1:6379> INCRBY counter 5
(integer) 6

127.0.0.1:6379> RPUSH mylist a b c
(integer) 3
127.0.0.1:6379> LRANGE mylist 0 -1
1) "a"
2) "b"
3) "c"

127.0.0.1:6379> SET session "token" EX 10
OK
127.0.0.1:6379> TTL session
(integer) 9
127.0.0.1:6379> TTL session
(integer) -2   # expired
```

---

## Run Tests

```bash
cd build
ctest --output-on-failure
```

---

## Project Structure

```
MiniRedis/
├── CMakeLists.txt
├── main.cpp                 # Entry point, port parsing
├── server.h / server.cpp    # TCP server, client thread management
├── store.h / store.cpp      # Core KV store — LRU, TTL, thread safety
├── resp.h / resp.cpp        # RESP protocol parser and serializer
├── command_handler.h / command_handler.cpp   # Command dispatch
└── test_resp.cpp            # RESP protocol tests
```

---

## What's Next

- [ ] Persistence — AOF logging and RDB snapshots
- [ ] Hash type — `HSET`, `HGET`, `HGETALL`, `HDEL`
- [ ] Set type — `SADD`, `SMEMBERS`, `SINTER`, `SDIFF`
- [ ] Pub/Sub — `PUBLISH`, `SUBSCRIBE`, `UNSUBSCRIBE`
- [ ] `epoll`-based async I/O to replace thread-per-client model
- [ ] Replication — leader/follower with RESP replication stream
