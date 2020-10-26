//Edited for VNDS on 15-nov-2008
//* Set JPEG_DEBUG to 1
//* Changed the JPEG_Assert implementation

#ifndef GBA_JPEG_H
#define GBA_JPEG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef JPEG_DEBUG
#define JPEG_DEBUG 1
    /**< Enable assertion checks for input.  This is useful if you want to use
     * the library in non-embedded environments, such as to confirm that a
     * JPEG file will be compliant.
     *
     * Enabling this will define JPEG_Assert(TEST) if it's not predefined.
     * The default prints some information on the error to stderr and then
     * returns zero.
     */
#endif /* JPEG_DEBUG */

#ifndef JPEG_HANDLE_ANY_FACTORS
#define JPEG_HANDLE_ANY_FACTORS 1
    /**< If this is set, any component factors are valid.  Otherwise
     * it will only handle 2:1:1 (the typical form that sacrifices colour
     * resolution).  Note that Photoshop will only generate such files if you
     * use Save for Web.  Resetting this saves 508 bytes of IWRAM.
     */
#endif /* JPEG_HANDLE_ANY_FACTORS */

#ifndef JPEG_FASTER_M211
#define JPEG_FASTER_M211 1
    /**< If this is set, then the most common JPEG format is not given
     * special, faster treatment.  You must set JPEG_HANDLE_ANY_FACTORS
     * in this case, or you will not see anything.  Resetting this saves
     * 532 bytes of IWRAM, at the cost of speed.
     */
#endif /* JPEG_FASTER_M211 */

#ifndef JPEG_USE_IWRAM
#define JPEG_USE_IWRAM 0
    /**< If this is set, the JPEG decompressor will use IWRAM for huge
     * benefits to decompression speed (249% faster than reset).  Resetting
     * this saves up to 3348 bytes of IWRAM, depending upon
     * JPEG_HANDLE_ANY_FACTORS and JPEG_FASTER_M211.
     */
#endif /* JPEG_USE_IWRAM */

#define JPEG_DCTSIZE 8
    /**< The number of samples across and down a JPEG DCT.  This cannot be
     * configured, as the inverse DCT only handles 8x8.
     */

#define JPEG_DCTSIZE2 (JPEG_DCTSIZE * JPEG_DCTSIZE)
    /**< The number of samples in a full 2-D DCT. */

#ifndef JPEG_MAXIMUM_COMPONENTS
#define JPEG_MAXIMUM_COMPONENTS 4
    /**< The maximum number of components that can be involved in an image.
      * Each value costs 8 bytes of stack space and 8 bytes of allocations.
      */
#endif /* JPEG_MAXIMUM_SCAN_COMPONENTS */

#ifndef JPEG_FIXSHIFT
#define JPEG_FIXSHIFT 8
    /**< The shift used for converting to and from fixed point.  A higher value
      * here (up to 10 for 32-bit) results in better quality; a lower value
      * (down to 2) results in lesser quality.  Lower values can be somewhat
      * faster depending upon the hardware's clockings for multiplication.
      */
#endif /* JPEG_FIXSHIFT */

#ifndef JPEG_MAXIMUM_SCAN_COMPONENT_FACTORS
#define JPEG_MAXIMUM_SCAN_COMPONENT_FACTORS 10
    /**< The limit of the sum of the multiplied horizontal scaling factors in
      * the components.  For example, if Y is 1x1, Cb is 2x2, and Cr is 2x2,
      * that comes out to (1 * 1 + 2 * 2 + 2 * 2), or 9.  The limit here is
      * what is specified in the standard (B.2.3).
      */
#endif /* JPEG_MAXIMUM_SCAN_COMPONENT_FACTORS */

#ifndef JPEG_FIXED_TYPE
#define JPEG_FIXED_TYPE long int
    /**< The fixed data type.  This requires a minimum size of
      * JPEG_FIXSHIFT plus 12.
      */
#endif /* JPEG_FIXED_TYPE */

/* If this value is defined as 1, then it outputs to RGB in 32-bit words, with
 * red in the first eight bits, green in the second eight bits, and blue in the
 * third eight bits.
 */

#if JPEG_OUTPUT_RGB8
    #define JPEG_OUTPUT_TYPE unsigned int

    #define JPEG_Convert_Limit(VALUE) ((VALUE) < 0 ? 0 : (VALUE) > 255 ? 255 : (VALUE))

    #define JPEG_Convert(OUT, Y, Cb, Cr) \
        do { \
            int eY = (Y) + 63; \
            int R = ((eY) + ((Cr) * 359 >> 8)) * 2; \
            int G = ((eY) - ((Cb) * 88 >> 8) - ((Cr) * 183 >> 8)) * 2; \
            int B = ((eY) + ((Cb) * 454 >> 8)) * 2; \
            \
            R = JPEG_Convert_Limit (R); \
            G = JPEG_Convert_Limit (G) << 8; \
            B = JPEG_Convert_Limit (B) << 16; \
            (OUT) = R | G | B; \
        } while (0)

    #define JPEG_Convert_From(IN, Y, Cb, Cr) \
        do { \
            int R = IN & 255; \
            int G = (IN >> 8) & 255; \
            int B = (IN >> 16) & 255; \
            \
            Y = (((R * 66 >> 8) + (G * 129 >> 8) + (B * 25 >> 8)) >> 1) - 63; \
            Cb = ((R * -38 >> 8) + (G * -74 >> 8) + (B * 112 >> 8)) >> 1; \
            Cr = ((R * 112 >> 8) + (G * -94 >> 8) + (B * 18 >> 8)) >> 1; \
        } while (0)
