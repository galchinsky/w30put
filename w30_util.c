#include <stdlib.h>
#include <stdio.h>                         
#include <string.h>

typedef struct picfileformatheader_ { unsigned char HEADERSIGNATURE[8];
    // “HXCPICFE”
    unsigned char formatrevision; // Revision 0
    unsigned char number_of_track;      // Number of track in the file
    unsigned char number_of_side; // Number of valid side (Not used by the emulator)
    unsigned char track_encoding; // Track Encoding mode
    // (Used for the write support - Please see the list above)
    unsigned short bitRate; // Bitrate in Kbit/s. Ex : 250=250000bits/s // Max value : 500
    unsigned short floppyRPM; // Rotation per minute  (Not used by the emulator)
    unsigned char floppyinterfacemode; // Floppy interface mode. (Please see the list above.)
    unsigned char dnu; // Free  unsigned short track_list_offset;
    unsigned short track_list_offset; // Offset of the track list LUT in block of 512 bytes  // (Ex: 1=0x200)
    unsigned char write_allowed; // The Floppy image is write protected ?
    unsigned char single_step;  // 0xFF : Single Step – 0x00 Double Step mode
    unsigned char track0s0_altencoding; // 0x00 : Use an alternate track_encoding for track 0 Side 0
    unsigned char track0s0_encoding;   // alternate track_encoding for track 0 Side 0
    unsigned char track0s1_altencoding; // 0x00 : Use an alternate track_encoding for track 0 Side 1
    unsigned char track0s1_encoding;   // alternate track_encoding for track 0 Side 1
} Picfileformatheader; 

#define IBMPC_DD_FLOPPYMODE 0x00
#define IBMPC_HD_FLOPPYMODE 0x01
#define ATARIST_DD_FLOPPYMODE      0x02
#define ATARIST_HD_FLOPPYMODE      0x03
#define AMIGA_DD_FLOPPYMODE 0x04
#define AMIGA_HD_FLOPPYMODE 0x05
#define CPC_DD_FLOPPYMODE 0x06
#define GENERIC_SHUGGART_DD_FLOPPYMODE 0x07
#define IBMPC_ED_FLOPPYMODE 0x08
#define MSX2_DD_FLOPPYMODE 0x09
#define C64_DD_FLOPPYMODE  0x0A
#define EMU_SHUGART_FLOPPYMODE 0x0B
#define S950_DD_FLOPPYMODE 0x0C
#define S950_HD_FLOPPYMODE 0x0D
#define DISABLE_FLOPPYMODE 0xFE

#define SECTOR_SIZE (512)
#define SIDE_SIZE (SECTOR_SIZE*9)
#define TRACK_SIZE (SIDE_SIZE*2)


typedef struct pictrack_ { 
    unsigned short offset;  // Offset of the track data in block of 512 bytes (Ex: 2=0x400)
    unsigned short track_len;  // Length of the track data in byte. 
} Pictrack;

unsigned char reverse(unsigned char value) {
    unsigned char result = 0;
    for (int i = 0, j = 7; i < 8; ++i, --j) {
        result |= ((value & (1 << j)) >> j) << i;
    }
    return result;
}

void bit_reverse(unsigned char *block, unsigned int len)
{
    while (len--) {
        unsigned char x = *block, y, k;
        for (k = y = 0; k < 8; k++) {
            y <<= 1;
            y |= x&1;
            x >>= 1;
        }
        *block++ = y;
    }
}

void print_bits(unsigned char value) {
    unsigned char result = 0;
    for (int i = 7; i >= 0; --i) {
        printf("%d ", (value >> i) & 1);
    }
}

int LoadSector(FILE* image, int side, int track, int sector, char* buffer) {
    sector--;
    int pos = track*TRACK_SIZE + side*SIDE_SIZE + sector*SECTOR_SIZE;
    fseek(image, pos, SEEK_SET);
    fread(buffer, 512, 1, image);

    return 1;
}


int LoadSectors(FILE* image, int side, int track, int sector, int number, char* buffer) {
    for (int i = 0; i < number; ++i) {
        if (LoadSector(image, side, track, sector, buffer)) {
        	buffer+=512;
        	sector++;
        	if (sector>9) {
            	sector = 1;
            	side++;            	
            	if (side>2) {   
            		side=0;
            		track++;
            	}  	
        	}
        } else {
        	return 0;
        }
    }
    return 1;
}

int SaveSector(FILE* image, int side, int track, int sector, char* buffer) {
    sector--;

    int pos = track*TRACK_SIZE + side*SIDE_SIZE + sector*SECTOR_SIZE;
    fseek(image, pos, SEEK_SET);
    fwrite(buffer, 512, 1, image);

    return 1;
}

int SaveSectors(FILE* image, int side, int track, int sector, int number, char* buffer) {
    for (int i = 0; i < number; ++i) {
        if (SaveSector(image, side, track, sector, buffer)) {
        	buffer += 512;
        	sector++;

        	if (sector>9) {
            	sector = 1;
            	side++;            	
            	if (side>2) {   
            		side = 0;
            		track++;
            	}  	
        	}
        } else {
        	return 0;
        }
    }
    return 1;
}

unsigned long Get12BitValue(unsigned char *array) {
    unsigned long us;
	us = array[0];
	us = us * 256 + array[1];
	us = us * 256 + array[2];      
	return us;
}

void Put12BitValue(unsigned char *array, unsigned long us) {
	array[2] = us&0x000000FF;
	us >>= 8;
	array[1] = us & 0x000000FF;
	us >>= 8;
	array[0] = us & 0x000000FF;
}

