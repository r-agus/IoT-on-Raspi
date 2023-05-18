#ifndef TICKER_H
#define TICKER_H

#include <atomic>
#include <chrono>
#include <thread>

class Ticker {
private:
    std::atomic<int> count_;
    bool alive_;
public:
    Ticker();
    void start();
    void stop();
    void increment();
    int getCount() const;
};

#endif  // TICKER_H
