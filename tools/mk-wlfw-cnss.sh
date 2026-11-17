#!/bin/bash
# Give the kernel's build the descriptions of the messages an "adrastea" (WCN3990-class) radio's firmware speaks, for
# drivers/wifi/hakios.wlfw.c: the phone's own. They are taken out of the Android kernel's sources as they are -- the
# structures of cnss_utils' wlan_firmware_service_v01.h and the tables of its .c, which is what the phone's icnss2
# driver talks to the same firmware with -- and written as one header into the kernel's build directory, where the
# compiler finds it and git does not: that text is GPL-2.0-only and is not kept in the repository.
# Without that header (and without ath11k's, tools/mk-wlfw.sh) hakios.wlfw.c compiles to a line that says so.
#
# usage: tools/mk-wlfw-cnss.sh [KERNEL-TREE]      then build the kernel (./xnu-build.sh); again after a clean build
#        KERNEL-TREE: a checkout of the phone's kernel (MiCode Xiaomi_Kernel_OpenSource, branch garnet-t-oss).
#        Only needed for the step-by-step bring-up (wifi_step=15 without wifi_lx); wifi=1 does not use it.
set -e
ROOT=$(cd "$(dirname "$(readlink -f "$0")")/.." && pwd)
source "$ROOT/xnu-env.sh"
SRC=${1:?usage: tools/mk-wlfw-cnss.sh KERNEL-TREE}/drivers/net/wireless/cnss_utils
[ -f "$SRC/wlan_firmware_service_v01.c" ] || { echo "mk-wlfw-cnss.sh: no $SRC/wlan_firmware_service_v01.c" >&2; exit 1; }
OUT=$XNU_SRC/BUILD/obj/DEVELOPMENT_ARM64_${MACHINE:-BCM2837}/bsd/DEVELOPMENT
[ -d "$OUT" ] || { echo "mk-wlfw-cnss.sh: no build directory $OUT: build the kernel once first" >&2; exit 1; }
python3 - "$SRC" > "$OUT/hakios.wlfw_cnss.h.new" <<'PY'
import re
import sys

src = sys.argv[1]
h = open(src + '/wlan_firmware_service_v01.h').read()
c = open(src + '/wlan_firmware_service_v01.c').read()
print('/* cnss_utils\' wlan_firmware_service_v01.h and the message tables of its .c (the phone\'s Android kernel,')
print(' * GPL-2.0-only), put together by tools/mk-wlfw-cnss.sh: not to be edited, not to be kept. */')
print('''#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
''')
# the header: without what it includes of Linux, and without the tables' declarations (they are static here)
h = re.sub(r'^#include [<"][^>"]*[>"]\n', '', h, flags=re.M)
h = re.sub(r'^extern struct qmi_elem_info \w+\[\];\n', '', h, flags=re.M)
print(h)
# the tables, in the file's order (a table is used by the ones after it), every one static: most are not used
# (only the tables: the file ends with a module's own functions, which are Linux's to run)
tables = re.findall(r'^(?:static )?struct qmi_elem_info \w+\[\] = \{.*?\n\};\n', c, re.S | re.M)
c = '\n'.join(re.sub(r'^(?:static )?struct qmi_elem_info', 'static struct qmi_elem_info', t) for t in tables)
print('#pragma clang diagnostic push')
print('#pragma clang diagnostic ignored "-Wunused-variable"')
print(c)
print('#pragma clang diagnostic pop')
PY
cmp -s "$OUT/hakios.wlfw_cnss.h.new" "$OUT/hakios.wlfw_cnss.h" 2>/dev/null && rm "$OUT/hakios.wlfw_cnss.h.new" ||
	mv "$OUT/hakios.wlfw_cnss.h.new" "$OUT/hakios.wlfw_cnss.h"
echo "mk-wlfw-cnss.sh: $OUT/hakios.wlfw_cnss.h ($(grep -c 'qmi_elem_info .*\[\] = {' "$OUT/hakios.wlfw_cnss.h") tables)"
