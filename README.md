#EMSTune

Written by Michael Carpenter (malcom2073@gmail.com), and licensed under GPLv2.

### Overview:

EMSTune is the tuning application for the EMStudio suite.

It is compatible with LibreEMS out of the box, with plugins for Megasquirt and DynamicEFI/GMECU.

### Binaries:

Binaries are not currently maintained. A build server is in progress and will hopefully be up and running again soon.

### Compiling:

#### Linux:

A few packages are needed to compile EMSTune. In Ubuntu 14.04 or later:
```
$ sudo apt-get update
$ sudo apt-get install git qt5-qmake qt5-default qtscript5-dev libqt5webkit5-dev libqt5serialport5-dev libqt5svg5-dev flite1-dev libssl-dev libudev-dev qtquick1-5-dev freeglut3-dev
```
Once this has completed (or the equivalant packages on your own distro), you can then clone the repository and build it. 

It has been mentioned before to run qmake -project, NEVER EVER EVER DO THIS!. This will break things.

To checkout and build:
```
$ git clone https://github.com/malcom2073/emstune.git
$ cd emstune
$ qmake -r
$ make
$ cd core
$ ./emstune
```

#### Windows:

I really can't support compiling on Windows. It's too much of a hassle and there are way too many variables and different
machine configurations to do any real support. If someone is interested in maintaining Windows binaries let me know and I'll
work through it with you, but it's too much of a mess to provide general support. I'll handle compiling binaries for now.


##More Information

http://malcom2073.github.com/emstudio/
