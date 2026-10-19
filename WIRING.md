# HauntCue wiring guide

See **Pins & trigger → Where the wires go** for selectable header/pad diagrams and the soldering close-up, or open [the standalone visual atlas](BOARD-WIRING-VISUALS.html). The **Features & help** tab lists capabilities and limits.

The full six-output examples describe a classic ESP32-WROOM. HauntCue also has experimental C3/S2/S3/C6 family builds; check the detected family and channel limits in Devices. Two rows of pins on each side do not identify a carrier. Do not apply a single-row 30/38-pin drawing to an unverified four-row board. Confirm the PCB model, revision and printed labels. Adjacent inner/outer pins are not assumed to be duplicates.

The visual atlas contains named carrier layouts and links to source schematics/reference guides. Some entries are references only; H2 is not supported by this Wi-Fi firmware. Chip-family support does not verify every carrier or clone. Wire overlays are shown only after confirming that the selected diagram matches the board. Selecting a diagram never changes GPIO settings.

Use the GPIO numbers displayed in **Pins & trigger**. Six switching channels can each use a compatible driver; the hobby servo has its own extra signal pin. GPIOs are 3.3V control signals and cannot power motors, pumps, relay coils, or servos.

For a new installation using the skeleton preset:

| Channel | GPIO | Use | Driver |
|---|---:|---|---|
| 1 | 25 | Spine / retching | Motor-rated relay or suitable MOSFET module |
| 2 | 26 | Sway forward | H-bridge direction input A |
| 3 | 27 | Sway reverse | H-bridge direction input B |
| 4 | 18 | Puke pump | Relay or suitable DC MOSFET module |
| 5 | 19 | Spare | Compatible relay / MOSFET input |
| 6 | 23 | Spare | Compatible relay / MOSFET input |
| Servo | 13 | Optional hobby servo | Servo signal wire |

Existing saved GPIO settings take precedence. Applying the skeleton preset keeps existing pins and polarity. Leave prop power disconnected while checking the output settings and wiring.

## Relay module — one channel

For a module whose input accepts 3.3V logic:

```
ESP32 chosen GPIO -------- relay module IN
ESP32 GND --------------- relay module control GND
module's rated supply --- relay module VCC

load supply + -- fuse -- relay COM
relay NO --------------- load +
load - ----------------- load supply -
```

Use NO (normally open) so the load is off when the relay is inactive. Set the channel to Active LOW if the module energizes when IN is LOW; otherwise use Active HIGH. Match the module's VCC to its specification (often 5V), and confirm its IN pin is safe for a 3.3V ESP32 GPIO. A 5V-powered module can still require an input level shifter. Never let a module pull the ESP GPIO to 5V.

Relay contacts can switch a separate load voltage; load current does not flow through the ESP32. The control side usually shares ESP32 ground; genuinely isolated modules may have separate optocoupler supplies and JD-VCC wiring, which must follow their own schematic. Relay contacts need a **DC inductive/motor rating** adequate for startup/stall current. A large printed AC contact rating does not establish that rating.

For a one-way DC motor or pump, suppress the load's inductive kick with a suitable diode across the load (stripe/cathode to load +, anode to load −), or the manufacturer's prescribed suppression. A relay module's coil diode does not protect the separate motor contacts. This diode arrangement is for a load that never reverses polarity.

## MOSFET module — DC load, one direction

For a verified 3.3V-compatible low-side switching module:

```
ESP32 GPIO ------------ module signal / IN
ESP32 GND ------------- module signal GND and load supply -
load supply + -- fuse - load +
load - ---------------- module switched negative / drain
module power GND ------ load supply -
```

Module terminal labels and internal wiring vary; follow its diagram rather than assuming a terminal marked OUT is positive or negative. Check its voltage/current rating, its actual operation at 3.3V input, and suppression across an inductive load. Some modules accept their own power supply on V+/V− and provide a switched load pair; those require their specified connection scheme. Set Active HIGH for a driver that conducts with a HIGH input. A MOSFET module does not generally provide relay-style galvanic isolation. Different positive DC voltages can still use separate suitable MOSFET drivers with a common negative reference.

Use a suitable inactive bias at the module input so reset/unpowered ESP GPIOs cannot switch the load. The firmware's inactive levels begin after boot; wiring must define the state before that.

## Sway H-bridge — reversing door actuator

