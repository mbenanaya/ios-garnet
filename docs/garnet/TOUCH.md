# Touch on garnet

Garnet ships with one of two touch chips. Both sit on the same SPI bus (QUP1 SE3, `0x00a8c000`) and use the same pins.
Android's device tree node `soc/spi@a8c000/n16-touch@0` lists both: `compatible = "goodix,gt9916S", "focaltech,n16-3683g-spi"`.

- Goodix GT9916S. Android's driver is `gtx8_spi` (Goodix "Berlin"). The phone reports `rom_pid BERLIN`,
  `patch_pid 9916R`, firmware `goodix_firmware_CSOT.bin`.
- FocalTech FT3683G. Android's driver is `fts_ts`.

Pins and supplies, from Android's device tree. They are the same for both chips:

| | |
|---|---|
| SPI | GPIO 10 MISO, 11 MOSI, 12 CLK, 13 CS (`qup1_se3_l0..l3`), no pull |
| Reset | GPIO 96, pull-up |
| Interrupt | GPIO 98, active low, pull-up |
| AVDD | `pm6150l_l9` (RPMh `ldoe9`) |
| IOVDD | `touch_vddio`, a fixed 1.8 V regulator on PM6150L GPIO 3, `regulator-boot-on` |
| Panel | 1220 x 2712 |

Result: the driver used to hard-code FocalTech, so it never answered on a Goodix phone and every read returned zeros.
`drivers/input/hakios.focaltech.c` now tries Goodix first and falls back to FocalTech.
