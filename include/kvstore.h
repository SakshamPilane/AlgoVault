#pragma once
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <vector>
#include <chrono>

class Persistence;
class LRUCache;

class KeyValueStore {
public:
    explicit KeyValueStore(LRUCache* cachePtr = nullptr);

    bool put(const std::string& key, const std::string& value, bool persist = true);
    std::string get(const std::string& key, bool& found);
    bool del(const std::string& key, bool persist = true);
    bool exists(const std::string& key);
    size_t size();

    std::unordered_map<std::string, std::string> snapshot();

    void setPersistence(Persistence* p);
    void attachCache(LRUCache* cachePtr);

    void onCacheEvict(const std::string& key);

    LRUCache* getCache() const;

    // ---------- TTL SUPPORT ----------
    void setTTL(const std::string& key, long long ttlSeconds);
    long long getTTL(const std::string& key);   // remaining seconds
    bool isExpired(const std::string& key);
    void cleanupExpired();                      // background thread calls this

private:
    std::unordered_map<std::string, std::string> store;
    std::unordered_map<std::string, long long> expiry;  // epoch ms expiry

    mutable std::shared_mutex mutex_;

    Persistence* persistence = nullptr;
    LRUCache* cache = nullptr;

    void onPut(const std::string& key, const std::string& value);
    void onDelete(const std::string& key);

    long long nowMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};
