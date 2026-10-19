# Scare and Ambient timelines

Under Build → Sequence, use Scare timeline or Ambient timeline. Switching tabs and browsing scare scenes changes only the editor; it does not change the scare assigned to triggers.

In Scare, choose a scene to edit. Click Use this scare for triggers only when you want that scene to run on a trigger, then Save prop. Random scare selection still chooses among all non-ambient scenes when enabled.

In Ambient, edit the assigned idle scene. If none is assigned, create a separate scene or assign an existing non-trigger scene. Save prop to apply. The ambient-scene selector changes the idle assignment explicitly. Ambient runs while armed between scares; a trigger interrupts it. Its automatic scare-track field is ignored, so use explicit audio timeline cues for ambient-scene commands.

GPIO, servo, lighting and audio tools, cue groups and local previews all target the scene currently being edited. Undo/Redo includes scene role assignments. Existing four-scene limits and saved JSON fields are unchanged. Fire scare always runs the saved trigger assignment, not the scene being browsed in the editor. Existing prop JSON remains compatible.

Overlapping ON/OFF cues on one output merge into a continuous ON window. Leave gaps for distinct pulses. Existing pulse limits and direction interlocks still apply.
