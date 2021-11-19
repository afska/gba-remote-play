# raspi

## Install

### Raspberry Pi

- Enable SSH and SPI

### Development PC

- Install VSCode, with these extensions:
  * `Remote Development`
  * `C/C++ Extensions`
- Run the command: _Remote-SSH: Connect to Host..._
  * Connect to the Raspberry's IP Address
  * Open the `raspi` directory

## Build options

Before compiling, you can comment/uncomment these build parameters in `src/BuildConfig.h`:

Name | Description
--- | ---
`WITH_AUDIO` | Enables GSM audio encoding.
`PROFILE` | Outputs the amount of frames per second to _stdout_.
`PROFILE_VERBOSE` | Outputs how much time in milliseconds every step takes.
`DEBUG` | Enables Debug Mode, where frames are sent one by one, on every user input from _stdin_.
`DEBUG_PNG` | Writes a `debug.png` file on every frame with the screen content. In order to use this, uncomment the `#ifdef`s in `lib/code/lodepng.c` and `lib/code/lodepng.h`.

## Commands

- `./out/multiboot.tool out/gba.mb.gba`: Sends the ROM via Multiboot to the GBA
- `./build.sh`: Compiles the code. The output file is `out/raspi.run`. Run with **sudo**!
- `./out/gbarplay.sh`: Sends the ROM and runs the compiled code
