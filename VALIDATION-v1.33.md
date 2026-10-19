# HauntCue 1.33 validation

Verified on a classic ESP32-WROOM with 4MB flash on 19 October 2026. Prop load power was disconnected.

- Application: 1,300,976 bytes in the original 1,310,720-byte slot; 9,744 bytes spare.
- SHA-256: defc412993c6b924a4d507ae4981615cf4e40731f95fad9059448512aa2a226a.
- Eight JavaScript suites passed, including grouped edits, channel masks, precise field validation and SHA-256. Safe minifier transformations were also checked; the exact packaged interface was tested in a browser.
- Eight C++ suites passed with assertions explicitly enabled, including 128-cue execution, channel masks, physical arm debounce, held-at-boot protection, interlocks and rollover.
- The actual firmware decoder/encoder preserved every existing configuration field through a round trip. Missing channel masks default to all enabled.
- Linked Xtensa stack budgets passed. Configuration initialization avoids large stack temporaries.
- USB upgrade changed the application only. The partition table, boot selector and all NVS bytes matched before/after.
- The controller booted disarmed with its original prop, layout, inventory, stop-button and status-light settings.
- An authenticated Wi-Fi upload of the same verified image succeeded, followed by restart and exact API settings comparisons.

Private backups, credentials, device identifiers and live prop files are not part of this repository or release.

Connected-load performance, actual actuator movement, servo calibration, player playback latency and non-classic boards were not validated in this session. Existing C3/S2/S3/C6 release layouts remain single-slot and cannot gain OTA without a separate, deliberate layout migration. No automatic repartitioning is performed.
