#pragma once
#include <vector>
#include "struc.h"
#include <QDebug>

class QWidget;

void drawValidationWindow(QWidget *parent, const std::vector<wlogFileInfo> &files);