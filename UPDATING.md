# Updating without replacing your settings

Version 1.33 adds an application-only Wi-Fi updater. Install it once over USB if your existing firmware has no update page. Export your complete prop first and keep a verified recovery backup before that initial USB upgrade. Do not choose Fresh installation/ERASE or change partition layouts on a configured controller.

## Wi-Fi updates

1. Open Settings → Wi-Fi & access → Firmware updates.
2. Stop and disarm, save drafts, and download the saved-prop backup. Keep that file.
3. Choose the HauntCue application .bin for your chip family from Releases. A ZIP, bootloader or merged flash image is not an application upload.
4. Confirm you kept the backup, then Install firmware & restart. Keep power connected. Reconnect and check the installed version.

The updater writes an inactive application slot. It checks the target family, image size and SHA-256; it does not upload or erase the filesystem, NVS, partition table or bootloader. Props, pin assignments, timings, visual layout, inventory, network credentials, physical-stop settings and status LEDs stay in place. A separate arm-input setting is disabled by default for existing controllers. Playback and new motion commands are blocked during the upload.

The SHA-256 check detects a damaged upload; it is not a signature proving who authored the firmware. Use a trusted HauntCue release. This implementation does not promise automatic rollback after an application boots incorrectly. Keep your recovery backup.

## Layout limits

The classic default 4MB layout has two 1,310,720-byte application slots and supports OTA without moving saved data. Existing experimental C3/S2/S3/C6 release layouts have one 2MB application slot: the page explains why OTA is unavailable there. It does not repartition them automatically. USB updates and prop exports remain available.

Old prop JSON is accepted without changing existing fields. New scene channel masks default to all enabled when absent. Increasing the cue limit does not truncate an existing scene. A damaged settings filesystem is not silently formatted during an upgrade.

## Building the classic application

The classic 1.33 release uses ESP32 Arduino core 3.3.12, ArduinoJson 7.4.3 and GCC link-time optimization to fit the original application slots. The SDK link flags contain -fno-lto: simply appending -flto does not fix that conflict. The build helper removes that conflicting option from its invocation and compiles the vendor panic-handler translation unit without LTO to avoid an Xtensa relocation problem. It does not patch the installed SDK.

Install Node.js 18+, run npm install in the source folder, and regenerate WebUI.h with python build_web.py. The source interface remains readable; minification and gzip affect the embedded copy only. Use tools/build_classic.py with the documented --packages and --arduinojson paths to compile the classic application. Arduino IDE can still open the sources, but changing to a larger application partition is not an appropriate workaround when preserving an existing controller.

Software checks do not replace a disarmed boot test and verification against the controller’s saved backup.

Example build commands after installing the pinned ESP32 core and ArduinoJson library:

```shell
npm install
python build_web.py
python -m pip install esptool==5.3.1
python tools/build_classic.py --packages /path/to/packages/esp32 --arduinojson /path/to/ArduinoJson/src
```

On Windows, use your actual SDK/library directories and optionally pass `--esptool C:/path/to/esptool.exe` to use the standalone converter. The helper compiles and creates a size-checked application file; it never connects to or flashes a device. Preserve the core/SDK component licenses when redistributing the resulting binary.
