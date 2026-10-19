# Physical arm button or switch

Under Devices → Channels & triggers, open Physical arm button / switch. This is separate from the scare trigger and physical stop. Existing installations leave it disabled until you choose a free supported GPIO and save. Its settings stay with the controller when you import another prop.

With power off for wiring, connect a dry-contact, normally open button’s COM terminal to ESP GND and its NO terminal to the selected GPIO. The offered pins have an internal pull-up: no extra resistor or external voltage is needed. Do not connect a powered sensor output or 5V/12V to this input. Match the printed GPIO label on your board.

- Momentary button: press once to arm; press again to stop and disarm.
- Maintained switch: close to arm; open to stop and disarm.

After boot, the contact must first be released, then pressed. A held button cannot arm at startup. A latched physical stop must be released and explicitly reset before arming; the arm input cannot clear it. Arming enables the saved ambient, sensor and timer behavior; it is not itself an immediate scare trigger.

The interface rejects GPIOs already assigned to outputs, lighting, servo, audio, the scare trigger, physical stop or status strip. Free and save a GPIO before assigning it. The physical arm setting never automatically moves your existing wires.
