# Paradox Achievement Enabler
Enables achievements, with mods enabled, for the following supported games using DLL proxying:
- *Hearts of Iron IV*
- *Stellaris*
- *Europa Universalis IV*
## Overview
Paradox Achievement Enabler (PAE) enables achievements without modifying or patching the game executable on disk. The game binary remains untampered where only the functions, in memory, are patched when the proxy DLL is loaded.

A proxy `winmm.dll` is placed alongside the game executable, causing Windows to load it when the game requests the system `winmm.dll`. The proxy exports the required functions, and forwards the games calls to the original implementations.

The proxy DLL patches all the valid instructions found, and it makes these checks always return a valid result regardless of the actual checksum, allowing the game to treat the installation as unmodified and enable achievements.
```cpp
// Actual pseudocode 
int result = std::strcmp("<actual-checksum>", "<generated-checksum>");
auto instance = CAchievementManager::GetInstance();
instance->bGameOk = (result == 0);

// Patched pseudocode
int result = std::strcmp("<actual-checksum>", "<generated-checksum>");
auto instance = CAchievementManager::GetInstance();
instance->bGameOk = true;
```
## Motivation
I really dislike the game not allowing achievements when using some graphical or quality of life mods, even though they don't affect gameplay. Sure, a mod could make it easier to earn achievements, but why should Paradox get to decide which mods I can use while still earning achievements?

I also didn't want to create a patcher that modifies the actual game binary. The proxy DLL approach keeps the binary untouched, and makes installation and usage easier with less hassle.
## Installation
Drop the proxy DLL into the games root folder where the game executable is located: `hoi4.exe`, `stellaris.exe`, or `eu4.exe`

[Download](https://github.com/latvianfarmer17/Paradox-Achievement-Enabler/releases) a compiled DLL here.
## Compatibility
This should work for all versions of the game (past and future) as long as `winmm.dll` is loaded, as the DLL does not rely on "global" byte pattern scanning to locate addresses directly.
