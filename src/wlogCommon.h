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
    struct LogFormat{
        uint16_t version;
        uint16_t headerSize;
        uint16_t recordSize;
        const column *columns;
        size_t columnCount;
        bool (*validRecord)(const uint8_t *rec);
    };
}
