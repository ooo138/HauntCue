# Dedicated lighting channels and effects — v1.13

The controller has six general prop outputs, L1 and L2 lighting, and a servo: nine total. L1 defaults to GPIO14 (TMS); L2 to GPIO33 (IO33). On your photographed four-row board, underside facing you with USB at bottom, these are right outer row 7 and right outer row 5 respectively. Use the on-device board diagram to match labels before soldering. Both lighting GPIOs are configurable and cannot overlap other configured hardware.

Upgrading from v1.11/v1.12 disables old lighting settings because those used prop outputs. Move lighting signal wires from GPIO23/19 to GPIO14/33 (or your chosen free GPIOs), review profiles and enable/save while disarmed. GPIO23/19 remain ordinary prop outputs.

## Profiles and timeline
Each light has separate ambient and event profiles. Ambient runs while armed when its checkbox is selected. A Lighting event cue selects L1/L2 and its time window; the event profile overrides ambient and ambient resumes when the window ends. Event cues work even with ambient unchecked. Maximum 16 lighting cues per scene; windows on the same light must not overlap.

Choose Off, Steady, Lightning storm, Candle/fire flicker, Breathing glow, Double pulse/heartbeat, Strobe or Puke/toxic churn. Addressable strips additionally support Chase/running lights, Rainbow wave and Twinkle/sparkle. Customize brightness, cycle time, primary/secondary colors and chase width. White strips ignore color. Lightning also uses quiet interval, burst count, flash and gap settings; both locations have independent timing. The skeleton preset gives L2 a toxic event from 7.0 to 9.4 seconds, without enabling lighting hardware.

Local preview animates the on-screen strip only. Prop stage parts can bind to L1/L2. Preview patterns are illustrative; live status is sampled and may miss short flashes. Boot starts disarmed; Stop & disarm commands darkness. There is no automatic thunder synchronization.

## Two-wire white strip
Use a PWM-capable MOSFET driver accepting ESP32 3.3V logic and rated for the strip current. Match supply voltage to the strip label.

- Lighting GPIO → MOSFET signal input.
- ESP GND → driver logic GND → LED supply negative.
- Fused supply positive → strip positive.
- Strip negative → driver's switched-negative load terminal.
- Follow the exact module markings for its power/load terminals.

Brightness uses 5kHz PWM. Mechanical relays cannot provide this control. A bare MOSFET needs a gate pull-down and a device specified for low resistance at 3.3V gate drive. Module circuitry may already provide this. Power the strip from its own supply, never a GPIO.

## WS2812B addressable RGB
Supports 1–60 pixels per location, 800kHz GRB RGB. RGBW and APA102 are unsupported.

- Strip power → separate fused supply at its specified voltage.
- ESP, level shifter, strip and supply grounds → common ground.
- Lighting GPIO → 74AHCT125 input; output → 330-ohm series resistor → strip DIN.
- For 5V WS2812B, power the AHCT shifter at 5V and connect its active-low enable to GND.
- Add a polarized 500–1000µF capacitor across strip power near DIN, rated above the supply voltage.

Follow the strip arrows, size supply/wires for its maximum current and add power injection as needed. Brightness is not overcurrent protection. Each location needs its own signal path; never join the two data outputs.

Hardware RMT sends independent buffered frames. Status reports driver faults. An addressable strip may retain its last color if wiring prevents an OFF frame. Physical strip/PWM operation has not been tested on this USB-only controller.

References: [Adafruit wiring](https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices), [Espressif RMT](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/rmt.html).
