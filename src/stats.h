#pragma once
#include "wlogCommon.h"
#include "wlogV1.h"
#include "wlogTableModel.h"

columnStats combineStats(const std::vector<wlogFileInfo> &files, int column);

void computeStats(wlogFileInfo &info);