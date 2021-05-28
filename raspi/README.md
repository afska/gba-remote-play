# raspi

## Install

### Raspberry Pi

- Install Raspberry Pi OS
- Enable SSH, SPI, and Serial Port

### Development PC

- Install VSCode
- Install the _Remote Development_ extension
- Run the command: _Remote-SSH: Connect to Host..._
  * Connect to the Raspberry's IP Address
  * Open the `raspi` directory

## VSCode

- Recommended plugins: `C/C++ Extensions`, `EditorConfig`
- Recommended settings: [here](scripts/vscode_settings.json)

## Commands

- `./out/multiboot.tool out/gba.mb.gba`: Sends the ROM via Multiboot to the GBA
- `./run.sh`: Compiles and run the code. The output file is `out/raspi.run`