```
12V supply + -- fuse --- H-bridge motor supply +
12V supply - ---------- H-bridge power GND
ESP32 GND ------------- H-bridge logic GND
GPIO26 ---------------- direction input A
GPIO27 ---------------- direction input B
H-bridge motor outputs - the actuator's two motor wires
```

This firmware expects a **two-input, active-HIGH direction interface**: A only = one direction, B only = the other, both LOW = inactive. Verify your module's truth table, enable pins, logic voltage, and coast/brake behavior. It does not directly support every H-bridge. Extra enable pins need their specified safe wiring. For example, a bridge with separate enables cannot simply omit them. Software interlock deactivates both directions on conflicting requests and waits the configured dead time before reversing. The preset uses a 200ms gap and 200ms maximum pulses. Select a bridge rated for the actuator's measured stall current.

Do not put the one-way diode described above across a reversing motor. Use the bridge's required recirculation/protection circuitry. Two ordinary relays need a proper polarity-reversing circuit and interlock; attaching one relay to each actuator wire is not an equivalent driver.

The spine actuator must fall back by gravity when its power is removed. A driver that electrically brakes the motor may hinder that motion; check the mechanical return with the intended driver before adding water.

## Hobby servo

```
GPIO13 ---------------- servo signal (often yellow/orange/white)
regulated servo supply + servo positive (often red)
servo supply - -------- servo GND (often brown/black)
ESP32 GND ------------- servo supply -
```

Follow the servo's wire labels and specified supply voltage. Use a separate regulated supply sized for servo stall current, not the ESP32's 3.3V pin. Do not connect 12V directly to a typical 5–6V hobby servo. Confirm that it accepts a 3.3V control signal. Arming commands the configured home position; disarming removes the signal but does not disconnect its supply. Calibrate pulse and angle limits with the linkage unloaded. Live-control loss removes PWM within two seconds.

## DFPlayer Mini

```
ESP GPIO21 TX -- 1kΩ --- DFPlayer RX
ESP GPIO22 RX --------- DFPlayer TX
ESP GND --------------- DFPlayer GND
stable 5V supply ------- DFPlayer VCC
speaker ---------------- SPK1 and SPK2
```

Keep the speaker between SPK1 and SPK2; neither speaker terminal is a ground connection. Follow the module's supported speaker/load and power specification. For an external amplifier, use the specified DAC outputs instead. Audio GPIOs are configurable and must not overlap other channels. See [DFRobot wiring documentation](https://wiki.dfrobot.com/dfr0299) and [SD-card instructions](DFPLAYER-AUDIO.md).

## Trigger contact

A dry-contact switch can connect default GPIO32 to ESP32 GND, with Active LOW and internal pull-up selected. A powered sensor needs an ESP-safe signal interface; do not apply 12V or 5V sensor output directly to GPIO32. GPIO34–39 require an external pull resistor if selected.

Use separate load wiring, an appropriately rated fuse near each supply, and a physical prop-power cutoff. Start with sound and dry motion; only enable the pump after timing and gravity return are verified. Exact terminal-by-terminal instructions for your relay/MOSFET/H-bridge require their model numbers or clear label photos.


## Your photographed four-row board (underside, USB at bottom)

LO = left outer, LI = left inner, RI = right inner, RO = right outer. Count rows from the top. These are drawing coordinates, not manufacturer pin numbers. Do not mirror onto the component side.

| Function (default) | Printed label | Pad |
|---|---|---|
| CH1 | IO25 | LO:3 |
| CH2 | IO26 | RI:3 |
| CH3 | IO27 | LO:2 |
| CH4 | IO18 | RI:4 |
| CH5 | IO19 | RI:5 |
| CH6 | IO23 | RI:6 |
| Trigger | IO32 | LO:4 |
| Servo signal | TCK (GPIO13) | RI:9 |
| ESP TX → DFPlayer RX via 1kΩ | IO21 | LI:4 |
| ESP RX ← DFPlayer TX | IO22 | LI:3 |
| Control ground | GND | LO:1, LI:7 or RO:1 |

Audio and servo are disabled until configured. This table assumes default GPIO choices; the web diagram follows actual settings. Power pads are not assigned to prop loads; VCC voltage is not inferred from this photo. Leave SD0/SD1/SD2/SD3/CLK/CMD flash pads alone.

