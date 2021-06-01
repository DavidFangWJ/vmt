//
// Created by david on 2021/5/21.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <fontconfig/fontconfig.h>

#include "fontObject.h"

FcConfig* config;

inline static void deleteFont(Font* f)
{
    fclose(f->fontFile);
    free(f->tableRecords);
    if (f->cmapTableType == 4)
    {
        struct Type4CmapTable* p = &f->cmap.type4;
        free(p->endCode);
        free(p->startCode);
        free(p->idDelta);
        free(p->idRangeOffset);
        free(p->glyphIdArray);
    }
    if (f->cmapTableType == 12)
    {
        struct Type12CmapTable* p = &f->cmap.type12;
        free(p->startCode);
        free(p->endCode);
        free(p->startGID);
    }
}

// hash table
struct FontNode {
    char fontName[64];
    char path[256];
    int index;
    Font current;
    struct FontNode* next;
};

inline static unsigned hashFromString(char* str)
{
    unsigned hash = 0;
    while (*str)
    {
        hash *= 31;
        hash += *str++;
    }
    return hash % 64;
}

struct FontNode* fontLibrary[64];

void initiateFontLibrary()
{
    config = FcInitLoadConfigAndFonts();
    for (int i=0; i<64; ++i) fontLibrary[i] = NULL;
}

void deleteFontLibrary()
{
    for (int i=0; i<64; ++i)
    {
        struct FontNode* current = fontLibrary[i], *next;
        while (current)
        {
            next = current->next;
            deleteFont(&current->current);
            free(current);
            current = next;
        }
    }
}

inline static void switchEndian16(uint16_t* num)
{
    static uint8_t c1, c2;
    c1 = *num & 0xFFu;
    c2 = (*num & 0xFF00u) >> 8u;
    *num = (c1 << 8u) + c2;
}

inline static void switchEndian32(uint32_t* num)
{
    static uint8_t c1, c2, c3, c4;
    c1 = *num & 0xFFu;
    c2 = (*num & 0xFF00u) >> 8u;
    c3 = (*num & 0xFF0000u) >> 16u;
    c4 = (*num & 0xFF000000u) >> 24u;
    *num = (c1 << 24u) + (c2 << 16u) + (c3 << 8u) + c4;
}

inline static uint16_t readUint16BE(FILE* f)
{
    static uint8_t c1, c2;
    c1 = fgetc(f);
    c2 = fgetc(f);
    return (c1 << 8u) + c2;
}

inline static uint32_t readUint32BE(FILE* f)
{
    static uint8_t c[4];
    fread(c, 1, 4, f);
    return (c[0] << 24u) + (c[1] << 16u) + (c[2] << 8u) + c[3];
}

uint16_t findIndexOfTable(Font* obj, const char* tagStr)
{
    uint32_t tag = (tagStr[0] << 24) + (tagStr[1] << 16) + (tagStr[2] << 8) + tagStr[3];
    // 二分查找
    uint16_t left = 0, right = obj->numTables - 1, mid = (left + right) / 2;
    while (left < right)
    {
        uint32_t current = obj->tableRecords[mid].tableTag;
        if (current == tag) break;
        if (current > tag) right = mid - 1;
        else left = mid + 1;
        mid = (left + right) / 2;
    }
    return mid;
}

inline static uint32_t findOffsetOfTable(Font* obj, const char* tagStr)
{
    uint16_t index = findIndexOfTable(obj, tagStr);
    return obj->tableRecords[index].offset;
}

struct CmapTableRecord {
    uint16_t platfotmID;
    uint16_t encodingID;
    uint32_t offset;
};

/**
 * 读取4型表格。
 * @param table 读取的位置（已构造）
 * @param fontFile 字体文件（已移位）
 */
void readType4Table(struct Type4CmapTable* table, FILE* fontFile)
{
    uint16_t length, doubleSegCount;
    fseek(fontFile, 2, SEEK_CUR);
    length = readUint16BE(fontFile);
    fseek(fontFile, 2, SEEK_CUR);
    doubleSegCount = readUint16BE(fontFile);
    fseek(fontFile, 6, SEEK_CUR);
    length -= 4 * doubleSegCount + 16;

    table->segCount = doubleSegCount / 2;
    table->endCode = malloc(doubleSegCount);
    table->startCode = malloc(doubleSegCount);
    table->idDelta = malloc(doubleSegCount);
    table->idRangeOffset = malloc(doubleSegCount);

    fread(table->endCode, doubleSegCount, 1, fontFile);
    fseek(fontFile, 2, SEEK_CUR);
    fread(table->startCode, doubleSegCount, 1, fontFile);
    fread(table->idDelta, doubleSegCount, 1, fontFile);
    fread(table->idRangeOffset, doubleSegCount, 1, fontFile);
    for (int i = 0; i < table->segCount; ++i)
    {
        switchEndian16(table->endCode + i);
        switchEndian16(table->startCode + i);
        switchEndian16(table->idDelta + i);
        switchEndian16(table->idRangeOffset + i);
    }

    if (length > 0)
    {
        table->glyphIdArray = malloc(length);
        length /= 2;
        for (uint16_t i = 0; i < length; ++i)
            table->glyphIdArray[i] = readUint16BE(fontFile);
    }
    else table->glyphIdArray = NULL;
}

void readType12Table(struct Type12CmapTable* table, FILE* fontFile)
{
    fseek(fontFile, 12, SEEK_CUR);
    table->numGroups = readUint32BE(fontFile);
    table->startCode = malloc(table->numGroups * 4);
    table->endCode   = malloc(table->numGroups * 4);
    table->startGID  = malloc(table->numGroups * 4);

    for (int i=0; i<table->numGroups; ++i)
    {
        table->startCode[i] = readUint32BE(fontFile);
        table->endCode[i] = readUint32BE(fontFile);
        table->startGID[i] = readUint32BE(fontFile);
    }
}

