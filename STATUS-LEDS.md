# Independent controller status strip

HauntCue 1.32 adds a controller-level WS2812B strip, independent of the two prop-lighting channels. Default: three pixels, 15% brightness, disabled until wired and enabled. Supports 1–8 GRB RGB pixels.

On the pictured ESP32-WROOM carrier, use printed IO17 for data. Connect ESP IO17 → 74AHCT125 (or suitable 3.3V-to-5V logic shifter) → 330 Ω resistor → first pixel DIN. Strip +5V goes to a suitable 5V supply. Strip GND, supply GND and ESP GND must be common. Do not power the strip through a GPIO. Follow strip data arrows. Keep data wiring short; a capacitor across the strip supply helps with transients. Match module/carrier labels; ESP32-WROVER may reserve GPIO17 for PSRAM.

Open Devices → Controller status LEDs. Select the data pin, pixel count and brightness, enable and Save status LED settings while disarmed. Settings belong to this controller, not the prop JSON. An enabled indicator GPIO cannot be assigned to prop devices or the stop button.

- Blue: disarmed.
- Green: armed, including ambient animation.
- Amber: scare sequence running.
- Red: physical stop latched or a reported servo/prop-light driver fault.

The indicator continues during a stop so it can show red; prop L1/L2 still turn off. A driver/resource failure is shown on the webpage and may prevent the status strip from lighting. Prop-lighting drivers have allocation priority. Smaller boards may lack enough RMT channels for three simultaneous addressable strips; the enable request reports the resource conflict. All five family builds are compile-checked, but non-classic hardware remains experimental.

This reports controller commands/state, not measured actuator positions, relay power or a safety-rated stop confirmation. Physical wiring, color order, resource coexistence and stop response require testing on the actual hardware.
