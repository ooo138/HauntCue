# Scare and Ambient timelines

Under Build â†’ Sequence, use Scare timeline or Ambient timeline. Switching tabs and browsing scare scenes changes only the editor; it does not change the scare assigned to triggers.

In Scare, choose a scene to edit. Click Use this scare for triggers only when you want that scene to run on a trigger, then Save prop. Random scare selection still chooses among all non-ambient scenes when enabled.

In Ambient, edit the assigned idle scene. If none is assigned, create a separate scene or assign an existing non-trigger scene. Save prop to apply. The ambient-scene selector changes the idle assignment explicitly. Ambient runs while armed between scares; a trigger interrupts it. Its automatic scare-track field is ignored, so use explicit audio timeline cues for ambient-scene commands.

GPIO, servo, lighting and audio tools, cue groups and local previews all target the scene currently being edited. Undo/Redo includes scene role assignments. Existing four-scene limits and saved JSON fields are unchanged. Fire scare always runs the saved trigger assignment, not the scene being browsed in the editor. Existing prop JSON remains compatible.

Overlapping ON/OFF cues on one output merge into a continuous ON window. Leave gaps for distinct pulses. Existing pulse limits and direction interlocks still apply.


## Channel checkboxes and one-shot tests (1.33)

Each lane has an Include checkbox. Unchecking mutes that channel in the viewed scene without deleting its cues. This applies to normal scene playback as well as testing, and is saved with the prop. It does not change GPIO assignments or globally disable the device. Existing props default to all channels included.

To test one channel’s sequence, uncheck the others, Save prop, then choose Test checked channels. It plays the viewed scene once while disarmed, leaving the triggered-scare assignment unchanged. Stop ends it immediately. Re-enable and save any channels you want in the finished prop. Quick-test pulse buttons remain independent of these scene checkboxes; pulse limits and direction interlocks still apply.

## Move and reuse cue patterns

For GPIO cues, Ctrl/Cmd-click selects several cues; Shift-click selects a range. On a phone, enable Select multiple and tap the cues. Copy keeps their relative timing. In Paste, select & move, choose a start time and destination channel, or keep their original channels. You can switch scenes before pasting. Extend scene to fit is optional and never exceeds 120 seconds. Duplicate places a copy after the selected pattern; Undo restores the previous edit.

Drag a selected group together, or nudge it with arrow keys. Left/right moves by 0.1 seconds (Alt: 0.01 seconds); up/down moves between GPIO channels. Ctrl/Cmd+C/V/D copies, pastes or duplicates when you are not typing into a field. Delete removes the selected cues. Invalid group edits are rejected together, so a failed paste cannot leave half a pattern behind.

The GPIO limit is 128 cues per scene. Servo movements retain their separate 32-block limit, lighting has 16 event cues, and audio has eight commands per scene. Those devices retain their contextual action editors.
