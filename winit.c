#include <stdio.h>
#include "w30.h"
#include "w30_util.c"

char emptyfile[737280];
char buf[0x200];

int main(int argc, char* argv[]) {
    ToneDirElem Directory[32];
 
    FILE* image = fopen(argv[1], "wb");

    memset(emptyfile, 0, sizeof(emptyfile));
    fwrite(emptyfile, sizeof(emptyfile), 1, image);
    fseek(image, 0, SEEK_SET);

    /* write W30 header */
    for (int i = 0; i<0x80; ++i) {
        buf[i] = DiskHeader[i];
    }
    for (int i = 0x80; i < 0x200; ++i) {
        buf[i] = 0;
    }

    SaveSector(image, 0, 0, 1, buf);

    /* clear directory */
    for (int i = 0; i < 32; ++i) {
        strcpy(Directory[i].name, "        ");
        Directory[i].ramNumber = RAM_UNUSED;
    }

    SaveSectors(image, 1, 7, 9, 1, (char*)Directory);

    fclose(image);
}

