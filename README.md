# Chordshift

A diatonic transform chord sequencer for the [Expert Sleepers disting NT](https://expert-sleepers.co.uk/distingNT.html).

Record chords from a MIDI keyboard or select from built-in templates, then play them back as a sequence with a rich transform pipeline — transposition, inversion, rotation, spread, strumming, and more. All pitch logic operates in scale-degree space, so chords are always in-key and respond instantly when you change root or scale.

## Installation

1. Download the latest `chordshift.o` from [Releases](../../releases)
2. Copy to an SD card or push via MIDI using [ntpush](https://github.com/expertsleepersltd/distingNT/blob/main/tools/push_plugin_to_device.py)
3. Load the plugin on your disting NT — it appears as **Chordshift** under Instrument/Utility

## Quick Start

1. **Patch** a gate to **In1** (Run) and a clock to **In2** (Clock)
2. **Connect** a MIDI keyboard (via USB, breakout, or internal bus)
3. **Set** your Root and Scale on the Scale page
4. **Record**: Set Record to On, play a chord on the keyboard — it captures when you release all keys, then auto-advances to the next step
5. Repeat for each step (up to 8)
6. **Record Off**, raise the Run gate, and send clock pulses to play back your sequence

The display shows a step grid (top) and a velocity/note visualization of the current playing chord (bottom). The edit step blinks when recording.

## Inputs

| Input           | Function                                                         |
| --------------- | ---------------------------------------------------------------- |
| **In1** — Run   | Rising edge starts transport; falling edge stops (all notes off) |
| **In2** — Clock | Rising edge advances to the next step                            |

## Parameter Pages

### Setup

| Parameter   | Range                                       | Default  | Description                         |
| ----------- | ------------------------------------------- | -------- | ----------------------------------- |
| Run         | CV input                                    |          | Gate input for transport start/stop |
| Clock       | CV input                                    |          | Trigger input for step advance      |
| MIDI In Ch  | All, 1–16                                   | 1        | MIDI input channel filter           |
| MIDI Out Ch | 1 – 16                                      | 2        | MIDI output channel                 |
| Destination | Breakout, SelectBus, USB, Internal, All     | Internal | MIDI output routing                 |
| Velocity    | 1 – 127                                     | 100      | Base velocity for output notes      |

### Scale

| Parameter | Range              | Default | Description                                                          |
| --------- | ------------------ | ------- | -------------------------------------------------------------------- |
| Root      | C – B              | C       | Scale root note                                                      |
| Scale     | Ionian – Min Penta | Ionian  | Scale type (7 modes + Harmonic Min, Melodic Min, Maj/Min Pentatonic) |
| Octave    | C1 – C7            | C3      | Base octave for degree-to-MIDI conversion                            |

### Record

| Parameter    | Range      | Default | Description                                                   |
| ------------ | ---------- | ------- | ------------------------------------------------------------- |
| Record       | Off, On   | Off     | Enable chord capture from MIDI input                          |
| Edit Step    | 1 – 8      | 1       | Which step to record into (auto-advances after capture)       |
| Capture Norm | Off, On   | Off     | Normalize captured chords so the lowest note becomes degree 0 |
| Clear Step   | No, Yes   | No      | Clear the current edit step                                   |
| Clear All    | No, Yes   | No      | Clear all steps                                               |
| Copy Step    | No, Yes   | No      | Copy current edit step to clipboard                           |
| Paste Step   | No, Yes   | No      | Paste clipboard to current edit step                          |
| Reset All    | No, Yes   | No      | Reset all step parameters to defaults                         |

### Playback

| Parameter | Range                            | Default | Description                                  |
| --------- | -------------------------------- | ------- | -------------------------------------------- |
| Play Mode | Forward, Reverse, Pendulum, Random | Forward | Step sequencer direction                     |
| Steps     | 1 – 8                           | 8       | Number of active steps                       |
| Clock Div | /1, /2, /3, /4, /6, /8, /12, /16 | /4     | Divide incoming clock before step advance    |

### Pitch

Global pitch transforms — combined additively with per-step values.

| Parameter     | Range                         | Default | Description                                |
| ------------- | ----------------------------- | ------- | ------------------------------------------ |
| Transpose     | -14 – +14                     | 0       | Shift chord by scale degrees               |
| Reflect       | Off, Root, Lowest, Highest | Off     | Mirror notes around a reference point      |
| Spread        | -7 – +7                       | 0       | Expand or compress intervals between notes |
| Spread Anchor | Lowest, Center               | Lowest  | Reference point for spread                 |

### Voicing

| Parameter | Range                     | Default | Description                          |
| --------- | ------------------------- | ------- | ------------------------------------ |
| Inversion | -4 – +4                                                                     | 0       | Move bottom/top notes across octaves                 |
| Rotation  | -7 – +7                                                                     | 0       | Rotate the note order cyclically                     |
| Normalize | None, Lowest=0, First=0                                                     | None    | Re-center degrees after transforms                   |
| Direction | Up, Down, Pendulum, PingPong, Diverge, Converge, Random, Pedal Lo, Pedal Hi | Up      | Note playback order within the chord (affects strum) |
| Reverse   | No, Yes                                                                     | No      | Reverse the note order                               |

### Articulate

| Parameter  | Range                                           | Default | Description                                 |
| ---------- | ----------------------------------------------- | ------- | ------------------------------------------- |
| Strum      | 0 – 100 ms                                      | 0       | Delay between successive notes in the chord |
| Vel Shape  | Ramp, Curve, Peak, Step, Random              | Ramp    | Velocity distribution across strummed notes |
| Vel Depth  | 0 – 100%                                        | 0       | Amount of velocity curve applied            |
| Vel Deviation | 0 – 100%                                     | 5       | Random velocity variation per note          |
| Time Shape | Off, Ramp, Curve, Peak, Step, Random        | Off     | Strum timing distribution                   |
| Time Depth | 0 – 100%                                        | 0       | Amount of time curve applied                |

### Randomize

Generate random step parameter variations with contour-shaped distribution.

| Parameter    | Range                                                    | Default | Description                                      |
| ------------ | -------------------------------------------------------- | ------- | ------------------------------------------------ |
| Randomize    | No, Yes                                                  | No      | Trigger randomization                            |
| Contour      | Random, Arc, Rise, Fall                                  | Arc     | Shape of parameter variation across steps        |
| Seq Length   | None, 8, 16, 32, 64, 128, 256                           | 32      | Sequence length preset (sets Steps + Repeat)     |
| Seq Div      | None, 1, 2, 4, 8, 16, 32                                | 4       | Clock division preset                            |
| Seq Hold     | Varied, Uniform                                          | Varied  | Hold step variation style                        |
| Template     | Off, 5th, Triad, 7th, 5th+Tri, 5th+7th, Tri+7th, All   | Tri+7th | Chord templates to pick from                     |
| Transpose %  | 0 – 100%                                                 | 40      | Probability of randomizing transpose per step    |
| Inversion %  | 0 – 100%                                                 | 25      | Probability of randomizing inversion per step    |
| Rotation %   | 0 – 100%                                                 | 10      | Probability of randomizing rotation per step     |
| Spread %     | 0 – 100%                                                 | 10      | Probability of randomizing spread per step       |
| Reverse %    | 0 – 100%                                                 | 25      | Probability of randomizing reverse per step      |
| Gate %       | 0 – 100%                                                 | 25      | Probability of randomizing gate length per step  |
| Repeat %     | 0 – 100%                                                 | 0       | Probability of randomizing repeat per step       |

### Per-Step (Step 1–8)

Each step has its own set of transforms that combine with the global values.

| Parameter | Range                                                                       | Default | Combination                                          |
| --------- | --------------------------------------------------------------------------- | ------- | ---------------------------------------------------- |
| Template  | Custom, Note, Fifth, Triad, 7th, Sus2, Sus4, Shell, Quartal, Cluster       | Custom  | Uses template degrees instead of captured chord      |
| Enabled   | No, Yes                                                                     | Yes     | —                                                    |
| Transpose | -14 – +14                                                                   | 0       | Additive (global + step)                             |
| Inversion | -4 – +4                                                                     | 0       | Additive                                             |
| Rotation  | -7 – +7                                                                     | 0       | Additive                                             |
| Spread    | -7 – +7                                                                     | 0       | Additive                                             |
| Reverse   | No, Yes                                                                     | No      | XOR (global ^ step)                                  |
| Strum     | 0 – 100 ms                                                                  | 0       | Additive                                             |
| Velocity  | -64 – +64                                                                   | 0       | Offset from base velocity                            |
| Gate      | 10 – 200%                                                                   | 100     | Gate length as percentage of step duration            |
| Prob      | 0 – 100%                                                                    | 100     | Probability the step plays                           |
| Reflect   | Off, Root, Lowest, Highest                                                  | Off     | Overrides global if nonzero                          |
| Repeat    | 1 – 4                                                                       | 1       | Ratchet — repeat the chord N times per step          |
| Hold      | 1 – 8                                                                       | 1       | Hold step for N clock ticks, scaling gate duration    |
| Direction | Up, Down, Pendulum, PingPong, Diverge, Converge, Random, Pedal Lo, Pedal Hi | Up      | Overrides global direction if nonzero                |

## How Capture Works

1. **Build your chord**: With Record on, play and hold the notes you want. Each time you press a new key, the plugin updates the chord it's listening to.
2. **Release to save**: When you lift all keys, the chord is saved to the current edit step.
3. **Auto-advance**: The edit step moves forward automatically, wrapping back to step 1 after the last step.

You always hear what you play — notes pass through to the output whether Record is on or off.

## Transform Pipeline

On each clock tick the sequencer copies the base chord and runs it through four transform stages:

```
Base Chord → Pitch → Voicing → Normalize → Order → Render → MIDI Out

Pitch:     Transpose → Reflect → Spread
Voicing:   Inversion → Rotation
Normalize: None / LowestTo0 / FirstTo0
Order:     Reverse → Direction
```

## Building from Source

Requires `arm-none-eabi-g++` (ARM GCC toolchain).

```bash
git clone --recurse-submodules <repo-url>
cd midi-chords
make            # Build chordshift.o
make push       # Build and push to disting NT via ntpush
```
