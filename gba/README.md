# gba

## Install (Windows)

- Choose a folder (from now, `GBA_DIR`), and use this file structure:
	* `gba`
		* `tools`
			* `devkitPro`
		* `projects`
			* `gba-remote-play`
- Install the toolchain:
  * Dev
    * devkitPro: The devkit for compiling GBA roms
    * make: The build automation tool
  * Other
    * [Git Bash](https://gitforwindows.org): Linux shell and tools
    * [VSCode](https://code.visualstudio.com): The IDE
- Add to `~/.bash_profile`:
```bash
export GBA_DIR="/c/Work/gba" # <<< CHANGE THIS PATH
export GBARP_OUT_DIR="//192.168.0.199/pi/gba-remote-play/raspi/out" # <<< CHANGE THIS PATH

export DEVKITPRO="$GBA_DIR/tools/devkitPro"
export PATH="$PATH:$GBA_DIR/tools/devkitPro/bin"
export PATH="$PATH:$GBA_DIR/tools/devkitPro/devkitARM/bin"
export PATH="$PATH:$GBA_DIR/tools/devkitPro/tools/bin"
```

## VSCode

- Recommended plugins: `C/C++ Extensions`, `EditorConfig`
- Recommended settings: [here](scripts/vscode_settings.json)

## Build options

Before compiling, you can comment/uncomment these build parameters in `src/BuildConfig.h`:

Name | Description
--- | ---
`WITH_AUDIO` | Enables GSM audio decoding.

## Commands

- `make clean`: Cleans build artifacts
- `make assets`: Compiles the needed assets in `src/data/_compiled_files` (required for compiling)
- `make build`: Compiles and generates a `.gba` file
- `make start`: Starts the compiled ROM
- `make rebuild`: Recompiles (clean+build) a ROM
- `make restart`: Recompiles and copies the rom to `GBARP_OUT_DIR`
