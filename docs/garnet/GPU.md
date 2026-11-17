# Adreno 710 on garnet

## Results

| | Before | Now |
|---|---|---|
| GPU core clock | 295 MHz at every level | 345-940 MHz, `adreno_speed=8` = 940 MHz |
| Home-screen page swipes | about 36 fps | 62-68 fps |
| GMEM (tile) rendering | hung (SMMU fault at address 0) | works |
| Rearrange-mode delete badges, "Done" label | dark green squares, a dark box | round grey badges, plain text |
| Fast page swipes with GMEM | coloured blocks beside moving icons | clean |
| Frames on the panel | copied across the line being scanned (torn) | written behind the scan |
| Page swipes, frames over 16.7 ms | 10-11% | 1-6% |
| Settings list scroll | 62 fps, a third of the frames late | 60 fps, none late |
| CoreAnimation's full-screen GMEM pass | 10.7 ms | 8.0 ms |
| Opening Control Center | 2-3 frames (the window server stalled 2.5 s) | animates, about 35 fps |
| Opening an app, finger up to its first frame | about 330 ms | about 120 ms |

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

**Constant copies in tiles.** CoreAnimation binds its uniforms at 8-byte offsets, which `CP_LOAD_STATE6` cannot load
from, so for each such draw the command processor copies them into the draw's own table and waits for the copy
(`CP_WAIT_MEM_WRITES`, `CP_WAIT_FOR_ME`). In a GMEM pass that copy and its waits ran again in every tile. They now run
once before the tiles (`encodeInPass`'s `hoisted` stream); `metal_gmem_off=8192` puts them back in the draws.

**The panel's scan.** The boot loader leaves the panel (its 120 Hz timing, 2752 lines a frame) refreshing 60 times a
second, and the display interface's line counter counts each line twice (up to 5504). The display driver's tear-free
mode (`fb_beam`: frames written behind the line being scanned) refused that counter and stayed off, so every frame was
copied into the panel's memory across the scan. It now reads the frame's length from the interface's registers
(`INTF_VSYNC_PERIOD_F0` over the line length in `INTF_HSYNC_CTL`), accepts a counter of two counts a line, and starts
3 s after boot instead of a minute.

**Pipelines waiting for specialisations.** All shader compilations share one compiler and one lock. The plug-in
specialises CoreAnimation's uber shaders on a background queue (200-440 ms each, the uber shader drawing meanwhile),
while the window server makes each new pipeline on its render thread, in bursts as a screen first appears. Those waited
behind the queue: Control Center's 22 pipelines took 2.5 s to make, 36 ms of it compiling. The lock now serves a
waiting foreground compilation before any background one (`hakios_metal_compile_background`), and a specialisation
starts only after 400 ms without a new pipeline. The `linked ...` log line says how long each pipeline took and how
much of it was waiting.

**Frames waiting for the panel.** The Metal boot arguments use `fb_queue=2` (the driver's default): with `fb_queue=1`
the window server waited in its swap for the frame before it to be taken.

## Known issues

- Control Center draws two full-screen passes: 35-40 fps.
- Opening and closing apps: 30-40 fps. Their frames draw some 15 full-screen layers (wallpaper, the app's snapshot,
  backdrops), close to what the GPU can fill at 1125x2436.
- SpringBoard spends about 100 ms on its main thread after a tap before the launch animation's first frame.
- The first boot of a session compiles CoreAnimation's shaders anew (about 4 s before the lock screen's first frames):
  the compiled stages are kept only with `var_store=1`.
- The panel refreshes at 60 Hz. The boot loader runs its 120 Hz timing on a half-rate pixel clock (no adaptive
  refresh and no front porch to shorten), so 120 Hz needs the DSI clock reprogrammed.
- `adreno_vote=1` (GMU bandwidth vote) makes the GMU fail; do not use it.

## Arguments

`gpu=1 gl=1 adreno_speed=8 ddr=2 metal=1 metal_all=1 metal_gmem=1 metal_gmem_draws=24`
