# Media fixtures

Original test images generated with Pillow: four color quadrants (64×48),
constant grayscale (17×9 and 4096×8), solid green (241×269), progressive
and CMYK JPEG variants. metadata.jpg inserts an APP1 segment containing
an embedded JPEG-like SOI/EOI pair; two_frames.mjpeg concatenates metadata
and grayscale frames with multipart boundaries. No downloaded images.

examples/sd/media contains an original 240×180 color-bars/title image and
30 baseline JPEG frames with a moving white ball, target 10 FPS.
