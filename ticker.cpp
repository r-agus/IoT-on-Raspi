/// @brief This class is a simple ticker that every second adds one to an atomic integer until it gets to 10, and then it restores it to 0
#include "ticker.h"

Ticker::Ticker() {
    count_.store(0);
    alive_ = false;
}

void Ticker::start() {
    alive_ = true;
    while (alive_) {
        increment();
    }
}

void Ticker::stop() {
    alive_ = false;
}

void Ticker::increment() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (++count_ == 10) {
        count_ = 0;
    }
}

int Ticker::getCount() const {
    return count_.load();
}