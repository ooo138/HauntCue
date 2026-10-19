# One installer, five chip families

Extract the whole HauntCue v1.32.0 Windows ZIP, plug in the board by USB only, and run INSTALL-START-HERE.cmd. Choose the COM port. The installer detects the chip and flash size, selects that family's firmware, and asks which carrier layout you have. It will refuse unsupported families or less than 4MB flash. The exact carrier cannot be detected automatically: match its printed labels in the web wiring guide after installation.

| Family | Switched outputs | Lighting | Servo | Serial players | Status |
|---|---:|---:|---:|---:|---|
| Classic ESP32-WROOM | 6 | 2 | 1 | 2 | Flashed and boot-tested on classic WROOM |
| ESP32-C3 SuperMini / DevKitM-1 | 4 | 2 | 1 | 1 | Experimental; compiled only |
| ESP32-S3 DevKitC-1 | 6 | 2 | 1 | 2 | Experimental; compiled only |
| ESP32-S2 Saola-1 / DevKit | 6 | 2 | 1 | 1 | Experimental; compiled only |
| ESP32-C6 DevKitC-1 | 6 | 2 | 1 | 1 | Experimental; compiled only |

All have one digital trigger. Audio, servo and lights are optional and start disabled. The recommended layouts reserve pins so these features can coexist. You can remove switched outputs and assign unused supported pins in Pins & trigger. The second player is unavailable on single-player builds; GPIO0/1 remain usable on C3 (USB uses18/19). USB pins, flash pins and selected boot-sensitive pins are excluded from the offered GPIO lists. The S2/S3 profiles use a conservative common subset, avoiding octal-memory pins.

For first installation or replacing another firmware, choose Fresh installation and confirm ERASE. Updates within the same HauntCue family/layout can keep saved settings. Never carry raw flash partitions between chip families. Classic ESP32 retains its existing partition layout; new families use a 2MB application plus a filesystem in a 4MB layout. Larger flash boards use only the first 4MB. No OTA update workflow is included.

After upload, join HauntCue-XXXXXX Wi-Fi and open http://192.168.4.1. Fresh password is hauntcue123; username admin. Complete the wizard, then review the capability note and exact board diagram in Pins & trigger. Match your carrier revision and GPIO labels before wiring. Experimental builds need bench testing on their actual board before connecting loads; a successful compiler build does not establish physical timing or wiring compatibility.

C3 recommended pins: outputs0/1/3/4, trigger10, servo5, L1 GPIO6, L2 GPIO7, audio TX21/RX20. This supports the skeleton's four used outputs, both lights and head servo; two spare switched channels are omitted. Audio uses one module, so a scare interrupts ambient audio.

To move a prop between families, export its complete JSON. On the destination, enable Adapt imported prop to this board before importing. This explicitly remaps pins to the destination defaults, removes extra output cues and unavailable player-2 commands, and unbinds removed visual devices. Review the draft before saving. Without this option, incompatible imports are rejected. Keep the original backup. Device profiles can still be imported separately without moving GPIO assignments.

ESP8266, H2, P4, C5 and other families are not included. H2 lacks Wi-Fi, while others require additional architecture/network work. ESP32-CAM, XIAO, tiny S2/S3 clones and boards with built-in peripherals need carrier-specific checking; chip compatibility alone does not guarantee the default pins are accessible or unused. WROVER/PSRAM and unknown clones are not considered automatically verified.

Free for fellow fiends — Rick Scarlett. Features/bugs: rick.scarlett@gmail.com. Optional donations: $rickscarlett.
