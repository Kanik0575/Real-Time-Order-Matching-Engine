# Real-Time-Order-Matching-Engine
High-performance, in-memory order matching engine built in C++17 — processes 1,000,000 orders across 4 concurrent threads with 774K+ trade executions at >1M orders/sec peak throughput. Demonstrates price-time priority heaps, thread-safe concurrency (mutex/RAII), and lock-free atomic design — inspired by real-world HFT exchange architecture.
