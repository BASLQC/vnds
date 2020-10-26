/** @module gba-image_jpeg.h
 *
 *  A JPEG decompressor, targeted for the GameBoy Advance (although there
 * should be no machine-specific aspects if you disable JPEG_USE_IWRAM
 * and JPEG_MARK_TIME).  On the GBA it consumes, all with slight potential
 * variance:
 *
 * 3348 bytes of IWRAM, temporary
 * 7756 bytes of ROM
 * 4720 bytes of stack space, usually in IWRAM
 * 350 milliseconds for decompressing a representative image
 *
 * Unlike before when IWRAM was permanently used, it's now loaded in just
 * before decompression, allowing you to spend IWRAM on more tools called
 * constantly rather than one you call only once in awhile.  There is no
 * permanent IWRAM usage with this library.
 * 
 * It has a low capacitance for unusual JPEGs.  They cannot be progressive,
 * use arithmetic coding, have more than 4 components in a scan, and must be
 * 8-bit.  They can be colour or grayscale, and any component scaling factors
 * are valid (unless if JPEG_HANDLE_ANY_FACTORS is reset, in which case only
 * 2:1:1 is allowed).  The maximum component scale factors cannot be three.  In
 * general, you'll be all right, but if it doesn't like your input it will not
 * react sensibly in embedded.
 * 
 * This code is in the public domain.  JPEG is used for both its standard
 * meaning and for JFIF.
 * 
 * Revision 1: Inflicted stricter warnings, fixed C99-using code, and reduced
 *     allocation footprint (6144 bytes less).
 * Revision 2: Reduced ROM usage by 276 bytes, with the body going to 832 bytes
 *     of IWRAM.  I made it more configurable, particularly in YCbCr->RGB
 *     conversion.  Some brute force ROM usage reduction.
 * Revision 3: Removed all memset, malloc, and free dependencies.  This
 *     increases stack use drastically but also makes it completely
 *     self-sufficient.
 * Revision 4: Saved 6176 bytes of JPEG_Decoder state by exploiting an
 *     allowance of baseline JPEG decoding.  This requires 3088 more bytes of
 *     stack space, however.
 * Revision 5: Made the fixed-point shift configurable.  Can now be compiled
 *     with -ansi -pedantic, and fixed stack usage so that it is always
 *     predictable by exploiting a JPEG restriction.
 * Revision 6: A fixed type has been added and is configurable.  16-bit
 *     fixed is valid if you reduce JPEG_FIXSHIFT to 4 or lower.
 * Revision 7: Inserted assertions for when you're not on the embedded
 *     environment; good for confirming that a given file is compatible with
 *     the decompressor.  "this" is no longer used as a variable name.  Added
 *     necessary fluff for dealing with C++.
 *
 * - Burton Radons (loth@users.sourceforge.net)
 */

#ifndef GBA_JPEG_DECODE_H
#define GBA_IMAGE_JPEG_H

#include "gba-jpeg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct JPEG_HuffmanTable JPEG_HuffmanTable;
typedef struct JPEG_Decoder JPEG_Decoder;
typedef struct JPEG_FrameHeader JPEG_FrameHeader;
typedef struct JPEG_FrameHeader_Component JPEG_FrameHeader_Component;
typedef struct JPEG_ScanHeader JPEG_ScanHeader;
typedef struct JPEG_ScanHeader_Component JPEG_ScanHeader_Component;

/** A huffman table. */
struct JPEG_HuffmanTable
{
    const unsigned char *huffval; /**< Pointer to values in the table (256 entries). */
    int maxcode [16]; /**< The maximum code for each length - 1. */
    const unsigned char *valptr [16]; /**< Items are subtracted by mincode and then indexed into huffval. */
    
    unsigned char look_nbits [256]; /**< The lookahead buffer lengths. */
    unsigned char look_sym [256]; /**< The lookahead buffer values. */
};

/** An image component in the frame. */
struct JPEG_FrameHeader_Component
{
    unsigned char selector; /**< Component identifier, must be unique amongst the identifiers (C). */
    unsigned char horzFactor; /**< Horizontal sampling factor. */
    unsigned char vertFactor; /**< Vertical sampling factor. */
    unsigned char quantTable; /**< Quantization table destination selector. */
};

