#include "OrderBook.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <vector>



static constexpr uint64_t TOTAL_ORDERS  = 1'000'000;


static constexpr uint64_t NUM_THREADS   = 4;


static constexpr uint64_t PRICE_MIN     = 9'500;   
static constexpr uint64_t PRICE_MAX     = 10'500;  

// Quantity range per order
static constexpr uint64_t QTY_MIN       = 1;
static constexpr uint64_t QTY_MAX       = 100;



void producerTask(OrderBook*      book,
                  uint64_t        ordersToGenerate,
                  uint32_t        threadSeed)
{
    
    std::mt19937_64 rng(threadSeed);   

    
    std::uniform_int_distribution<uint64_t> priceDist(PRICE_MIN, PRICE_MAX);
    std::uniform_int_distribution<uint64_t> qtyDist  (QTY_MIN,   QTY_MAX);
   
    std::uniform_int_distribution<int>      typeDist (0, 1);

    for (uint64_t i = 0; i < ordersToGenerate; ++i) {
        Order order(
            engine::nextOrderId(),                              
            typeDist(rng) == 0 ? OrderType::BUY
                               : OrderType::SELL,             
            priceDist(rng),                                    
            qtyDist(rng),                                       
            std::chrono::high_resolution_clock::now()          
        );

        book->processOrder(order);
    }
}




std::string formatNumber(uint64_t n) {
    std::string s = std::to_string(n);
    int insertPos = static_cast<int>(s.size()) - 3;
    while (insertPos > 0) {
        s.insert(static_cast<size_t>(insertPos), ",");
        insertPos -= 3;
    }
    return s;
}



int main() {

   
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════╗\n";
    std::cout << "  ║     Real-Time Order Matching Engine  v1.0        ║\n";
    std::cout << "  ║     C++17  |  HFT Portfolio Project              ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════╝\n\n";

    std::cout << "  Configuration:\n";
    std::cout << "    Total orders  : " << formatNumber(TOTAL_ORDERS) << "\n";
    std::cout << "    Producer threads : " << NUM_THREADS << "\n";
    std::cout << "    Price range    : [" << PRICE_MIN << ", " << PRICE_MAX << "] cents\n";
    std::cout << "    Qty range      : [" << QTY_MIN   << ", " << QTY_MAX   << "]\n\n";

    
    OrderBook book;

    
    uint64_t baseLoad      = TOTAL_ORDERS / NUM_THREADS;
    uint64_t remainder     = TOTAL_ORDERS % NUM_THREADS;

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    
    auto startTime = std::chrono::high_resolution_clock::now();

    std::cout << "  Spawning " << NUM_THREADS
              << " producer threads…  (trade log below)\n";
    std::cout << "  ─────────────────────────────────────────────────\n";

    for (uint64_t t = 0; t < NUM_THREADS; ++t) {
        
        uint64_t load = baseLoad + (t == NUM_THREADS - 1 ? remainder : 0);

        
        uint32_t seed = static_cast<uint32_t>((t + 1) * 0xDEAD'BEEF);

       
        threads.emplace_back(producerTask, &book, load, seed);
    }

    
    for (auto& th : threads) {
        th.join();
    }

   
    auto endTime  = std::chrono::high_resolution_clock::now();

    
    double elapsedMs =
        std::chrono::duration<double, std::milli>(endTime - startTime).count();

    double elapsedSec  = elapsedMs / 1000.0;
    double throughput  = static_cast<double>(TOTAL_ORDERS) / elapsedSec;

    
    std::cout << "\n";
    std::cout << "  ══════════════════════════════════════════════════\n";
    std::cout << "   BENCHMARK RESULTS\n";
    std::cout << "  ══════════════════════════════════════════════════\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "   Total orders submitted  : "
              << formatNumber(TOTAL_ORDERS) << "\n";
    std::cout << "   Orders processed        : "
              << formatNumber(book.ordersProcessed()) << "\n";
    std::cout << "   Trades executed         : "
              << formatNumber(book.tradesExecuted()) << "\n";
    std::cout << "   Remaining buy depth     : "
              << formatNumber(static_cast<uint64_t>(book.buyBookDepth())) << "\n";
    std::cout << "   Remaining sell depth    : "
              << formatNumber(static_cast<uint64_t>(book.sellBookDepth())) << "\n";
    std::cout << "   Best bid (cents)        : "
              << book.bestBid() << "\n";
    std::cout << "   Best ask (cents)        : "
              << book.bestAsk() << "\n";
    std::cout << "   ─────────────────────────────────────────────\n";
    std::cout << "   Elapsed time            : "
              << elapsedMs << " ms\n";
    std::cout << "   Throughput              : "
              << formatNumber(static_cast<uint64_t>(throughput))
              << " orders/sec\n";
    std::cout << "  ══════════════════════════════════════════════════\n\n";

    std::cout << "  Tip: recompile with -O3 -march=native for peak throughput.\n\n";

    return 0;
}
