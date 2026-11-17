# Wi-Fi on garnet

## Result

Settings lists 2.4 and 5 GHz networks and joins WPA2 networks; DHCP, DNS and Safari work. The radio is up about 6 s after
the kernel starts it, beside the system's boot.

## The hardware

| | |
|---|---|
| Radio | WCN3990-class ("adrastea"), chip id 0x320, family 0x4001 |
| Firmware | runs on the WPSS DSP (`wpss.mdt` + `wpss.bNN`, loaded at 0x85300000, PAS id 6) |
| Registers | 0x22800000, 8 MB |
| Firmware memory (MSA) | 0x82a00000, 3 MB |
| SMMU stream | 0x4c0 (apps SMMU) |
| Interrupts | 12 level-triggered SPIs: 0x185 0x19d 0x1a0 0x1a1 0x1b2 0x1b3 0x1b6 0x1b7 0x1b8 0x1c3 0x1c5 0x1c6 |
| Supply rails (RPMh) | ldob11 752 mV, ldoe1 1800 mV, ldob14 1304 mV, ldoe11 3312 mV, ldoe10 3312 mV |
| Board data | Xiaomi's `bd_n16*.bin` by region (board id 0xff) |
| Linux driver | `ath10k` (snoc bus); Android's own is `icnss2` |

## How it is built

1. `drivers/wifi/hakios.wifi.c` loads the firmware into the WPSS DSP through TrustZone, starts it, and brings up
   GLINK and QRTR to its Wi-Fi firmware service.
2. Linux's `ath10k` (unmodified, Linux 7.0.14) runs on FreeBSD's LinuxKPI headers and this repository's `wifi/lx`
   layer, compiled into one object (`tools/mk-ath10k.sh`) that the kernel links.
3. `wifi/lx/src10k/lx_snoc.c` is the platform the driver expects: resources, rails, interrupts, firmware memory.
4. `drivers/wifi/hakios.apple80211.c` and `hakios.wlan.cpp` answer `wifid` and SystemConfiguration, so Settings
   sees a Wi-Fi interface (`en0`).

## Findings

- The radio sleeps when idle; reading its registers then stops the kernel. Its handlers run only on real interrupts.
- The firmware needs the supply rails on before it reports "ready".
- ath10k reads a firmware feature file (`firmware-5.bin`) that Linux ships; the kernel generates its equivalent.
- Without a regulatory domain ath10k transmits at 0 dBm; the driver's own domain is applied.
- configd must run, and `en0` must exist from the start, or Settings hangs and joins take a minute.

## Limits

No WPA3. Channels 12-14 off. On 5 GHz a network is listed only once its access point has been heard. Nothing is
remembered across reboots.

## Arguments

`wifi=1`; optional `wifi_mac=XX:XX:XX:XX:XX:XX`. Step-by-step bring-up for debugging: `wifi_step=N` (see
[docs/boot-args.md](../boot-args.md)).
