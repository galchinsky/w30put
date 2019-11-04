#include <string.h>
#include <stdio.h>
#include <math.h>

#include "w30.h"
#include "w30_util.c"

unsigned char outputBuffer[3072*3] ;

int main(int argc, char* argv[]) {
    char *Current;

    /* Check Usage */

    if (argc != 4) {
        fprintf (stderr,"\nUsage : %s SampleRate ImageName SoundFile\n\n",argv[0]) ;
        return -1;
    }

    /* Try to open ImageFile */

    FILE* image = fopen(argv[2], "rb+");
    if (!image) {
        fprintf(stderr, "Image file don't exist\n");
        return -1;
    }
        
    /* Try to open SoundFile */
    FILE *fin = fopen(argv[3],"rb");
    if (fin == NULL) {
        fprintf (stderr,"\nError opening %s for input\n",argv[2]);
        return -1;
    }
    fseek(fin, 0L, SEEK_END);
    unsigned long dataSize = ftell(fin);
    fseek(fin, 0L, 0);    
   
    int srate = atoi(argv[1]);
    int sampleSize = 16 / 8;
    int nChannels = 1;
	int use16Bits = (sampleSize == 2);
	
    /* get disk content */
    ToneDirElem Directory[32];
    LoadSector(image, 1, 7, 9, (char*)Directory);

    /* compute free ram (a/b) */
    int RamA = 18;
    int RamB = 18;

    for (int j = 0; j < 32; ++j) {
        if (Directory[j].ramNumber != RAM_UNUSED) {
            if (Directory[j].subType == 0) { // NO sub
                switch (Directory[j].ramNumber) {
                case RAM_A:
                    RamA -= Directory[j].size;
                    break ;
                case RAM_B:
                    RamB -= Directory[j].size;
                    break;
                }
            }
        }
    }

    int blockCount = (int)ceil((double)dataSize / 12288.0);

    printf ("Free Ram: RAMA:%2.2d  RAMB:%2.2d Need %d blocks ", RamA, RamB, blockCount);

    // Look out for some free RAM
    int ram;
    int track;
    int position;
    if (RamA >= blockCount) {
        ram = RAM_A;
        position = 18 - RamA;
        track = 8 + 2 * position;
        RamA -= blockCount;
    } else {
        if(RamB>=blockCount) {
            ram=RAM_B;
            position = 18 - RamB;
            track = 44 + 2 * position;
            RamB -= blockCount;
        } else {
            printf ("No free Ram left\n");
            fclose(fin);
            return -1;
        }
    }
		
    // Look for free tone	
    int destTone =- 1;		    
    for (int j = 0; j < 32; ++j) {
        if ((Directory[j].ramNumber == RAM_UNUSED) || (Directory[j].size == 0)) {
            destTone = j;
            break;
        }
    }     
    if (destTone == -1) {
        printf ("No tone left\n");
        return -1;
    }
        
    printf ("Storing in ram %c and tone #%d\n", ram == RAM_A ? 'A' : 'B', destTone + 1);
			
    // Initiate file position
    fseek(fin, 0L, 0);    
  		
    // Go on with convertion
    int outputPos = 0;
    int sampleCount = dataSize;
		
    while (sampleCount > 0) {
        short w16s1 = 0;
        short w16s2 = 0;
        unsigned short us1 = 0;
        unsigned short us2 = 0;
        fread(&w16s1, 1, sizeof(short), fin);
        sampleCount--;
			
        if (sampleCount>0) {
            fread(&w16s2, 1, sizeof(short), fin);
            sampleCount--;
        } else {
            w16s2 = 0;
        }

        memcpy(&us1, &w16s1, sizeof(short));
        memcpy(&us2, &w16s2, sizeof(short));

        // Store in buffer
	                                   
        outputBuffer[outputPos++] = us1 >> 8 ;
        outputBuffer[outputPos++] = (us1 & 0x00F0) + ((us2 & 0x00F0) >> 4);
        outputBuffer[outputPos++] = us2 >> 8;

        // if one disk track is filled, write it to disk
      	  		
        if (outputPos == 3072 * 3) {
            if (!SaveSectors(image, 0, track, 1, 18, outputBuffer)) {
                printf ("Failed to write wave data to disk\n");
                return -1;
            }
            track += 1;
            outputPos = 0;
        }
    } 
	  	
    // if buffer needs to be output, do it
	  
    if (outputPos != 0) {
        memset(&outputBuffer[outputPos], 0, (3072 * 3 - outputPos));
        if (!SaveSectors(image, 0, track, 1, 18, outputBuffer)) {
            printf ("Failed to write wave data to disk\n");
            return -1;
        }
    }

    //printf ("Updating tone data\n") ;
		    
    // load tone data
    LoadSectors(image, 1, 7, 1, 8, outputBuffer);
		
    // Writes tone data
    ToneParams *tone = (ToneParams *)&outputBuffer[destTone * 0x80];
    memset(tone, 0, 0x80);
		
    // Tone name       	  
    Current = argv[3];
    int i = 0;
    while ((*Current != 0) && (*Current != '.') && (i < 8)) {
        tone->name[i++] = *Current++;
    }
		
    tone->outputNumber=0;
    tone->rate = srate == 30000 ? RATE_30K : RATE_15K;
    tone->orgKey=60;   // C5
    tone->ramNumber=ram;
    tone->position=position;
    tone->size=blockCount;
    Put12BitValue(tone->stop, dataSize);
    tone->loopMode = LOOP_ONESHOT;
    tone->outputLevel = 127;
    tone->tvaSustainSegment = 1;
    tone->tvaReleaseSegment = 2;
    tone->tvaSegments[0].rate = 127;
    tone->tvaSegments[1].rate = 127;
    tone->tvaSegments[2].rate = 127;
    tone->tvaSegments[0].level = 127;
    tone->tvaSegments[1].level = 127;
    tone->pitchFollow = 1;	
		
    // writes tone data
	
    SaveSectors(image, 1, 7, 1, 8, outputBuffer);
		  			   
    // updates directory ( located in the same buffer )
    //printf ("Updating Directory\n") ;
		  
    memcpy(Directory[destTone].name, tone, 16);
	
    SaveSector(image, 1, 7, 9, (char*)Directory);

    // now create patch    
    PatchElem patches[16];
    LoadSectors(image, 0, 7, 1, sizeof(patches)/SECTOR_SIZE, (char*)patches);

    PatchElem* patch = NULL;
    for (int j = 0; j < 16; ++j) {
        if (patches[j].name[0] == 0) {
            patch = &patches[j];
            break;
        }
    }

    if (patch == NULL) {
        printf ("No free patches left\n");
        return -1;
    }

    Current = argv[3];
    i = 0;
    while ((*Current != 0) && (*Current != '.') && (i < 12)) {
        patch->name[i++] = *Current++;
    }
    patch->bendRange = 2;
    for (int i = 0; i < 109; ++i) {
        patch->keys_lo[i] = destTone;
        patch->keys_hi[i] = destTone;
    }
    patch->volume = 127;
    SaveSectors(image, 0, 7, 1, sizeof(patches)/SECTOR_SIZE, (char*)patches);


	  
}
