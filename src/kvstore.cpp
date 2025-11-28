#include "kvstore.h"
#include "persistence.h"
#include "lru_cache.h"
#include <iostream>

KeyValueStore::KeyValueStore(LRUCache* cachePtr)
    : persistence(nullptr), cache(cachePtr) 
{
    if (cache) {
        cache->setEvictionCallback([this](const std::string& k) {
            this->onCacheEvict(k);
        });
    }
}

// ---------------- PUT ----------------
bool KeyValueStore::put(const std::string& key, const std::string& value, bool persist) {
    {
        std::unique_lock lock(mutex_);
        store[key] = value;
    }

    if (cache) cache->put(key, value);

    if (persist) onPut(key, value);
    return true;
}

// ---------------- GET ----------------
std::string KeyValueStore::get(const std::string& key, bool& found) {

    // TTL check
    if (isExpired(key)) {
        found = false;
        return "";
    }

    // Cache lookup
    if (cache) {
        std::string v;
        if (cache->get(key, v)) {
            found = true;
            return v;
        }
    }

    // Store lookup
    std::shared_lock lock(mutex_);
    auto it = store.find(key);

    if (it != store.end()) {
        found = true;
        if (cache) cache->put(key, it->second);
        return it->second;
    }

    found = false;
    return "";
}

// ---------------- DELETE ----------------
bool KeyValueStore::del(const std::string& key, bool persist) {
    {
        std::unique_lock lock(mutex_);
        if (store.erase(key) == 0) return false;
        expiry.erase(key);
    }

    if (cache) cache->remove(key);
    if (persist) onDelete(key);
    return true;
}

// ---------------- EXISTS ----------------
bool KeyValueStore::exists(const std::string& key) {
    if (isExpired(key)) return false;

    if (cache && cache->exists(key)) return true;

    std::shared_lock lock(mutex_);
    return store.find(key) != store.end();
}

// ---------------- SIZE ----------------
size_t KeyValueStore::size() {
    std::shared_lock lock(mutex_);
    return store.size();
}

// ---------------- SNAPSHOT ----------------
std::unordered_map<std::string, std::string> KeyValueStore::snapshot() {
    std::shared_lock lock(mutex_);
    return store;
}

// ---------------- WAL PERSISTENCE HOOKS ----------------
void KeyValueStore::setPersistence(Persistence* p) { persistence = p; }

void KeyValueStore::attachCache(LRUCache* cachePtr) {
    cache = cachePtr;
    if (cache) {
        cache->setEvictionCallback([this](const std::string& k) {
            this->onCacheEvict(k);
        });
    }
}

void KeyValueStore::onPut(const std::string& key, const std::string& value) {
    if (persistence) persistence->appendSet(key, value);
}

void KeyValueStore::onDelete(const std::string& key) {
    if (persistence) persistence->appendDel(key);
}

void KeyValueStore::onCacheEvict(const std::string& key) {
    std::unique_lock lock(mutex_);
    store.erase(key);
    expiry.erase(key);
}

// ------------------------------------------------------------
//                        TTL LOGIC
// ------------------------------------------------------------

void KeyValueStore::setTTL(const std::string& key, long long ttlSeconds) {
    std::unique_lock lock(mutex_);
    expiry[key] = nowMs() + ttlSeconds * 1000;
}

long long KeyValueStore::getTTL(const std::string& key) {
    std::shared_lock lock(mutex_);
    if (expiry.find(key) == expiry.end()) return -1;

    long long remaining = expiry[key] - nowMs();
    return remaining > 0 ? remaining / 1000 : 0;
}

bool KeyValueStore::isExpired(const std::string& key) {
    std::shared_lock lock(mutex_);
    auto it = expiry.find(key);
    if (it == expiry.end()) return false;

    bool expired = nowMs() > it->second;
    if (!expired) return false;

    lock.unlock();
    del(key, true);
    return true;
}

void KeyValueStore::cleanupExpired() {
    std::vector<std::string> expiredKeys;

    {
        std::shared_lock lock(mutex_);
        for (auto &p : expiry) {
            if (nowMs() > p.second) expiredKeys.push_back(p.first);
        }
    }

    for (auto &k : expiredKeys) {
        del(k, true);
    }
}

// ---------------- GET CACHE POINTER ----------------
LRUCache* KeyValueStore::getCache() const {
    return cache;
}