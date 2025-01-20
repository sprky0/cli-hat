# clihat

A minimal command-line utility in C that generates a TR-606–style hi-hat as a 24-bit, 48 kHz WAV.  
It writes the WAV data either to `stdout` or to a specified file.

## Features

- A swarm of **detuned square oscillators** simulating a metallic “cymbal” tone.
- Single **amplitude envelope** that can be short (closed hat) or longer (open hat).
- Simple command-line parameters for easy fine-tuning.

## Building

Make sure you have a C compiler (e.g., `gcc`) and (optionally) `make`.

```bash
   make

```

This will produce an executable, e.g. clihat.
Or, compile directly with whatever compatible compiler you want!  I'm not your dad.


## Usage:

```bash
  ./clihat <tune> <decay> <level> <open> [-o <output.wav>]

```

Where each parameter is in the range 0.0 to 1.0.

- tune: Shift the base frequencies up or down.
- decay: Sets the baseline envelope decay from short to long.
- level: Overall output volume.
- open: Controls how “closed” or “open” the hat is:
    - 0.0 => short decay (closed)
    - 1.0 => longer decay (open)
- By default, if -o <file> is not provided, the program writes the WAV to stdout.

## Examples

### Closed hat:

```bash
./clihat 0.5 0.3 1.0 0.0 -o closedHat.wav

```
Middle tune, moderate decay, full volume, fully closed.

### Open hat:

```bash
./clihat 0.5 0.3 1.0 1.0 -o openHat.wav

```

Same tuning, but extended decay for an open-style sound.

### Pipe directly to playback (Linux + SoX):

```bash
./clihat 0.7 0.5 1.0 1.0 | play -t wav -

```

This skips writing to disk by piping raw WAV data to play.

