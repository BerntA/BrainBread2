# BrainBread 2 Source Code

### Introduction
BrainBread 2, also known as BrainBread: Source, has been in development for almost a decade. 
The goal was to rebuild the ancient master-piece mod for Half-Life 1, BrainBread.

In 2016 BrainBread 2 was released on Steam as a standalone sourcemod. 
It contained a vast spectrum of gamemodes, maps, weapons and customization.
Now anyone can feel free to contribute to this development, now that BrainBread 2 is open source.

### Compiling
Use the VPC scripts to generate the necessary project file(s).
[Source SDK 2013 Compiling Help](https://developer.valvesoftware.com/wiki/Source_SDK_2013)
* Windows: You can use VS2010, VS2013, 2015, 2017, 2019 to compile this project, however newer versions of Visual Studio might work as well. (Use the 2013 toolset, WinXP)
* Linux: Use gcc compiler, make sure that you have the latest version of libcurl. Make use of Steam Runtime to get the right headers for compatability. Run creategameprojects then simply write 'make -f games.mak'
* OSX: Use XCode, run the osx_compile.sh script after running creategameprojects.

### Contributing
Feel free to post issues, pull requests and such.
However, if you create a pull request, be sure to test your changes properly before submitting the request!

### Future Implementations & Improvements
- [ ] Add skill tree templates for fast switching between skill choices.
- [ ] Improve movement mechanics, such as camera movement when reloading weapons, more viewmodel swaying/movement.
- [ ] Fix tiny HUD / GUI text for some languages like Russian, Chinese, etc...

### Current Bugs
* Writing 'connect ip:port' in the console while in-game will bug if the server is password protected, you have to open the main menu in order to write in the desired password. (assuming you're using the console when the main menu isn't up)
* Very rare and random crash which occurs when you click OK on the motd in-game, when you're about to enter the game. (spawn)
* Engine related render crash, due to too many frames being rendered, happens in Termoil sometimes.
* Sometimes viewmodels will randomly go invisible until you respawn/get a new weapon of the same type.
* Very rarely some players will suddenly become invisible.
* When recording via HLTV, the objective HUD will be hidden.
* Changing resolution while the motd is up will create unforeseen consequences.
* Spectating starts inside the player, you have to refresh the spectate mode to fix this issue at this time.