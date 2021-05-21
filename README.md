# gba-remote-play

```cpp
// TODO: Describe
```


## Open-source projects involved

```cpp
// TODO: Describe
```

## Development

### Install (Windows)

- Choose a folder (from now, `GBA_DIR`), and use this file structure:
	* `gba`
		* `tools`
			* `devkitPro`
		* `projects`
			* `gba-remote-play`
- Install the toolchain:
  * Dev
    * devkitPro: The devkit for compiling GBA roms. It comes with:
      * *grit*: Used to convert paletted bitmaps to C arrays or raw binary files
      * *gbfs*: Used to create a package with all the game assets
    * make: The build automation tool
  * Other
    * [Git Bash](https://gitforwindows.org): Linux shell and tools
    * [VSCode](https://code.visualstudio.com): The IDE
- Add to `~/.bash_profile`:
```bash
export GBA_DIR="/c/Work/gba" # <<< CHANGE THIS PATH

export DEVKITPRO="$GBA_DIR/tools/devkitPro"
export PATH="$PATH:$GBA_DIR/tools/devkitPro/bin"
export PATH="$PATH:$GBA_DIR/tools/devkitPro/devkitARM/bin"
export PATH="$PATH:$GBA_DIR/tools/devkitPro/tools/bin"
```

### VSCode

- Recommended plugins: `C/C++ Extensions`, `EditorConfig`
- Recommended settings: [here](scripts/vscode_settings.json)

### Commands

- `make clean`: Cleans build artifacts
- `make assets`: Compiles the needed assets in `src/data/_compiled_files` (required for compiling)
- `make build`: Compiles and generates a `.gba` file
- `make start`: Starts the compiled ROM
- `make rebuild`: Recompiles (clean+build) a ROM
- `make restart`: Recompiles and starts the ROM
