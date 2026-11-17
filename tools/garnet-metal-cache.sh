#!/bin/bash
# The window server's compiled shader stages, carried in the system image so that no boot compiles them again.
#
# Every boot runs from RAM and starts with no compiled shaders: the window server compiles CoreAnimation's on first
# use (some 4 s at boot, and again as each new screen first appears). A run collects them, the loader hands them to
# the computer through RAM (nothing is written to the phone's storage), and the next image carries them in
# /redwood/share/hakios-metal-cache, where the Metal plug-in looks (HM_CACHE_SEED).
#
#   tools/garnet-metal-cache.sh build    out/stage_put (userland/src/stage_put.c)
#   then: tools/garnet-wifi-image.sh FIRMWARE-DIR out/root/root-cache.apfs devices/garnet/rc.jobs.cache
#         boot that image and let the run end on its own (its last job hands the stages to the kernel)
#   tools/garnet-metal-cache.sh pull     fastboot oem recording + get_staged, unpacked into userland/hakios-metal-cache
#   tools/garnet-metal-cache.sh unpack F the same from a file fetched by hand (Windows: fastboot oem recording;
#                                        fastboot get_staged stage.bin)
#   then: tools/garnet-wifi-image.sh FIRMWARE-DIR    the image with the stages in it
#
# The stages are compiled from Apple's shaders: they stay out of the repository (userland/ is not tracked). They are
# good for the plug-in build that made them; a build whose compiler changed looks for other names and compiles again.
set -e
cd "$(dirname "$(readlink -f "$0")")/.."
case ${1:-} in
build)
	SDK=${THEOS:-$HOME/theos}/sdks/iPhoneOS16.5.sdk
	mkdir -p out
	xnu-clang -arch arm64 -isysroot "$SDK" -O1 -Wall -o out/stage_put userland/src/stage_put.c
	echo "garnet-metal-cache.sh: out/stage_put"
	;;
pull)
	fastboot oem recording
	fastboot get_staged out/stage.bin
	python3 tools/metal-cache-unpack.py out/stage.bin userland/hakios-metal-cache
	;;
unpack)
	python3 tools/metal-cache-unpack.py "${2:?usage: tools/garnet-metal-cache.sh unpack FILE}" userland/hakios-metal-cache
	;;
*)
	sed -n '2,22p' "$0" | sed 's/^# \{0,1\}//'
	exit 2
	;;
esac
