#pragma once

#include <algorithm>        
#include <atomic>           
#include <chrono>           
#include <cstdint>          
#include <functional>       
#include <mutex>            
#include <queue>            
#include <string>
#include <iostream>
#include <sstream>


enum class OrderType : uint8_t {
    BUY,
    SELL
};


struct Order {
    uint64_t    orderId;        
    OrderType   type;           
    uint64_t    price;          
    uint64_t    quantity;       
    std::chrono::high_resolution_clock::time_point timestamp; 

    
    Order(uint64_t id,
          OrderType t,
          uint64_t  p,
          uint64_t  q,
          std::chrono::high_resolution_clock::time_point ts)
        : orderId(id), type(t), price(p), quantity(q), timestamp(ts) {}
};


struct BuyBookComparator {
    bool operator()(const Order& a, const Order& b) const {
        if (a.price != b.price) {
           
            return a.price < b.price;
        }
        
        return a.timestamp > b.timestamp;
    }
};


struct SellBookComparator {
    bool operator()(const Order& a, const Order& b) const {
        if (a.price != b.price) {
            
            return a.price > b.price;
        }
        
        return a.timestamp > b.timestamp;
    }
};



class OrderBook {
public:
    
    void processOrder(Order order) {
        
        std::lock_guard<std::mutex> lock(bookMutex_);
       

        if (order.type == OrderType::BUY) {
            matchBuyOrder(order);
        } else {
            matchSellOrder(order);
        }
    }

   

    uint64_t tradesExecuted()  const { return tradesExecuted_.load(); }
    uint64_t ordersProcessed() const { return ordersProcessed_.load(); }

    
    uint64_t bestBid() const {
        std::lock_guard<std::mutex> lock(bookMutex_);
        return buyBook_.empty() ? 0 : buyBook_.top().price;
    }

    
    uint64_t bestAsk() const {
        std::lock_guard<std::mutex> lock(bookMutex_);
        return sellBook_.empty() ? 0 : sellBook_.top().price;
    }

    size_t buyBookDepth()  const {
        std::lock_guard<std::mutex> lock(bookMutex_);
        return buyBook_.size();
    }

    size_t sellBookDepth() const {
        std::lock_guard<std::mutex> lock(bookMutex_);
        return sellBook_.size();
    }

private:
   
    void matchBuyOrder(Order& buy) {
        ++ordersProcessed_;

        
        while (buy.quantity > 0 && !sellBook_.empty()) {

            
            const Order& bestSell = sellBook_.top();

            
            if (buy.price < bestSell.price) {
                
                break;
            }

            
            uint64_t fillQty = std::min(buy.quantity, bestSell.quantity);

            
            logTrade(bestSell.orderId, buy.orderId, bestSell.price, fillQty);
            ++tradesExecuted_;

           
            buy.quantity -= fillQty;

            if (bestSell.quantity == fillQty) {
               
                sellBook_.pop();  // O(log N)
            } else {
               
                Order remainSell = bestSell;           
                sellBook_.pop();                       
                remainSell.quantity -= fillQty;
                sellBook_.push(remainSell);           
            }
        }

        
        if (buy.quantity > 0) {
            buyBook_.push(buy);   // O(log N)
        }
    }

   
    void matchSellOrder(Order& sell) {
        ++ordersProcessed_;

        while (sell.quantity > 0 && !buyBook_.empty()) {

            const Order& bestBuy = buyBook_.top();

            
            if (bestBuy.price < sell.price) {
               
                break;
            }

            uint64_t fillQty = std::min(sell.quantity, bestBuy.quantity);

            logTrade(sell.orderId, bestBuy.orderId, bestBuy.price, fillQty);
            ++tradesExecuted_;

            sell.quantity -= fillQty;

            if (bestBuy.quantity == fillQty) {
                buyBook_.pop();   
            } else {
                Order remainBuy = bestBuy;
                buyBook_.pop();
                remainBuy.quantity -= fillQty;
                buyBook_.push(remainBuy);
            }
        }

        if (sell.quantity > 0) {
            sellBook_.push(sell);
        }
    }

   
    void logTrade(uint64_t sellId, uint64_t buyId,
                  uint64_t price,  uint64_t qty) const {
        std::ostringstream oss;
        oss << "[TRADE] BuyOrder#"  << buyId
            << " x SellOrder#"      << sellId
            << "  Price: "          << price
            << "  Qty: "            << qty
            << '\n';
        std::cout << oss.str();
    }

   
    mutable std::mutex bookMutex_;

   
    std::priority_queue<Order, std::vector<Order>, BuyBookComparator>  buyBook_;

    
    std::priority_queue<Order, std::vector<Order>, SellBookComparator> sellBook_;

   
    std::atomic<uint64_t> tradesExecuted_  {0};
    std::atomic<uint64_t> ordersProcessed_ {0};
};


namespace engine {
    inline std::atomic<uint64_t> globalOrderId {1};

    inline uint64_t nextOrderId() {
        
        return globalOrderId.fetch_add(1, std::memory_order_relaxed);
    }
}
