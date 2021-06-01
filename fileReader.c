//
// Created by david on 2021/5/29.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fileReader.h"

#define ERROR_CHAR 0xFFFD

#define HALF_CHARACTER_ERROR "文件在不合法位置处终结"

FILE* inFile;

int32_t charCodeBuffer[3];
int charCodeBufferCurrentPos;

void initFileReading(char* fileName)
{
    inFile = fopen(fileName, "r");
    charCodeBufferCurrentPos = 3;
}

void finFileReading()
{
    fclose(inFile);
}

/**
 * 读取下一个字节中的6位（用于UTF8编码的读取）。
 * @return
 */
inline static uint8_t getSixBit()
{
    int8_t tmp = fgetc(inFile);
    if (tmp == EOF)
    {
        fputs(HALF_CHARACTER_ERROR, stderr);
        return -1;
    }
    if ((tmp & 0xC0) != 0x80)
    {
        fprintf(stderr, "不合法的字节 <%2x>", (uint8_t) tmp);
        return -1;
    }
    return tmp & 0x3F;
}

/**
 * 从UTF8编码的文件中读取下一个Unicode字符。
 * @return
 */
int32_t getNextUTF8Char()
{
    static const int32_t FW_FFEX[6] = {0xA2, 0xA3, 0xAC, 0xAF, 0xA6, 0xA5};

    int8_t tmp;
    uint32_t result;
    int8_t byteLeft;

    if (charCodeBufferCurrentPos < 3) return charCodeBuffer[charCodeBufferCurrentPos++];

    tmp = fgetc(inFile);
    if (tmp == EOF) return EOF; // 文件结束
    if (tmp > 0) return tmp; // 单字节字符
    if (tmp < -64 || tmp >= -8) // 不合法的第一字节
    {
        fprintf(stderr, "不合法的字节 <%2x>", (uint8_t) tmp);
        return ERROR_CHAR;
    }
    if (tmp < -32) // 二字节
        byteLeft = 1;
    else if (tmp < -16) // 三字节
        byteLeft = 2;
    else // 四字节
        byteLeft = 3;
    result = 63 >> byteLeft;
    result = result & tmp; // 取出首字节需要的内容
    for (int i = 0; i < byteLeft; ++i)
    {
        tmp = getSixBit();
        if (tmp == -1) return ERROR_CHAR; // 不合法的UTF-8字符
        result = (result << 6) + tmp;
    }
    // 对码点做归一化
    if (result == 0x002D) return 0x2010; // 半角hyphen-minus作为连字符
    if (result == 0xFF0D) return 0x2212; // 全角hyphen-minus作为减号
    if (result >= 0xFFE0 && result < 0xFFE6) return FW_FFEX[result - 0xFFE0]; // 部分全角字符

    if (result >= 0xFF00 && result < 0xFF5F) result -= (0xFF00 - 0x0020); // 全角字符的半角化

    return result;
}