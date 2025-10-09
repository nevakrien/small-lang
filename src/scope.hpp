#pragma once

#include <vector>
#include <map>
#include <string_view>

namespace small_lang {

template <typename T>
class Scope {
    std::vector<std::map<std::string_view, T>> parts;

public:
	void clear() { parts.clear(); parts.emplace_back();}
    void push()  { parts.emplace_back(); }
    void pop()   { parts.pop_back(); }

    Scope(){push();}

    T* end() {return nullptr;}
    T* find(std::string_view k) {
        for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
            auto found = it->find(k);
            if (found != it->end())
                return &found->second;
        }
        return nullptr;
    }

    void insert(std::string_view k, const T& v) {
        parts.back()[k] = v;
    }

    T& operator[](std::string_view k) {
        return parts.back()[k];
    }

    //const lookup variant (throws if not found)
    const T& operator[](std::string_view k) const {
        for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
            auto found = it->find(k);
            if (found != it->end())
                return found->second;
        }
        throw std::out_of_range("Scope: key not found in any scope");
    }
};

}