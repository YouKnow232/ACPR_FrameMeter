# ACPR_FrameMeter

Frame meter mod for Guilty Gear XX Accent Core Plus R (Steam) via [GearLoader](https://github.com/YouKnow232/GearLoader).

A partial port of [ggxxacpr_overlay](https://github.com/YouKnow232/ggxxacpr_overlay).

<div align="center">
  <img src="./docs/img/Preview1.jpg" width="480" align="center" alt="Frame meter displayed along side the normal ggxxacpr HUD"/>
  <br/>
</div>


## Installation

* Download and install [GearLoader](https://github.com/YouKnow232/GearLoader) v1.1.0 or newer.
* Download "FrameMeter.zip" from [releases](https://github.com/YouKnow232/ACPR_FrameMeter/releases)
* Unzip "FrameMeter.zip" and place the contents into the "mods" folder

```text
Guilty Gear XX Accent Core Plus R/
└── mods/
    └── FrameMeter/
```


## Frame Meter Legend
![#01b597](https://placehold.co/15x15/01b597/01b597.png) Startup / Counter Hit State <br>
![#CB2B67](https://placehold.co/15x15/CB2B67/CB2B67.png) Active <br>
![#006FBC](https://placehold.co/15x15/006FBC/006FBC.png) Recovery <br>
![#C8C800](https://placehold.co/15x15/C8C800/C8C800.png) Blockstun / Hitstun <br>
![#969600](https://placehold.co/15x15/969600/969600.png) Knockdown / Techable Hitstun <br>
![#41F8FC](https://placehold.co/15x15/41F8FC/41F8FC.png) Movement <br>
### Under Lines
![#FFFF00](https://placehold.co/15x15/FFFF00/FFFF00.png) FRC <br>
![#FF0000](https://placehold.co/15x15/FF0000/FF0000.png) Slash Back <br>
![#FFFFFF](https://placehold.co/15x15/FFFFFF/FFFFFF.png) Full Invuln <br>
![#FF8000](https://placehold.co/15x15/FF8000/FF8000.png) Throw Invuln Only <br>
![#0080FF](https://placehold.co/15x15/0080FF/0080FF.png) Strike Invuln Only <br>
![#785000](https://placehold.co/15x15/785000/785000.png) Armor / Guardpoint / Parry<br>


## Reading a FrameMeter
The frame meter is simply a timeline of [frames](https://glossary.infil.net/?t=Frame). There are two meters, one for each player, that can be compared with each other to help gauge [startup](https://glossary.infil.net/?t=Startup) and [advantage](https://glossary.infil.net/?t=Advantage) as well as other timing concepts. The frame meter color codes each frame based on the state the player character was in during that frame.


## Settings
<div align="center">
  <img src="./docs/img/SettingsPreview1.jpg" width="480" align="center" alt="settings menu"/>
  <br/>
</div>

This mod features a settings menu that can be found at "Pause Menu -> Help & Options -> Mod Settings".


## Build Instructions
Build instructions are similar to [GearLoader's](https://github.com/YouKnow232/GearLoader).


### Prerequisites
* CMake v3.21 or later
* gcc v15.2.0 or later
    * i686-w64-mingw32-g++

For Windows users:
* MSYS2 environment


### Instructions
Run `Compile.sh` from the project root directory (in a MINGW32 shell for Windows users). You can set the environment variable `GGXXACPR_MOD_DIR` and the `Compile.sh` script will copy the build output to that directory.


## Known Issues
The white bracket startup indicators on the frame meter behave inconsistently for certain charged moves or moves with followups.
