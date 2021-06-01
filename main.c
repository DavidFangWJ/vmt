#include <stdio.h>
#include <stdint.h>

#include "fileReader.h"
#include "tokenAnalysis.h"

void printToken(Token* t)
{
    switch (t->tokenType) {
        case TOKEN_TYPE_DEFAULT:
            printf("Chr\t");
            break;
        case TOKEN_TYPE_COMMAND:
            printf("Cmd\t");
            break;
        case TOKEN_TYPE_ENV_END:
            printf("EOE\t");
            break;
        case TOKEN_EOF:
            printf("EOF\t");
            break;
    }
    printf(" 0x%04x\n", t->charCode);
}

int main() {
    //initiateFontLibrary();

    //Font* font1 = fontFromFile("思源宋体");
    //Font* font2 = fontFromFile("FreeSerif");

    //deleteFontLibrary();

    initFileReading("test.txt");

    Token curToken;
    for (int i=0; i<11; ++i)
    {
        getNextToken(&curToken);
        printToken(&curToken);
    }

    finFileReading();

    return 0;
}
