#pragma once
#include <cstdint>
#include <QFile>
#include "./wlogV1.h"
#include "./struc.h"

bool getRecord(uint64_t globalRow, const std::vector<wlogFileInfo> &files, wlogv1::record &output);