terminibbles
============

A terminal-based nibbles (snake) game.

Project homepage: http://matthew.malensek.net/projects/terminibbles

Compile and Install
-------------------
As long as you have a modern C compiler and ncurses, you should be able to:
```
cd /path/to/terminibbles
make
```
The default install directory is /usr/local. If you want to change this, set the PREFIX environment variable when installing. For instance, you might want to install to ~/.local:
```
PREFIX=$HOME/.local make install
```
If you're running Homebrew, you can also install via:
```
brew install malensek/brew/terminibbles
```

Changelog
---------
* 1.4 - Update command line args and add high score printing (@noahmorrison)
* 1.3 - Discover user config dir at run time rather than compile time
* 1.2 - High Score tracking by Noah Morrison (@noahmorrison)
* 1.1 - Minor bugfixes
* 1.0 - Initial Release
