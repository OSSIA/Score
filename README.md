i-score
=======


i-score is an interactive intermedia sequencer - read more on http://i-score.org/ or come chat with us ! [![join the chat at https://gitter.im/OSSIA/i-score](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/OSSIA/i-score?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

With i-score you can sequence OSC, MIDI, etc. parameters between multiple software and hardware, to create **i**nteractive **score**s.

#### Download the [latest release here](https://github.com/OSSIA/i-score/releases/) and read the Releases section in this README.

![i-score screenshot](/Documentation/iscore.png?raw=true)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bhttps%3A%2F%2Fgithub.com%2FOSSIA%2Fi-score.svg?type=shield)](https://app.fossa.io/projects/git%2Bhttps%3A%2F%2Fgithub.com%2FOSSIA%2Fi-score?ref=badge_shield)

## Releases

Releases are available for [Windows, Linux (via AppImage) and Mac OS X](https://github.com/OSSIA/i-score/releases/latest).

* Windows : install and run.
* OS X : extract and run i-score.app.
* Linux : make executable (right click -> permissions or `chmod +x`) and run the AppImage.

## Build status
* Linux, OS X : [![Travis Status](https://travis-ci.org/OSSIA/i-score.svg?branch=master)](https://travis-ci.org/OSSIA/i-score)
* Windows : [![Appveyor Status](https://ci.appveyor.com/api/projects/status/github/OSSIA/i-score?branch=master&svg=true)](https://ci.appveyor.com/project/JeanMichalCelerier/i-score)
* [![OpenHub](https://www.openhub.net/p/i-score/widgets/project_thin_badge.gif)](https://www.openhub.net/p/i-score)

In order to build i-score, follow the [instructions](https://github.com/OSSIA/i-score/wiki/Build-and-install).
Current builds tested on Mac OS X, Ubuntu 14.04, ArchLinux, Windows 8.1.

## Contributing

When updating the i-score repository through the command line, don't forget to update the submodules, they change often.

    cd i-score
    git pull
    git submodule update --init --recursive
    
Some useful and easy potential contributions are listed [on the website](http://i-score.org/contributing/).

There are also many fixable areas in the code : 

* [**TODO**](https://github.com/OSSIA/i-score/search?q=TODO) : for general problems. Some are [five-minute fixes](https://github.com/OSSIA/i-score/blob/2e393a1786154c11d766e6c6476cc2bd5faa95d0/base/plugins/iscore-lib-process/Process/Style/ScenarioStyle.cpp#L3), other are [a multiple-day quest](https://github.com/OSSIA/i-score/blob/2e393a1786154c11d766e6c6476cc2bd5faa95d0/base/lib/core/plugin/PluginDependencyGraph.hpp#L67)
* [**FIXME**](https://github.com/OSSIA/i-score/search?q=FIXME) : for critical bugs / problems.
* [**MOVEME**](https://github.com/OSSIA/i-score/search?q=REMOVEME) : when something is not where it should be.
* [**REMOVEME**](https://github.com/OSSIA/i-score/search?q=REMOVEME) : when something is redundant.
* [**RENAMEME**](https://github.com/OSSIA/i-score/search?q=RENAMEME) : when the class does not match with the file it's in.
* [**OPTIMIZEME**](https://github.com/OSSIA/i-score/search?q=OPTIMIZEME) : when an easy-to-write-but-suboptimal algorithm is used.

Finally, one can write its own i-score plug-ins to add custom features to the software.
An example plug-in with the most common classes reimplemented is provided [here](https://github.com/OSSIA/iscore-addon-tutorial).


## License
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bhttps%3A%2F%2Fgithub.com%2FOSSIA%2Fi-score.svg?type=large)](https://app.fossa.io/projects/git%2Bhttps%3A%2F%2Fgithub.com%2FOSSIA%2Fi-score?ref=badge_large)