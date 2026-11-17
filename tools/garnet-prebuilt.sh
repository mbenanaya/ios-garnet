#!/bin/bash
# The binary parts of the garnet port that this repository does not carry: the UEFI image, its device tree, the splash,
# BootShim, and the small root image (it holds Darwin's launchd job files, which are Apple's). They come from the
# POCO X6 5G device repository of HakiOS (hakios-sm7435, by Montfort and Jotan) at a pinned revision, into
# devices/garnet/prebuilt, devices/garnet/userland and devices/garnet/uefi/files/Resources/DTBs, which git ignores.
#
# usage: tools/garnet-prebuilt.sh
set -e
REPO=https://github.com/JosephMontfort/hakios-sm7435.git
REVISION=3ab42b4fcc2989f7e92c4d9fdf3e4fd75d765a88
cd "$(dirname "$(readlink -f "$0")")/.."
DEV=devices/garnet
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
git clone -q --filter=blob:none --no-checkout "$REPO" "$TMP/src"
git -C "$TMP/src" sparse-checkout set --no-cone /prebuilt/ /userland/root.apfs /uefi/files/Resources/DTBs/
git -C "$TMP/src" checkout -q "$REVISION"
mkdir -p "$DEV/prebuilt" "$DEV/userland" "$DEV/uefi/files/Resources/DTBs"
cp -a "$TMP/src/prebuilt/." "$DEV/prebuilt/"
cp -a "$TMP/src/userland/root.apfs" "$DEV/userland/root.apfs"
cp -a "$TMP/src/uefi/files/Resources/DTBs/." "$DEV/uefi/files/Resources/DTBs/"
echo "garnet-prebuilt.sh: $(ls "$DEV/prebuilt" | wc -l) prebuilt files, root.apfs and the UEFI device tree, at ${REVISION:0:7}"
