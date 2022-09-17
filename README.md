# Nintendo Switch Port of ONScripter

![Best girl!](https://user-images.githubusercontent.com/44071820/164560834-23aee131-bccc-47f1-9648-2016526cbccf.png)


A port of [onscripter-20060724-insani-sdl2](http://www.github.com/clamintus/onscripter-20060724-insani-sdl2) to the Switch.

I specifically made this to support running older NScripter-based games that would break on newer versions of ONScripter or ONScripter-EN.

## Features
- 4:3 aspect ratio simulation
- Touchscreen support
- RomFS integration
- Optimized flush implementation

## Tested games
- Tsukihime
- Kagetsu Tohya

I'm planning on testing other visual novels too.

Any game that uses legacy ONScripter should run just fine though. If not, please open an issue! :)

## How to Play
- Extract `onscripter-nx.zip` (or a game-specific zip from the releases) to the root of your SD card
- Go to the folder where `onscripter-nx.nro` is located (`/switch/` followed by `onscripter-nx/` or game-specific folder name)
- Copy the game files in that folder:
    - `arc*.nsa` / `arc*.sar`
    - `nscript.dat` / `nscript.___`
        - You don't need this if you are using a game-specific version of ONScripter-NX.
    - Soundtracks (`CD` folder, ...)
    - `default.ttf` font
        - You don't need this if you are using a game-specific version of ONScripter-NX.
- Launch ONScripter-NX from hbmenu
    - _Note:_ Launching from the album applet seems to work, but it is strongly advised to launch it by holding R on an installed game to avoid potential memory problems since that's the normal environment where Switch homebrews are designed to run.
- Enjoy!

## Controls
### Joy-Con Controls
- A -> Enter (or text advance)
- B -> Go back (or text advance)
- X -> Right-click Menu
- Y -> Toggle "Draw full page at once" mode
- D-Pad Up or L -> Scroll up text history
- D-Pad Down or R -> Scroll down text history
- Left stick -> Change text velocity (slow/normal/fast)
- Hold ZL or ZR -> Skip text
- Plus -> Exit to homebrew menu
### Joystick Controls
- Joystick Up or Joystick Left -> Previous Element
- Joystick Down or Joystick Right -> Next Element

Touch input is supported too.

## Building instructions
- You need [devkitPro](https://switchbrew.org/wiki/Setting_up_Development_Environment) in order to build ONScripter-NX and its external libraries.
- Open the shell (devkitPro MSYS if on Windows)
- Install the dependencies packages
    - the switch-dev metapackage: `pacman -S --needed switch-dev`
    - SDL2: `pacman -S --needed switch-sdl2 switch-sdl2_image switch-sdl2_mixer switch-sdl2_ttf`
    - SMPEG2: `pacman -S --needed switch-smpeg2`
    - libvorbis: `pacman -S --needed switch-libvorbis`
- **IMPORTANT!** At the time of writing this, the switch-sdl2_ttf package available through devkitPro pacman is broken and causes the app to not display text properly.
If it happens to you, please see [the following section for building the external libraries](#building-external-libraries), where I've provided a patch for switch-sdl2_ttf.
- If you want to integrate all your game files in one executable, put them in the `romfs` folder
- In your open shell, `cd` to the folder of this repository
- Run `make`. If you want to enable HD support, do `make FORCE_HD=1` instead. See [HD support remarks](#hd-support-remarks).

### Building external libraries
- Open the shell (devkitPro MSYS if on Windows)
- Install the dependencies packages
    - the patch utility: `pacman -S --needed patch`
    - devkitPro helper scripts: `pacman -S --needed dkp-toolchain-vars`
- In your open shell, `cd` to the folder of this repository
- Run `make -f Makefile.extlibs`

## HD support remarks
Starting from v0.3, ONScripter-NX supports HD resolution.\
In HD mode, the engine can take advantage of Switch's higher screen resolution to render text and other internal graphics at higher quality, making for a noticeable improvement, even if the visual novel being played itself was not made for HD screens (see [this picture](pics/hd-comparison.jpg) for reference).\
Game assets, however, **won't get any quality improvement**, and they will only be rescaled in order to fit the screen.\
Note that HD rescaling may occasionally cause [minor graphical glitches](pics/glitches.jpg) (due to the fact that the upscale ratios don't always match the exact desired resolution).
It will also overload the Switch's CPU a bit, causing slightly delayed input, in addition to some slowdown in moments where high CPU use is demanded (for example when skipping text).

If your visual novel was made for absurdly low screen resolutions, causing its graphics to get too blurry when rescaled, or if you are unhappy with the game's performances (or even if you just want to have the original feel to the game, just like me :P) you can always go with the normal, non-HD-enabled executable of the engine. Both versions are provided in the Releases section of this GitHub project.


## Credits
- [SDL2 Port](http://www.github.com/clamintus/onscripter-20060724-insani-sdl2) by clamintus
- Switch Port by clamintus
- Studio O.G.A. Ogapee - [original ONScripter](http://onscripter.osdn.jp/onscripter.html) and ONScripter Logo
- insani - [modified version of ONScripter](http://nscripter.insani.org/onscripter.html)
- Tomi - SDL2_ttf patch

And a huge thanks to SciresM for helping me out, and for his awesome work on Atmosphére's GDB stub <3


## License
See COPYING
