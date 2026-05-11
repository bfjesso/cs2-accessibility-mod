# Counter Strike 2 accessibility mod
This mod is designed for accessibility and educational use only. It should not be used to gain an unfair advantage over other players. The features provided are for those who require additional assistance to compete on even ground. It includes aim assist and visual aids to acomplish this. With this mod, no one is excluded from the fun of playing CS2.

# Installation
You can build it yourself with Microsoft Visual Studio, or use the precompiled binaries already in the build folder. 

# How it works
When the injector is run, the dll is manually mapped into the cs2 game process, and a new thread is created for it to run. It still relies on the windows API to do this though, as it calls RPM, WPM, CRT, etc. It also runs in user mode.
Only the entity list offset is updated automatically using a signature. You can update the others by editing player.h if they no longer work.

# Dependencies
Menu made with ImGui: https://github.com/ocornut/imgui