Source: supplied underside photograph; GPIO/JTAG aliases cross-checked against [Espressif WROOM-32 datasheet](https://documentation.espressif.com/esp32-wroom-32_datasheet_en.html).


## Lightning strip channel
See LIGHTNING-WIRING.md or Pins & trigger → Dedicated lighting. L1 defaults to GPIO14/TMS and L2 to GPIO33/IO33. Each GPIO drives a PWM MOSFET signal for a two-wire strip, or level-shifted DIN for a WS2812B strip. These are two additional channels; all six prop outputs and the servo remain available. Older lightning configurations are disabled on upgrade: rewire their lighting signals to the dedicated GPIOs before enabling them again.


## v1.16 logo, compact web UI, independent audio and stage controls
The supplied HauntCue logo is embedded in the controller and showcase. The firmware serves a deterministic gzip-compressed webpage; the standalone HTML remains editable. Original logo retained as HauntCue-logo-original.jpg.

Sound now supports two independent serial modules, either DFPlayer Mini or DY-SV8F in each slot. Player 1 handles ambient; enabled player 2 handles each scene's scare track. Ambient is not stopped or restarted at scare boundaries. With player 2 disabled, original single-player interruption/resume behavior remains. Each slot has its own volume, UART pins, queue and 2-second initialization. Both stop on disarm; startup remains disarmed. Physical dual playback remains untested.

Player 1 defaults: TX GPIO21 → module RX; RX GPIO22 ← module TX. Player 2 defaults: TX GPIO4 → module RX; RX GPIO34 ← module TX. On your photo-matched four-row board, underside with USB at bottom, GPIO4/IO4 is left outer row 6 and GPIO34/IO34 is right outer row 6. Use a 1k-ohm series resistor for DFPlayer RX and the existing DY DIP/UART instructions for DY. Every player needs files on its own storage, a suitable supply and its own audio output path. Never join amplified speaker outputs. Use two speakers/amplifiers or a suitable line-level mixer. GPIO34 accepts only ESP-safe logic and cannot drive an output. Default pairs coexist with all nine prop/light/servo channels; conflicts are rejected. The USB programming UART stays available. Slot 2 is disabled when upgrading.

In Prop stage, click a device (or use Select a part). Edit the shared bound output, servo, trigger or lighting settings and Save channel settings. A GPIO test is bounded to 500ms and the configured max-ON/interlock limits; lighting tests run an ambient/event profile for at most 3 seconds while disarmed. Live servo uses its saved travel bounds and a 2-second connection watchdog; release removes PWM. Sensors show the live input state rather than synthesizing a physical sensor signal. Tests require saved settings and a disarmed controller. Local demo tests never drive hardware. Layout/name/binding changes still use Save prop layout separately from channel settings.


Pneumatic valve presets: single spring-return coil or paired extend/retract coils, configurable pulses/waits/cycles, timeline integration, active-HIGH/LOW drivers and direction interlock. See [PNEUMATICS-WIRING.md](PNEUMATICS-WIRING.md) for wiring and air setup. One direction pair is available across motors and valves.


## Outer-row wiring preset (v1.20.1)
For your photo-matched four-row board, select **Outer-row wiring preset** in Pins & trigger. This prepares a draft; review and rewire with power disconnected before saving. Existing wiring is never changed automatically. GPIO menus list suitable outer pins first and label outer/inner positions. New output channels try unused outer pins first.

CH1–6: **25,27,33,14,18,19**; servo **4**; trigger **32**; L1 **23**; L2 **26**; player 1 TX/RX **21/22**; player 2 TX/RX **13/34**. The first four switched outputs, servo and trigger are outer-row connections. Second audio RX is outer too. All channels coexist without conflicts; remaining signals use inner pads. GPIO34/35/39 are input-only. Boot-strapping and flash pins remain excluded; adjacent inner/outer pads are not duplicates. This does not alter the universal factory defaults for other carrier boards.


## ESP32-C3 SuperMini reference image
Choose ESP32-C3 SuperMini in Where the wires go for a compact labeled SVG diagram: USB-C end at top, eight pads per side, BOOT/RESET, antenna and bare chip. GPIO2/8/9 are boot-sensitive. Match printed labels on clones. Use the matching experimental C3 build from the multi-board installer; the classic ESP32 binary cannot run on C3. Source: [Apache NuttX SuperMini board documentation](https://nuttx.apache.org/docs/latest/platforms/risc-v/esp32c3/boards/esp32c3-supermini/index.html).


## Family-specific assignments (v1.22)
Read MULTI-BOARD-INSTALL.md. The live wiring table uses the detected build's assignments; static classic GPIO examples in this document must be translated to those assignments. Different carrier boards may share a chip but expose different pads or attached devices. Only match a reference after checking every printed label.
