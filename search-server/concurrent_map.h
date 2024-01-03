#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct SubMap {
        std::mutex mutex;
        std::map<Key, Value> map;
    };

    struct Access {
        private:
            std::lock_guard<std::mutex> mutex_;
        public:
            Value& ref_to_value;

            Access(SubMap& submap, const Key& key) : mutex_{submap.mutex}, ref_to_value{submap.map[key]} {}
    };

    explicit ConcurrentMap(size_t bucket_count) : maps_{bucket_count} {}

    Access operator[](const Key& key) {
        SubMap& submap = maps_[static_cast<uint64_t>(key) % maps_.size()];
        return {submap, key};
    }

    size_t Erase(const Key& key) {
        SubMap& submap = maps_[static_cast<uint64_t>(key) % maps_.size()];
        std::lock_guard<std::mutex> mutex_{submap.mutex};
        return submap.map.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : maps_) {
            std::lock_guard<std::mutex> mutex_{mutex};
            result.insert(begin(map), end(map));
        }
        return result;
    }

private:
    std::vector<SubMap> maps_;
};

template <typename Key>
class ConcurrentSet {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct SubSet {
        std::mutex mutex;
        std::set<Key> set;
    };

    struct Access {
        private:
            std::lock_guard<std::mutex> mutex_;
        public:
            Key& ref_to_value;

            Access(SubSet& subset, const Key& key) : mutex_{subset.mutex}, ref_to_value{*(subset.set.insert(key).first)} {}
    };

    explicit ConcurrentSet(size_t bucket_count) : sets_{bucket_count} {}

    Access operator[](const Key& key) {
        SubSet& subset = sets_[static_cast<uint64_t>(key) % sets_.size()];
        return {subset, key};
    }

    std::set<Key> BuildOrdinarySet() {
        std::set<Key> result;
        for (auto& [mutex, set] : sets_) {
            std::lock_guard<std::mutex> mutex_{mutex};
            result.insert(begin(set), end(set));
        }
        return result;
    }

private:
    std::vector<SubSet> sets_;
};