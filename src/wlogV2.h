#pragma once
#include <cstdint>
#include <cstring>
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

    inline uint32_t crc32(const uint8_t *p, size_t n){
        uint32_t c = 0xFFFFFFFF;
        for (size_t i = 0; i < n; i++){
            c ^= p[i];
            for (int k = 0; k < 8; k++){
                c = (c >> 1) ^ (0xEDB88320 & (0 - (c &  1)));
            }
        }
        return ~c;
    }

    inline bool passSignatures(const uint8_t *rec, const wlog::column *, double){
        return validRecord(rec);
    }

    inline bool passCrc(const uint8_t *rec, const wlog::column *, double){
        uint32_t stored;
        memcpy(&stored, rec + offsetof(record, crc32), sizeof(stored));
        return crc32(rec + offsetof(record, timestamp), offsetof(record, crc32) - offsetof(record, timestamp)) == stored;
    }

    inline constexpr wlog::column columns[] = {
        {"time", "Time", "HH:mm:ss; dd MM yy", wlog::type::uint64, wlog::displayFormat::datetime, offsetof(record, timestamp)},
        {"temperature", "Temperature", "DEG C", wlog::type::float32, wlog::displayFormat::number, offsetof(record, temperature), 0, 50},
        {"humidity", "Humidity", "% RH", wlog::type::float32, wlog::displayFormat::number, offsetof(record, humidity), 0, 100},
        {"pressure", "Pressure", "hPa", wlog::type::float32, wlog::displayFormat::number, offsetof(record, pressure), 800, 2000} //see wlogv1.h for notes on defaults
    };

    inline constexpr wlog::validationCheck checks[] = {
        {"signature", "Bad signatures", "Start or end signature is malformed", "Record", wlog::checkScope::record, wlog::checkParam::none, passSignatures},
        {"crc", "CRC mismatch", "CRC32 checksum does not match record content", "Record", wlog::checkScope::record, wlog::checkParam::none, passCrc}
    };
    inline constexpr size_t checkCount = sizeof(checks) / sizeof(checks[0]);
    inline constexpr size_t columnCount = sizeof(columns) / sizeof(columns[0]);
    inline constexpr wlog::LogFormat logFormat = {2, 8, sizeof(record), columns, columnCount, validRecord, checks, checkCount, true};
}

static_assert(sizeof(wlogv2::header) == 8);
static_assert(sizeof(wlogv2::record) == 28);