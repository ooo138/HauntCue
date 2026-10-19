# Third-party components

The custom HauntCue Source-Available License covers HauntCue-authored material. It does not replace licenses on dependencies, SDKs, tools or independently licensed media. Preserve upstream notices and satisfy the licenses applicable to the exact components you distribute.

| Component | Baseline used | Source and licensing information |
|---|---|---|
| Arduino-ESP32 | 3.3.12 | https://github.com/espressif/arduino-esp32/tree/3.3.12 — core source includes LGPL notices; check each bundled component |
| ESP-IDF and vendor SDK components | As supplied by that ESP32 board package | https://github.com/espressif/esp-idf — Apache-2.0 and component-specific notices; the package includes other separately licensed components |
| ArduinoJson | 7.4.3 | https://github.com/bblanchon/ArduinoJson/tree/v7.4.3 — MIT; copyright Benoit Blanchon; license copied to third-party/ArduinoJson-LICENSE.txt |
| esptool | 5.3.1 | https://github.com/espressif/esptool/tree/v5.3.1 — GNU GPLv2; the release carries installer-tools/LICENSE-esptool.txt and esptool-v5.3.1-source.tar.gz |

This source repository does not vendor the Arduino core, SDKs or ArduinoJson. Install them through the documented board/library managers. Build versions are recorded in README.md. The installer’s executable, source archive and compiled firmware belong in downloadable release assets; they are excluded from source history.

Before redistributing a changed binary package, retain its third-party notices and corresponding-source materials, and review the exact SDK/tool licenses. The release’s firmware sources and build instructions must accompany or otherwise be supplied according to applicable license terms. Do not present an entire compiled firmware image or bundled flashing executable as HauntCue-only code.

Board diagrams link to source schematics/reference pages in board-reference.json. Product names are used for identification. Audio files are not included in the GitHub source package; separately sourced audio may require its own license and credit.

The custom HauntCue sales restriction does not apply to independent third-party components. Preserve any pre-existing rights and obligations under those components’ licenses. The procedural audio-starter files, if included in a release, retain their separate permission notice in audio-starter/README.txt.
