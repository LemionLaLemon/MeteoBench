#pragma once
#include <cstdint>
#include <cstddef>

namespace wlogv1{
    struct [[gnu::packed]] header {
        char magic[4];
        uint16_t version;
        uint16_t recordSize;
    };
    struct [[gnu::packed]] record {
        uint64_t timestamp;
        float temperature;
        float humidity;
        float pressure;
    };
    enum class type {
        uint64,
        float32
    };
    enum class format {
        number,
        datetime
    };
    struct column {
        const char *id;
        const char *title;
        const char *unit;
        type dataType;
        format displayFormat;
        size_t offset;
    };
    inline constexpr column columns[] = {
        {"time", "Time", "HH:mm:ss; dd MM yy", type::uint64, format::datetime, offsetof(record, timestamp)},
        {"temperature", "Temperature", "DEG C", type::float32, format::number, offsetof(record, temperature)},
        {"humidity", "Humidity", "% RH", type::float32, format::number, offsetof(record, humidity)},
        {"pressure", "Pressure", "hPa", type::float32, format::number, offsetof(record, pressure)}
    };
    inline constexpr size_t columnCount = sizeof(columns) / sizeof(columns[0]);
}

static_assert(sizeof(wlogv1::header) == 8);
static_assert(sizeof(wlogv1::record) == 20);