#endif /* JPEG_OUTPUT_RGB8 */

#ifndef JPEG_OUTPUT_TYPE
#define JPEG_OUTPUT_TYPE unsigned short
    /**< This is the data type that JPEG outputs to.  The interpretation of
     * this type is dependent upon JPEG_Convert.
     */
#endif /* JPEG_OUTPUT_TYPE */

#ifndef JPEG_Convert
/** Convert YCbCr values (each in the nominal range -64 to 63) to RGB and store
  * in the output value (of type JPEG_OUTPUT_TYPE).  By default this stores to
  * 15-bit RGB.
  */

#define JPEG_Convert(OUT, Y, Cb, Cr) \
    do { \
        int eY = (Y) + 63; \
        int R = (eY) + ((Cr) * 359 >> 8); \
        int G = (eY) - ((Cb) * 88 >> 8) - ((Cr) * 183 >> 8); \
        int B = (eY) + ((Cb) * 454 >> 8); \
        \
        R = ComponentRange [R >> 2]; \
        G = ComponentRange [G >> 2] << 5; \
        B = ComponentRange [B >> 2] << 10; \
        (OUT) = R | G | B | (1 << 15); \
    } while (0)

#endif /* JPEG_Convert  */

#ifndef JPEG_Assert
    #if JPEG_DEBUG
        #include <stdio.h>
		#include <stdlib.h>
		#include <string.h>
        #include "vnlog.h"

		#define __FILENAME__ (strrchr(__FILE__,'/') ? strrchr(__FILE__,'/')+1 : __FILE__)

        #define JPEG_Assert(TEST) \
            do { \
                if (TEST) \
                    break; \
                vnLog(EL_warning, "gba-jpeg", "%s(%d) Assertion Failed: " #TEST, __FILENAME__, __LINE__); \
                return 0; \
            } while (0);
    #else
        #define JPEG_Assert(TEST) do { } while (0)
    #endif /* JPEG_DEBUG */
#endif /* JPEG_Assert */

/** The markers that can appear in a JPEG stream. */
enum JPEG_Marker
{
    JPEG_Marker_APP0 = 0xFFE0, /**< Reserved application segment 0. */
    JPEG_Marker_APP1 = 0xFFE1, /**< Reserved application segment 1. */
    JPEG_Marker_APP2 = 0xFFE2, /**< Reserved application segment 2. */
    JPEG_Marker_APP3 = 0xFFE3, /**< Reserved application segment 3. */
    JPEG_Marker_APP4 = 0xFFE4, /**< Reserved application segment 4. */
    JPEG_Marker_APP5 = 0xFFE5, /**< Reserved application segment 5. */
    JPEG_Marker_APP6 = 0xFFE6, /**< Reserved application segment 6. */
    JPEG_Marker_APP7 = 0xFFE7, /**< Reserved application segment 7. */
    JPEG_Marker_APP8 = 0xFFE8, /**< Reserved application segment 8. */
    JPEG_Marker_APP9 = 0xFFE9, /**< Reserved application segment 9. */
    JPEG_Marker_APP10 = 0xFFEA, /**< Reserved application segment 10. */
    JPEG_Marker_APP11 = 0xFFEB, /**< Reserved application segment 11. */
    JPEG_Marker_APP12 = 0xFFEC, /**< Reserved application segment 12. */
    JPEG_Marker_APP13 = 0xFFED, /**< Reserved application segment 13. */
    JPEG_Marker_APP14 = 0xFFEE, /**< Reserved application segment 14. */
    JPEG_Marker_APP15 = 0xFFEF, /**< Reserved application segment 15. */
    JPEG_Marker_COM = 0xFFFE, /**< Comment. */
    JPEG_Marker_DHT = 0xFFC4, /**< Define huffman table. */
    JPEG_Marker_DQT = 0xFFDB, /**< Define quantization table(s). */
    JPEG_Marker_DRI = 0xFFDD, /**< Define restart interval. */
    JPEG_Marker_EOI = 0xFFD9, /**< End of image. */
    JPEG_Marker_SOF0 = 0xFFC0, /**< Start of Frame, non-differential, Huffman coding, baseline DCT. */
    JPEG_Marker_SOI = 0xFFD8, /**< Start of image. */
    JPEG_Marker_SOS = 0xFFDA /**< Start of scan. */
};

typedef enum JPEG_Marker JPEG_Marker;
typedef JPEG_FIXED_TYPE JPEG_QuantizationTable [JPEG_DCTSIZE2]; /**< Quantization table elements, in zigzag order, fixed. */

/** Compute the multiplication of two fixed-point values. */
#define JPEG_FIXMUL(A, B) ((A) * (B) >> JPEG_FIXSHIFT)

/** Convert a fixed-point value to an integer. */
#define JPEG_FIXTOI(A) ((A) >> JPEG_FIXSHIFT)

/** Convert an integer to a fixed-point value. */
#define JPEG_ITOFIX(A) ((A) << JPEG_FIXSHIFT)

/** Convert a floating-point value to fixed-point. */
#define JPEG_FTOFIX(A) ((int) ((A) * JPEG_ITOFIX (1)))

/** Convert a fixed-point value to floating-point. */
#define JPEG_FIXTOF(A) ((A) / (float) JPEG_ITOFIX (1))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GBA_JPEG_H */
