#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define WAV_FORMAT_IMA_ADPCM 0x14
#define CD_BUFFER_SIZE 8192
#define CD_BUFFER_CHUNK_SIZE (CD_BUFFER_SIZE >> 2)

#define u32     unsigned int
#define u16     unsigned short

#define s32     int
#define s16     short

#define MAX_PATH    260

struct CD_WaveHeader
{
    char        riff[4];        // 'RIFF'
    u32         size;           // Size of the file
    char        wave[4];        // 'WAVE'

    // fmt chunk
    char        fmt[4];         // 'fmt '
    u32         fmtSize;        // Chunk size
    u16         fmtFormatTag;   // Format of this file
    u16         fmtChannels;    // Num channels
    u32         fmtSamPerSec;   // Samples per second
    u32         fmtBytesPerSec; // Bytes per second
    u16         fmtBlockAlign;  // Block alignment
    u16         fmtBitsPerSam;  // Bits per sample

    u16         fmtExtraData;   // Number of extra fmt bytes
    u16         fmtExtra;       // Samples per block (only for IMA-ADPCM files)
}; // __attribute__ ((packed));
    
struct CD_chunkHeader
{
    char        name[4];    
    u32         size;
}; // __attribute__ ((packed));

struct CD_Header
{
    s16         firstSample;
    char        stepTableIndex;
    char        reserved;
}; // __attribute__ ((packed));

struct CD_decoderFormat
{
    s16 initial;
    unsigned char tableIndex;
    unsigned char test;
    unsigned char   sample[1024];
}; // __attribute__ ((packed));

bool CD_active = false;
CD_WaveHeader CD_waveHeader;
CD_Header CD_blockHeader;
FILE* CD_file;
int CD_fillPos;
bool CD_isPlayingFlag = false;

s16* CD_audioBuffer;
u32 CD_sampleNum;
s16* CD_decompressionBuffer;
int CD_numLoops;
int CD_blockCount;
int CD_dataChunkStart;

// These are from Microsoft's document on DVI ADPCM
const int CD_stepTab[ 89 ] =
{
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

const int CD_indexTab[ 16 ] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

void main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("too few parameters - need a filename\n");
        return;
    }

    char filename[MAX_PATH];
    strcpy(filename, argv[1]);
    FILE* inFile = fopen(filename, "rb");

    if(inFile == NULL)
    {
        printf("file not found - %s\n", filename);
        return;
    }

    char outfilename[MAX_PATH];
    sprintf(outfilename, "%s.raw", filename);
    FILE* outFile = fopen(outfilename, "wb");

    if(outFile == NULL)
    {
        printf("%s could not be created\n", outfilename);
        fclose(inFile);
        return;
    }
    
    fread((void*)&CD_waveHeader, sizeof(CD_waveHeader), 1, inFile);
    
    printf("In file:\n");
    printf("Format: %d\n", CD_waveHeader.fmtFormatTag);
    printf("Rate  : %d\n", CD_waveHeader.fmtSamPerSec);
    printf("Bits  : %d\n", CD_waveHeader.fmtBitsPerSam);
    printf("BlkSz : %d\n", CD_waveHeader.fmtExtra);
    
    if((CD_waveHeader.fmtFormatTag != 17) && (CD_waveHeader.fmtFormatTag != 20))
    {
        printf("Wave file is in the wrong format!  You must use IMA-ADPCM 4-bit mono.\n");
        return;
    }
    
    // Skip chunks until we reach the data chunk
    CD_chunkHeader chunk;
    fread((void*)&chunk, sizeof(CD_chunkHeader), 1, inFile);
    
    while(!((chunk.name[0] == 'd') && (chunk.name[1] == 'a') && (chunk.name[2] == 't') && (chunk.name[3] == 'a')))
    {
        fseek(inFile, chunk.size, SEEK_CUR);
        fread((void*)&chunk, sizeof(CD_chunkHeader), 1, inFile);
    }
    
    int blocksize = CD_waveHeader.fmtBlockAlign - sizeof(CD_blockHeader);
    char* block = (char*)malloc(blocksize);

    int firstblock = 1;

    while( !feof(inFile) )
    {
        fread((void*)&CD_blockHeader, sizeof(CD_blockHeader), 1, inFile);
        if(firstblock)
        {
            fwrite((const void*)&CD_blockHeader, sizeof(CD_blockHeader), 1, outFile);
            firstblock = 0;
        }

        size_t bytesread = fread(block, 1, blocksize, inFile);
        fwrite(block, 1, bytesread, outFile);
    }

    free(block);
    fclose(inFile);
    fclose(outFile);
}
