# Nintendo Switch Port of ONScripter

![Definitely the best girl](https://user-images.githubusercontent.com/44071820/151049031-e6a37a39-9ca2-4830-a533-2577130e57e0.jpg)


A port of [onscripter-20060724-insani-sdl2](http://www.github.com/clamintus/onscripter-20060724-insani-sdl2) to the Switch.

I specifically made this to support running older NScripter-based games that would break on ONScripter-EN.

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

- A : Enter (or proceed in text mode)
- B : Back (or proceed in text mode)
- X : Right-click Menu
- Y : Draw full page at once
- Joystick UP / Joystick LEFT : Previous element
- Joystick DOWN / Joystick RIGHT : Next element
- D-Pad UP / L : Scroll up text history
- D-Pad DOWN / R : Scroll down text history
- ZL / ZR (held) : Skip mode
- Plus : Exit to homebrew menu

## Building instructions
- You need [devkitPro](https://switchbrew.org/wiki/Setting_up_Development_Environment) in order to build ONScripter-NX and its external libraries.
- Open the shell (devkitPro MSYS if on Windows)
- Install the dependencies packages
    - SDL2: `pacman -S switch-sdl2 switch-sdl2_image switch-sdl2_mixer switch-sdl2_ttf`
    - SMPEG2: `pacman -S switch-smpeg2`
- **IMPORTANT!** At the time of writing this, the switch-sdl2_ttf package available through devkitPro pacman is broken and causes the app to not display text properly.
If it happens to you, please see the following section for building the external libraries, where I've provided a patch for switch-sdl2_ttf.
- If you want to integrate all your game files in one executable, put them in the `romfs` folder
- In your open shell, `cd` to the folder of this repository
- Run `make`

### Building external libraries
- Open the shell (devkitPro MSYS if on Windows)
- Install the dependencies packages
    - pkg-config: `pacman -S switch-pkg-config`
    - devkitPro helper scripts: `pacman -S dkp-toolchain-vars`
- In your open shell, `cd` to the folder of this repository
- Run `make -f Makefile.extlibs`


## Credits
- [SDL2 Port](http://www.github.com/clamintus/onscripter-20060724-insani-sdl2) by clamintus
- Switch Port by clamintus
- Studio O.G.A. Ogapee - [original ONScripter](http://onscripter.osdn.jp/onscripter.html)
- insani - [modified version of ONScripter](http://nscripter.insani.org/onscripter.html) and ONScripter Logo
- Tomi - SDL2_ttf patch

And a huge thanks to SciresM for helping me out, and for his awesome work on Atmosphére's GDB stub <3


## License
See COPYING
