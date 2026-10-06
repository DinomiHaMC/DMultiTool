# DMultiTool

The project's original firmware, companion tools and tests are licensed under
GNU GPL version 3 only (`GPL-3.0-only`). See LICENSE for the full terms.

PikaPython is vendored under its MIT license, retained in
`src/third_party/pikapython/LICENSE`. Arduino ESP32, NimBLE-Arduino, Adafruit
and IRremote dependencies retain their respective licenses and copyright
notices. GPL licensing does not replace third-party license notices.

The directory and Arduino sketch filename remain `hacker-pro2000` to preserve
the existing checkout and Arduino build compatibility. User-facing branding
is DMultiTool. The legacy `hp2000` NVS namespace preserves saved settings.

TJpgDec R0.03 by ChaN, with Bodmer's byte-swap extension, is vendored in
`src/third_party/tjpgd/` from TJpg_Decoder commit
`71bfc2607b6963ee3334ff6f601345c5d2b7a8da`. Its permissive license and
copyright notice are retained at the top of `tjpgd.c` and `tjpgd.h`.
See the dependency README for provenance and configuration.
