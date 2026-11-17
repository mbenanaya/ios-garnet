# Adreno 710 on garnet

## Results

| | Before | Now |
|---|---|---|
| GPU core clock | 295 MHz at every level | 345-940 MHz, `adreno_speed=8` = 940 MHz |
| Home-screen page swipes | about 36 fps | 62-68 fps |
| GMEM (tile) rendering | hung (SMMU fault at address 0) | works |
| Rearrange-mode delete badges, "Done" label | dark green squares, a dark box | round grey badges, plain text |
| Fast page swipes with GMEM | coloured blocks beside moving icons | clean |

## Findings

**The GPU clock.** The boot chain leaves `gpu_cc_pll0` unprogrammed, so the GPU's root clock was the PLL's bypass rate
divided by two (295 MHz) whatever level was asked for. The kernel now programs the PLL itself (Lucid EVO: L value at +0x10,
alpha at +0x14, mode register bit 14 latch / bit 13 ack, bit 31 lock) for twice the wanted core clock, as Linux's
`clk-alpha-pll.c` does. `adreno_pll=0` leaves the old behaviour.

**GMEM rendering.** A7xx needs the binning pass's stream addresses set with `CP_SET_PSEUDO_REG`
(8 `DRAW_STRM_ADDRESS`, 9 `DRAW_STRM_SIZE_ADDRESS`, 10 `PRIM_STRM_ADDRESS`). Without them the GPU reads streams at
address 0. `hakios-metal` now emits it in the binning setup on A7xx.

**Missing constant buffers** are bound as zero-filled buffers instead of being skipped.

**Stand-in shader variants.** The iOS 16.5 image has no generic CoreAnimation shaders, only Apple's precompiled
variants (`default.pipelinelib`). For a pipeline CoreAnimation makes at run time, the plug-in takes the nearest
variant by label. Some of those read a different channel of their image than the one asked for: `..._Igrn` (alpha in
green) got `..._Ialp`, `..._Isrg` (grey and alpha in RG8, read r r r g) got `..._Isrc`. Grey-and-alpha images then
drew as opaque squares. The plug-in now binds such an image through a texture swizzle that turns the stand-in's read
into the one asked for (`hm_input_swizzle` in `userland/src/HakiosMetalCA.m`).

**GMEM loads and framebuffer fetch.** On A7xx a tile's load (an event blit) stays in the blit cache until a
`CCU_RESOLVE_CLEAN`. A framebuffer fetch reads GMEM through UCHE, so before that event it read the bin as the previous
tile left it: during page swipes, blocks of the right tile column one tile row up showed in the left column. Each
tile whose draws read their target now sends `CCU_RESOLVE_CLEAN` after its loads, before the cache invalidate (as
turnip's `TU_CMD_FLAG_BLIT_CACHE_CLEAN`).

## Known issues

- Control Center draws two full-screen passes: 35-40 fps.
- `adreno_vote=1` (GMU bandwidth vote) makes the GMU fail; do not use it.

## Arguments

`gpu=1 gl=1 adreno_speed=8 ddr=2 metal=1 metal_all=1 metal_gmem=1 metal_gmem_draws=24`
