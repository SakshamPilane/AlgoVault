#include "lru_cache.h"
#include <iostream>

LRUCache::LRUCache(size_t capacity) : capacity(capacity) {
    if (this->capacity == 0) this->capacity = 1;
}

bool LRUCache::put(const std::string& key, const std::string& value) {
    std::unique_lock lock(mutex_);

    auto it = map.find(key);
    if (it != map.end()) {
        it->second->second = value;
        lruList.splice(lruList.begin(), lruList, it->second);
        map[key] = lruList.begin();
        return true;
    }

    if (lruList.size() >= capacity) evict();

    lruList.emplace_front(key, value);
    map[key] = lruList.begin();

    return true;
}

bool LRUCache::get(const std::string& key, std::string& value) {
    std::unique_lock lock(mutex_);
    auto it = map.find(key);
    if (it == map.end()) {
        misses.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    value = it->second->second;
    lruList.splice(lruList.begin(), lruList, it->second);
    map[key] = lruList.begin();
    hits.fetch_add(1, std::memory_order_relaxed);
    return true;
}

bool LRUCache::exists(const std::string& key) {
    std::shared_lock lock(mutex_);
    return map.find(key) != map.end();
}

bool LRUCache::remove(const std::string& key) {
    std::unique_lock lock(mutex_);
    auto it = map.find(key);
    if (it == map.end()) return false;
    lruList.erase(it->second);
    map.erase(it);
    return true;
}

size_t LRUCache::size() {
    std::shared_lock lock(mutex_);
    return lruList.size();
}

void LRUCache::moveToFront(const std::string& key) {
    auto it = map.find(key);
    if (it == map.end()) return;
    lruList.splice(lruList.begin(), lruList, it->second);
    map[key] = lruList.begin();
}

void LRUCache::evict() {
    if (lruList.empty()) return;
    auto last = lruList.back();
    std::string keyToRemove = last.first;
    map.erase(keyToRemove);
    lruList.pop_back();
    evictions.fetch_add(1, std::memory_order_relaxed);
    if (onEvict) {
        try { onEvict(keyToRemove); } catch (...) {}
    }
    std::cout << "[LRU] Evicted key: " << keyToRemove << std::endl;
}

void LRUCache::setEvictionCallback(const std::function<void(const std::string&)>& cb) {
    std::unique_lock lock(mutex_);
    onEvict = cb;
}

LRUCache::Stats LRUCache::getStats() const {
    Stats s;
    s.hits = hits.load(std::memory_order_relaxed);
    s.misses = misses.load(std::memory_order_relaxed);
    s.evictions = evictions.load(std::memory_order_relaxed);
    return s;
}

void LRUCache::resetStats() {
    hits.store(0, std::memory_order_relaxed);
    misses.store(0, std::memory_order_relaxed);
    evictions.store(0, std::memory_order_relaxed);
}
