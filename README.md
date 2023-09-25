# gba-remote-play

https://user-images.githubusercontent.com/1631752/125162840-7cb0be80-e160-11eb-8c10-cf8a09f7af2b.mp4

This software streams games from a Raspberry Pi to a Game Boy Advance, through its Link Port. Video and audio are compressed and sent in real time to the GBA, while the latter responds with its current input, allowing to play games of any platform by using the GBA (hence, Remote Play).

**Features:**

- Plays any game using [RetroPie](https://retropie.org.uk/) on the GBA!
- _120x80_ pixels of power!
- ~60fps using the default display mode
- Retro scanlines 😎
- Experimental audio support!
- Crashes on the GB Micro! _(yep, that's a feature)_

> <img alt="rlabs" width="16" height="16" src="https://user-images.githubusercontent.com/1631752/116227197-400d2380-a72a-11eb-9e7b-389aae76f13e.png" /> Created by [[r]labs](https://r-labs.io).

**Check out my other GBA projects!**

- [piuGBA](https://github.com/afska/piugba/): A PIU emulator 💃↙️↘️⬜↖️↗️🕺
- [gba-link-connection](https://github.com/afska/gba-link-connection): A multiplayer library 🎮🔗🎮

# GBA Jam 2021

All this code was made during the [GBA Jam 2021](https://itch.io/jam/gbajam21). Since this project doesn't fit well into the jam (as it requires external hardware), there's a Demo available in the [Releases](https://github.com/afska/gba-remote-play/releases) section where one GBA sends a video with audio to another GBA via Link Cable.

Here's a video of it:

https://user-images.githubusercontent.com/1631752/125164337-129c1780-e168-11eb-9bc8-9e2719a7180f.mp4

The code of that demo is in the [#gba-jam](https://github.com/afska/gba-remote-play/compare/gba-jam?expand=1) branch.

# Demo with audio

https://user-images.githubusercontent.com/1631752/125162670-a0273980-e15f-11eb-80fb-fee16ae5a6f7.mp4

# How it works

> :warning: This section will talk about implementation details. For setup instructions, scroll down to [Setup](#setup)! :warning:

Basically, there are two programs:

- On the GBA, a ROM that receives data.
- On the RPI, a program that collects and sends data.

The ROM is sent to the GBA by using the multiboot protocol, which allows small programs to be sent via [Link Cable](https://en.wikipedia.org/wiki/Game_Link_Cable). No cartridge is required.

## Serial communication

Communication is done through a GBA's Link Cable, soldered to the Raspberry Pi's pins.

<p align="center">
  <i>GBA Link Cable's pinout</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/124884342-8ee7fc80-dfa8-11eb-9bd2-4741a4b9acc6.png">
</p>

### Communication modes

The GBA supports [several serial communication modes](https://problemkaputt.de/gbatek.htm#gbacommunicationports). Depending on what _mode_ you use, the pins behave differently. The most common ones are:

- **Normal Mode**: It's essentially [SPI mode 3](https://en.wikipedia.org/wiki/Serial_Peripheral_Interface), but they call it "Normal mode" here. The transfer rate can be either _256Kbit/s_ or _2Mbit/s_, and packets can be 8-bit or 32-bit.
- **Multiplayer Mode**: What games normally use for multiplayer with up to 4 simultaneous GBAs. The maximum transfer rate is _115200bps_ and packets are always 16-bit.
- **General Purpose Input/Output**: Classic [GPIO](https://en.wikipedia.org/wiki/General-purpose_input/output), used for controlling LEDs, rumble motors, and that kind of stuff.

To have a decent frame rate, this project uses the maximum available speed: that's **Normal Mode at 2Mbps**, with 32-bit transfers.

### Normal Mode / SPI

SPI is a synchronous protocol supported by hardware in many devices, that allows full-duplex transmission. There's a master and a slave, and when the master issues a clock cycle, the two devices send data to each other (one bit at a time).

<p align="center">
  <i>SPI cycle</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/124885912-2437c080-dfaa-11eb-9eac-006ba64429b8.png">
</p>

> This is what happens on an SPI cycle. Both devices use shift registers to move the bits of data circularly. You can read more about the data transmission protocol [here](https://en.wikipedia.org/wiki/Serial_Peripheral_Interface#Data_transmission).

The GBA can work both as master or as slave, but the Raspberry Pi only works as master. So, the Raspberry controls the clock.

As for the connection, only 4 pins are required for the transmission: **CLK** (clock), **MOSI** (master out, slave in), **MISO** (master in, slave out), and **GND** (ground).

- On the GBA, these are Pin 5 (_SC_), Pin 3 (_SI_), Pin 2 (_SO_), and Pin 6 (_GND_).
- On the RPI, these are GPIO 11 (_SPI0 SCLK_), GPIO 10 (_SPI0 MOSI_), GPIO 9 (_SPI0 MISO_), and one of its multiples GNDs.

<p align="center">
  <i>GBA <-> Raspberry Pi connection diagram</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/124893645-497bfd00-dfb1-11eb-89d7-34fa7b3f77c8.png">
</p>

Some peculiarities about GBA's Normal Mode:

- When linking two GBAs, you need to use a GBC Link Cable. If you use a GBA one, the communication will be one-way: the slave will receive data but the master will receive zeroes.
- Communication at 2Mbps is only reliable when using very short wires, as it's intended for special expansion hardware. Or so they say, I've tested it with a long cable and it's not "unreliable", just slower 🤷‍♂️

**Related code:**

- [SPIMaster](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/SPIMaster.h#L12) on the Raspberry Pi
- [SPISlave](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/SPISlave.h#L17) on the GBA
- [SPIMaster](https://github.com/afska/gba-remote-play/blob/gba-jam/gba/src/gbajam/SPIMaster.h#L8) on the GBA (used for the GBA Jam demo)

### Reaching the maximum speed

In my tests with a Raspberry Pi 3, the maximum transfer rates I was able to achieve were:

- **Bidirectional**: 1.6Mbps. From here, the Raspberry Pi starts receiving garbage from the GBA.
- **One-way**: 2.56Mbps. Crank this up, and nothing good will happen.

One-way transfers are fine in this case, because we only care about input and some sync packets from the GBA. That means that the code is constantly switching between two frequencies depending of if it needs a response or not.
In all cases the Raspberry Pi has to wait a small number of microseconds to let the poor GBA's CPU rest.

<p align="center">
  <i>Speed benchmark</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/125121356-314bd100-e0ca-11eb-950d-6e848a53891f.png">
</p>

> The first dot means 40000 packets/second and each extra dot adds 5000 more. At maximum speed they should be all green. The one at the right indicates if we're free of corrupted packets. If it's red, adjust!

**Related code:**

- [Delay before each transfer](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/SPIMaster.h#L64)
- [GBA benchmark code](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/Benchmark.h#L53)
- [RPI benchmark code](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/Benchmark.h#L23)
- [SPI config](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/out/config.cfg#L1)

### MISO waits

In classic SPI, the master blindly issues clock cycles and it's responsability of the slave to catch up and process all packets on time. But here, sometimes the GBA is very busy doing things like putting pixels on screen or whatever it has to do, so it needs a way to tell the master to stop.

As recommended in the GBA manual, the slave can put MISO on HIGH when it's idle, and master can read its value as a GPIO input pin and wait to send until it's LOW.

<p align="center">
  <i>Pls don't send me anything</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/125120423-d2398c80-e0c8-11eb-8f67-62f2b0089949.gif">
</p>

- [Disable transfers (slave)](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/SPISlave.h#L48)
- [MISO wait (master)](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/SPIMaster.h#L67)

## Video

### Reading screen pixels

First, we need to configure [Raspbian](https://en.wikipedia.org/wiki/Raspberry_Pi_OS) to use a frame buffer size that that matches the GBA's resolution: **240x160**. There are two properties called `framebuffer_width` and `framebuffer_height` inside `/boot/config.txt` that let us change this.

Linux can provide all the pixel data shown on the screen (frame buffers) in devfiles like `/dev/fb0`. That works well when using desktop applications, but not for fullscreen games that use OpenGL -for example-, since they talk directly to the Raspberry Pi's GPU. So, to gather the colors no matter what application is running, we use the _dispmanx API_ (calling `vc_dispmanx_snapshot(...)` once per frame), which provides us a nice [RGBA32](https://en.wikipedia.org/wiki/RGBA_color_model#ARGB32) pixel matrix with all the screen data.

<p align="center">
  <i>Here's one of the many ways of reading the frame buffer wrong</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/125160284-77e50e00-e152-11eb-8d83-d94206e13ca5.jpg">
</p>

**Related code:**

- [FrameBuffer](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/FrameBuffer.h#L17)

### Drawing on the GBA screen

Instead of _RGBA32_, the GBA understands _RGB555_ (or _15bpp color_), which means 5 bits for red, 5 for green, and 5 for blue with no alpha channel. As it's a little-endian system, first one is red.

To draw those colors on the screen, it supports 3 [different bitmap modes](https://www.coranac.com/tonc/text/bitmaps.htm). For this project, I used _mode 4_, where each pixel is an **8-bit** reference to a palette of **256** 15bpp colors. The only consideration to have when using mode 4 is that VRAM doesn't support 8-bit writes, so you have to read first what's on the address to rewrite the complete halfword/word.

<p align="center">
  <i>15bpp color representation</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/125148694-8c9db380-e10a-11eb-96f9-43646f638491.png">
</p>

**Related code:**

- [GBA writing a pixel in mode 4](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/Utils.h#L30)

### Color quantization

So, the Raspberry Pi has to [quantize](https://en.wikipedia.org/wiki/Color_quantization) every frame to a 256 colors palette. In an initial iteration, I was using a quantization library that generated the most optimal palette for each frame. Though that's the best regarding image quality, it was too slow. The implemented solution ended up using a fixed palette ([this one](https://en.wikipedia.org/wiki/List_of_software_palettes#6-8-5_levels_RGB) in particular), and approximate every color to a byte referencing palette's colors.

<table border="0">
  <tr>
    <td>
      <p align="center">
        <i>Original image</i>
        <br>
        <img src="https://user-images.githubusercontent.com/1631752/125025069-60295f00-e058-11eb-930c-3c1b2c80d5ee.png">
      </p>
    </td>
    <td>
      <p align="center">
        <i>Quantized image</i>
        <br>
        <img src="https://user-images.githubusercontent.com/1631752/125025072-63bce600-e058-11eb-8f44-b33a782549fc.png">
      </p>
    </td>
  </tr>
</table>

To approximate colors faster, when running the code for the first time, it creates a 16MB [lookup table](https://en.wikipedia.org/wiki/Lookup_table) called "palette cache" with all the possible color convertions. It's 16MB because there are 2^24 possible colors and each palette index is one byte.

**Related code:**

- [15bpp palette on GBA](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/Palette.h#L6)
- [24bpp palette on RPI](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/Palette.h#L13)
- [Closest color math](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/Palette.h#L52)
- [Palette cache](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/Palette.h#L68)
- [JS code used to construct the table](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/Palette.h#L111)

### Scaling

The frame buffer is _240x160_ but what's sent to the GBA is configurable, so if you prefer a killer frame rate over detail you can send _120x80_ and use the [mosaic effect](https://www.coranac.com/tonc/text/gfx.htm#sec-mos) to scale the image so it fills the entire screen. Or, if you like old [CRT](https://en.wikipedia.org/wiki/Cathode-ray_tube)s, you could send _240x80_ and draw artificial scanlines between each actual line.

The Raspberry Pi discards each pixel that is not a multiple of the drawing scale. For example, if you use a 2x width scale factor, it will discard odd pixels and the resulting width will be _120_ instead of _240_.

At the time of rendering, you have to take this into account because GBA's _mode 4_ expects a _240x160_ pixel matrix. If you give it less, you'd only fill a part of the screen.

<table border="0">
  <tr>
    <td>
      <p align="center">
        <i>No scaling</i>
        <br>
        <img src="https://user-images.githubusercontent.com/1631752/125113283-d6f94300-e0be-11eb-9274-e6ef407e566f.gif">
      </p>
    </td>
    <td>
      <p align="center">
        <i>2x mosaic</i>
        <br>
        <img src="https://user-images.githubusercontent.com/1631752/125113332-ea0c1300-e0be-11eb-9ee0-8fd6e0d4eb49.gif">
      </p>
    </td>
    <td>
      <p align="center">
        <i>Scanlines</i>
        <br>
        <img src="https://user-images.githubusercontent.com/1631752/125113327-e8424f80-e0be-11eb-8dc5-9d057d7d39fc.gif">
      </p>
    </td>
  </tr>
</table>

> Here are 3 ways of scaling the same _120x80_ clip.

**Related code:**

- [RPI ignoring pixels](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/GBARemotePlay.h#L279)
- [GBA setting up mosaic](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/_main.cpp#L71)
- [GBA selecting the draw cursor](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/_main.cpp#L153)

### Image compression

#### Temporal diffs

The code only sends the pixels that changed since the previous frame, and what "changed" means can be configured: there's a `DIFF_THRESHOLD` parameter in the configuration file that controls how far should be a color to the previous one in order to refresh it.

At the compression stage, it creates a [bit array](https://en.wikipedia.org/wiki/Bit_array) where 1 means that a pixel did change, and 0 that it didn't. Then, it sends that array + the pixels with '1'.

<p align="center">
  <i>Example of a 13x1 diff array</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/125102399-9b0bb100-e0b1-11eb-9f71-32cbe1b16734.png">
</p>

**Related code:**

- [ImageDiffRLECompressor](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/ImageDiffRLECompressor.h#L8)
- [Bitarray transfer](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/GBARemotePlay.h#L192)
- [Diff decompression](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/_main.cpp#L211)
- [Diff threshold config](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/out/config.cfg#L4)

#### Run-length encoding

The resulting buffer of the temporal compression is run-length encoded.

When using palette images, it's highly likely that there are consecutive pixels with the same color. Or, for example, during screen transitions where all pixels are black, instead of sending N black pixels (N bytes) we can send 1 byte for N and then the black color (2 bytes). That's [RLE](https://en.wikipedia.org/wiki/Run-length_encoding).

However, RLE doesn't always make things better: it can sometimes produce a longer buffer than the original one because it has to add the "count" byte for every payload byte. For that reason, the encoding is made of two stages, and it only applies RLE if it helps compressing the data. Then, the frame's metadata stores a bit that represents if the payload is RLE'd or not.

<p align="center">
  <i>Encoding the compressed buffer</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/125102404-9cd57480-e0b1-11eb-9490-3622d2161991.png">
</p>

**Related code:**

- [RLE bit in frame's metadata](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/_main.cpp#L111)
- [RLEncoding](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/GBARemotePlay.h#L246)
- [RLDecoding](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/_main.cpp#L156)

#### Trimming the diffs

For a render resolution of _120x80_, the bit array would be _120x80/8 = 1200bytes_. That's a lot to transfer every frame, so it only sends the chunk from the first '1' to the last '1', but of course in 32-bit packets.

```
                                                                v startPacket                   v endPacket
PACKET 0                        PACKET 1                        PACKET 2                        PACKET 3                        PACKET 4                        PACKET 5
BYTE 0  BYTE 1  BYTE 2  BYTE 3  BYTE 4  BYTE 5  BYTE 6  BYTE 7  BYTE 8  BYTE 9  BYTE 10 BYTE 11 BYTE 12 BYTE 13 BYTE 14 BYTE 15 BYTE 16 BYTE 17 BYTE 18 BYTE 19 BYTE 20 BYTE 21 BYTE 22 BYTE 23
00000000000000000000000000000000000000000000000000000000000000000000000000100100110010000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
                                                                          ^ startPixel           ^endPixel
                                                                ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ packetsToSend

// 24x1 screen
maxPackets = 6
startPixel = 74
startPacket = startPixel / 8 / 4 = 2
endPixel = 97
endPacket = endPixel / 8 / 4 = 3
totalPackets = endPacket - startPacket + 1;
```

## Input

Each frame, the GBA sends its pressed keys to the Raspberry Pi. It does so by reading [REG_KEYINPUT](https://www.coranac.com/tonc/text/keys.htm) and transferring it on the initial metadata exchange.

<p align="center">
  <i>Bits are set when a key is **not** pressed. Weird design!</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/125149055-fd45cf80-e10c-11eb-8634-a1dbe27db9e1.png">
</p>

In Linux, there's `/dev/uinput` which lets user space processes create virtual devices and update its state. You can create your virtual gamepad however you like, for example, add analog sticks and then map GBA's D-pad to analog values. The current implementation just registers a simple gamepad with the same layout as the GBA.

**Related code:**

- [VirtualGamepad](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/VirtualGamepad.h#L27)
- [Gamepad name config](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/out/config.cfg#L5)

## Protocol overview

For every frame, the steps to run are:

- _(Reset if needed)_
- Build frame _(RPI only)_
- Sync frame start
- Metadata exchange (described below)
- _(If the frame has audio, sync and transfer audio)_
- Sync pixels start
- Transfer pixels
- Sync frame end
- Render _(GBA only)_

**Related code:**

- [GBA Main loop](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/_main.cpp#L81)
- [RPI Main loop](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/GBARemotePlay.h#L42)
- [Protocol](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/Protocol.h#L4)

### Metadata exchange

In this step, the GBA sends its input and receives a _frame metadata_ packet:

```
00000000000000000000000000000000
^#**************$$$$$$$$$$$$$$$$
|||             |
|||              > start pixel index (for faster GBA rendering)
|| > number of expected pixel packets
| > compressed flag: if 1, the frame is RLEncoded
 > audio flag: if 1, the frame includes an audio chunk
```

As a sanity check, this transfer is done twice. The second time, each device sends the received packet during the first transfer. If it doesn't match => **Reset!**

**Related code:**

- [Metadata exchange on the GBA](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/_main.cpp#L104)
- [Metadata exchange on the RPI](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/GBARemotePlay.h#L180)

## Audio

For the audio, the GBA runs [a port](https://github.com/pinobatch/gsmplayer-gba) of the [GSM Full Rate](https://en.wikipedia.org/wiki/Full_Rate) audio codec. It expects 33-byte audio frames, but in order to survive frame drops, GSM frames are grouped into chunks, with its length defined by a build time constant called `AUDIO_CHUNK_SIZE`.

**Related code:**

- [Audio chunk size constant](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/Protocol.h#L22)

### Reading system audio

On the Raspberry Pi side, we use [a virtual sound card](https://www.alsa-project.org/wiki/Matrix:Module-aloop) that is preinstalled on the system. When you start the module (`sudo modprobe snd-aloop`), two new sound devices appear (both for playing and recording).

<table border="0">
  <tr>
    <td>
      <p align="center">
        <i>Playback audio devices</i>
        <br>
        <img src="https://user-images.githubusercontent.com/1631752/125156531-419c9400-e13c-11eb-91df-91ca3dafc386.png">
      </p>
    </td>
    <td>
      <p align="center">
        <i>Capture audio devices</i>
        <br>
        <img src="https://user-images.githubusercontent.com/1631752/125156637-e15a2200-e13c-11eb-8ce3-8e42b57c28d5.png">
      </p>
    </td>
  </tr>
</table>

How it works is that if some application plays sound on -for example- `hw:0,0,0` (card 0, device 0, substream 0), another application can record on `hw:0,1,0` and obtain that sound. The loopback cards have to be set as the default output on the system, so we can record whatever sound is running on the OS.

### Encoding GSM frames

GSM encoding is done with [ffmpeg](http://ffmpeg.org/). The GBA port requires a non-standard rate of 18157Hz, so we have to tell it to ignore its checks, like "yeah, this is not officially supported, I don't care", as well as the new rate.

This is what the recording command looks like:

```
ffmpeg -f alsa -i hw:0,1 -y -ac 1 -af 'aresample=18157' -strict unofficial -c:a gsm -f gsm -loglevel quiet -
```

<p align="center">
  <i>I swear this is audio!</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/125157099-52023e00-e13f-11eb-9316-42b76ef35a27.png">
</p>

**Related code:**

- [LoopbackAudio](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/LoopbackAudio.h#L21)

### Controlling Linux pipes

The `-` at the end of the _ffmpeg_ command means "send the result to `stdout`". The code launches this process with [popen](https://man7.org/linux/man-pages/man3/popen.3.html) and reads through the created pipe.

Since transferring a frame takes time, it can sometimes happen that more audio frames are generated than what we can actually use. If we don't do anything about it, when reading the pipe we'd be actually reading audio _from the past_, producing a snowball of audio lag!

<p align="center">
  <i>Our GBA vibing to outdated audio frames</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/125158350-03f13880-e147-11eb-914d-cced48d84969.gif">
</p>

To fix that, there's an [ioctl](https://man7.org/linux/man-pages/man2/ioctl.2.html) we can use (called [FIONREAD](https://docs.oracle.com/cd/E19683-01/806-6546/kermes8-28/index.html)) to retrieve the amount of queued bytes. To skip over those, we call the [splice](<https://en.wikipedia.org/wiki/Splice_(system_call)>) system call to redirect them to `/dev/null`.

**Related code:**

- [Skipping outdated bytes](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/LoopbackAudio.h#L82)

### Decompressing on time

This was the most complex part of the project. Drawing pixels on the bitmap modes is already a lot of work for the GBA, and now it has to decompress GSM frames! Also, it can't lag. Lots of people tolerate low frame rates on video, but I don't think of anyone who can find acceptable hearing high pitch noises or even silence between audio samples.

What I understand _GSMPlayer_ does, is decoding GSM frames, putting the resulting audio samples in a double buffer, and setting up [DMA1](https://www.coranac.com/tonc/text/dma.htm) to copy them to a GBA's audio address, by using a special timing mode that syncs the copy with [Timer 0](https://www.coranac.com/tonc/text/timers.htm).

<p align="center">
  <i>Me attempting to modify GSMPlayer code</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/125159152-0d30d400-e14c-11eb-95ca-a5b1cccf8386.png">
</p>

Audio must be copied on time to prevent stuttering, noises, etc. Regular games do this by using [VBlank](https://www.coranac.com/tonc/text/video.htm#sec-blanks) [interrupts](https://www.coranac.com/tonc/text/interrupts.htm), but that doesn't work here. When transferring at 2.56Mbps there are very few cycles available to process data, and adding an interrupt handler just messes up the packets.

I had to make it so every transfer is cancellable: if it's time to run the audio (we're on the VBlank part), we stop everything, run the audio, and then start a recovery process where we say to the Raspberry Pi where we're at. On start, end, and every `TRANSFER_SYNC_PERIOD` packets of every stream, the Raspi sends a bidirectional packet (at the slow rate) to check if it needs to start the "recovery mode".

**Related code:**

- [ReliableStream](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/ReliableStream.h#L9)
- [Starting recovery process](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/_main.cpp#L246)
- [Handling recovery process](https://github.com/afska/gba-remote-play/blob/v0.9/raspi/src/ReliableStream.h#L87)
- [Transfers can be interrupted](https://github.com/afska/gba-remote-play/blob/v0.9/gba/src/SPISlave.h#L39)

## EWRAM Overclock

The GBA code overclocks the external RAM at the beginning, to use only one [wait state](https://en.wikipedia.org/wiki/Wait_state) instead of two. This process crashes on a GB Micro, but who would use this ~~on a Micro~~ anyway?

<p align="center">
  <i>A guy using a GB Micro with a Raspberry Pi attached to it</i>
  <br>
  <img src="https://user-images.githubusercontent.com/1631752/125161540-4d4a8380-e159-11eb-8d0a-804bfaf14a49.png">
</p>

# Setup

- Solder a Link Cable to the Raspberry Pi according to the **Normal Mode / SPI** section of this document.
- Install [RetroPie](https://retropie.org.uk/docs/First-Installation/).
- Set the following attributes in `/boot/config.txt`:

```
# Disable splash screen
disable_splash=1

# Aspect ratio (4:3)
hdmi_safe=0
disable_overscan=1
hdmi_group=2
hdmi_mode=6

# GBA render resolution
framebuffer_width=240
framebuffer_height=160

# Memory Split (for RetroPie)
gpu_mem_256=128
gpu_mem_512=256
gpu_mem_1024=256
#scaling_kernel=8
```

- In `raspi-config`, enable **SPI**.
- Set RetroArch to a 4:3 aspect ratio: Settings -> Video -> Aspect ratio -> **4:3**.
- Pick the required files from the [Releases](https://github.com/afska/gba-remote-play/releases) section of this GitHub repo.
- Load the GBA ROM with `./multiboot.tool gba.mb.gba`.
- Run the RPI backend with `sudo ./raspi.run`

### Audio (optional)

> It's optional because the Raspberry Pi already has pins for good old analog audio, and you could attach a speaker to it and have clean high-quality sound. On the other hand, audio support here is experimental and heavily decreases the frame rate.

If you want audio coming out from the GBA speakers anyway, here's how:

Change `/etc/modprobe.d/alsa-base.conf` and make it look like this:

```
options snd_aloop index=0
options snd_bcm2835 index=1
options snd_bcm2835 index=2
options snd slots=snd-aloop,snd-bcm2835
```

Then, when you run `cat /proc/asound/modules` you should see:

```
 0 snd_aloop
 1 snd_bcm2835
 2 snd_bcm2835
```

Now run `sudo modprobe snd-aloop` and set **Loopback (Stereo Full Duplex)** as the default output audio device from the UI.

As a last step, open the config file of GBA Remote Play (`config.cfg`) and make sure that `SPI_DELAY_MICROSECONDS` is `4`. **It won't work with smaller values!**

# Credits

This project relies on the following open-source libraries:

- GBA hardware access by [libtonc](https://www.coranac.com/projects/#tonc)
- GBA audio decompression by [gsmplayer-gba](https://github.com/pinobatch/gsmplayer-gba)
- GBA multiboot writer by [gba_03_multiboot](https://github.com/akkera102/gba_03_multiboot)
- Raspberry PI SPI transfers by the [C library for Broadcom BCM 2835](https://www.airspayce.com/mikem/bcm2835/)

The GBA Jam demo, uses these two open Blender clips with Creative Commons licenses:

- [Caminandes 2: Gran Dillama](https://www.youtube.com/watch?v=Z4C82eyhwgU)
- [Caminandes 3: Llamigos](https://www.youtube.com/watch?v=SkVqJ1SGeL0)

Also, here are some documentation links that I made use of:

- [TONC](https://www.coranac.com/tonc/text/toc.htm): The greatest GBA tutorial ever made.
- [GBATEK](https://problemkaputt.de/gbatek.htm): Documentation of NO$GBA with GBA internals.
- [rpi-fbcp's dispmanx API usage](https://github.com/tasanakorn/rpi-fbcp/blob/master/main.c)
- [Linux uinput's usage tutorial](https://blog.marekkraus.sk/c/linuxs-uinput-usage-tutorial-virtual-gamepad/)
- [Playing with ALSA loopback devices](https://sysplay.in/blog/linux/2019/06/playing-with-alsa-loopback-devices/)

Special thanks to my friend Lucas Fryzek ([@Hazematman](https://github.com/Hazematman)), who has a deep knowledge of embedded systems and helped me a lot with design decisions.
