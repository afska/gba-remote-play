# raspi

## Install (Linux)

- Install C/C++ Build Tools
- Install VSCode
- Open `workspace.code-workspace`

## VSCode

- Recommended plugins: `C/C++ Extensions`, `EditorConfig`
- Recommended settings: [here](scripts/vscode_settings.json)

## Commands

- `./out/multiboot.tool out/gba.mb.gba`: Sends the ROM via Multiboot to the GBA
- `./run.sh`: Compiles and run the code. The output file is `out/raspi.run`