/** The frame header state. */
struct JPEG_FrameHeader
{
    JPEG_Marker marker; /**< The marker that began this frame header, one of JPEG_Marker_SOFn. */
    int encoding; /**< 0 for Huffman coding, 1 for arithmetic coding. */
    char differential; /**< Differential (1) or non-differential (0). */
    
    unsigned char precision; /**< Sample precision - precision in bits for the samples of the components in the frame. */
    unsigned short height; /**< Maximum number of lines in the source image, equal to the number of lines in the component with the maximum number of vertical samples.  0 indicates that the number of lines shall be defined by the DNL marker and parameters at the end of the first scan. */
    unsigned short width; /**< Number of samples per line in the source image, equal to the number of samples per line in the component with the maximum number of horizontal samples. */
    JPEG_FrameHeader_Component componentList [JPEG_MAXIMUM_COMPONENTS]; /**< Components. */
    int componentCount; /**< Number of components. */
};

/** A component involved in this scan. */
struct JPEG_ScanHeader_Component
{
    unsigned char selector; /**< Selector index corresponding to one specified in the frame header (Csj). */
    unsigned char dcTable; /**< DC entropy coding table destination selector (Tdj). */
    unsigned char acTable; /**< AC entropy coding table destination selector (Taj). */
};

/** Scan header state. */
struct JPEG_ScanHeader
{
    JPEG_ScanHeader_Component componentList [JPEG_MAXIMUM_COMPONENTS]; /**< Components involved in this scan. */
    int componentCount; /**< Number of components involved in this scan. */
    unsigned char spectralStart; /**< In DCT modes of operation, the first DCT coefficient in each block in zig-zag order which shall be coded in the scan (Ss).  For sequential DCT this is zero. */
    unsigned char spectralEnd; /**< Specify the last DCT coefficient in each block in zig-zag order which shall be coded in the scan. */
    unsigned char successiveApproximationBitPositionHigh; /**< (Ah). */
    unsigned char successiveApproximationBitPositionLow; /**< (Al). */
};

/** The complete decoder state. */
struct JPEG_Decoder
{
    const unsigned char *acTables [4]; /**< The AC huffman table slots. */
    const unsigned char *dcTables [4]; /**< The DC huffman table slots. */
    JPEG_QuantizationTable quantTables [4]; /**< The quantization table slots. */
    unsigned int restartInterval; /**< Number of MCU in the restart interval (Ri). */
    JPEG_FrameHeader frame; /**< Current frame. */
    JPEG_ScanHeader scan; /**< Current scan. */
};

/** Start reading bits. */
#define JPEG_BITS_START() \
    unsigned int bits_left = 0; \
    unsigned long int bits_data = 0
    
/** Rewind any bytes that have not been read from and reset the state. */
#define JPEG_BITS_REWIND() \
    do { \
        int count = bits_left >> 3; \
        \
        while (count --) \
        { \
            data --; \
            if (data [-1] == 0xFF) \
                data --; \
        } \
        \
        bits_left = 0; \
        bits_data = 0; \
    } while (0)
    
/** Fill the buffer. */
#define JPEG_BITS_CHECK() \
    do { \
        while (bits_left < 32 - 7) \
        { \
            bits_data = (bits_data << 8) | (*data ++); \
            if (data [-1] == 0xFF) \
                data ++; \
            bits_left += 8; \
        } \
    } while (0)
   
/** Return and consume a number of bits. */
#define JPEG_BITS_GET(COUNT) \
    ((bits_data >> (bits_left -= (COUNT))) & ((1 << (COUNT)) - 1))
    
/** Return a number of bits without consuming them. */
#define JPEG_BITS_PEEK(COUNT) \
    ((bits_data >> (bits_left - (COUNT))) & ((1 << (COUNT)) - 1))
    
/** Drop a number of bits from the stream. */
#define JPEG_BITS_DROP(COUNT) \
    (bits_left -= (COUNT))

