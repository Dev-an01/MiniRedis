# MiniRedis

A small Redis-like server in C++: single-threaded event loop over `poll()`, a
binary request/response protocol, strings and sorted sets, key TTLs, and idle
connection timeouts.

## Build

```sh
make          # builds ./server and ./client
make test     # unit tests + end-to-end command tests
```

Requires a C++ compiler and `python3` (for `test_cmds.py`). The build uses
`-std=gnu++17` because `container_of()` in `common.h` relies on the GNU
`typeof` and statement-expression extensions.

## Run

```sh
./server                      # listens on 0.0.0.0:1234
./client set greeting hello
./client get greeting
./client keys
```

The client sends one command per invocation and prints the typed reply:

```
$ ./client set k hello
(nil)
$ ./client get k
(str) hello
$ ./client pexpire k 100
(int) 1
$ ./client pttl k
(int) 98
$ ./client get nope
(nil)
$ ./client bogus
(err) 1 unknown command.
```

## Commands

| Command | Reply |
| --- | --- |
| `get key` | string, or nil if absent |
| `set key val` | nil |
| `del key` | 1 if deleted, else 0 |
| `keys` | array of all keys |
| `pexpire key ttl_ms` | 1 if the key exists, else 0. Negative `ttl_ms` clears the TTL |
| `pttl key` | remaining ms, `-1` if no TTL, `-2` if the key is gone |
| `zadd zset score name` | 1 if inserted, 0 if an existing score was updated |
| `zrem zset name` | 1 if removed, else 0 |
| `zscore zset name` | double, or nil |
| `zquery zset score name offset limit` | flat array of `name, score, name, score, …` |

`zquery` seeks to the first member `>= (score, name)`, steps `offset` places
from there (may be negative), and returns up to `limit` values. A missing key
is treated as an empty sorted set.

Errors come back as a code plus a message: `1` unknown command, `2` response
too big, `3` wrong value type, `4` bad argument.

## Protocol

All integers are little-endian. A request is a length-prefixed array of strings:

```
+------+------+-----+------+-----+------+-----+------+
| nstr | len  | str1| len  | str2| ...  | len | strn |
+------+------+-----+------+-----+------+-----+------+
   4B     4B     …     4B     …            4B     …
```

preceded by a 4-byte total body length. A response is a 4-byte length followed
by one tagged value:

| Tag | Type | Payload |
| --- | --- | --- |
| 0 | nil | — |
| 1 | err | `u32` code, `u32` len, bytes |
| 2 | str | `u32` len, bytes |
| 3 | int | `i64` |
| 4 | dbl | `double` |
| 5 | arr | `u32` count, then that many tagged values |

Arrays nest, so a reply is a small self-describing tree. Requests are
pipelined: the server parses as many complete requests as a read yields before
writing anything back.

## Layout

| File | What's in it |
| --- | --- |
| `server.cpp` | event loop, connection buffers, protocol, command handlers |
| `client.cpp` | one-shot CLI client and reply pretty-printer |
| `hashtable.{h,cpp}` | chained hashtable with progressive rehashing (two tables, 128 keys migrated per operation, load factor 8) |
| `avl.{h,cpp}` | AVL tree with subtree counts, so `avl_offset()` is O(log n) rank lookup |
| `zset.{h,cpp}` | sorted set: an AVL tree keyed by `(score, name)` plus a hashtable keyed by name |
| `heap.{h,cpp}` | binary min-heap of expiry timestamps for TTLs |
| `list.h` | intrusive doubly-linked list, used as the idle-connection LRU |
| `threadpool.{h,cpp}` | pthread pool; frees large sorted sets off the event loop |
| `common.h` | `container_of()` and the FNV-1a string hash |

The data structures are intrusive: nodes are embedded in the payload structs
and recovered with `container_of()`, so a value can sit in several indexes at
once without extra allocation. An `Entry` is in the main hashtable and, if it
has a TTL, in the heap; a `ZNode` is in both the AVL tree and the sorted set's
own hashtable.

### Timers

Two timer sources feed the `poll()` timeout. Idle connections live on a
linked list in last-active order, so the head is always the next to expire —
O(1). TTLs live in the min-heap, so the root is the next key to expire —
O(log n) to insert or update. Expired keys are reaped after each poll, at most
2000 per pass so a mass expiry can't stall the loop.

## Limits and defaults

- Port `1234`, hardcoded.
- Idle connections are closed after 5s.
- Max message: 32 MiB, on both sides.
- Data is in memory only — nothing is persisted.

## Tests

```sh
make test
```

- `test_avl.cpp` — AVL insert/delete against a `std::multiset`, verifying
  height, subtree counts and parent links after every operation.
- `test_offset.cpp` — `avl_offset()` rank arithmetic against every offset from
  every node, for trees of size 1..200.
- `test_heap.cpp` — heap updates against a `std::multimap`, checking the heap
  property and that each item's back-reference tracks its index.
- `test_cmds.py` — end-to-end: runs `./client` against a live server and diffs
  stdout against expected transcripts. It needs a server on port 1234; `make
  test` starts and stops one for you.