/**
 * 给半成品的字体对象寻找合适的cmap表。
 * @param obj 字体对象
 * @return 初始化完成的cmap表
 */
void initCmapTable(Font* f)
{
    union FontCmapTable* result = malloc(sizeof(union FontCmapTable));

    uint32_t offsetCmap = findOffsetOfTable(f, "cmap");
    fseek(f->fontFile, offsetCmap + 2, SEEK_SET);
    uint16_t numTables = readUint16BE(f->fontFile);

    struct CmapTableRecord* records = malloc(sizeof(struct CmapTableRecord) * numTables);
    fread(records, sizeof(struct CmapTableRecord), numTables, f->fontFile);
    struct CmapTableRecord* end = records + numTables;
    struct CmapTableRecord* required = NULL;
    for (required = records; required != end; ++required)
    {
        uint16_t val = required->platfotmID + (required->encodingID >> 8u);
        if (val == 0x0301u) f->cmapTableType = 4;
        if (val == 0x0310u) f->cmapTableType = 12;
        if (val > 0x0310u) break;
    }
    switchEndian32(&required->offset);
    offsetCmap += required->offset;
    fseek(f->fontFile, offsetCmap, SEEK_SET);

    switch (f->cmapTableType)
    {
        case 4:
            readType4Table(&result->type4, f->fontFile);
            break;
        case 12:
            readType12Table(&result->type12, f->fontFile);
            break;
        default: // 没有合适的内容
            fprintf(stderr, "字体没有合适的cmap表。\n");
            exit(1);
    }
}

// 读取SFNT格式的字体
Font* fontFromFile(char* fontName)
{
    unsigned hash = hashFromString(fontName);
    struct FontNode* current = fontLibrary[hash];
    // 如果没有这个hash值，自然不会进入循环
    while (current)
    {
        if (!strcmp(current->fontName, fontName)) return &current->current;
        current = current->next;
    }

    struct FontNode* new = malloc(sizeof(struct FontNode));
    // 把新节点加入链表
    new->next = fontLibrary[hash];
    fontLibrary[hash] = new;

    Font* curFont = &new->current;
    curFont->fontFile = NULL;

    // 找不到字体，使用fontconfig确定位置
    char* dir = NULL;
    int index = 0;

    FcPattern* pat = FcNameParse(fontName);
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcResult res;
    FcPattern* font = FcFontMatch(config, pat, &res);
    if (!font)
    {
        fprintf(stderr, "找不到字体：%s\n", fontName);
        exit(1);
    }
    if (FcPatternGetString(font, FC_FILE, 0, (FcChar8**) &dir) != FcResultMatch)
    {
        fprintf(stderr, "fontconfig 出现未知错误");
        exit(1);
    }
    if (FcPatternGetInteger(font, FC_INDEX, 0, &index) != FcResultMatch)
        index = 0;
    strcpy(new->path, dir);
    new->index = -1;
    curFont->fontFile = fopen(dir, "rb");
    FcPatternDestroy(font);
    FcPatternDestroy(pat);

    // 读取magic number，确定是不是OTF字体
    uint32_t tmp;
    fread(&tmp, 4, 1, curFont->fontFile);
    if (tmp == 0x66637474U)
    {
        new->index = index;
        fseek(curFont->fontFile, 8l, SEEK_SET);
        tmp = readUint32BE(curFont->fontFile); // numFonts
        if (tmp <= index) return NULL;
        fseek(curFont->fontFile, index * 4l, SEEK_CUR);
        tmp = readUint32BE(curFont->fontFile); // proper index
        fseek(curFont->fontFile, tmp, SEEK_SET);
        fread(&tmp, sizeof(uint32_t), 1, curFont->fontFile);
    }

    // 读取各表索引
    curFont->numTables = readUint16BE(curFont->fontFile);
    curFont->tableRecords = malloc(curFont->numTables * sizeof(struct FontTableRecord));
    fseek(curFont->fontFile, 6l, SEEK_CUR);
    fread(curFont->tableRecords, sizeof(struct FontTableRecord), curFont->numTables, curFont->fontFile);
    for (uint16_t i = 0; i < curFont->numTables; ++i)
    {
        struct FontTableRecord* current = &(curFont->tableRecords[i]);
        switchEndian32(&current->tableTag);
        switchEndian32(&current->offset);
        switchEndian32(&current->length);
    }

    // 读取cmap表
    initCmapTable(curFont);

    strcpy(new->fontName, fontName);
    return curFont;
}

uint16_t getGIDType4(int32_t UID, struct Type4CmapTable* table)
{
    uint16_t currentSegment = 0;
    for (; currentSegment < table->segCount; ++currentSegment)
        if (table->endCode[currentSegment] >= UID)
            break;

    if (currentSegment == table->segCount || table->startCode[currentSegment] > UID)
        return 0; // UID not found

    if (table->idRangeOffset[currentSegment] == 0)
        return UID + table->idDelta[currentSegment];
    uint32_t index = UID - table->startCode[currentSegment];
    uint16_t* pos = table->glyphIdArray + table->idRangeOffset[currentSegment] / 2
                    + currentSegment - table->segCount;
    return pos[index] + table->idDelta[currentSegment];
}

uint16_t getGIDType12(int32_t UID, struct Type12CmapTable* table)
{
    uint32_t currentSegment = 0;
    for (; currentSegment < table->numGroups; ++currentSegment)
        if (table->endCode[currentSegment] >= UID)
            break;

    if (currentSegment == table->numGroups || table->startCode[currentSegment] > UID)
        return 0; // UID not found

    return (uint16_t) (UID - table->startCode[currentSegment] + table->startGID[currentSegment]);
}