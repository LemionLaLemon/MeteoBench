#pragma once
#include <cstdint>
#include <cstddef>
#include "wlogCommon.h"

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
    inline constexpr wlog::column columns[] = {
        {"time", "Time", "HH:mm:ss; dd MM yy", wlog::type::uint64, wlog::displayFormat::datetime, offsetof(record, timestamp)},
        {"temperature", "Temperature", "DEG C", wlog::type::float32, wlog::displayFormat::number, offsetof(record, temperature), 0, 50}, //no one will ever agree on a default, 0 is reasonable for thailand
        {"humidity", "Humidity", "% RH", wlog::type::float32, wlog::displayFormat::number, offsetof(record, humidity), 0, 100}, //if your humidity is -1 or 101 you have horrible horrible problems
        {"pressure", "Pressure", "hPa", wlog::type::float32, wlog::displayFormat::number, offsetof(record, pressure), 800, 2000}
    };
    inline constexpr size_t columnCount = sizeof(columns) / sizeof(columns[0]);
    inline constexpr wlog::LogFormat logFormat = {1, sizeof(header), sizeof(record), columns, columnCount, nullptr, {}, 0, false};
}

static_assert(sizeof(wlogv1::header) == 8);
static_assert(sizeof(wlogv1::record) == 20);