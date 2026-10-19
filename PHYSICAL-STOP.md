# Two-wire physical emergency-stop button

HauntCue 1.30.1 defaults to GPIO16 on classic ESP32-WROOM. It has an internal pull-up, so no external resistor or 3.3V wire is needed.

With the ESP32 unplugged:
- Button NC → GPIO16, printed IO16 on your carrier.
- Button COM → ESP GND.
- Leave NO unused. Use a latching mushroom button with a normally closed, voltage-free contact. Never connect 12V to this GPIO.

Reconnect USB. Click the Physical button badge or open Devices → Physical emergency-stop button. Select GPIO16, check Button wired · enable input, and Save button settings while disarmed. Release the button, then Reset physical stop. Arm separately when ready. Test with loads disconnected first.

LOW is healthy because the released NC contact grounds the pin. Pressing the button opens the contact; the internal pull-up makes it HIGH and latches a stop. Releasing the button does not resume. A healthy contact must remain closed for 100 ms before reset. Enabled boot requires reset. Prop presets and imports do not remove controller-level stop settings.

GPIO16 is appropriate for the pictured ESP32-WROOM carrier. It is not a universal recommendation for ESP32-WROVER/PSRAM boards, where it may be reserved. Match your board and printed IO16 label. Existing configured GPIO35 inputs are preserved; GPIO34–39 still require an external 10 kΩ pull-up to ESP 3.3V. The interface shows wiring appropriate to the selected input.

Other family defaults: C3 GPIO3, S2/S3 GPIO12, C6 GPIO18. Smaller boards may need a prop channel freed before enabling. Enabled stop pins are reserved against device GPIO assignments.

This is a firmware stop, not a safety-rated power cutoff. Serial audio commands have latency, and servo signal release does not cut servo power. Use independent hardware to cut prop power or manage stored pressure where needed. Actual button wiring and physical stopping must be tested on your hardware; software checks do not verify them.
