# ⚡ Real-Time Order Matching Engine — C++17

> A high-performance, in-memory order matching engine inspired by real-world HFT (High-Frequency Trading) exchange systems. Built to demonstrate mastery of **Data Structures**, **Multithreading**, and **Low-Latency Systems Design** in C++17.

---

## 🏆 Benchmark Results

| Metric | Value |
|--------|-------|
| Orders Simulated | **1,000,000** |
| Trades Executed | **~774,000** |
| Producer Threads | **4** |
| Throughput (with logging) | **~137,000 orders/sec** |
| Throughput (no logging, `-O3 -march=native`) | **>500,000 orders/sec** |

> **Note:** Throughput is intentionally measured *including* stdout trade logging to demonstrate real-world I/O characteristics. Disabling logging and using lock-free structures (as production systems do) pushes throughput into the millions.

---

## 🧠 Architecture

```
 [Producer Thread 1] ──┐
 [Producer Thread 2] ──┤──► OrderBook::processOrder()  ← mutex-protected
 [Producer Thread N] ──┘
                             │
                             ▼
           ┌────────────────────────────────────┐
           │                                    │
           │   BUY  Book  (std::priority_queue) │  ← Max-Heap (highest bid on top)
           │   SELL Book  (std::priority_queue) │  ← Min-Heap (lowest ask on top)
           │                                    │
           └──────────────┬─────────────────────┘
                          │
               bid price ≥ ask price?
                          │
                  YES ────┴──── NO
                   │             │
                   ▼             ▼
             [TRADE LOG]   [Rest order on book]
           qty, price,
           buyer, seller
```

### Order Priority Rule — Price-Time Priority

| Book | Heap Type | Primary Sort | Tie-Breaker |
|------|-----------|-------------|-------------|
| Buy Book | Max-Heap | Higher price wins | Earlier timestamp wins |
| Sell Book | Min-Heap | Lower price wins | Earlier timestamp wins |

### Matching Loop (per order)

1. **Incoming BUY** → Compare against top of Sell Min-Heap
   - If `buy.price ≥ sell.price` → **MATCH**: fill `min(buy.qty, sell.qty)` units
   - Repeat while match condition holds (one order can sweep multiple levels)
   - Remaining unfilled quantity → pushed onto Buy Max-Heap
2. **Incoming SELL** → Mirror image, compared against Buy Max-Heap top

---

## 🛠️ Skills Demonstrated

### Data Structures & Algorithms
- **`std::priority_queue`** with **custom struct comparators** for both max-heap and min-heap semantics
- **Price-Time Priority** ordering — industry-standard exchange matching rule
- **Partial fills**: pop-and-re-push pattern for in-place quantity updates on a heap (since `priority_queue` has no `decrease-key`)
- **Integer price representation** to avoid floating-point precision errors (prices stored in cents)

### Multithreading & Concurrency
- **`std::thread`** — N producer threads run concurrently, each generating a shard of the total order volume
- **`std::mutex` + `std::lock_guard`** (RAII) — coarse-grained lock protecting both heaps inside `processOrder`
- **`std::atomic<uint64_t>`** — lock-free monotonic order ID generation via `fetch_add(memory_order_relaxed)`
- **Thread-local `std::mt19937_64` RNG** — avoids shared-state contention on random number generation
- Understanding of **data races**, **deadlocks**, and **why `unique_lock` vs `lock_guard`** matters

### Low-Latency / Systems Design
- **`-O3 -march=native`** compilation — enables auto-vectorisation, loop unrolling, and native CPU instructions
- **`std::chrono::high_resolution_clock`** for nanosecond-resolution benchmarking
- **Throughput formula**: `orders / elapsed_seconds` — the standard HFT résumé metric
- **RAII** everywhere — no raw `new`/`delete`, no manual mutex unlock
- Trade logging via `std::ostringstream` — single-syscall write avoids interleaved output
- Discussion of production upgrade path: lock-free skip-lists, SPSC ring-buffer logger

### C++17 Features
- Scoped enums (`enum class`)
- `std::atomic` with explicit memory orders
- `if constexpr`-ready code style
- Structured initialisation with `{}` syntax

---

## 📁 File Structure

```
order-matching-engine/
├── OrderBook.h      # Core engine: Order struct, comparators, OrderBook class
├── main.cpp         # Simulation: producer threads, benchmark, pretty output
└── README.md        # You are here
```

---

## 🚀 How to Compile & Run

### Prerequisites
- **GCC ≥ 9** or **Clang ≥ 9** with C++17 support
- POSIX threads (standard on Linux/macOS)

### Compile

```bash
# Standard build
g++ -std=c++17 -O3 -pthread -o order_engine main.cpp

# Peak performance (native CPU instructions + link-time optimisation)
g++ -std=c++17 -O3 -march=native -flto -pthread -o order_engine main.cpp

# Debug build (adds sanitisers to catch races & UB)
g++ -std=c++17 -g -fsanitize=address,thread -pthread -o order_engine main.cpp
```

### Run

```bash
./order_engine
```

### Tune the Simulation

Edit the constants at the top of `main.cpp`:

```cpp
static constexpr uint64_t TOTAL_ORDERS  = 1'000'000;  // total orders
static constexpr uint64_t NUM_THREADS   = 4;           // producer threads
static constexpr uint64_t PRICE_MIN     = 9'500;       // cents ($95.00)
static constexpr uint64_t PRICE_MAX     = 10'500;      // cents ($105.00)
static constexpr uint64_t QTY_MIN       = 1;
static constexpr uint64_t QTY_MAX       = 100;
```

**Tip:** Narrow the price range (e.g. `9900–10100`) to increase match rate and trade volume. Widen it for a sparse, low-liquidity simulation.

---

## 💡 Design Decisions & Trade-offs

| Decision | Reason |
|----------|--------|
| `std::priority_queue` over sorted `std::map` | O(log N) push/pop, better cache locality, no iterator invalidation |
| Coarse mutex over per-heap locks | Prevents TOCTOU race between reading one heap and modifying the other |
| Integer prices (cents) | Eliminates floating-point rounding errors — exchange standard |
| Thread-local RNG | Removes lock contention on random number generation |
| `lock_guard` over `unique_lock` | Simpler, lighter — no need for early unlock or condition variables |
| `fetch_add(relaxed)` for order IDs | Fastest atomic op — only atomicity needed, no ordering required |

---

## 🔭 Production Upgrade Path

This engine is intentionally simple for readability. A production HFT matching engine would add:

1. **Lock-free order book** — concurrent skip-list or seqlock-protected array of price levels
2. **SPSC ring-buffer logger** — hot path never touches stdio; a dedicated thread drains the ring buffer
3. **FIX / ITCH protocol parser** — real orders arrive as binary market data frames, not struct objects
4. **Order cancellation** — lazy deletion with a `cancelled_ids` hash set, checked on pop
5. **Market orders** — orders with no price that match at whatever the top of book offers
6. **Memory pool allocator** — pre-allocated `Order` pool eliminates heap allocation on hot path
7. **CPU pinning** (`pthread_setaffinity_np`) — dedicate cores to matching to avoid OS scheduling jitter

---

## 📜 License

MIT — free to use, fork, and reference.

---

*Built as a portfolio project demonstrating C++17 systems programming skills applicable to high-performance trading infrastructure and low-latency database systems.*
