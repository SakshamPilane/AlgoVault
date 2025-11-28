#include "server.h"
#include "kvstore.h"
#include "persistence.h"
#include "json.hpp"
#include "httplib.h"
#include "lru_cache.h"
#include <iostream>

using json = nlohmann::json;

void startServer(KeyValueStore &store, Persistence &wal) {
    httplib::Server svr;

    // ----------- PUT (supports ttl) -----------
    svr.Post("/put", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            json body = json::parse(req.body);

            std::string key = body["key"];
            std::string value = body["value"];

            store.put(key, value);

            if (body.contains("ttl")) {
                long long ttl = body["ttl"];
                store.setTTL(key, ttl);
            }

            res.set_content(R"({"status":"OK","message":"Key added"})", "application/json");
        }
        catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"Invalid JSON"})", "application/json");
        }
    });

    // ----------- GET -----------
    svr.Get("/get", [&](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("key")) {
            res.status = 400;
            res.set_content(R"({"error":"Missing key"})", "application/json");
            return;
        }

        std::string key = req.get_param_value("key");
        bool found = false;
        std::string value = store.get(key, found);

        json resp;
        resp["found"] = found;
        resp["key"] = key;
        if (found) resp["value"] = value;

        res.set_content(resp.dump(), "application/json");
    });

    // ----------- DELETE -----------
    svr.Delete("/delete", [&](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("key")) {
            res.status = 400;
            res.set_content(R"({"error":"Missing key"})", "application/json");
            return;
        }

        std::string key = req.get_param_value("key");
        bool ok = store.del(key);

        json resp = { {"deleted", ok}, {"key", key} };
        res.set_content(resp.dump(), "application/json");
    });

    // ----------- COMPACT WAL -----------
    svr.Post("/compact", [&](const httplib::Request &, httplib::Response &res) {
        auto snap = store.snapshot();
        bool ok = wal.compact(snap);

        json resp = { {"compacted", ok} };
        res.set_content(resp.dump(), "application/json");
    });

    // ----------- WAL/STORE STATS -----------
    svr.Get("/stats", [&](const httplib::Request &, httplib::Response &res) {
        json resp = {
            {"keys", store.size()},
            {"wal_path", wal.path()}
        };
        res.set_content(resp.dump(), "application/json");
    });

    // ----------- CACHE STATS -----------
    svr.Get("/cache/stats", [&](const httplib::Request &, httplib::Response &res) {
        LRUCache* c = store.getCache();
        if (!c) {
            res.status = 404;
            res.set_content(R"({"error":"no cache attached"})", "application/json");
            return;
        }

        auto s = c->getStats();
        json resp = {
            {"hits", s.hits},
            {"misses", s.misses},
            {"evictions", s.evictions},
            {"items", c->size()}
        };
        res.set_content(resp.dump(), "application/json");
    });

    // ----------- RESET CACHE STATS -----------
    svr.Post("/cache/stats/reset", [&](const httplib::Request &, httplib::Response &res) {
        LRUCache* c = store.getCache();
        if (!c) {
            res.status = 404;
            res.set_content(R"({"error":"no cache attached"})", "application/json");
            return;
        }

        c->resetStats();
        auto s = c->getStats();
        json resp = {
            {"reset", true},
            {"hits", s.hits},
            {"misses", s.misses},
            {"evictions", s.evictions},
            {"items", c->size()}
        };
        res.set_content(resp.dump(), "application/json");
    });

    // ----------- TTL LOOKUP -----------
    svr.Get("/ttl", [&](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("key")) {
            res.status = 400;
            res.set_content(R"({"error":"Missing key"})", "application/json");
            return;
        }

        std::string key = req.get_param_value("key");
        long long ttl = store.getTTL(key);

        json resp = {
            {"key", key},
            {"ttl", ttl}
        };
        res.set_content(resp.dump(), "application/json");
    });

    std::cout << "[Server] Running at http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
}
