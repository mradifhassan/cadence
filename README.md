# Cadence — Typing Test with a Mechanical Keyboard

A fast, minimal typing speed test written in C. It shows a modern on-screen mechanical keyboard that reacts to every key you press, plays realistic key sounds, and tracks your words per minute and accuracy.

- **GTK 4** for the window and interface
- **Cairo + Pango** for the keyboard and text rendering
- **SDL2** for polyphonic key-click audio

> **Platform status:** developed and tested on **Linux (Ubuntu 24.04, GTK 4.14)**. The Windows and macOS instructions below use the standard toolchains (MSYS2 and Homebrew) and should work, but they have not been tested by the author. If something differs on your system, see [Troubleshooting](#troubleshooting).

---

## Table of contents

1. [Features](#features)
2. [How to use it](#how-to-use-it)
3. [Get the source code](#get-the-source-code)
4. [Install and run on Linux](#install-and-run-on-linux)
   - [Debian, Ubuntu, Linux Mint, Pop!_OS](#debian-ubuntu-linux-mint-pop_os-deb)
   - [Fedora, openSUSE, other RPM-based](#fedora-opensuse-and-other-rpm-based-distros)
   - [Arch, Manjaro, EndeavourOS](#arch-linux-manjaro-endeavouros)
5. [Install and run on Windows](#install-and-run-on-windows)
6. [Install and run on macOS](#install-and-run-on-macos)
7. [Installing system-wide (optional)](#installing-system-wide-optional)
8. [Customizing](#customizing)
9. [Troubleshooting](#troubleshooting)
10. [Project layout](#project-layout)

---

## Features

- Modern 60% ANSI keyboard with two-line symbol legends, homing bumps on **F** and **J**, and smooth key-travel animation.
- Every physical key lights up as you press it: **green** for a correct character, **red** for a mistake, **blue** for keys such as Backspace and Shift.
- A **purple outline** shows the next key to press. If you need Shift, the opposite-hand Shift key lights up too.
- Four test modes: **25 words**, **15 s**, **30 s**, **60 s**.
- Live WPM, accuracy and time, with a result summary when you finish.
- Realistic key sounds with slight pitch variation. Your own `assets/click.wav` is used if present; otherwise a built-in click is generated.
- Dark theme, scrolling three-line text window, no clicking required: just start typing.
- A tiny GitHub icon under the keyboard opens the project repository in your browser.

---

## How to use it

| Action | What to do |
| --- | --- |
| Start a test | Just start typing. The timer begins on your first keystroke. |
| Fix a mistake | **Backspace** |
| New random text | **Tab** |
| Restart with the same text | **Esc** |
| Change mode | Click **25 words / 15 s / 30 s / 60 s** at the top |
| Mute or unmute sounds | Speaker button in the title bar |

**How the numbers are calculated**

- **WPM** = (correctly typed characters ÷ 5) ÷ minutes elapsed. This is the standard definition, where one "word" is five characters.
- **Accuracy** = correct keystrokes ÷ total keystrokes. Mistakes you correct with Backspace still count against accuracy.
- In timed modes the test ends automatically when the time runs out. In word mode it ends when you type the last character.

Shortcuts that include **Ctrl**, **Alt** or **Super/Cmd** are passed through to your system, so things like Alt+Tab still work.

---

## Get the source code

Choose one of these:

**Option A: Git (recommended)**

```bash
git clone https://github.com/mradifhassan/cadence.git
cd cadence
```


**Option B: Download a ZIP**

1. Download the ZIP from the repository page (usually *Code → Download ZIP*).
2. Extract it.
3. Open a terminal inside the extracted folder (the one containing `meson.build` and `src/`).

All commands in this README are run **from the project root**, the folder that contains `meson.build`.

---

## Install and run on Linux

You need three things: a C compiler, the build tools (Meson and Ninja), and the development packages for **GTK 4 (version 4.6 or newer)** and **SDL2**.

First check whether your distribution is new enough. After installing the packages below, this should print `4.6.0` or higher:

```bash
pkg-config --modversion gtk4
```

### Debian, Ubuntu, Linux Mint, Pop!_OS (deb)

Requires Ubuntu 22.04+, Debian 12+, or Linux Mint 21+.

```bash
sudo apt update
sudo apt install build-essential git meson ninja-build pkg-config \
                 libgtk-4-dev libsdl2-dev
```

Build and run:

```bash
meson setup build
ninja -C build
./build/cadence
```

### Fedora, openSUSE and other RPM-based distros

**Fedora**

```bash
sudo dnf install gcc git meson ninja-build pkgconf-pkg-config \
                 gtk4-devel SDL2-devel
```

**openSUSE (Tumbleweed or Leap 15.5+)**

```bash
sudo zypper install gcc git meson ninja pkgconf-pkg-config \
                    gtk4-devel SDL2-devel
```

**RHEL, AlmaLinux, Rocky Linux:** version 9 ships GTK 4.4, which is older than the 4.6 this project requires. Use a newer release (10+) or Fedora. Check with `pkg-config --modversion gtk4` after installing `gtk4-devel`.

Build and run:

```bash
meson setup build
ninja -C build
./build/cadence
```

### Arch Linux, Manjaro, EndeavourOS

Arch packages include the headers, so there is no separate `-dev` package.

```bash
sudo pacman -S --needed base-devel git meson ninja pkgconf gtk4 sdl2
```

If pacman asks which provider to use for `sdl2`, choose `sdl2-compat` (the current default).

Build and run:

```bash
meson setup build
ninja -C build
./build/cadence
```

### Without Meson (plain gcc)

If you prefer not to install Meson, any Linux distribution with the packages above can build with one command:

```bash
gcc -Wall -O2 -o cadence src/main.c $(pkg-config --cflags --libs gtk4 sdl2 pangocairo) -lm
./cadence
```

The `$(pkg-config ...)` part is essential. It supplies the include paths and libraries. Compiling with just `gcc main.c` fails with `gtk/gtk.h: No such file or directory`.

The included helper script does the same thing and picks Meson or gcc automatically:

```bash
./build.sh          # Meson/Ninja build  -> build/cadence
./build.sh gcc      # plain gcc build    -> ./cadence
```

---

## Install and run on Windows

The most reliable way to build GTK 4 programs on Windows is **MSYS2**, which provides GTK 4, SDL2 and a compiler as ready-made packages.

### 1. Install MSYS2

1. Download the installer from <https://www.msys2.org> and run it. The default location `C:\msys64` is fine.
2. When it finishes, open **"MSYS2 UCRT64"** from the Start menu. Use this one, not the plain "MSYS2 MSYS" shell and not "MINGW32".

### 2. Update MSYS2

In the UCRT64 terminal:

```bash
pacman -Syu
```

If it asks you to close the terminal, close it, open **MSYS2 UCRT64** again and run `pacman -Syu` once more.

### 3. Install the tools and libraries

```bash
pacman -S --needed git \
    mingw-w64-ucrt-x86_64-toolchain \
    mingw-w64-ucrt-x86_64-meson \
    mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-pkgconf \
    mingw-w64-ucrt-x86_64-gtk4 \
    mingw-w64-ucrt-x86_64-SDL2
```

Press Enter to accept the default selection when asked about the toolchain group.

### 4. Get the code and build

Your Windows folders appear under `/c/` in MSYS2 (for example `C:\Users\You\cadence` is `/c/Users/You/cadence`).

```bash
git clone <your-repository-url> cadence
cd cadence
meson setup build
ninja -C build
```

### 5. Run

Run it **from the project root** so it can find `assets/click.wav`:

```bash
./build/cadence.exe
```

The program will still work if it can't find the file; it falls back to its built-in click sound.

**Running outside the MSYS2 terminal:** the executable needs the GTK and SDL2 DLLs. Either add `C:\msys64\ucrt64\bin` to your Windows `PATH`, or copy the DLLs listed by `ldd build/cadence.exe` (only the ones under `/ucrt64/`) next to `cadence.exe`. If you do the second, GTK also needs its resources (icons, schemas), so adding the MSYS2 `bin` folder to `PATH` is the simpler option.

**Hiding the console window (optional):** add `win_subsystem: 'windows'` to the `executable(...)` call in `meson.build` and rebuild.

---

## Install and run on macOS

Uses [Homebrew](https://brew.sh). Works on both Apple Silicon and Intel Macs.

### 1. Install Homebrew and Xcode command-line tools

```bash
xcode-select --install
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Skip whichever you already have.

### 2. Install the dependencies

```bash
brew install gtk4 sdl2 meson ninja pkg-config git
```

### 3. Get the code and build

```bash
git clone <your-repository-url> cadence
cd cadence
meson setup build
ninja -C build
```

### 4. Run

Run it **from the project root** so it can find `assets/click.wav` (the built-in click is used otherwise):

```bash
./build/cadence
```

On a Mac keyboard, **Cmd** appears as the on-screen **Super** key and **Option** as **Alt**.

---

## Installing system-wide (optional)

On Linux, macOS, or MSYS2 you can install the program and click sound into your system:

```bash
sudo ninja -C build install      # on MSYS2, omit "sudo"
```

After that, `cadence` is available from any terminal. The binary goes to `<prefix>/bin` and `click.wav` to `<prefix>/share/cadence`, where the program looks for it automatically. The default prefix is `/usr/local`. To choose another one, configure with `meson setup build --prefix=$HOME/.local`.

---

## Customizing

- **Key sound:** replace `assets/click.wav` with any short WAV file. It is converted automatically to the right format, and only the first second is used. Delete the file to use the built-in click.
- **Word list:** edit the `WORDS[]` array near the top of `src/main.c`.
- **Words per test or time options:** change `WORDS_MODE_COUNT`, `MODE_SECONDS[]` and `MODE_LABELS[]` in `src/main.c` (keep `N_MODES` in sync).
- **Colors:** the interface colors are in the `CSS` string in `src/main.c`; the keyboard colors are the `PAL_*` palettes above `draw_key`. The theme is based on Catppuccin Mocha.
- **GitHub link:** the repository opened by the footer icon is the `CADENCE_REPO_URL` define near the top of `src/main.c`.
- **Text font:** change the `TEXT_FONT` define. The first installed family in the list is used.

After any change, rebuild with `ninja -C build`.

---

## Troubleshooting

**`fatal error: gtk/gtk.h: No such file or directory`**
The GTK 4 development package isn't installed, or you compiled without `pkg-config` flags. Install the packages for your platform from the sections above and use the Meson commands or the `gcc ... $(pkg-config ...)` command.

**`Dependency "gtk4" not found` / `Package gtk4 was not found`**
The dev package is missing or too old. Run `pkg-config --modversion gtk4`. You need 4.6 or newer. On older distributions, upgrade the OS, or use a newer one.

**The build fails and the error mentions `/tmp/...` paths, or you changed machines or packages**
The `build/` folder holds a cached configuration from an earlier setup. Delete it and start fresh:

```bash
rm -rf build
meson setup build
ninja -C build
```

**No sound**
- Check the speaker button in the title bar isn't muted.
- Make sure another app isn't holding your audio device exclusively.
- Force a specific SDL audio driver, for example `SDL_AUDIODRIVER=pulseaudio ./build/cadence` (also try `alsa` or `pipewire`).
- The program still runs fine without audio.

**The window renders strangely or is blank (virtual machines, old graphics drivers)**
Switch GTK to software rendering:

```bash
GSK_RENDERER=cairo ./build/cadence
```

**Warning: `Unable to acquire session bus: ... dbus-launch`**
Harmless. It only appears in minimal environments without a D-Bus session.

**The text or key legends use a different font than the screenshots**
Install JetBrains Mono (text) and Ubuntu/Inter (key legends) for the intended look. Without them the program falls back to your system fonts, which works fine.

**Keys appear stuck**
Click the window or switch away and back. All key states are released whenever the window loses focus.

**Non-English keyboard layouts**
The test text is English ASCII. Keys that produce non-ASCII characters are ignored, so switch to a US/UK-style layout while practicing.

---

## Project layout

```
cadence/
├── src/
│   └── main.c        # the entire application
├── assets/
│   └── click.wav     # optional custom key sound
├── meson.build       # Meson build definition
├── cadence.desktop   # desktop entry for the application menu
├── LICENSE           # GPL-3.0-only
└── build.sh          # helper: builds with Meson, or plain gcc with "./build.sh gcc"
```

---

## License

GPL-3.0-only, as declared in `meson.build`.
# cadence
