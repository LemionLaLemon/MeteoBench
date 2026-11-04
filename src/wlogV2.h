#pragma once
#include <cstdint>
#include <cstddef>
#include "wlogCommon.h"

namespace wlogv2{
    struct [[gnu::packed]] header {
        char magic[4];
        uint16_t version;
        uint16_t recordSize;
    };
    struct [[gnu::packed]] record {
        uint8_t startSignature[2];
        uint64_t timestamp;
        float temperature;
        float humidity;
        float pressure;
        uint32_t crc32;
        uint8_t endSignature[2];
    };
    inline bool validRecord(const uint8_t *rec){
        return rec[0] == 0xA5 && rec[1] == 0x5A && rec[offsetof(record, endSignature)] == 0x5A && rec[offsetof(record, endSignature) + 1] == 0xA5;
    }
    inline constexpr wlog::column columns[] = {
        {"time", "Time", "HH:mm:ss; dd MM yy", wlog::type::uint64, wlog::displayFormat::datetime, offsetof(record, timestamp)},
        {"temperature", "Temperature", "DEG C", wlog::type::float32, wlog::displayFormat::number, offsetof(record, temperature)},
        {"humidity", "Humidity", "% RH", wlog::type::float32, wlog::displayFormat::number, offsetof(record, humidity)},
        {"pressure", "Pressure", "hPa", wlog::type::float32, wlog::displayFormat::number, offsetof(record, pressure)}
    };
    inline constexpr size_t columnCount = sizeof(columns) / sizeof(columns[0]);
    inline constexpr wlog::LogFormat logFormat = {2, 8, sizeof(record), columns, columnCount, validRecord};
}

static_assert(sizeof(wlogv2::header) == 8);
static_assert(sizeof(wlogv2::record) == 28);