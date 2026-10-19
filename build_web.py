"""Embed the standalone web UI into the firmware; no filesystem upload needed."""
from pathlib import Path
import gzip
import os
import shutil
import subprocess
root = Path(__file__).resolve().parent
node = os.environ.get('HAUNTCUE_NODE') or shutil.which('node')
if not node:
    raise SystemExit('Node.js is needed to compress the interface. Install Node.js, then run npm install before regenerating WebUI.h.')
result = subprocess.run([node, str(root / 'tools/minify_web.cjs'), str(root / 'web/index.html'), '-'],
                        check=True, capture_output=True, text=True, encoding='utf-8')
html = result.stdout
packed = gzip.compress(html.encode('utf-8'), compresslevel=9, mtime=0)
assert gzip.decompress(packed).decode('utf-8') == html
chunks = ['"' + ''.join('\\x%02x' % b for b in packed[i:i+64]) + '"' for i in range(0,len(packed),64)]
(root / 'firmware/HauntCue/WebUI.h').write_bytes((
    '#pragma once\n#include <Arduino.h>\nstatic const char WEB_UI[] PROGMEM =\n' + '\n'.join(chunks) + ';\nstatic const size_t WEB_UI_LENGTH = sizeof(WEB_UI)-1;\n'
    ).encode('utf-8'))
print(f'Embedded gzip UI: {len(packed):,} bytes (from {len(html.encode("utf-8")):,}); round-trip verified')
