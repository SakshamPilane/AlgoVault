#include "persistence.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <cstdio>      // perror
#if defined(__unix__) || defined(__APPLE__)
    #include <unistd.h>   // for fsync on POSIX
    #include <fcntl.h>
#endif
#include "json.hpp"
using json = nlohmann::json;

Persistence::Persistence(const std::string& path) : filepath(path) {
    // Create file if not exist (best-effort)
    std::ofstream ofs(filepath, std::ios::app);
    ofs.flush();
    ofs.close();
}

Persistence::~Persistence() {
    // nothing special
}

void Persistence::doFsync(std::ofstream& ofs) {
#if defined(__unix__) || defined(__APPLE__)
    ofs.flush();
    // Try to open the file and fsync the descriptor.
    int fd = open(filepath.c_str(), O_WRONLY);
    if (fd >= 0) {
        fsync(fd);
        close(fd);
    }
#else
    // On Windows we simply flush (could call _commit on FILE* if desired)
    ofs.flush();
#endif
}

bool Persistence::appendSet(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lg(fileMutex);
    try {
        std::ofstream ofs(filepath, std::ios::app);
        if (!ofs.is_open()) {
            std::cerr << "[WAL] cannot open file for append: " << filepath << "\n";
            return false;
        }

        json j;
        j["op"] = "SET";
        j["key"] = key;
        j["value"] = value;
        j["ts"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

        ofs << j.dump() << "\n";
        ofs.flush();
        doFsync(ofs);
        ofs.close();
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[WAL] appendSet exception: " << ex.what() << "\n";
        return false;
    }
}

bool Persistence::appendDel(const std::string& key) {
    std::lock_guard<std::mutex> lg(fileMutex);
    try {
        std::ofstream ofs(filepath, std::ios::app);
        if (!ofs.is_open()) {
            std::cerr << "[WAL] cannot open file for append: " << filepath << "\n";
            return false;
        }

        json j;
        j["op"] = "DEL";
        j["key"] = key;
        j["ts"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

        ofs << j.dump() << "\n";
        ofs.flush();
        doFsync(ofs);
        ofs.close();
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[WAL] appendDel exception: " << ex.what() << "\n";
        return false;
    }
}

bool Persistence::replay(const std::function<void(const std::string&, const std::string&)>& setCb,
                         const std::function<void(const std::string&)>& delCb) {
    std::lock_guard<std::mutex> lg(fileMutex);
    try {
        std::ifstream ifs(filepath);
        if (!ifs.is_open()) {
            std::cerr << "[WAL] replay: cannot open file: " << filepath << "\n";
            return false;
        }

        std::string line;
        while (std::getline(ifs, line)) {
            if (line.empty()) continue;
            try {
                json j = json::parse(line);
                std::string op = j.value("op", "");
                if (op == "SET") {
                    std::string key = j.value("key", "");
                    std::string value = j.value("value", "");
                    setCb(key, value);
                } else if (op == "DEL") {
                    std::string key = j.value("key", "");
                    delCb(key);
                } else {
                    // unknown op — ignore
                }
            } catch (const std::exception& ex) {
                std::cerr << "[WAL] skipping invalid line: " << ex.what() << "\n";
            }
        }
        ifs.close();
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[WAL] replay exception: " << ex.what() << "\n";
        return false;
    }
}

bool Persistence::compact(const std::unordered_map<std::string, std::string>& snapshot) {
    std::lock_guard<std::mutex> lg(fileMutex);
    std::string tmpPath = filepath + ".tmp";

    try {
        // write snapshot to tmp file as SET entries
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs.is_open()) {
            std::cerr << "[WAL] compact: cannot open tmp file: " << tmpPath << "\n";
            return false;
        }

        for (const auto& kv : snapshot) {
            json j;
            j["op"] = "SET";
            j["key"] = kv.first;
            j["value"] = kv.second;
            j["ts"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
            ofs << j.dump() << "\n";
        }
        ofs.flush();
        doFsync(ofs);
        ofs.close();

        // replace original file with tmp
        if (std::rename(tmpPath.c_str(), filepath.c_str()) != 0) {
            std::perror("rename");
            std::cerr << "[WAL] compact: rename failed\n";
            // try to remove tmp
            std::remove(tmpPath.c_str());
            return false;
        }
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[WAL] compact exception: " << ex.what() << "\n";
        return false;
    }
}

std::string Persistence::path() const {
    return filepath;
}
