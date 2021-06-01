//
// Created by david on 2021/5/31.
//

// 内部使用的长度单位为万分之一毫米。

#include <stdint.h>

#include "pageList.h"

int32_t chineseFontSize = 37500; // 15级，约10.67 pt
int32_t westernFontSize; // 暂定
int32_t baseLineSkip = 60000; // 24级，即1.6倍行距

int32_t textWidth = 1500000; // 150毫米，即40字
int32_t textHeight = 2497500; // 42字+41行间距，即41行

int32_t leftMargin = 300000; // 30毫米
int32_t topMargin = 236250; // 23.625毫米