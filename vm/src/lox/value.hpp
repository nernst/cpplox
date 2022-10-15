#include "common.hpp"
#include <cassert>
#include <vector>

namespace lox
{
    typedef double Value;

    void print_value(Value value);

    class ValueArray
    {
    public:
        ValueArray() = default;
        ValueArray(ValueArray const&) = default;
        ValueArray(ValueArray&&) = default;

        ~ValueArray() = default;

        ValueArray& operator=(ValueArray const&) = default;
        ValueArray& operator=(ValueArray&&) = default;

        void write(Value value) { data_.push_back(value); }
        size_t size() const { return data_.size(); }

        Value operator[](size_t index) const
        {
            assert(index < data_.size());
            return data_[index];
        }

    private:
        std::vector<Value> data_;
    };
 
}