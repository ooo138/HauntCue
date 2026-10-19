# HauntCue

**Make the night move.** A free ESP32 Halloween prop controller by **Rick Scarlett**.

Build timed scares, ambient animations, sound cues, servo movements and lighting effects from a mobile-friendly web interface. HauntCue runs on the controller: join its Wi-Fi access point or connect it to your home network. No cloud account or separate web hosting is required to run a prop.

<p align="center"><img src="web/hauntcue-logo.webp" alt="HauntCue DIY ESP32 prop controller" width="300"></p>

[Repository](https://github.com/ooo138/HauntCue) · [Downloads / Releases](https://github.com/ooo138/HauntCue/releases) · [Report an issue](https://github.com/ooo138/HauntCue/issues)

**Development source: 1.33.0. Latest published installer: 1.32.0.** Version 1.33 adds Wi-Fi updates, a physical arm input, channel checkboxes, grouped cue editing, precise validation and 128 output cues. Its classic ESP32 application has passed software checks, USB boot verification, saved-settings comparisons and an actual Wi-Fi self-update. See [validation notes](VALIDATION-v1.33.md).

**Published baseline: 1.32.0.** Classic ESP32-WROOM has been flashed and boot-tested. C3, S2, S3 and C6 builds are experimental and compile-checked; actual boards and connected loads need testing.

## Install and make your first prop

1. Open this repository’s **Releases** section and download the **HauntCue Windows installer ZIP** attached to a release.
2. Extract the entire ZIP. Connect the ESP32 using a USB data cable, with prop loads disconnected.
3. Run **INSTALL-START-HERE.cmd** and follow the board-detection prompts. Fresh installation can erase existing settings; export any props you want to keep first.
4. Join **HauntCue-XXXXXX** Wi-Fi. The initial password is **hauntcue123**.
5. Open **http://192.168.4.1**. Initial login: **admin / hauntcue123**. Change the password during setup.
6. Run the setup wizard, match the board’s printed GPIO labels, and configure your channels, trigger and optional devices.
7. Add cues under **Build**, save while disarmed, and test individual channels before arming.

**GitHub’s “Code → Download ZIP” contains source code, not the ready-to-run Windows installer.** The installer needs the firmware images and flashing tool supplied in the release asset. If no release asset has been published yet, build from source below.

## What you can build

- **Sequences:** four scenes, up to 128 GPIO cues per scene (1.33), and scenes up to 120 seconds. Drag, resize and move cues between channels; edit exact times and use Undo/Redo. Select GPIO cue groups to copy, paste, duplicate, delete or nudge across times, channels and scenes.
- **Scare and ambient timelines:** separate editing tabs; browsing a scene does not change the triggered scare. Explicitly assign the scare you want to run.
- **Prop outputs:** up to six named switching outputs, configurable GPIO/polarity, pulse limits and one protected direction pair for a motor or pneumatic valve arrangement.
- **Channel selection:** per-scene enable checkboxes plus a disarmed, one-shot test of the viewed scene’s checked channels. These choices also apply to normal playback until changed and saved.
- **Physical arm input:** an optional, separate GPIO-to-ground button or maintained switch, with debounce and held-at-boot protection. Existing controllers start with this input disabled.
- **Wi-Fi updates:** authenticated application-only uploads on layouts with two application slots. Saved prop data, NVS, network settings and passwords remain in place; the update page checks chip family, file size and SHA-256.
- **Triggers:** a digital sensor/contact, manual fire, or a fixed/random repeat timer, with debounce and cooldown.
- **Servo:** one extra hobby-servo channel, travel calibration, live control, recorded movements, motion presets and timeline cues.
- **Lighting:** two prop-lighting channels, each supporting two-wire PWM through a suitable MOSFET or WS2812B pixels, with separate ambient/event profiles and multiple effects.
- **Audio:** up to two independent serial players, including DFPlayer Mini, DY-SV8F or a mixed pair. Use separate players for uninterrupted ambient audio plus scares. Browser waveform references help align cues; files must be loaded onto the sound modules separately.
- **Physical stop input:** a controller-level normally closed button input with a latched stop and explicit reset. The classic WROOM default is GPIO16, using its internal pull-up.
- **Status pixels:** a separate optional WS2812B indicator strip, default three pixels on GPIO17 for classic WROOM: blue disarmed, green armed/ambient, amber scare, red physical stop or a reported prop-driver fault.
- **Reusable projects:** device presets, custom profiles, an inventory, complete prop import/export and reusable cue groups.
- **Visual prop preview:** sketch a prop, bind devices to channels, inspect properties, preview timing and run bounded live tests.
- **Puking skeleton preset:** sway, retches, a 15-second puke window, two lighting channels and ambient/head-servo behavior.
- **Network setup:** home Wi-Fi plus the controller’s access point, a controller name, and discovery links for nearby controllers. Discovery does not synchronize playback.

Basic editing and expandable tools keep the main workflow accessible without removing advanced functionality. GPIO, servo, lighting, audio and status resources depend on the selected board.

## Boards and channel limits

| Chip family | Switching outputs | Prop lights | Servo | Serial players | Validation |
|---|---:|---:|---:|---:|---|
| ESP32-WROOM | 6 | 2 | 1 | 2 | Flashed and boot-tested |
| ESP32-C3 | 4 | 2 | 1 | 1 | Experimental; compile-checked |
| ESP32-S2 | 6 | 2 | 1 | 1 | Experimental; compile-checked |
| ESP32-S3 | 6 | 2 | 1 | 2 | Experimental; compile-checked |
| ESP32-C6 | 6 | 2 | 1 | 1 | Experimental; compile-checked |

The optional status strip needs another free GPIO and LED-driver resource. Small boards may not support three addressable strips at once. The firmware gives prop lighting allocation priority. Chip-family support does not establish that every carrier exposes the same pins; WROVER/PSRAM and boards with built-in peripherals need particular care. ESP8266, H2 and other families are not included.

## Wiring and operating limits

GPIOs are **3.3V control signals**. Use appropriate relay, MOSFET, motor or valve drivers and separate supplies for loads; never connect 12V directly to a GPIO. Match your actual carrier’s labels and driver polarity. Software interlocks and the physical-stop input are not safety-rated power isolation or measured position feedback. Use appropriate independent power/pressure controls for your prop and test with loads disconnected first.

- [General wiring](WIRING.md) · [Board wiring visuals](BOARD-WIRING-VISUALS.html)
- [Physical arm button](ARM-INPUT.md) · [Wi-Fi updates and preserving settings](UPDATING.md)
- [Physical stop button](PHYSICAL-STOP.md) · [Status-strip wiring](STATUS-LEDS.md)
- [Lighting](LIGHTNING-WIRING.md) · [Pneumatic valves](PNEUMATICS-WIRING.md)
- [DFPlayer audio loading](DFPLAYER-AUDIO.md) · [DY-SV8F audio and DIP switches](DY-SV8F-AUDIO.md)
- [Scare/Ambient timelines](TIMELINES.md) · [Props and device profiles](PROPS-AND-DEVICES.md)
- [Board-family installation](MULTI-BOARD-INSTALL.md)

Audio playback has serial/module latency; timing is not sample-accurate. The preview displays commanded states, not measurements from motors or lights. Overlapping ON/OFF cues on one output merge; leave gaps for distinct pulses. A reported software stop does not confirm that a relay or mechanism physically stopped.

**Known lighting validation detail:** Ambient/Event cycle must be **100–10,000 ms**. Disabled lighting channels are still validated. Lightning flash duration is a different field. Version 1.33 reports the exact numeric field and range, with a Show setting shortcut; it retains the draft rather than silently changing values.

## Build from source

The baseline toolchain used for the supplied builds is **ESP32 Arduino core 3.3.12** with **ArduinoJson 7.4.3**. Install these through Arduino IDE’s board/library managers. The other includes come from the ESP32 board package.

For classic ESP32-WROOM:

1. Open `firmware/HauntCue/HauntCue.ino` in Arduino IDE. Keep its sibling headers in the same directory.
2. Select **ESP32 Dev Module**, **4 MB flash**, and **Default 4MB with spiffs** partitions. HauntCue uses LittleFS in the data partition.
3. The 1.33 classic build needs the size-optimized build described in [UPDATING.md](UPDATING.md#building-the-classic-application). The generated header alone does not guarantee that an unoptimized Arduino IDE build fits the default OTA slot. Keep the existing partition layout when updating a configured controller.

The generated `WebUI.h` is included. After editing `web/index.html`, regenerate it from the repository root:

```shell
npm install
python build_web.py
```

Regeneration requires Node.js 18 or newer. Terser is a pinned build-only dependency: it compresses the embedded JavaScript before gzip so the application fits the existing flash layout. No JavaScript packages run on the ESP32 or load from a CDN. The readable interface source remains in `web/index.html`.

Experimental targets select the corresponding ESP32-C3/S2/S3/C6 development-module board. The supplied experimental release images use the partition layout in `partitions/experimental-4mb.csv`; for an equivalent sketch build, copy it to `firmware/HauntCue/partitions.csv` for that build. Remove that copy before returning to the classic/default layout. Export settings before changing layouts. Experimental hardware still needs validation.

## Preview and software checks

A browser-only preview does not operate an ESP32:

```shell
python -m http.server 8770 --directory web
```

Then open **http://localhost:8770/?demo**. Preview data is separate from a physical controller.

With Node.js installed, run these from the repository root; no npm packages are required:

```shell
node tests/web_test.cjs
node tests/channel_editing_test.cjs
node tests/workspace_test.cjs
node tests/emergency_test.cjs
node tests/physical_stop_test.cjs
node tests/scene_tabs_test.cjs
node tests/status_led_test.cjs
node tests/sequence_tools_test.cjs
```

Additional C++ engine tests are in `tests/`. Browser/source tests and successful builds do not verify electrical wiring, physical stop latency or connected hardware.

## Source files and downloads

| Location | Purpose |
|---|---|
| `firmware/HauntCue/` | Firmware, scheduling, audio, lighting and embedded UI |
| `web/` | Editable web interface and logo assets |
| `tests/` | JavaScript and C++ checks |
| `board-visuals/`, `board-reference.json` | Carrier diagrams and references |
| `partitions/` | Experimental release partition layout |
| `Install-HauntCue.ps1`, `INSTALL-START-HERE.cmd` | Release installer scripts; require release payloads |
| GitHub Releases | Ready-to-install ZIPs, compiled images and bundled flashing tools |

The repository excludes live device configurations, flash backups, build caches, logs and audio tracks. Do not add your Wi-Fi credentials or private prop exports. `.gitignore` helps Git avoid these files, but does not scan file contents or protect manual browser uploads.

## Bugs, feature ideas and contributions

Open an issue in this repository or email **rick.scarlett@gmail.com**. Include the firmware version, chip/carrier model, steps to reproduce and the exact error. Redact passwords, Wi-Fi details and private information before attaching configurations or screenshots.

Contributions must use the project’s source-available terms and preserve applicable notices. Only contribute material you have the right to license. Keep existing configuration compatibility, bounded tests, driver interlocks and safe boot behavior intact. Explain what changed and how it was checked.

## Free for fellow fiends

HauntCue by **Rick Scarlett** is free to use. Donations are optional: contribute whatever you can afford to help keep it free for fellow fiends.

**Cash App:** [$rickscarlett](https://cash.app/$rickscarlett) · **Contact:** [rick.scarlett@gmail.com](mailto:rick.scarlett@gmail.com)

## License

Copyright © 2026 Rick Scarlett. HauntCue-authored code and documentation use the custom **HauntCue Source-Available License 1.0**.

- Free to use and modify for your own props, including operating your own paid haunt.
- Free sharing must retain this license and provide the corresponding editable source for shared changes.
- **Selling HauntCue, or selling controllers, kits or props with HauntCue included, requires Rick Scarlett’s written permission first.** Email [rick.scarlett@gmail.com](mailto:rick.scarlett@gmail.com). Notification alone is not permission.
- Private modifications do not have to be published. Donations remain optional.

This is **source-available**, not GPL/MIT or OSI-approved open source. Read [LICENSE](LICENSE), [LICENSING.md](LICENSING.md), and [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md). Third-party components retain their own terms. The software is supplied without warranty.
