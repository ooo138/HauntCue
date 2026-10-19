# Pneumatic valves

In **Hardware → Pneumatic valve presets**, choose single or double solenoid, assign output channels, and set start time, coil pulse, waits and cycles. Preview the timeline, then save. Nothing is energized just by adding a preset. Existing cues on those channels are preserved unless Replace is selected.

- **Single solenoid:** energizes one coil, then releases it. The valve spool returns according to its spring-return design. Cylinder behavior depends on the valve and plumbing; verify its symbol.
- **Double solenoid:** pulses extend, waits, pulses retract, then waits. Intended for valves that latch after a pulse. A valve requiring continuous energization needs suitably rated hold cues instead. Use the coil manufacturer's minimum pulse and duty-cycle ratings.
- The one direction interlock can serve a motor pair or a pneumatic pair. Pneumatic mode supports active HIGH or active LOW driver modules; motor mode keeps its active HIGH requirement. A different existing pair must be reassigned before adding double-coil presets. Dead time applies to tests and scene cues.

## DC coil wiring

Use a driver rated for coil voltage and current, with a 3.3V-compatible logic input. Never connect a coil directly to ESP32 GPIO.

```text
MOSFET low-side driver:
Fused coil supply + ───────────── coil +
Coil − ───────────────────────── driver switched load −
Supply − ─────────────────────── driver power ground
ESP GPIO chosen in Hardware ──── driver logic IN
ESP GND ──────────────────────── driver logic ground

Relay contacts:
Fused coil supply + ── COM
NO ────────────────── coil +
Coil − ────────────── coil supply −
ESP GPIO ──────────── compatible relay module IN
```

Follow the driver's terminal diagram: module names vary. Relay contacts must be DC-rated for an inductive load. Use flyback suppression at each DC coil (diode cathode/stripe toward coil +, anode toward coil −), unless the valve already provides appropriate suppression. Follow the manufacturer for integrated LED/suppressor polarity and release timing. These instructions are for DC coils; AC coils need different suppression and drivers.

For a double-coil valve, use one independent driver per coil. Match GPIO and active polarity to each driver, and set a maximum ON time suitable for the coil. Hardware inactive bias is needed during ESP reset.

## Air connections and testing

Read the valve's printed symbol and manual. Common designations are P/1 supply, A/2 and B/4 cylinder ports, R/3 and S/5 exhaust, but physical port positions vary. Use a regulator, suitable tubing and exhaust mufflers. Meter-out flow controls adjust cylinder speed; coil PWM is not a substitute.

First check logic with air isolated, then test at low regulated pressure with the mechanism clear. A latching valve can retain its position and pressure after Stop/disarm turns both coils off. Provide a physical air isolation/dump arrangement appropriate to the mechanism. The screen shows commanded coil states, not measured cylinder position; a position sensor is required for feedback.

References: [Festo valve behavior](https://www.festo.com/gb/en/e/blog/in-practice/pneumatic-valves-id_1517691), [Adafruit DC solenoid and flyback wiring](https://learn.adafruit.com/circuitpython-ble-remote-control-on-off/wiring-the-solenoid).
