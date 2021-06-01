//
// Created by david on 2021/5/21.
//

#ifndef VMT_FONTOBJECT_H
#define VMT_FONTOBJECT_H

struct FontTableRecord {
    uint32_t tableTag;
    uint32_t checkSum;
    uint32_t offset;
    uint32_t length;
};

struct Type4CmapTable {
    uint16_t segCount;
    uint16_t* endCode;
    uint16_t* startCode;
    uint16_t* idDelta;
    uint16_t* idRangeOffset;
    uint16_t* glyphIdArray;
};

struct Type12CmapTable {
    uint32_t numGroups;
    uint32_t* startCode;
    uint32_t* endCode;
    uint32_t* startGID;
};

typedef struct {
    uint16_t numTables;
    uint8_t cmapTableType;
    FILE* fontFile;
    struct FontTableRecord* tableRecords;
    union FontCmapTable {
        struct Type4CmapTable type4;
        struct Type12CmapTable type12;
    } cmap;
} Font;

void initiateFontLibrary();
void deleteFontLibrary();

uint16_t findIndexOfTable(Font*, const char*);

Font* fontFromFile(char*);

#endif //VMT_FONTOBJECT_H
