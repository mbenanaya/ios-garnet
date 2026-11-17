# Notes for contributors and coding agents

- `main` holds what works on the phone; work goes to `dev`.
- Boots are RAM-only. Never add steps that flash, wipe or write to the phone's storage without saying so plainly.
- Never commit Apple files (IPSW contents, shared caches, frameworks) or vendor firmware. Users copy firmware from their
  own phone.
- Never commit anything that identifies a particular phone or person: serial numbers, MAC addresses, network names,
  passwords, local paths.
- The kernel's changes live in `xnu/` (patch and added files) and `drivers/`; the build copies them into `xnu-src/`.
- After adding or changing a boot argument run `python3 tools/boot-args.py --markdown > docs/boot-args.md`,
  `--lst > configs/boot-args.lst`, `--symbols > configs/symbols.tsv`, then `--check`.
- Record results in `docs/garnet/`: the finding and the final result, not the steps that led there.
