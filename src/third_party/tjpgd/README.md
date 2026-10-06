# TJpgDec R0.03

Vendored from https://github.com/Bodmer/TJpg_Decoder at
71bfc2607b6963ee3334ff6f601345c5d2b7a8da (src/tjpgd.c, tjpgd.h, tjpgdcnf.h).
ChaN copyright/license notices are retained in the source files; Bodmer's
byte-swap extension is retained. No Arduino wrapper or global decoder is used.
RGB565, 512-byte input buffer, scaling enabled, FASTDECODE=1.

Local change: IDCT intermediates and dequantization products use int64_t
to avoid signed overflow observed under UBSan on damaged JPEG fixtures.
