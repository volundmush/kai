#pragma once

// C++ STD libraries
#include <string>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <set>
#include <random>
#include <chrono>
#include <optional>
#include <filesystem>
#include <array>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <mutex>
#include <bitset>

// non-standard libraries
#include "fmt/core.h"
#include "fmt/printf.h"
#include <spdlog/spdlog.h>
#include "effolkronium/random.hpp"
using Random = effolkronium::random_static;

#include <boost/asio.hpp>

class GameObject;
class GameModule;
class PlayView;

template <typename Key, typename T>
class DebugMap : public std::map<Key, T> {
public:
    T& operator[](const Key& key) {
        if (key < 0) {
            throw std::runtime_error("Invalid key");
        }
        return std::map<Key, T>::operator[](key);
    }
};

template<typename T = bool>
using OpResult = std::pair<T, std::optional<std::string>>;

extern std::shared_ptr<spdlog::logger> logger;

template <typename Iterator, typename Key = std::function<std::string(typename std::iterator_traits<Iterator>::value_type)>>
Iterator partialMatch(
        const std::string& match_text,
        Iterator begin, Iterator end,
        bool exact = false,
        Key key = [](const auto& val){ return std::to_string(val); }
)
{
    // Use a multimap to automatically sort by the transformed key.
    using ValueType = typename std::iterator_traits<Iterator>::value_type;
    std::multimap<std::string, ValueType> sorted_map;
    std::for_each(begin, end, [&](const auto& val) {
        sorted_map.insert({key(val), val});
    });

    for (const auto& pair : sorted_map)
    {
        if (boost::iequals(pair.first, match_text))
        {
            return std::find(begin, end, pair.second);
        }
        else if (!exact && boost::istarts_with(pair.first, match_text))
        {
            return std::find(begin, end, pair.second);
        }
    }
    return end;
}
