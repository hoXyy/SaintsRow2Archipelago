# Saints Row 2 Archipelago

A heavily work-in-progress Archipelago implementation for Saints Row 2.

## Supported versions of the game

Steam and GOG versions of the game are supported. The Steam version will require [Juiced Patch](https://www.kobraworks.com/juiced), due to the vanilla Steam version including an encrypted EXE which is *not* supported.

> ⚠️ **NOTE:** The [Gentlemen of the Row](https://www.saintsrowmods.com/forum/threads/gentlemen-of-the-row.24/) and [PC DLC](https://www.kobraworks.com/sr2pcdlc) mods are not currently supported!

## Recommended mods

- [Juiced Patch](https://github.com/kobraworksmodding/Saints-Row-2-Juiced-Patch/releases) - **heavily recommended**, makes the PC port actually playable
- [High Quality Radio](https://www.saintsrowmods.com/forum/threads/high-quality-radio-mod.9515/) - if you intend to use the radio, this makes the radio quality actually bearable

## Installation

1. Download `SR2Archipelago.zip` and `saints_row_2.apworld` from the `Releases` tab, and get an ASI loader (recommended is the [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)).
2. Open the Archipelago client, and use the `Install APWorld` option to install the .apworld file you downloaded.
3. Follow your chosen's ASI loader install instructions (when using the Ultimate ASI Loader use `dinput8.dll` as the DLL name of choice).
4. Extract the `scripts` folder from `SR2Archipelago.zip` into the game folder (same folder that includes `SR2_pc.exe`).
5. In the Archipelago Launcher, open the `Saints Row 2 Client` and connect to your room.
6. Launch Saints Row 2, the AP client should show a message that the game integration plugin connected successfully.


### Extra instruction for Linux users

You need to add an extra environment variable, the way to do it depends fully on how you launch the game. As an example, for Steam you'd add this to the game's launch options:

> WINEDLLOVERRIDES=dinput8=n,b %command

Replace `dinput8` with the DLL name you use for your ASI loader if it's different.

## Building from source

The project is split into 2 parts, the native game integration plugin that lives in the `game` directory and the APWorld code that lives in `world`.

### APWorld

The APWorld can be built using the source-code version of the Archipelago Client as noted in the client's [documentation](https://github.com/ArchipelagoMW/Archipelago/blob/main/docs/apworld%20specification.md#build-apworlds-launcher-component). Clone the launcher's code, put the contents of this repository's `world` folder into a folder in the client's `worlds` directory, then run the launcher and use the `Build APWorlds` option. You'll find the generated .apworld in the client's `custom_worlds` directory.

### Game integration

The build instruction for the game integration plugin depend on the system you're building it on.

#### Linux

This project uses Microsoft's MSVC compiler even on Linux. It can be installed on Linux using the [msvc-wine](https://github.com/mstorsjo/msvc-wine) project. On Arch you can use the `msvc-wine-git` to simplify the installation (the Makefile is set up with this package in mind). Additionally, you'll need `make` and `cmake`.

After setting up the compiler, you can run `make` in the `game` directory to build the plugin. The output will be in the `build-msvc-linux` folder.

#### Windows

> Note: These instructions haven't been fully tested as this project is being developed on Linux.

On Windows you require Visual Studio 2022 (or possibly later) with the following components:
- Desktop development with C++
- MSVC x86/x64 build tools
- Windows SDK

Additionally, you need:
- [CMake 3.25+](https://cmake.org/download/)
- [Ninja](https://github.com/ninja-build/ninja/releases)
- GNU Make

You can also install `clang-format` if you want to use code formatting setup in this repo.

Make sure all these tools are available in the PATH before continuing.

Open an **x86 Native Tools Command Prompt for VS 2022**. Navigate to the `game` directory, and run `make`. The output will be in `build-msvc-win`.

## General future roadmap 

*Very much not an exhaustive list*

- More checks:
    - other collectibles like tags and stunt jumps
    - style level
- In-game AP indicators ([ImGui](https://github.com/ocornut/imgui)-based?)
- Better progression balancing