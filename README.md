[![OpendTect header logo][header-img]](https://dgbes.com/index.php/software#free)
[![Example][example-img]]()

OpendTect is a free, open source seismic interpretation system and software development platform. The system supports all tools needed for visualizing, analyzing and interpreting 2D, 3D and 4D seismic and Geo_Radar data. The software is written in C++ and the same codebase compiles and runs on [Windows, MacOS and Linux](https://dgbes.com/index.php/software/supported-platforms). It also has a mature plugin programming interface that allows third parties to [develop plugins](https://dgbes.com/index.php/services/research-development#develop-your-own-plugins) to add functionality to the system without touching the OpendTect source code. A binary installer for OpendTect can be downloaded from the [dGB download page](https://dgbes.com/index.php/download).

This source code is released under the [GPLv3 or higher](http://www.gnu.org/copyleft/gpl.html) license. Commercial and Academic licenses are offered by [dGB Earth Sciences](https://dgbes.com) for [OpendTect Pro](https://dgbes.com/index.php/software#commercial), an extension of OpendTect that adds special functions for professional users and the potential to [rent or purchase commercial plugins](https://prostore.dgbes.com/) offering access to [unique seismic interpretation workflows](https://dgbes.com/index.php/software/plugins). The differences in functionality of the open source OpendTect, commercial OpendTect Pro and commercial plugins is compared [here](https://dgbes.com/index.php/software/supported-functionality).

OpendTect is a flexible and powerful R&D foftware platform that you can extend for seismic data analysis. [dGB Earth Sciences](https://dgbes.com/index.php/services/research-development) is also available to undertake industry funded (single or multi-client) projects to enhance OpendTect itself or create advanced plugin functionality. 

OpendTect is used worldwide by thousands of open source users, thousands of academic users and hundreds of commercial users.
## Table of Contents

- [Repository Structure](#repository-structure)
- [License](#license)
- [Building the Software](#building-the-software)
	- [Requirements](#requirements)
	- [Dependencies](#dependencies)
		- [Qt Install](#qt-install)
		- [OpenSceneGraph Build](#openscenegraph-build)
		- [osgQt Build](#osgqt-build)
 	- [Windows](#windows)
 	- [MacOS](#macos)
 	- [Linux](#linux)
- [Contributing](#contributing)
- [Contacts and More Information](#contacts-and-more-information)

## Repository Structure
The repository contains a number of release branches and 3 development branches. The current stable release branch is [6.4.4](https://github.com/OpendTect/OpendTect/tree/od6.4.4)

The development branches are:

| BRANCH | DESCRIPTION |
| -------------| ----------------- |
| [master](https://github.com/OpendTect/OpendTect/tree/master) | This is the bleeding edge where migration of OpendTect to new versions of its major dependencies, Qt and OpenSceneGraph, is tested and major new functionality is added.  |
| [od6.4](https://github.com/OpendTect/OpendTect/tree/od6.4)  | This is the main development branch  for the next stable release series 6.6. No new features are currently being added to this branch as it is being prepared for release. |
| [od6.5](https://github.com/OpendTect/OpendTect/tree/od6.5)  |  This branch is used for developing new incremental features against the 6.4 series codebase for future integration into the software.  |

## License
OpendTect is released under the [GPLv3 or higher](http://www.gnu.org/copyleft/gpl.html) license.

## Building the Software
### Requirements

- A C++ compiler and compilation tool chain:
	- Windows: msvc2017 64 bit. The free community edition is sufficient.
	- macOS:
	- Linux: gcc 64 bit version 4.8.5 or higher
- CMake version 2.8 or higher

### Dependencies
To build the software you need to also download and install/build a few dependencies which probably are not installed in your system. The version of dependencies varies between the branches. The Qt dependencies are available in binary installers, the others have to be built from source.

| BRANCH | DEPENDENCIES |
| -------------| ----------------- |
| od6.4.4, od6.4, od6.5 | [Qt 5.9.6](http://download.qt.io/archive/qt/5.9/5.9.6/), [OpenSceneGraph 3.6.3](http://download.qt.io/archive/qt/5.9/5.9.6/qt-opensource-linux-x64-5.9.6.run), [osgQt 3.5.7](https://github.com/openscenegraph/osgQt/archive/3.5.7.tar.gz) |
| master | [Qt 5.12.3](http://download.qt.io/archive/qt/5.12/5.12.3/), [OpenSceneGraph 3.6.3](http://download.qt.io/archive/qt/5.9/5.9.6/qt-opensource-linux-x64-5.9.6.run), [osgQt 3.5.7](https://github.com/openscenegraph/osgQt/archive/3.5.7.tar.gz) |

#### Qt Install
For the Qt install the following components must be selected depending on your build platform:

-  Desktop msvc2017 64- bit (Windows), XXXXX (macOS) or gcc 64 bit (Linux) 
-  QtWebEngine
-  Optionally source code or debug information files

#### OpenSceneGraph Build
Configure using CMake, compile and install. 

#### osgQt Build
Configure CMake ensuring to set the following variables:

- DESIRED\_QT\_VERSION=5
- CMAKE\_PREFIX\_PATH= set this to the location of the Qt cmake folder (Linux: "Qt install location"/gcc_64/lib/cmake)
- OSG_DIR="OpenSceneGraph install location"
- OpenGL\_GL\_PREFERENCE=LEGACY  (Linux only)

Compile and install. Note that osgQt will need to be built against the versions of Qt required by the respective OpendTect branch 

### Windows
Configure CMake ensuring to set the following variables:

- QTDIR= set this to the Qt install location for the appropriate version of Qt for the OpendTect version
- OSGQT= set this to the install location of osgQt for the version of Qt being used
- OSG_DIR="OpenSceneGraph install location"

Start msvc2017, open the OpendTect build solution and build.

### MacOS
### Linux
Configure CMake ensuring to set the following variables:

- QTDIR= set this to the Qt install location for the appropriate version of Qt for the OpendTect version
- OSGQT= set this to the install location of osgQt for the version of Qt being used
- OSG_DIR="OpenSceneGraph install location"
- OpenGL\_GL\_PREFERENCE=LEGACY 
- ZLIB\_INCLUDE\_DIR=  set this if not being found by CMake
- ZLIB\_LIBRARY= set this if not being found by CMake

Run make in the toplevel folder (i.e. where this README.MD file is located)

## Contributing
**PROBABLY WANT TO EXPAND THIS WITH GUIDANCE ON WHAT TYPE OF CONTRIBUTIONS WILL BE WELCOME AND WHERE  IN THE CODEBASE**
You can contribute to the enhancement of OpendTect either by:

-  providing bug fixes or enhancements to the OpendTect source code following the usual Github Fork-Pull Request process. 
- or independently by developing and releasing open source plugins from your own Github or equivalent repository. See the [wmplugins repository](https://github.com/waynegm/OpendTect-Plugins) as an example of this approach.

An overview of the design principles and preferred coding style/practices employed by **dGB** in the development of OpendTect are described in [dGB's coding guide](http://doc.opendtect.org/6.4.0/doc/Programmer/overview.html).

## Contacts and More Information

- [dGB Earth Sciences](https://dgbes.com/index.php/contact)
- [OpendTect Documentation](https://dgbes.com/index.php/support#documentation)
- [OpendTect Programmer's Manual](http://doc.opendtect.org/6.4.0/doc/Programmer/index.html)
- [OpendTect developers Google Group](https://dgbes.com/index.php/support/faq-developers-google-group)

[header-img]: doc/pics/opendtect_header.png
[example-img]: doc/pics/supported-functionality.jpg
