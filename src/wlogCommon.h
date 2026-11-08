#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>

namespace wlog{
    enum class type {
        uint64,
        float32
    };
    enum class displayFormat {
        number,
        datetime
    };
    struct column{
        const char *id;
        const char *title;
        const char *unit;
        type dataType;
        displayFormat format;
        size_t offset;
    };
    inline double readValue(const uint8_t *rec, const column &c){
        switch (c.dataType){
            case type::uint64: {
                uint64_t v;
                memcpy(&v, rec + c.offset, sizeof(v));
                return static_cast<double>(v);
            }
            case type::float32: {
                float v;
                memcpy(&v, rec + c.offset, sizeof (v));
                return v;
            }
        }
        return NAN;
    }
    struct [[gnu::packed]] headerPrefix {
        char magic[4];
        uint16_t version;
        uint16_t recordSize;
    };
    enum class checkScope{
        record,
        column
    };
    enum class checkParam{
        none,
        number
    };
    struct validationCheck{
        const char *id;
        const char *title;
        const char *description;
        const char *group;
        checkScope scope;
        checkParam param;
        bool (*run)(const uint8_t *rec, const column *col, double param);
    };
    struct LogFormat{
        uint16_t version;
        uint16_t headerSize;
        uint16_t recordSize;
        const column *columns;
        size_t columnCount;
        bool (*validRecord)(const uint8_t *rec);
        const validationCheck *checks = nullptr;
        size_t checkCount = 0;
        bool resyncable = false; //can a good record be found after a bad record is found
    };

    inline bool passNotNaN(const uint8_t *r, const column *c, double){
        return !std::isnan(readValue(r, *c));
    }
    inline bool passMin(const uint8_t *r, const column *c, double p){
        double value = readValue(r, *c);
        return std::isnan(value) || value >= p;
    }
    inline bool passMax(const uint8_t *r, const column *c, double p){
        double value = readValue(r, *c);
        return std::isnan(value) || value <= p;
    }

    inline constexpr validationCheck numberChecks[] = {
        {"nan", "NaN records", "Value is not a number", "number", checkScope::column, checkParam::none, passNotNaN},
        {"min", "Below minimum", "Value is below the limit", "number", checkScope::column, checkParam::number, passMin},
        {"max", "Above maximum", "Value is above the limit", "number", checkScope::column, checkParam::number, passMin}
    };
}
