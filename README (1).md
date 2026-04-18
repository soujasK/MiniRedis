# MiniRedis 🔴

A Redis-compatible in-memory key-value store written in C++17.
Speaks the real **RESP protocol** — connect with `redis-cli` or any Redis client.

---

## Features

| Category     | Commands                                      |
|--------------|-----------------------------------------------|
| **Strings**  | `SET`, `GET`, `DEL`, `EXISTS`                 |
| **Expiry**   | `SET key val EX secs`, `EXPIRE`, `TTL`        |
| **Counters** | `INCR`, `DECR`, `INCRBY`                      |
| **Lists**    | `LPUSH`, `RPUSH`, `LPOP`, `RPOP`, `LRANGE`, `LLEN` |
| **Server**   | `PING`, `KEYS`, `DBSIZE`, `FLUSHALL`          |

**Architecture highlights:**
- 🔒 Thread-safe with `shared_mutex` (concurrent reads)
- ⏱️ Background thread for lazy TTL expiry
- 🗑️ LRU eviction when max keys exceeded
- 📡 Full RESP protocol (works with `redis-cli`)
- 🧵 One thread per client (detached)

---

## Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Run

```bash
./miniredis          # default port 6379
./miniredis 6380     # custom port
```

## Connect

```bash
redis-cli -p 6379

127.0.0.1:6379> SET name "MiniRedis"
OK
127.0.0.1:6379> GET name
"MiniRedis"
127.0.0.1:6379> SET counter 0
OK
127.0.0.1:6379> INCR counter
(integer) 1
127.0.0.1:6379> RPUSH mylist a b c
(integer) 3
127.0.0.1:6379> LRANGE mylist 0 -1
1) "a"
2) "b"
3) "c"
127.0.0.1:6379> SET temp "bye" EX 5
OK
127.0.0.1:6379> TTL temp
(integer) 4
```

## Run Tests

```bash
cd build && ctest --output-on-failure
```

---

## Project Structure

```
miniredis/
├── CMakeLists.txt
├── src/
│   ├── main.cpp            # Entry point
│   ├── server.h/cpp        # TCP server, client threads
│   ├── store.h/cpp         # Core KV store (LRU + TTL)
│   ├── resp.h/cpp          # RESP protocol parser/serializer
│   └── command_handler.h/cpp  # Command dispatch
└── tests/
    ├── test_store.cpp
    └── test_resp.cpp
```

---

## Roadmap / Elite Extensions

- [ ] Persistence (AOF / RDB snapshots)
- [ ] Hash type (`HSET`, `HGET`, `HGETALL`)
- [ ] Set type (`SADD`, `SMEMBERS`, `SINTER`)
- [ ] Pub/Sub (`PUBLISH`, `SUBSCRIBE`)
- [ ] Cluster mode / replication
- [ ] epoll-based async I/O (replacing thread-per-client)
