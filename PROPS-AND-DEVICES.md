# Props and device inventory

Open **Device inventory** to create a reusable profile. Pick one of 20 types, give it a name, and edit its settings. Model, supply voltage and notes help identify your hardware. You can also capture settings from an existing channel. Save the inventory to keep it on this controller.

Select a profile and a matching channel, then apply it. GPIO and enabled state stay as they are; review and save the controller settings to activate changes. Editing a profile does not automatically change channels already using it. Sensor presets expect a digital trigger signal, not raw analog sensor data. Driver and supply voltage compatibility still depends on your actual hardware.

Use **Export selected profile** or **Export inventory** to reuse devices on another controller. Import the JSON on that controller, choose its channel, and apply the profile. Profiles do not transfer wiring assignments. You can import copies when you want separate versions. The controller holds up to 32 profiles; inventory files are limited to 32 KB. The browser also caches profiles for reuse; export files are the reliable way to move them between phones or controller addresses.

Open **Props & controllers** to name and save a whole prop, load one, or start a blank prop. The local shelf holds up to 12 props in this browser at this site address. Export a prop JSON for a portable backup. It includes cue sheets, channel settings and GPIOs, the visual layout, and profiles used by that prop. It does not include Wi-Fi credentials, audio files or waveform references.

Loading a prop prepares a draft. Review its GPIOs and driver polarity, then select **Save complete prop to controller** while disarmed. That saves settings, layout and device assignments together in sequence. If a save step fails, the interface reports it; retry while disarmed. Older versions of a reused device profile are kept as separate copies when needed to restore a prop accurately.

**New blank prop** clears scenes and the layout, restores default pins and disables optional servo, lighting and audio. Your device inventory remains available. **Fresh controller setup** prepares this blank draft and opens the setup wizard. Give each controller a recognizable name; the Wi-Fi page can discover other HauntCue controllers on the same network.

For a new friend's ESP32, extract the Windows release ZIP and run **INSTALL-START-HERE.cmd**. Follow START-HERE.html, connect to the controller's Wi-Fi and complete the wizard. Review the wiring guide before attaching loads. Choose a fresh installation only when you intend to erase that board's saved configuration.
