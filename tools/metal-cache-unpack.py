#!/usr/bin/env python3
"""metal-cache-unpack.py BLOB DIR: the compiled shader stages a run staged with stage_put (kern.hakios.stage, fetched
with "fastboot oem recording" and "fastboot get_staged BLOB") written into DIR as *.stage files, the ones DIR had
already kept; then DIR is what tools/garnet-root-apfs.sh --metal-cache puts into the image's seed directory."""
import os
import struct
import sys

if len(sys.argv) != 3:
    sys.exit(__doc__)
data = open(sys.argv[1], 'rb').read()
out = sys.argv[2]
if data[:8] != b'HKSTAGE1':
    sys.exit('metal-cache-unpack: %s is not a stage_put blob (%r)' % (sys.argv[1], data[:8]))
count = struct.unpack_from('<I', data, 8)[0]
os.makedirs(out, exist_ok=True)
at, wrote, skipped = 12, 0, 0
for _ in range(count):
    n = struct.unpack_from('<I', data, at)[0]
    name = data[at + 4:at + 4 + n].decode('utf-8', 'replace')
    size = struct.unpack_from('<Q', data, at + 4 + n)[0]
    body = data[at + 12 + n:at + 12 + n + size]
    at += 12 + n + size
    # only finished stages, by plain names (a half-written one is a .tmp; nothing with a path in it)
    if not name.endswith('.stage') or '/' in name or name.startswith('.') or len(body) != size:
        skipped += 1
        continue
    with open(os.path.join(out, name), 'wb') as f:
        f.write(body)
    wrote += 1
print('metal-cache-unpack: %d stages written to %s (%d skipped); %d there now' %
      (wrote, out, skipped, len([x for x in os.listdir(out) if x.endswith('.stage')])))
