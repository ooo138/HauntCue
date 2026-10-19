"""Embed the standalone web UI into the firmware; no filesystem upload needed."""
from pathlib import Path
import gzip
root = Path(__file__).resolve().parent
html = (root / 'web/index.html').read_text(encoding='utf-8')
packed = gzip.compress(html.encode('utf-8'), compresslevel=9, mtime=0)
assert gzip.decompress(packed).decode('utf-8') == html
chunks = ['"' + ''.join('\\x%02x' % b for b in packed[i:i+64]) + '"' for i in range(0,len(packed),64)]
(root / 'firmware/HauntCue/WebUI.h').write_text(
    '#pragma once\n#include <Arduino.h>\nstatic const char WEB_UI[] PROGMEM =\n' + '\n'.join(chunks) + ';\nstatic const size_t WEB_UI_LENGTH = sizeof(WEB_UI)-1;\n',
    encoding='utf-8')
print(f'Embedded gzip UI: {len(packed):,} bytes (from {len(html.encode("utf-8")):,}); round-trip verified')
