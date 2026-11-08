#pragma once
#include <vector>
#include "struc.h"
#include <functional>

class QWidget;

void drawValidationWindow(QWidget *parent, const std::vector<wlogFileInfo> &files, const std::function<void()> &onFilesChanged);