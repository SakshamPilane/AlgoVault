#pragma once
#include <unordered_map>
#include <list>
#include <string>
#include <shared_mutex>
#include <functional>
#include <atomic>

class LRUCache {
public:
    struct Stats {
        std::size_t hits = 0;
        std::size_t misses = 0;
        std::size_t evictions = 0;
    };

    explicit LRUCache(size_t capacity);

    bool put(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value);
    bool exists(const std::string& key);
    bool remove(const std::string& key);
    size_t size();

    void setEvictionCallback(const std::function<void(const std::string&)>& cb);

    Stats getStats() const;
    void resetStats();

private:
    size_t capacity;
    std::list<std::pair<std::string, std::string>> lruList;
    std::unordered_map<std::string, std::list<std::pair<std::string, std::string>>::iterator> map;
    mutable std::shared_mutex mutex_;
    std::function<void(const std::string&)> onEvict;
    std::atomic<std::size_t> hits{0};
    std::atomic<std::size_t> misses{0};
    std::atomic<std::size_t> evictions{0};
    void moveToFront(const std::string& key);
    void evict();
};
