//
// Created by david on 2021/5/31.
//

#ifndef VMT_TOKENANALYSIS_H
#define VMT_TOKENANALYSIS_H

#define TOKEN_TYPE_DEFAULT 0
#define TOKEN_TYPE_EOL 1
#define TOKEN_TYPE_IDEOGRAPHIC 2

#define TOKEN_TYPE_COMMAND 128
#define TOKEN_TYPE_ENV_END 129
#define TOKEN_EOF 255

typedef struct {
    uint8_t tokenType;
    uint32_t charCode;
} Token;

void getNextToken();

//// 有限小数，前16位是整数部分，后16位是小数部分。
typedef int32_t fixed;

#endif //VMT_TOKENANALYSIS_H
