#pragma once
#include "wlogV1.h"
#include "wlogV2.h"

inline const wlog::LogFormat *findFormat(uint16_t version){
    static const wlog::LogFormat *all[] = {&wlogv1::logFormat, &wlogv2::logFormat};

    for (const auto *f : all){
        if (f->version == version){
            return f;
        }
    }
    return nullptr;
}