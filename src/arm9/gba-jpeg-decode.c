#include "gba-jpeg-decode.h"

/* Setup the IWRAM-loading definitions if this has been enabled.  There are
 * three sections of this; the register definitions, the IWRAM end
 * determination, and the DMA copying functions.  Register definitions are the
 * same as anywhere else, only with a JPEG_IWRAM prefix.  The IWRAM end
 * determination uses a generated variable that DevKit Advance's linker script
 * creates.  Because of this, other linker scripts might not work with this
 * code.  Finally, the DMA copying uses DMA 3.
 *
 * Functions that are to be copied to IWRAM must obey certain restrictions.
 * They cannot refer to external constant data.  They must be declared static.
 * They must have a JPEG_FUNCTION_END(NAME) macro after them; see how it is
 * used in the code ahead for an example.  Finally, you should avoid external
 * references altogether because of how it limits your flexibility.  Instead,
 * pass necessary variable and function pointers in the arguments.
 */

#if JPEG_USE_IWRAM
    /* The source address pointer for DMA 3. */
    #define JPEG_IWRAM_REG_DM3SAD (*(volatile unsigned int *) 0x40000D4)
    
    /* The destination address pointer for DMA 3. */
    #define JPEG_IWRAM_REG_DM3DAD (*(volatile unsigned int *) 0x40000D8)
    
    /* The number of words or halfwords to transfer for DMA 3. */
    #define JPEG_IWRAM_REG_DM3CNT_L (*(volatile unsigned short *) 0x40000DC)
    
    /* DMA 3 control register. */
    #define JPEG_IWRAM_REG_DM3CNT_H (*(volatile unsigned short *) 0x40000DE)
    
    /* The address of this is the end of the .bss (uninitialized variables)
     * segment, which DevKit Advance's linker script puts last.
     */
    extern char __bss_end;
    
    /* Retrieve the pointer to the first free byte in the IWRAM segment. */
    #define JPEG_IWRAM_USED_END (&__bss_end)
    
    /* This creates a simple stub function that can be used with JPEG_FUNCTION_SIZE
     * to determine the size of a function in bytes.  If the function will be
     * IWRAM-loaded, this macro must be executed immediately after the
     * function with the name of the function in the NAME parameter, and the
     * function must be declared static.
     */
    #define JPEG_FUNCTION_END(NAME) static void NAME##End () { }
   
    /* Retrieve the size in bytes of a function that has a JPEG_FUNCTION_END
     * ballast. 
     */
    #define JPEG_FUNCTION_SIZE(NAME) ((int) ((char *) &NAME##End - (char *) &NAME) & ~3)
    
    /* Start a loading function by defining the necessary variables. */
    #define JPEG_IWRAM_LoadStart() char *iwramEnd = (char *) JPEG_IWRAM_USED_END
    
    /* Load the value named JPEG_NAME into the pointer named NAME,
     * adjusting the read pointer.  This copies SIZE bytes through DMA 3.
     */
    #define JPEG_IWRAM_LoadValue(NAME, SIZE) \
        *(void **) &NAME = iwramEnd; \
        while (JPEG_IWRAM_REG_DM3CNT_H & (1 << 15)) { } \
        JPEG_Assert (iwramEnd + (SIZE) < (char *) &iwramEnd); \
        JPEG_IWRAM_REG_DM3SAD = (unsigned int) &JPEG_##NAME; \
        JPEG_IWRAM_REG_DM3DAD = (unsigned int) iwramEnd; \
        JPEG_IWRAM_REG_DM3CNT_L = (SIZE + 3) >> 2; \
        JPEG_IWRAM_REG_DM3CNT_H = (1 << 10) | (1 << 15); \
        iwramEnd += (SIZE & ~3)
    
    #define JPEG_IWRAM_LoadFunction(NAME) JPEG_IWRAM_LoadValue (NAME, JPEG_FUNCTION_SIZE (JPEG_##NAME))
    #define JPEG_IWRAM_LoadData(NAME) JPEG_IWRAM_LoadValue (NAME, sizeof (JPEG_##NAME))
    
    /* Finish loading the IWRAM by waiting for the DMA transfers to finish and
     * making an assertion check that makes sure (with fairly good but not
     * perfect assurance) that we haven't written over the stack.
     */
    #define JPEG_IWRAM_LoadDone() \
        do { } while (JPEG_IWRAM_REG_DM3CNT_H & (1 << 15))
        
#else
    /* This stub does absolutely nothing. */
    #define JPEG_FUNCTION_END(NAME)
#endif /* JPEG_USE_IWRAM */



/* Converts left-to-right coefficient indices into zig-zagged indices. */
const unsigned char JPEG_ToZigZag [JPEG_DCTSIZE2] =
{
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63,
};

/* These macros are so that we can generate the AA&N multipliers at
 * compile-time, allowing configuration control of fixed point precision.
 */
#define JPEG_AAN_0 1.0
#define JPEG_AAN_1 1.387039845 
#define JPEG_AAN_2 1.306562965
#define JPEG_AAN_3 1.175875602
#define JPEG_AAN_4 1.0
#define JPEG_AAN_5 0.785694958
#define JPEG_AAN_6 0.541196100
#define JPEG_AAN_7 0.275899379

