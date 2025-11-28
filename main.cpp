#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>

#include "include/kvstore.h"
#include "include/lru_cache.h"
#include "include/persistence.h"
#include "include/server.h"

namespace fs = std::filesystem;

int main() {
    fs::create_directories("data");

    // Create cache (cap=3 for testing; increase in production)
    LRUCache cache(3);

    // Create KeyValueStore with cache
    KeyValueStore store(&cache);

    // Setup WAL
    Persistence wal("data/wal.log");
    store.setPersistence(&wal);

    // Replay WAL → recover previous state
    wal.replay(
        [&](const std::string& key, const std::string& value) { 
            store.put(key, value, /*persist=*/false); 
        },
        [&](const std::string& key) { 
            store.del(key, /*persist=*/false); 
        }
    );

    std::cout << "Recovered " << store.size() << " keys from WAL.\n";

    // ---------------------------
    // 🔥 TTL BACKGROUND CLEANER
    // ---------------------------
    std::thread([&store]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            store.cleanupExpired();
        }
    }).detach();

    std::cout << "[TTL] Background cleaner running every 1 second.\n";

    // ---------------------------
    // 🌐 START REST API SERVER
    // ---------------------------
    startServer(store, wal);

    return 0;
}
