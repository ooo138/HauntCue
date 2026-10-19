# DY-SV8F audio setup

## Switches on your pictured board

Switch numbers match CON1/CON2/CON3. Its ON side aligns with the PCB H marking.

| Use | Switch 1 | Switch 2 | Switch 3 |
|---|---|---|---|
| UART serial (choose DY-SV8F in Sound) | OFF / LOW | OFF / LOW | ON / HIGH |
| Eight independent track triggers, play to end after pulse | OFF / LOW | ON / HIGH | OFF / LOW |

Disconnect power before switching; power-cycle afterward. Match H/L markings if another board revision differs. The [original module manual](https://attach01.oss-us-west-1.aliyuncs.com/IC/Datasheet/13751.pdf) gives CON3/CON2/CON1 as 1/0/0 for UART, and 0/1/0 for independent mode 0. The other independent mode stops when the trigger is released, so it is unsuitable for a brief pulse that should start a complete sound.

## Wiring

- ESP TX (default GPIO21) → player RXD/IO1, through a 1kΩ series resistor.
- Player TXD/IO0 → ESP RX (default GPIO22), with a verified 3.3V-safe signal.
- ESP GND → player GND. Regulated 5V supply → player 5V+ / 5V−.
- Speaker goes between the two Speaker pads; do not ground either speaker terminal. Alternatively use the 3.5mm audio output. Begin with low software volume and a low trimmer setting.

Use the underside ESP pin guide for pad locations. UART uses two audio GPIOs and preserves six prop output channels. Set **Sound → Serial player → DY-SV8F**, enable sound, ambient track 1 and scare track 2, then save while disarmed. Playback on this physical module remains untested.

## Files and transfer

The module supports MP3/WAV and has approximately 8 MB onboard storage. Our example files use **WAV, PCM 16-bit mono, 22,050 Hz**. A practical compact MP3 starting setting is mono, 96 kbps. Renaming a file does not convert its format.

1. Disconnect external power/control wiring and plug a USB data cable into the player's Micro USB Download port.
2. Identify its removable drive. Back up existing audio before making changes. The guide does not require reformatting this device; do not blindly format another drive.
3. Copy **00001.wav** first, then **00002.wav**, directly to the drive root. Copy only the audio files, not the ZIP or README. For MP3 use five-digit names such as **00001.mp3**. Do not use the DFPlayer /mp3 folder layout here.
4. Safely eject, disconnect USB, set the UART switches, reconnect the player wiring and power-cycle.
5. Verify the actual track numbers before connecting prop loads. Numbered serial playback uses the player's file index, which may follow copy order rather than filename. See the [DYPlayer author's loading notes](https://github.com/SnijderC/dyplayer#loading-sound-files).

The original synthetic starter pack contains a 30-second ominous rumble/drone (track 1), and a 14-second rumble/retch/splash sequence (track 2) aligned to the skeleton preset's retch/pump timings. These are procedural effects, not human recordings. Their combined size is about 1.85 MiB, well below the player's advertised storage; actual playback compatibility needs a module test.

The ESP sends playback commands over UART. It does not transfer audio bytes into the player's flash, and the web page cannot write its USB drive. The webpage's numbered-file helper creates a renamed download for manual copying.

For trigger-pin mode, IO0 through IO7 select tracks 1 through 8 by pulling the selected pin LOW; BUSY is an output, not a ninth trigger. Use a verified safe interface or a transistor/contact that pulls to ground, and match ESP output polarity to that interface. These triggers share the six prop switching outputs. Serial avoids that channel cost.
