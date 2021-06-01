//
// Created by david on 2021/5/31.
//

#include <stdio.h>
#include <stdint.h>

#include "fileReader.h"
#include "tokenAnalysis.h"

Token curToken;

extern int32_t charCodeBuffer[3];
extern int charCodeBufferCurrentPos;

#define inUniCJK(x) ((x >= 0x4E00) && (x <= 0x9FFF))
#define inExtA(x) ((x >= 0x3400) && (x <= 0x4DBF))
#define inSupplementary(x) ((x >= 0x20000) && (x <= 0x3134A))

inline static uint8_t getCatCodeFromUCode(int32_t uCode)
{
    if (uCode == '\n') return TOKEN_TYPE_EOL;
    if (inUniCJK(uCode) || inExtA(uCode) || inSupplementary(uCode)) return TOKEN_TYPE_IDEOGRAPHIC;

    return TOKEN_TYPE_DEFAULT;
}

void getNextToken()
{
    int32_t uChar = getNextUTF8Char();

    // 文档结束符
    if (uChar == EOF) curToken.tokenType = TOKEN_EOF;

    if (uChar == 0x005B)
    {
        // 读取三个字符，检查第三个
        for (int i=0; i<3; ++i)
            charCodeBuffer[i] = getNextUTF8Char();
        if (charCodeBuffer[2] == 0x301B) // 完成"环境终"的结构
        {
            curToken.tokenType = TOKEN_TYPE_ENV_END;
            curToken.charCode = (charCodeBuffer[0] << 16) + charCodeBuffer[1];
            charCodeBufferCurrentPos = 3;
            return;
        }
        else charCodeBufferCurrentPos = 0; // 重新使用缓冲区
    }
    else if (uChar == 0x301A)
    {
        // 直接输出两个字符，将右括号留待interpret时考虑（因为有参数）
        curToken.tokenType = TOKEN_TYPE_COMMAND;
        curToken.charCode = getNextUTF8Char() << 16;
        curToken.charCode += getNextUTF8Char(); // 两个汉字用一个32位整数表示
        return;
    }
    // 普通情况
    curToken.tokenType = getCatCodeFromUCode(uChar);
    curToken.charCode = uChar;
}

void executeToken()
{
}