/** Read a single unsigned char from the current bit-stream by using the provided table. */
#define JPEG_HuffmanTable_Decode(TABLE, OUT) \
    do { \
        int bitcount, result; \
        \
        result = JPEG_BITS_PEEK (8); \
        \
        if ((bitcount = (TABLE)->look_nbits [result]) != 0) \
        { \
            JPEG_BITS_DROP (bitcount); \
            result = (TABLE)->look_sym [result]; \
        } \
        else \
        { \
            int i = 7; \
            \
            JPEG_BITS_DROP (8); \
            do result = (result << 1) | JPEG_BITS_GET (1); \
            while (result > (TABLE)->maxcode [++ i]); \
            \
            result = (TABLE)->valptr [i] [result]; \
        } \
        \
        (OUT) = result; \
    } while (0)

//extern const unsigned char JPEG_ToZigZag [JPEG_DCTSIZE2]; /* Converts row-major indices to zig-zagged order. */
//extern const unsigned char JPEG_FromZigZag [JPEG_DCTSIZE2]; /* Converts zig-zagged indices to row-major order. */
//extern const JPEG_FIXED_TYPE JPEG_AANScaleFactor [JPEG_DCTSIZE2]; /* AA&N scaling factors for quantisation in fixed point. */
//extern const unsigned char JPEG_ComponentRange [32 * 3]; /* A limited component clamp that keeps values in the 0..31 range if incremented by 32. */

/** Return whether this data matches as a JPEG input stream.  You only need
  * to read four bytes.
  */
  
extern int JPEG_Match (const unsigned char *data, int length);

/** Read a FrameHeader segment (SOFn) and store the new data pointer in
  * *data.  Returns true on success and false on failure (failure isn't
  * possible if JPEG_DEBUG is reset).
  */

extern int JPEG_FrameHeader_Read (JPEG_FrameHeader *frame, const unsigned char **data, JPEG_Marker marker);

/** Read a HuffmanTable segment (DHT) and store the new data pointer in
  * *data.  Returns true on success and false on failure (failure isn't
  * possible if JPEG_DEBUG is reset).
  */
  
extern int JPEG_HuffmanTable_Read (JPEG_HuffmanTable *table, const unsigned char **data);

/** Skip a HuffmanTable segment (DHT) and store the new data pointer in
  * *data on success.  Returns true on success and false on failure (failure
  * isn't possible if JPEG_DEBUG is reset).
  */
  
extern int JPEG_HuffmanTable_Skip (const unsigned char **data);

/** Read a ScanHeader segment (SOS) and store the new data pointer in
  * *data.  Returns true on success and false on failure (failure isn't
  * possible if JPEG_DEBUG is reset).
  */
  
extern int JPEG_ScanHeader_Read (JPEG_ScanHeader *scan, const unsigned char **data);

/** Read all headers up to the start of the image and store the new data
  * pointer in *data.  Returns true on success and false on failure (failure
  * isn't possible if JPEG_DEBUG is reset).
  */
  
extern int JPEG_Decoder_ReadHeaders (JPEG_Decoder *decoder, const unsigned char **data);

/** Read the entire image from the *data value and then store the new data pointer.
  * Returns true on success and false on failure (failure isn't possible if
  * JPEG_DEBUG is reset).
  */
  
extern int JPEG_Decoder_ReadImage (JPEG_Decoder *decoder, const unsigned char **data, volatile JPEG_OUTPUT_TYPE *out, int outWidth, int outHeight);

/** Perform a 2D inverse DCT computation on the input.
  *
  * @param zz The coefficients to process, JPEG_DCTSIZE2 in length.  The
  *     contents will be destroyed in the computations.
  * @param chunk The chunk to store the results in, nominally from -64 to 63,
  *     although some error is expected.
  * @param chunkStride The number of values in a row for the chunk array.
  */

extern void JPEG_IDCT (JPEG_FIXED_TYPE *zz, signed char *chunk, int chunkStride);

/** Create a decompressor, read the headers from the provided data, and then
  * read the image into the buffer given.  Returns true on success and false on
  * failure (failure isn't possible if JPEG_DEBUG is reset).
  */
  
extern int JPEG_DecompressImage (const unsigned char *data, volatile JPEG_OUTPUT_TYPE *out, int outWidth, int outHeight);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GBA_IMAGE_JPEG_H */
