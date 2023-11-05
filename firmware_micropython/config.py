import framebuf

# mono_vlsb = 0
# rgb565    = 1
# gs4_hmsb  = 2
# mono_hlsb = 3
# mono hmsb = 4
# gs2_hmsb  = 5
# gs8       = 6
BIT_FORMAT = framebuf.GS4_HMSB

BIT_DEPTH = {
    framebuf.MONO_VLSB: 1,
    framebuf.RGB565:    2,
    framebuf.GS4_HMSB:  4,
    framebuf.MONO_HLSB: 1,
    framebuf.MONO_HMSB: 1,
    framebuf.GS2_HMSB:  2,
    framebuf.GS8:       8,
}[BIT_FORMAT]