#define JPEG_AAN_LINE(B) \
    JPEG_FTOFIX (JPEG_AAN_0 * JPEG_AAN_##B), \
    JPEG_FTOFIX (JPEG_AAN_1 * JPEG_AAN_##B), \
    JPEG_FTOFIX (JPEG_AAN_2 * JPEG_AAN_##B), \
    JPEG_FTOFIX (JPEG_AAN_3 * JPEG_AAN_##B), \
    JPEG_FTOFIX (JPEG_AAN_4 * JPEG_AAN_##B), \
    JPEG_FTOFIX (JPEG_AAN_5 * JPEG_AAN_##B), \
    JPEG_FTOFIX (JPEG_AAN_6 * JPEG_AAN_##B), \
    JPEG_FTOFIX (JPEG_AAN_7 * JPEG_AAN_##B)

/* The AA&N scaling factors.  These should be multiplied against quantization
 * coefficients to determine their real value.
 */
const JPEG_FIXED_TYPE JPEG_AANScaleFactor [JPEG_DCTSIZE2] =
{
    JPEG_AAN_LINE (0),
    JPEG_AAN_LINE (1),
    JPEG_AAN_LINE (2),
    JPEG_AAN_LINE (3),
    JPEG_AAN_LINE (4),
    JPEG_AAN_LINE (5),
    JPEG_AAN_LINE (6),
    JPEG_AAN_LINE (7),
};

int jpeg_width = 256;

/* This converts values in the range [-32 .. 32] to [0 .. 32] by clamping
 * values outside of that range.  To use it, add 32 to your input.
 */
const unsigned char JPEG_ComponentRange [32 * 3] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31
};

/* Compute the columns half of the IDCT. */
void JPEG_IDCT_Columns (JPEG_FIXED_TYPE *zz)
{
    JPEG_FIXED_TYPE tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9, tmp10, tmp11;
    JPEG_FIXED_TYPE *ez = zz + JPEG_DCTSIZE;

    /* The first column will always have a non-zero coefficient, the DC. */
    goto skipFirstCheckb;
    
    for ( ; zz < ez; zz ++)
    {
        /* A column containing only zeroes will output only zeroes.  Since we
         * output in-place, we don't need to do anything in that case.
         */
        if (!zz [0 * JPEG_DCTSIZE] && !zz [1 * JPEG_DCTSIZE]
        && !zz [2 * JPEG_DCTSIZE] && !zz [3 * JPEG_DCTSIZE]
        && !zz [4 * JPEG_DCTSIZE] && !zz [5 * JPEG_DCTSIZE]
        && !zz [6 * JPEG_DCTSIZE] && !zz [7 * JPEG_DCTSIZE])
            continue;
        
    skipFirstCheckb:
        tmp0 = zz [0 * JPEG_DCTSIZE];
        tmp1 = zz [2 * JPEG_DCTSIZE];
        tmp2 = zz [4 * JPEG_DCTSIZE];
        tmp3 = zz [6 * JPEG_DCTSIZE];
        
        tmp6 = tmp1 + tmp3;
        tmp7 = JPEG_FIXMUL (tmp1 - tmp3, JPEG_FTOFIX (1.414213562)) - tmp6;
        tmp1 = tmp0 - tmp2 + tmp7;
        tmp0 = tmp0 + tmp2 + tmp6;
        
        tmp3 = tmp0 - (tmp6 << 1);
        tmp2 = tmp1 - (tmp7 << 1);
        
        tmp4 = zz [1 * JPEG_DCTSIZE];
        tmp5 = zz [3 * JPEG_DCTSIZE];
        tmp6 = zz [5 * JPEG_DCTSIZE];
        tmp7 = zz [7 * JPEG_DCTSIZE];
        
        tmp10 = tmp4 - tmp7;
        
        tmp8 = tmp6 + tmp5;
        tmp9 = tmp4 + tmp7;
        tmp7 = tmp9 + tmp8;
        tmp11 = JPEG_FIXMUL (tmp9 - tmp8, JPEG_FTOFIX (1.414213562));
        
        tmp8 = tmp6 - tmp5;
        tmp9 = JPEG_FIXMUL (tmp8 + tmp10, JPEG_FTOFIX (1.847759065));
        
        tmp6 = JPEG_FIXMUL (JPEG_FTOFIX (-2.613125930), tmp8) + tmp9 - tmp7;
        tmp5 = tmp11 - tmp6;
        tmp4 = JPEG_FIXMUL (JPEG_FTOFIX (1.082392200), tmp10) - tmp9 + tmp5; 
        
        zz [0 * JPEG_DCTSIZE] = tmp0 + tmp7;
        zz [1 * JPEG_DCTSIZE] = tmp1 + tmp6;
        zz [2 * JPEG_DCTSIZE] = tmp2 + tmp5;
        zz [3 * JPEG_DCTSIZE] = tmp3 - tmp4;
        zz [4 * JPEG_DCTSIZE] = tmp3 + tmp4;
        zz [5 * JPEG_DCTSIZE] = tmp2 - tmp5;
        zz [6 * JPEG_DCTSIZE] = tmp1 - tmp6;
        zz [7 * JPEG_DCTSIZE] = tmp0 - tmp7;
    }
}
JPEG_FUNCTION_END (JPEG_IDCT_Columns)

/* Compute the rows half of the IDCT, loading the component information into
 * chunk as values in the range -64 to 64, although it can go somewhat outside
 * of that range.  chunkStride is the number of bytes in a row in chunk.
 */
 
void JPEG_IDCT_Rows (const JPEG_FIXED_TYPE *zz, signed char *chunk, int chunkStride)
{
    JPEG_FIXED_TYPE tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13;
    JPEG_FIXED_TYPE tmp4, tmp5, tmp6, tmp7, z5, z10, z11, z12, z13;
    int row;
    
    for (row = 0; row < JPEG_DCTSIZE; row ++, zz += JPEG_DCTSIZE, chunk += chunkStride)
    {
        tmp10 = zz [0] + zz [4];
        tmp11 = zz [0] - zz [4];

        tmp13 = zz [2] + zz [6];
        tmp12 = JPEG_FIXMUL (zz [2] - zz [6], JPEG_FTOFIX (1.414213562)) - tmp13;

        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;
        
        z13 = zz [5] + zz [3];
        z10 = zz [5] - zz [3];
        z11 = zz [1] + zz [7];
        z12 = zz [1] - zz [7];

        tmp7 = z11 + z13;
        tmp11 = JPEG_FIXMUL (z11 - z13, JPEG_FTOFIX (1.414213562));

        z5 = JPEG_FIXMUL (z10 + z12, JPEG_FTOFIX (1.847759065));
        tmp10 = JPEG_FIXMUL (JPEG_FTOFIX (1.082392200), z12) - z5;
        tmp12 = JPEG_FIXMUL (JPEG_FTOFIX (-2.613125930), z10) + z5;
        
        tmp6 = tmp12 - tmp7;
        tmp5 = tmp11 - tmp6;
        tmp4 = tmp10 + tmp5;

        /* This shifts by an extra bit to remove the need for clamping at
         * this point.  Thus the normative samples are in the range -64 to 63.
         * This requires a later bit-shift, but that comes for free with the ARM
         * instruction set, and has an acceptable, likely imperceptible, loss
         * of quality.
         */
         
        chunk [0] = JPEG_FIXTOI (tmp0 + tmp7) >> 4;
        chunk [1] = JPEG_FIXTOI (tmp1 + tmp6) >> 4;
        chunk [2] = JPEG_FIXTOI (tmp2 + tmp5) >> 4;
        chunk [3] = JPEG_FIXTOI (tmp3 - tmp4) >> 4;
        chunk [4] = JPEG_FIXTOI (tmp3 + tmp4) >> 4;
        chunk [5] = JPEG_FIXTOI (tmp2 - tmp5) >> 4;
        chunk [6] = JPEG_FIXTOI (tmp1 - tmp6) >> 4;
        chunk [7] = JPEG_FIXTOI (tmp0 - tmp7) >> 4;
    }
}
JPEG_FUNCTION_END (JPEG_IDCT_Rows)

/* This function comes from jpeglib.  I feel all right about that since it comes from AA&N anyway. */
void JPEG_IDCT (JPEG_FIXED_TYPE *zz, signed char *chunk, int chunkStride)
{
    JPEG_IDCT_Columns (zz);
    JPEG_IDCT_Rows (zz, chunk, chunkStride);
}

/* Compute a signed value.  COUNT is the number of bits to read, and OUT is
 * where to store the result.
 */
 
#define JPEG_Value(COUNT, OUT) \
    do { \
        unsigned int value = JPEG_BITS_GET (COUNT); \
        \
        if (value < (unsigned int) (1 << ((unsigned int) (COUNT - 1)))) \
            value += (-1 << COUNT) + 1; \
        (OUT) = value; \
    } while (0)
 
/* Decode the coefficients from the input stream and do dequantization at the
 * same time.  dcLast is the previous block's DC value and is updated.  zz is
 * the output coefficients and will be all ready for an IDCT.  quant is the
 * quantization table to use, dcTable and acTable are the Huffman tables for
 * the DC and AC coefficients respectively, dataBase, bitsLeftBase, and
 * bitsDataBase are for input stream state, and toZigZag is a pointer to
 * JPEG_ToZigZag or to its IWRAM copy.
 */
 
void JPEG_DecodeCoefficients (
    JPEG_FIXED_TYPE *dcLast, JPEG_FIXED_TYPE *zz, JPEG_FIXED_TYPE *quant,
    JPEG_HuffmanTable *dcTable, JPEG_HuffmanTable *acTable,
    const unsigned char **dataBase, unsigned int *bitsLeftBase,
    unsigned long int *bitsDataBase, const unsigned char *toZigZag)
{
    unsigned bits_left = *bitsLeftBase, bits_data = *bitsDataBase; /* Input stream state. */
    const unsigned char *data = *dataBase; /* Input stream state. */
    int r, s, diff; /* Various temporary data variables. */
    int index = 1; /* The current zig-zagged index. */
    
    /* Clear all coefficients to zero. */
    {
        JPEG_FIXED_TYPE *ez = zz + JPEG_DCTSIZE2;
        do *-- ez = 0;
        while (ez > zz);
    }
    
    /* Read the DC coefficient. */
    JPEG_BITS_CHECK ();
    JPEG_HuffmanTable_Decode (dcTable, s);
    JPEG_Value (s, diff);
    
    /* Store the DC coefficient. */
    *dcLast += diff;
    zz [toZigZag [0]] = *dcLast * quant [0];

    while (1)
    {
        /* Read a bits/run-length value. */
        JPEG_BITS_CHECK ();
        JPEG_HuffmanTable_Decode (acTable, s);
        r = s >> 4;
        s &= 15;
    
        /* If there is a value at this cell +r, then read it. */
        if (s)
        {
            index += r;
            JPEG_Value (s, r);
            zz [toZigZag [index]] = r * quant [index];
            if (index == JPEG_DCTSIZE2 - 1)
                break;
            index ++;
        }
        /* Otherwise we skip 16 cells or finish up. */
        else
        {
            if (r != 15)
                break;
            index += 16;
        }
    }
    
    /* Restore state for the caller. */
    *bitsDataBase = bits_data;
    *bitsLeftBase = bits_left;
    *dataBase = data;
}
JPEG_FUNCTION_END (JPEG_DecodeCoefficients)

/* Convert a chunk of YCbCr data to the output format.  YBlock, CbBlock,
 * and CrBlock are the pointers to the relevant chunks; each sample is
 * between -64 and 64, although out-of-range values are possible.
 * nHorzFactor and nVertFactor, where n is Y, Cb, and Cr, hold the
 * multipliers for each coordinate.  Shift right by horzMax and vertMax to
 * get the actual point to sample data from.  M211 is true if the
 * component factors satisfy a 2:1:1 relationship; this leads to a much
 * faster conversion if JPEG_FASTER_M211 is enabled.
 * out and outStride are the output pointers and the number of samples
 * in an output row.  Finally, ComponentRange is a pointer to the
 * JPEG_ComponentRange array.
 */
 

 
void JPEG_ConvertBlock (
    signed char *YBlock, signed char *CbBlock, signed char *CrBlock,
    int YHorzFactor, int YVertFactor, int CbHorzFactor, int CbVertFactor, int CrHorzFactor, int CrVertFactor, int horzMax, int vertMax,
    char M211, volatile JPEG_OUTPUT_TYPE *out, int outStride, const unsigned char *ComponentRange)
{
    int px, py;
    
    /* Since we need to offset all indices into this anyway, we might as well do it once only. */
    ComponentRange += 32;
    
/* Do the faster 2:1:1 code if JPEG_FASTER_M211 is set and the image scan satisfies that relationship. */
#if JPEG_FASTER_M211                
    if (M211)
    {
        /* Nothing complex here.  Because of its nature, we can do Cb and Cr
         * conversion only once for every four pixels.  This optimization is
         * done implicitly, using GCC's optimizer for gleaning the actual
         * advantage.
         */
         
        for (py = 0; py < 2 * JPEG_DCTSIZE; py += 2)
        {
            volatile JPEG_OUTPUT_TYPE *row = &out [outStride * py];
            volatile JPEG_OUTPUT_TYPE *rowEnd = row + JPEG_DCTSIZE * 2;
            
            for ( ; row < rowEnd; row += 2, YBlock += 2, CbBlock ++, CrBlock ++)
            {
                int Cb = *CbBlock, Cr = *CrBlock;
                JPEG_Convert (row [0], YBlock [0], Cb, Cr);
                JPEG_Convert (row [1], YBlock [1], Cb, Cr);
                JPEG_Convert (row [jpeg_width], YBlock [2 * JPEG_DCTSIZE + 0], Cb, Cr); // 240
                JPEG_Convert (row [jpeg_width+1], YBlock [2 * JPEG_DCTSIZE + 1], Cb, Cr); // 241
            }
            
            YBlock += JPEG_DCTSIZE * 2;
        }
    }
#else
    if (0) { }
#endif /* JPEG_FASTER_M211 */

/* Otherwise we fall back on generic code, if JPEG_HANDLE_ANY_FACTORS is set.
 * If it is not, then this function does nothing at all!
 */
#if JPEG_HANDLE_ANY_FACTORS
    else for (py = 0; py < vertMax; py ++)
    {
        signed char *YScan = YBlock + (py * YVertFactor >> 8) * (horzMax * YHorzFactor >> 8);
        signed char *CbScan = CbBlock + (py * CbVertFactor >> 8) * (horzMax * CbHorzFactor >> 8);
        signed char *CrScan = CrBlock + (py * CrVertFactor >> 8) * (horzMax * CrHorzFactor >> 8);
        
        volatile JPEG_OUTPUT_TYPE *row = &out [outStride * py];
        
        for (px = 0; px < horzMax; px ++, row ++)
        {
            int Y = YScan [px * YHorzFactor >> 8];
            int Cb = CbScan [px * CbHorzFactor >> 8];
            int Cr = CrScan [px * CrHorzFactor >> 8];
            
            JPEG_Convert (*row, Y, Cb, Cr);
        }
    }
#endif /* JPEG_HANDLE_ANY_FACTORS */

    /* Make sure all variables are referenced. */
    (void) YHorzFactor; (void) YVertFactor; (void) CbHorzFactor;
    (void) CbVertFactor; (void) CrHorzFactor; (void) CrVertFactor;
    (void) horzMax; (void) vertMax; (void) px; (void) py;
    (void) YBlock; (void) CbBlock; (void) CrBlock;
    (void) M211; (void) out; (void) outStride;
}
JPEG_FUNCTION_END (JPEG_ConvertBlock)


/* Decode a Huffman table and initialize its data.  This expects to be called
 * after the DHT marker and the type/slot pair.
 */
int JPEG_HuffmanTable_Read (JPEG_HuffmanTable *huffmanTable, const unsigned char **dataBase)
{
    const unsigned char *data = *dataBase;
    const unsigned char *bits;
    int huffcode [256];
    unsigned char huffsize [256];
    int total = 0;
    int c;
    
    bits = data;
    for (c = 0; c < 16; c ++)
        total += *data ++;
    huffmanTable->huffval = data;
    data += total;
    
    /*void GenerateSizeTable ()*/
    {
        int k = 0, i = 1, j = 1;
        
        do
        {
            while (j ++ <= bits [i - 1])
                huffsize [k ++] = i;
            i ++;
            j = 1;
        }
        while (i <= 16);
            
        huffsize [k] = 0;
    }
    
    /*void GenerateCodeTable ()*/
    {
        int k = 0, code = 0, si = huffsize [0];

        while (1)
        {            
            do huffcode [k ++] = code ++;
            while (huffsize [k] == si);
                
            if (huffsize [k] == 0)
                break;
            
            do code <<= 1, si ++;
            while (huffsize [k] != si);
        }
    }
    
    /*void DecoderTables ()*/
    {
        int i = 0, j = 0;
        
        while (1)
        {
            if (i >= 16)
                break;
            if (bits [i] == 0)
                huffmanTable->maxcode [i] = -1;
            else
            {
                huffmanTable->valptr [i] = &huffmanTable->huffval [j - huffcode [j]];
                j += bits [i];
                huffmanTable->maxcode [i] = huffcode [j - 1];
            }
            i ++;
        }
    }
    
    /*void GenerateLookahead ()*/
    {
        int l, i, p, c, ctr;
        
        for (c = 0; c < 256; c ++)
            huffmanTable->look_nbits [c] = 0;
            
        p = 0;
        for (l = 1; l <= 8; l ++)
        {
            for (i = 1; i <= bits [l - 1]; i ++, p ++)
            {
                int lookbits = huffcode [p] << (8 - l);
                
                for (ctr = 1 << (8 - l); ctr > 0; ctr --)
                {
                    huffmanTable->look_nbits [lookbits] = l;
                    huffmanTable->look_sym [lookbits] = huffmanTable->huffval [p];
                    lookbits ++;
                }
            }
        }
    }
    
    *dataBase = data;
    return 1;
}

/* Skip past a Huffman table section.  This expects to be called after reading
 * the DHT marker and the type/slot pair.
 */
int JPEG_HuffmanTable_Skip (const unsigned char **dataBase)
{
    const unsigned char *data = *dataBase;
    int c, total = 16;
    
    for (c = 0; c < 16; c ++)
        total += *data ++;
    *dataBase += total;
    return 1;
}

/* Takes information discovered in JPEG_Decoder_ReadHeaders and loads the
 * image.  This is a public function; see gba-jpeg.h for more information on it.
 */
int JPEG_Decoder_ReadImage (JPEG_Decoder *decoder, const unsigned char **dataBase, volatile JPEG_OUTPUT_TYPE *out, int outWidth, int outHeight)
{
    JPEG_FrameHeader *frame = &decoder->frame; /* Pointer to the image's frame. */
    JPEG_ScanHeader *scan = &decoder->scan; /* Pointer to the image's scan. */
    int YHorzFactor = 0, YVertFactor = 0; /* Scaling factors for the Y component. */
    int CbHorzFactor = 1, CbVertFactor = 1; /* Scaling factors for the Cb component.  The default is important because it is used for greyscale images. */
    int CrHorzFactor = 1, CrVertFactor = 1; /* Scaling factors for the Cr component.  The default is important because it is used for greyscale images. */
    int horzMax = 0, vertMax = 0; /* The maximum horizontal and vertical scaling factors for the components. */
    JPEG_FrameHeader_Component *frameComponents [JPEG_MAXIMUM_COMPONENTS]; /* Pointers translating scan header components to frame header components. */
    JPEG_FrameHeader_Component *item, *itemEnd = frame->componentList + frame->componentCount; /* The frame header's components for loops. */
    JPEG_FIXED_TYPE dcLast [JPEG_MAXIMUM_COMPONENTS]; /* The last DC coefficient computed.  This is initialized to zeroes at the start and after a restart interval. */
    int c, bx, by, cx, cy; /* Various loop parameters. */
    int horzShift = 0; /* The right shift to use after multiplying by nHorzFactor to get the actual sample. */
    int vertShift = 0; /* The right shift to use after multiplying by nVertFactor to get the actual sample. */
    char M211 = 0; /* Whether this scan satisfies the 2:1:1 relationship, which leads to faster code. */
    const unsigned char *data = *dataBase; /* The input data pointer; this must be right at the start of scan data. */
    
    signed char blockBase [JPEG_DCTSIZE2 * JPEG_MAXIMUM_SCAN_COMPONENT_FACTORS]; /* Blocks that have been read and are alloted to YBlock, CbBlock, and CrBlock based on their scaling factors. */
    signed char *YBlock; /* Y component temporary block that holds samples for the MCU currently being decompressed. */
    signed char *CbBlock; /* Cb component temporary block that holds samples for the MCU currently being decompressed. */
    signed char *CrBlock; /* Cr component temporary block that holds samples for the MCU currently being decompressed. */
    
    JPEG_HuffmanTable acTableList [2]; /* The decompressed AC Huffman tables.  JPEG Baseline allows only two AC Huffman tables in a scan. */
    int acTableUse [2] = { -1, -1 }; /* The indices of the decompressed AC Huffman tables, or -1 if this table hasn't been used. */
    JPEG_HuffmanTable dcTableList [2]; /* The decompressed DC Huffman tables.  JPEG Baseline allows only two DC Huffman tables in a scan. */
    int dcTableUse [2] = { -1, -1 }; /* The indices of the decompressed DC Huffman tables, or -1 if this table hasn't been used. */
    int restartInterval = decoder->restartInterval; /* Number of blocks until the next restart. */
    
    /* Pointer to JPEG_ConvertBlock, which might be moved to IWRAM. */
    void (*ConvertBlock) (signed char *, signed char *, signed char *,
        int, int, int, int, int, int, int, int, char,
        volatile JPEG_OUTPUT_TYPE *, int, const unsigned char *)
        = &JPEG_ConvertBlock;

    /* Pointer to JPEG_IDCT_Columns, which might be moved to IWRAM. */
    void (*IDCT_Columns) (JPEG_FIXED_TYPE *) = &JPEG_IDCT_Columns;

    /* Pointer to JPEG_IDCT_Rows, which might be moved to IWRAM. */
    void (*IDCT_Rows) (const JPEG_FIXED_TYPE *, signed char *, int) = &JPEG_IDCT_Rows;
    
    /* Pointer to JPEG_DecodeCoefficients, which might be moved to IWRAM. */
    void (*DecodeCoefficients) (JPEG_FIXED_TYPE *, JPEG_FIXED_TYPE *, JPEG_FIXED_TYPE *, JPEG_HuffmanTable *,
        JPEG_HuffmanTable *, const unsigned char **, unsigned int *,
        unsigned long int *, const unsigned char *) = &JPEG_DecodeCoefficients;
    
    const unsigned char *ToZigZag = JPEG_ToZigZag; /* Pointer to JPEG_ToZigZag, which might be moved to IWRAM. */
    const unsigned char *ComponentRange = JPEG_ComponentRange; /* Pointer to JPEG_ComponentRange, which might be moved to IWRAM. */
   
    /* Start decoding bits. */
    JPEG_BITS_START ();
    
    /* The sum of all factors in the scan; this cannot be greater than 10 in JPEG Baseline. */
    int factorSum = 0;

/* Load the essential functions and data into IWRAM if this has been set. */
#if JPEG_USE_IWRAM
    JPEG_IWRAM_LoadStart (); /* Define variables. */
    JPEG_IWRAM_LoadFunction (ConvertBlock);
    JPEG_IWRAM_LoadFunction (DecodeCoefficients);
    JPEG_IWRAM_LoadFunction (IDCT_Columns);
    JPEG_IWRAM_LoadFunction (IDCT_Rows);
    JPEG_IWRAM_LoadData (ToZigZag);
    JPEG_IWRAM_LoadData (ComponentRange);
    JPEG_IWRAM_LoadDone (); /* Finished; run down DMA and check that we haven't overwritten the stack. */
#endif /* JPEG_USE_IWRAM */

    /* Find the maximum factors and the factors for each component. */    
    for (item = frame->componentList; item < itemEnd; item ++)
    {
        /* Find the opposing scan header component. */
        for (c = 0; ; c ++)
        {
            JPEG_ScanHeader_Component *sc;

            JPEG_Assert (c < scan->componentCount);
            sc = &scan->componentList [c];
            if (sc->selector != item->selector)
                continue;
            
            /* Decompress the DC table if necessary. */
            if (sc->dcTable != dcTableUse [0] && sc->dcTable != dcTableUse [1])
            {
                const unsigned char *tablePointer = decoder->dcTables [sc->dcTable];
                
                if (dcTableUse [0] == -1)
                    dcTableUse [0] = sc->dcTable, JPEG_HuffmanTable_Read (&dcTableList [0], &tablePointer);
                else if (dcTableUse [1] == -1)
                    dcTableUse [1] = sc->dcTable, JPEG_HuffmanTable_Read (&dcTableList [1], &tablePointer);
                else
                    JPEG_Assert (0);
            }
            
            /* Decompress the AC table if necessary. */
            if (sc->acTable != acTableUse [0] && sc->acTable != acTableUse [1])
            {
                const unsigned char *tablePointer = decoder->acTables [sc->acTable];
                
                if (acTableUse [0] == -1)
                    acTableUse [0] = sc->acTable, JPEG_HuffmanTable_Read (&acTableList [0], &tablePointer);
                else if (acTableUse [1] == -1)
                    acTableUse [1] = sc->acTable, JPEG_HuffmanTable_Read (&acTableList [1], &tablePointer);
                else
                    JPEG_Assert (0);
            }
            
            frameComponents [c] = item;
            break;
        }
        
        /* Add the sum for a later assertion test. */
        factorSum += item->horzFactor * item->vertFactor;
        
        /* Adjust the maximum horizontal and vertical scaling factors as necessary. */
        if (item->horzFactor > horzMax)
            horzMax = item->horzFactor;
        if (item->vertFactor > vertMax)
            vertMax = item->vertFactor;
            
        /* Update the relevant component scaling factors if necessary. */
        if (item->selector == 1)
        {
            YHorzFactor = item->horzFactor;
            YVertFactor = item->vertFactor;
        }
        else if (item->selector == 2)
        {
            CbHorzFactor = item->horzFactor;
            CbVertFactor = item->vertFactor;
        }
        else if (item->selector == 3)
        {
            CrHorzFactor = item->horzFactor;
            CrVertFactor = item->vertFactor;
        }
    }
   
    /* Ensure that we have enough memory for these factors. */
    JPEG_Assert (factorSum < JPEG_MAXIMUM_SCAN_COMPONENT_FACTORS);
     
    /* Split up blockBase according to the components. */
    YBlock = blockBase;
    CbBlock = YBlock + YHorzFactor * YVertFactor * JPEG_DCTSIZE2;
    CrBlock = CbBlock + CbHorzFactor * CbVertFactor * JPEG_DCTSIZE2;
    
    /* Compute the right shift to be done after multiplying against the scaling factor. */
    if (horzMax == 1) horzShift = 8;
    else if (horzMax == 2) horzShift = 7;
    else if (horzMax == 4) horzShift = 6;
    
    /* Compute the right shift to be done after multiplying against the scaling factor. */
    if (vertMax == 1) vertShift = 8;
    else if (vertMax == 2) vertShift = 7;
    else if (vertMax == 4) vertShift = 6;

    /* Adjust the scaling factors for our parameters. */    
    YHorzFactor <<= horzShift;
    YVertFactor <<= vertShift;
    CbHorzFactor <<= horzShift;
    CbVertFactor <<= vertShift;
    CrHorzFactor <<= horzShift;
    CrVertFactor <<= vertShift;
    
    /* Clear the Cb channel for potential grayscale. */
    {
        signed char *e = CbBlock + JPEG_DCTSIZE2;
        
        do *-- e = 0;
        while (e > CbBlock);
    }
    
    /* Clear the Cr channel for potential grayscale. */
    {
        signed char *e = CrBlock + JPEG_DCTSIZE2;
        
        do *-- e = 0;
        while (e > CrBlock);
    }

/* Compute whether this satisfies the sped up 2:1:1 relationship. */
#if JPEG_FASTER_M211
    if (YHorzFactor == 256 && YVertFactor == 256 && CbHorzFactor == 128 && CbVertFactor == 128 && CrHorzFactor == 128 && CrVertFactor == 128)
        M211 = 1;
#endif /* JPEG_FASTER_M211 */
        
    /* Clear the DC parameters. */
    for (c = 0; c < JPEG_MAXIMUM_COMPONENTS; c ++)
        dcLast [c] = 0;
    
    /* Now run over each MCU horizontally, then vertically. */
    for (by = 0; by < frame->height; by += vertMax * JPEG_DCTSIZE)
    {
        for (bx = 0; bx < frame->width; bx += horzMax * JPEG_DCTSIZE)
        {
            /* Read the components for the MCU. */
            for (c = 0; c < scan->componentCount; c ++)
            {
                JPEG_ScanHeader_Component *sc = &scan->componentList [c];
                JPEG_FrameHeader_Component *fc = frameComponents [c];
                JPEG_HuffmanTable *dcTable, *acTable;
                JPEG_FIXED_TYPE *quant = decoder->quantTables [fc->quantTable];
                int stride = fc->horzFactor * JPEG_DCTSIZE;
                signed char *chunk = 0;
                
                dcTable = &dcTableList [sc->dcTable == dcTableUse [1] ? 1 : 0];
                acTable = &acTableList [sc->acTable == acTableUse [1] ? 1 : 0];
                
                /* Compute the output chunk. */
                if (fc->selector == 1)
                    chunk = YBlock;
                else if (fc->selector == 2)
                    chunk = CbBlock;
                else if (fc->selector == 3)
                    chunk = CrBlock;
                    
                for (cy = 0; cy < fc->vertFactor * JPEG_DCTSIZE; cy += JPEG_DCTSIZE)
                {
                    for (cx = 0; cx < fc->horzFactor * JPEG_DCTSIZE; cx += JPEG_DCTSIZE)
                    {
                        int start = cx + cy * stride;
                        JPEG_FIXED_TYPE zz [JPEG_DCTSIZE2];

                        /* Decode coefficients. */
                        DecodeCoefficients (&dcLast [c], zz, quant, dcTable, acTable, &data, &bits_left, &bits_data, ToZigZag);

                        /* Perform an IDCT if this component will contribute to the image. */
                        if (chunk)
                        {
                            IDCT_Columns (zz);
                            IDCT_Rows (zz, chunk + start, stride);
                        }
                    }
                }
            }
            
            /* Check that our block will be in-range; this should actually use clamping. */
            if (bx + horzMax * JPEG_DCTSIZE > outWidth || by + vertMax * JPEG_DCTSIZE > outHeight)
                continue;
                
            /* Convert our block from YCbCr to the output. */
            ConvertBlock (YBlock, CbBlock, CrBlock,
                YHorzFactor, YVertFactor, CbHorzFactor, CbVertFactor, CrHorzFactor, CrVertFactor,
                horzMax * JPEG_DCTSIZE, vertMax * JPEG_DCTSIZE, M211, out + bx + by * outWidth, outWidth, ComponentRange);
            
            /* Handle the restart interval. */
            if (decoder->restartInterval && --restartInterval == 0)
            {
                restartInterval = decoder->restartInterval;
                JPEG_BITS_REWIND ();
                if (((data [0] << 8) | data [1]) == JPEG_Marker_EOI)
                    goto finish;
                JPEG_Assert (data [0] == 0xFF && (data [1] >= 0xD0 && data [1] <= 0xD7));
                for (c = 0; c < JPEG_MAXIMUM_COMPONENTS; c ++)
                    dcLast [c] = 0;
                data += 2;
            }
        }
    }
   
finish:
    /* Make sure we read an EOI marker. */ 
    JPEG_BITS_REWIND ();
    JPEG_Assert (((data [0] << 8) | data [1]) == JPEG_Marker_EOI);
    data += 2;
    
    /* Clear up and return success. */
    *dataBase = data;
    return 1;
}

/* Read an JPEG_Marker_SOFn marker into frame.  This expects to start
 * processing immediately after the marker.
 */
int JPEG_FrameHeader_Read (JPEG_FrameHeader *frame, const unsigned char **dataBase, JPEG_Marker marker)
{
    const unsigned char *data = *dataBase;
    unsigned short length = (data [0] << 8) | data [1];
    int index;

    (void) length;
    JPEG_Assert (length >= 8);        
    data += 2; /* Skip the length. */
    frame->marker = marker;
    frame->encoding = (marker >= 0xFFC0 && marker <= 0xFFC7) ? 0 : 1;
    frame->differential = !(marker >= 0xFFC0 && marker <= 0xFFC3 && marker >= 0xFFC8 && marker <= 0xFFCB);
    
    frame->precision = *data ++;
    frame->height = (data [0] << 8) | data [1]; data += 2;
    frame->width = (data [0] << 8) | data [1]; data += 2;
	jpeg_width = frame->width;
    frame->componentCount = *data ++;
    
    JPEG_Assert (frame->precision == 8);
    JPEG_Assert (frame->componentCount <= JPEG_MAXIMUM_COMPONENTS);
    JPEG_Assert (length == 8 + 3 * frame->componentCount);
    
    /* Read the frame components. */
    for (index = 0; index < frame->componentCount; index ++)
    {
        JPEG_FrameHeader_Component *c = &frame->componentList [index];
        unsigned char pair;
        
        c->selector = *data ++;
        pair = *data ++;
        c->horzFactor = pair >> 4;
        c->vertFactor = pair & 15;
        c->quantTable = *data ++;
        
        JPEG_Assert (c->horzFactor == 1 || c->horzFactor == 2 || c->horzFactor == 4);
        JPEG_Assert (c->vertFactor == 1 || c->vertFactor == 2 || c->vertFactor == 4);
        JPEG_Assert (c->quantTable <= 3);
    }
    
    *dataBase = data;
    return 1;
}

/* Read a JPEG_Marker_SOS marker into scan.  This expects to start processing
 * immediately after the marker.
 */
int JPEG_ScanHeader_Read (JPEG_ScanHeader *scan, const unsigned char **dataBase)
{
    const unsigned char *data = *dataBase;
    unsigned short length = (data [0] << 8) | data [1];
    JPEG_ScanHeader_Component *c, *cEnd;
    unsigned char pair;
 
    (void) length;
    JPEG_Assert (length >= 6);
    data += 2; /* Skip the length. */
    scan->componentCount = *data ++;
    
    JPEG_Assert (scan->componentCount <= JPEG_MAXIMUM_COMPONENTS);
    JPEG_Assert (length == 6 + 2 * scan->componentCount);

    /* Read the scan components. */    
    for (c = scan->componentList, cEnd = c + scan->componentCount; c < cEnd; c ++)
    {
        c->selector = *data ++;
        pair = *data ++;
        c->dcTable = pair >> 4;
        c->acTable = pair & 15;
        
        JPEG_Assert (c->dcTable < 4);
        JPEG_Assert (c->acTable < 4);
    }
    
    /* Read the spectral and approximation footers, which are used for
     * progressive.
     */
     
    scan->spectralStart = *data ++;
    scan->spectralEnd = *data ++;
    JPEG_Assert (scan->spectralStart <= 63);
    JPEG_Assert (scan->spectralEnd <= 63);
    pair = *data ++;
    scan->successiveApproximationBitPositionHigh = pair >> 4;
    scan->successiveApproximationBitPositionLow = pair & 15;
    JPEG_Assert (scan->successiveApproximationBitPositionHigh <= 13);
    JPEG_Assert (scan->successiveApproximationBitPositionLow <= 15);
    
    *dataBase = data;
    return 1;
}

/* Read all headers from the very start of the JFIF stream to right after the
 * SOS marker.
 */
 
int JPEG_Decoder_ReadHeaders (JPEG_Decoder *decoder, const unsigned char **dataBase)
{
    const unsigned char *data = *dataBase;
    JPEG_Marker marker;
    int c;
 
    /* Initialize state and assure that this is a JFIF file. */   
    decoder->restartInterval = 0;
    JPEG_Assert (((data [0] << 8) | data [1]) == JPEG_Marker_SOI);
    data += 2;
    
    /* Start reading every marker as it comes in. */
    while (1)
    {
        marker = (JPEG_Marker) ((data [0] << 8) | data [1]);
        data += 2;
        
        switch (marker)
        {
            /* This block is just skipped over. */
            case JPEG_Marker_APP0:
            case JPEG_Marker_APP1:
            case JPEG_Marker_APP2:
            case JPEG_Marker_APP3:
            case JPEG_Marker_APP4:
            case JPEG_Marker_APP5:
            case JPEG_Marker_APP6:
            case JPEG_Marker_APP7:
            case JPEG_Marker_APP8:
            case JPEG_Marker_APP9:
            case JPEG_Marker_APP10:
            case JPEG_Marker_APP11:
            case JPEG_Marker_APP12:
            case JPEG_Marker_APP13:
            case JPEG_Marker_APP14:
            case JPEG_Marker_APP15:
            case JPEG_Marker_COM:
                data += (data [0] << 8) | data [1];
                break;
            
            case JPEG_Marker_DHT: /* Define Huffman table.  We just skip it for later decompression. */
            {
                unsigned short length = (data [0] << 8) | data [1];
                const unsigned char *end = data + length;
                
                JPEG_Assert (length >= 2);
                data += 2;
                while (data < end)
                {
                    unsigned char pair, type, slot;
                    
                    pair = *data ++;
                    type = pair >> 4;
                    slot = pair & 15;
                    
                    JPEG_Assert (type == 0 || type == 1);
                    JPEG_Assert (slot <= 15);
                    
                    if (type == 0)
                        decoder->dcTables [slot] = data;
                    else
                        decoder->acTables [slot] = data;
                        
                    if (!JPEG_HuffmanTable_Skip (&data))
                        return 0;
                }
                
                JPEG_Assert (data == end);
                break;
            }
            
            case JPEG_Marker_DQT: /* Define quantization table. */
            {
                unsigned short length = (data [0] << 8) | data [1];
                const unsigned char *end = data + length;
                int col, row;
                JPEG_FIXED_TYPE *s;
                
                JPEG_Assert (length >= 2);
                data += 2;
                
                while (data < end)
                {
                    int pair, slot, precision;
                    
                    pair = *data ++;
                    precision = pair >> 4;
                    slot = pair & 15;
                    
                    JPEG_Assert (precision == 0); /* Only allow 8-bit. */
                    JPEG_Assert (slot < 4); /* Ensure the slot is in-range. */
                    JPEG_Assert (data + 64 <= end); /* Ensure it's the right size. */
                    
                    s = decoder->quantTables [slot];
                   
                    for (c = 0; c < JPEG_DCTSIZE2; c ++)
                        s [c] = JPEG_ITOFIX (*data ++);
                    
                    /* Multiply against the AAN factors. */
                    for (row = 0; row < JPEG_DCTSIZE; row ++)
                        for (col = 0; col < JPEG_DCTSIZE; col ++)
                        {
                            JPEG_FIXED_TYPE *item = &s [col + row * JPEG_DCTSIZE];
                            
                            *item = JPEG_FIXMUL (*item, JPEG_AANScaleFactor [JPEG_ToZigZag [row * JPEG_DCTSIZE + col]]);
                        }
                }
                
                JPEG_Assert (data == end); /* Ensure we've finished it. */
                break;
            }
        
            case JPEG_Marker_DRI: /* Define restart interval. */
                JPEG_Assert (((data [0] << 8) | data [1]) == 4); /* Check the length. */
                decoder->restartInterval = (data [2] << 8) | data [3];
                data += 4;
                break;
            
            case JPEG_Marker_SOF0: /* Start of Frame: Baseline Sequential Huffman. */
                if (!JPEG_FrameHeader_Read (&decoder->frame, &data, marker))
                    return 0;
                break;
            
            case JPEG_Marker_SOS: /* Start of scan, immediately followed by the image. */
                if (!JPEG_ScanHeader_Read (&decoder->scan, &data))
                    return 0;
                *dataBase = data;
                return 1;
                
            default: /* No known marker of this type. */
                JPEG_Assert (0);
                break;
        }
    }
}

/* Perform the two steps necessary to decompress a JPEG image.
 * Nothing fancy about it.
 */
int JPEG_DecompressImage (const unsigned char *data, volatile JPEG_OUTPUT_TYPE *out, int outWidth, int outHeight)
{
    JPEG_Decoder decoder;

    if (!JPEG_Decoder_ReadHeaders (&decoder, &data))
        return 0;
    if (!JPEG_Decoder_ReadImage (&decoder, &data, out, outWidth, outHeight))
        return 0;

    return 1;
}

/* Return whether this code is a JPEG file.  Unfortunately it will incorrectly
 * match variants such as JPEG 2000 and JPEG-LS.  A better function would
 * skip known markers until it reaches an unknown marker or a handled
 * SOFn.
 */
 
int JPEG_Match (const unsigned char *data, int length)
{
    if (length == 0) return 0;
    if (data [0] != 0xFF) return 0;
    if (length == 1) return 1;
    if (data [1] != 0xD8) return 0;
    if (length == 2) return 1;
    return 1;
    if (data [2] != 0xFF) return 0;
    if (length == 3) return 1;
    if (data [3] < 0xC0 || data [3] > 0xCF) return 0;
    if (data [3] == 0xC0) return 1;
    return 0;
}
