#pragma once
#include "wlogCommon.h"
#include "wlogTableModel.h"

columnStats combineStats(const std::vector<wlogFileInfo> &files, int column);

void computeStats(wlogFileInfo &info);