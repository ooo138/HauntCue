# Put sounds on the DFPlayer

You need a microSD card (32 GB or smaller) and a USB card reader. The ESP32 serial connection sends playback commands; it cannot copy files to the DFPlayer card.

1. Download a sound using **Sound → Sound library → Search**. Freesound search is filtered to CC0; check the individual sound's license before use. Pixabay uses its own Content License. Keep the source page and any credit requirements in the catalog.
2. Use an MP3 file. If the download is WAV or OGG, convert it to MP3 with an audio editor first; changing its extension does not convert audio.
3. Switch off the DFPlayer before removing or inserting its card.
4. Put the card in your computer's reader. Use FAT32 rather than exFAT. If formatting is needed, first back up existing files; formatting erases them.
5. In File Explorer, open the card and create a folder named **mp3** at its top level.
6. In **Sound → Prepare the DFPlayer SD card**, choose the MP3 and its track number. Click **Save numbered MP3**. This downloads an unchanged, numbered copy. Copy that file from Downloads into the card's **mp3** folder.
7. Eject the card in Windows, insert it into the unpowered DFPlayer, and power on.
8. Enable DFPlayer in **Sound**, select the same ambient or scare track number, and save while disarmed. Arm for ambient playback; Fire starts the selected scene's scare track.

Example card layout:

```
mp3/
  0001.mp3    creepy ambient loop — ambient track 1
  0002.mp3    complete skeleton sequence — scare track 2
```

The skeleton preset is 14 seconds long: quiet rumbling and intermittent sway, gagging with brief spine pulses, then the pump from 7.0–9.4 seconds and recovery. Assemble a single matching audio track in an audio editor. Load that same file as the waveform reference to adjust the cues. DFPlayer command timing and module startup add playback latency; tune the cues against the actual speaker and mechanism. No rumble/puke recording is bundled yet.

Sound catalog entries are saved in the current browser, separate from controller configuration. Use **Export catalog** for a backup and **Import catalog** on another device. Catalog entries record track numbers; they do not contain the audio files.

If silent: check FAT32, the exact `mp3` folder and four-digit filename, matching track number, volume, speaker wiring, and stable module power. Reinsert the card with power off. Keep the pump disabled during the first motion and sound tests.

Sources: [DFRobot card/file instructions](https://wiki.dfrobot.com/dfr0299/docs/20905), [DFRobot card capacities](https://wiki.dfrobot.com/dfr0299), [Freesound license help](https://freesound.org/help/faq/), [Pixabay Content License](https://pixabay.com/service/license-summary/).
