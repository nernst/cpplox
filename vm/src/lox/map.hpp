#pragma once

#include "common.hpp"
#include "value.hpp"
#include "object.hpp"
#include <memory>
#include <iosfwd>

namespace lox
{
    class Map
    {
    public:

        static constexpr const double max_load = 0.75;

        struct Entry
        {
            String const* key;
            Value value;

            Entry()
            : key{nullptr}
            , value{}
            {}

            ~Entry(){}

            Entry(Entry const&) = delete;
            Entry(Entry&&) = delete;
            Entry& operator=(Entry const&) = delete;
            Entry& operator=(Entry&&) = default;
        };

        Map()
        : size_{0}
        , capacity_{0}
        {}

        Map(Map const& other)
        : size_{0}
        , capacity_{other.capacity_}
        , entries_{new Entry[capacity_]}
        {
            copy(other.entries_.get(), capacity_);
        }
        
        Map(Map&& move)
        : size_{move.size_}
        , capacity_{move.capacity_}
        , entries_{std::move(move.entries_)}
        {
            move.size_ = 0;
            move.capacity_ = 0;
            move.entries_ = nullptr;
        }

        ~Map() = default;

        Map& operator=(Map const& other)
        {
            if (this != &other) {
                size_ = 0;
                capacity_ = other.capacity_;
                entries_.reset(new Entry[capacity_]);
                copy(other.entries_.get(), capacity_);
            }
            return *this;
        }
        Map& operator=(Map&& from) {
            if (this != &from) {
                std::swap(size_, from.size_);
                std::swap(capacity_, from.capacity_);
                std::swap(entries_, from.entries_);
            }
            return *this;
        }

        size_t size() const { return size_; }
        bool empty() const { return size() == 0; }

        template<typename V>
        bool add(String const* key, V&& value)
        {
            if (size_ + 1 > capacity_ * max_load) {
                grow_capacity();
            }
            Entry* entry = find_entry(key->view());
            const bool is_new = entry->key == nullptr;
            if (is_new && entry->value.is_nil())
                ++size_;

            entry->key = key;
            entry->value = std::forward<V>(value);
            return is_new;
        }

        bool get(String const* key, Value& out) const {
            if (empty())
                return false;

            Entry const* entry = find_entry(key->str());
            if (entry->key == nullptr)
                return false;

            out = entry->value;
            return true;
        }

        bool remove(String const* key) {
            if (empty())
                return false;

            Entry* entry = find_entry(key->view());
            if (entry->key == nullptr)
                return false;
            
            entry->key = nullptr;
            entry->value = Value{true};
            return true;
        }


    private:
        size_t size_;
        size_t capacity_;
        std::unique_ptr<Entry[]> entries_;

        Entry const* find_entry(std::string_view key) const
        {
            return const_cast<Map*>(this)->find_entry(key);
        }

        Entry* find_entry(std::string_view key)
        {
            const size_t hash{lox::hash(key)};
            size_t index{hash % capacity_};
            Entry* tombstone{nullptr};

            while (true)
            {
                Entry* entry = entries_.get() + index;
                if (entry->key == nullptr)
                {
                    if (entry->value.is_nil())
                    {
                        return tombstone != nullptr ? tombstone : entry;
                    }
                    else
                    {
                        if (tombstone == nullptr)
                            tombstone = entry;
                    }
                }
                else if (entry->key->view() == key)
                {
                    return entry;
                }
                index = (index + 1) % capacity_;
            }
        }

        void copy(Entry const* from, size_t count) {
            for (size_t i = 0; i < count; ++i) {
                if (from[i].key == nullptr)
                    continue;

                add(from[i].key, from[i].value);
            }

        }

        void grow_capacity() {
            const size_t old_capacity{capacity_};
            capacity_ = std::max(16ul, old_capacity * 2);
            size_ = 0;
            std::unique_ptr<Entry[]> old_entries = std::move(entries_);
            entries_.reset(new Entry[capacity_]);
            copy(old_entries.get(), old_capacity);
        }

        friend std::ostream& operator<<(std::ostream& os, Map const& map);
    };
}