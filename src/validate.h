#pragma once
#include <vector>
#include "struc.h"

class QWidget;

void drawValidationWindow(QWidget *parent, const std::vector<wlogFileInfo> &files);