# Kindle Touch Doom

Doom port for touchscreen Kindles, based on [doomgeneric](https://github.com/ozkl/doomgeneric).

![Doom running on Kindle](screenshots/kindle.png)

## Compatibility

- Only tested on [KT3](https://wiki.mobileread.com/wiki/Kindle_Serial_Numbers)
- Should work on other [KT](https://wiki.mobileread.com/wiki/Kindle_Serial_Numbers) models [with the same 600 × 800 screen](https://en.wikipedia.org/wiki/Amazon_Kindle#Specifications)
- Will currently not work on the higher-res Paperwhite models

## Installation Instructions

1. [Jailbreak](https://www.mobileread.com/forums/showthread.php?t=320564) your Kindle
2. Install the [KUAL](https://www.mobileread.com/forums/showthread.php?t=203326) extension by unzipping the GitHub release into the `extensions` folder on your Kindle.
3. Put your `doom.wad` into the folder of this extension.
4. Launch using KUAL
5. To exit, press the `ESC` on-screen button, select "Quit Game", then `Enter`, then `Y` (see below for the button locations)

### Controls

![Controls for Doom running on Kindle](screenshots/controls.png)

## Compilation Instructions

- TBD

## Credits

- Doomgeneric: [ozkl on GitHub](https://github.com/ozkl/doomgeneric)
- Framebuffer Display & Dithering Code: [geekmaster on the MobileRead Forums](https://www.mobileread.com/forums/showthread.php?t=177455)
- C Compiler Toolchain: [dtinth on GitHub](https://github.com/dtinth/docker-kindle-k5-toolchain)
- Finger Icon: [inspire-studio on Pixabay](https://pixabay.com/vectors/touch-digital-icon-finger-press-6602643/)
