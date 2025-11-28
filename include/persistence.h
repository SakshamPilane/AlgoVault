#pragma once
#include <string>
#include <functional>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <fstream>

struct LogEntry {
    std::string op;   // "SET" or "DEL"
    std::string key;
    std::string value; // empty for DEL
    long long ts;
};

class Persistence {
public:
    // path: path to wal file (e.g., "data/wal.log")
    explicit Persistence(const std::string& path);

    ~Persistence();

    // append a SET operation
    bool appendSet(const std::string& key, const std::string& value);

    // append a DEL operation
    bool appendDel(const std::string& key);

    // replay the WAL. callbacks are invoked in file order.
    // setCb: (key, value) for SET
    // delCb: (key) for DEL
    bool replay(const std::function<void(const std::string&, const std::string&)>& setCb,
                const std::function<void(const std::string&)>& delCb);

    // compact: overwrite wal with snapshot (map of current key->value)
    // The snapshot should be consistent (caller provides).
    bool compact(const std::unordered_map<std::string, std::string>& snapshot);

    // Get path (for debugging)
    std::string path() const;

private:
    std::string filepath;
    std::mutex fileMutex;

    // helper: fsync after flush (posix available)
    void doFsync(std::ofstream& ofs);
};
