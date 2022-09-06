[![OpendTect header logo][header-img]](https://dgbes.com/index.php/software#free)
[![Example][example-img]]()

OpendTect is a free, open source seismic interpretation system and software development platform. The system supports all tools needed for visualizing, analyzing and interpreting 2D, 3D and 4D seismic and Ground Penetrating Radar (GPR) data. The software is written in C++ and the same codebase compiles and runs on [Windows, MacOS and Linux](https://dgbes.com/index.php/software/supported-platforms). It also has a mature plugin programming interface that allows third parties to [develop plugins](https://dgbes.com/index.php/services/research-development#develop-your-own-plugins) to add functionality to the system without touching the OpendTect source code. A binary installer for OpendTect can be downloaded from the [dGB download page](https://dgbes.com/index.php/download).

This source code is released under the [GPLv3 or higher](http://www.gnu.org/copyleft/gpl.html) license. Commercial and Academic licenses are offered by [dGB Earth Sciences](https://dgbes.com) for [OpendTect Pro](https://dgbes.com/index.php/software#commercial), an extension of OpendTect that adds special functions for professional users and the potential to [rent or purchase commercial plugins](https://prostore.dgbes.com/) offering access to [unique seismic interpretation workflows](https://dgbes.com/index.php/software/plugins). The differences in functionality of the open source OpendTect, commercial OpendTect Pro and commercial plugins is compared [here](https://dgbes.com/index.php/software/supported-functionality).

OpendTect is a flexible and powerful R&D software platform that you can extend for seismic data analysis. [dGB Earth Sciences](https://dgbes.com/index.php/services/research-development) is also available to undertake industry funded (single or multi-client) projects to enhance OpendTect itself or create advanced plugin functionality. 

OpendTect is used worldwide by thousands of open source users, thousands of academic users and hundreds of commercial users.
## Table of Contents

- [Repository Structure](#repository-structure)
- [License](#license)
- [Building the Software](#building-the-software)
	- [Requirements](#requirements)
	- [Dependencies](#dependencies)
		- [Qt Install](#qt-install)
		- [OpenSceneGraph Build](#openscenegraph-build)
		- [Proj Build](#proj-build)
		- [Sqlite Install](#sqlite-install)
		- [HDF5 Install](#hdf5-install)
 	- [Windows](#windows)
 	- [MacOS](#macos)
 	- [Linux](#linux)
- [Contributing](#contributing)
- [Contacts and More Information](#contacts-and-more-information)

## Repository Structure
The repository contains a number of release branches and 2 development branches. The current stable release branch is [7.0](https://github.com/OpendTect/OpendTect/tree/od7.0)

The development branches are:

| BRANCH | DESCRIPTION |
| -------------| ----------------- |
| [main](https://github.com/OpendTect/OpendTect/tree/main) | This is the bleeding edge where migration of OpendTect to new versions of its major dependencies, Qt and OpenSceneGraph, is tested and major new functionality is added.  |
| [od7.0](https://github.com/OpendTect/OpendTect/tree/od7.0)  | This is the main development branch for the next stable release series 7.0. No major new features are currently being added to this branch as it is being prepared for release. |

## License
OpendTect is released under the [GPLv3 or higher](http://www.gnu.org/copyleft/gpl.html) license.

## Building the Software
### Requirements

- A C++ compiler and compilation tool chain:
	- Windows: msvc2022 64 bit (>= v17.3.1) or msvc2019 64 bit (>= v16.7.5). The free community edition is sufficient.
	- macOS: SDK 10.14
	- Linux: gcc 64 bit version 5.4.0 or higher
- CMake version 3.14 or higher
- The c++14 version is enabled by default on all platforms.

### Dependencies
To build the software you need to also download and install/build a few dependencies which probably are not installed in your system. The version of dependencies varies between the branches. The Qt dependencies are available in binary installers, the others have to be built from source.

| BRANCH | DEPENDENCIES |
| -------------| ----------------- |
| main | [Qt 5.15.2](http://download.qt.io/archive/qt/5.15/5.15.2/), [OpenSceneGraph 3.6.5](https://github.com/openscenegraph/OpenSceneGraph/archive/OpenSceneGraph-3.6.5.tar.gz), [Proj 9.0.1 (optional)](https://download.osgeo.org/proj/proj-9.0.1.tar.gz), [Sqlite 3.38 (optional)](https://www.sqlite.org/download.html), [HDF5 1.12.2 (optional)](https://www.hdfgroup.org/downloads/hdf5) |
| od7.0 | [Qt 5.15.2](http://download.qt.io/archive/qt/5.15/5.15.2/), [OpenSceneGraph 3.6.5](https://github.com/openscenegraph/OpenSceneGraph/archive/OpenSceneGraph-3.6.5.tar.gz), [Proj 9.0.1 (optional)](https://download.osgeo.org/proj/proj-9.0.1.tar.gz), [Sqlite 3.38 (optional)](https://www.sqlite.org/download.html), [HDF5 1.12.2 (optional)](https://www.hdfgroup.org/downloads/hdf5) |
| od6.6_rel, od6.6 | [Qt 5.15.2](http://download.qt.io/archive/qt/5.15/5.15.2/), [OpenSceneGraph 3.6.5](https://github.com/openscenegraph/OpenSceneGraph/archive/OpenSceneGraph-3.6.5.tar.gz), [HDF5 1.12.2 (optional)](https://www.hdfgroup.org/downloads/hdf5) |
| od6.4.5, od6.4 | [Qt 5.9.6](http://download.qt.io/archive/qt/5.9/5.9.6/), [OpenSceneGraph 3.6.3](https://github.com/openscenegraph/OpenSceneGraph/archive/OpenSceneGraph-3.6.3.tar.gz) |

#### Qt Install
For the Qt install the following components must be selected depending on your build platform:

-  Desktop msvc2019 64- bit (Windows), SDK 10.14 (macOS) or gcc 64 bit (Linux) 
-  QtWebEngine
-  Optionally source code or debug information files

#### OpenSceneGraph Build
Configure using CMake, compile and install. 

#### Proj Build
Configure using CMake, compile and install. 

#### Sqlite Install
Retrieve from their download site or the OpendTect SDK

#### HDF5 Install
The link to HDF5 requires to provide the path to an existing HDF5 installation. All versions above 1.10.3 are supported, but using the current API 1.12 is preferred. Installation is best done using their binary installations (on Windows especially), or from the package manager on Linux. Windows developers however need to recompile the sources since no debug binary libraries can be downloaded.

### Windows
Configure CMake ensuring to set the following variables:

- QTDIR= set this to the Qt install location for the appropriate version of Qt for the OpendTect version
- OSG_DIR="OpenSceneGraph install location"
- PROJ_DIR="PROJ install location" or BUILD_PROJ=ON or OD_NO_PROJ=ON to disable it
- SQLITE_DIR="SQLITE install location" (optional, but required by Proj)
- HDF5_ROOT="HDF5 install location" (optional)

Start Visual Studio 2022, open the OpendTect build solution and build.

### MacOS
### Linux
Configure CMake ensuring to set the following variables:

- QTDIR= set this to the Qt install location for the appropriate version of Qt for the OpendTect version
- OSG_DIR="OpenSceneGraph install location"
- PROJ_DIR="PROJ install location" or BUILD_PROJ=ON or OD_NO_PROJ=ON to disable it
- SQLITE_DIR="SQLITE install location" (optional, but required by Proj)
- HDF5_ROOT="HDF5 install location" (optional)
- OpenGL\_GL\_PREFERENCE=LEGACY 
- ZLIB\_INCLUDE\_DIR=  set this if not being found by CMake
- ZLIB\_LIBRARY= set this if not being found by CMake

Run make in the toplevel folder (i.e. where this README.MD file is located)

## Contributing
[//]: # (PROBABLY WANT TO EXPAND THIS WITH GUIDANCE ON WHAT TYPE OF CONTRIBUTIONS WILL BE WELCOME AND WHERE  IN THE CODEBASE)
You can contribute to the enhancement of OpendTect either by:

- providing bug fixes or enhancements to the OpendTect source code following the usual Github Fork-Pull Request process. 
- or independently by developing and releasing open source plugins from your own Github or equivalent repository. See the [wmplugins repository](https://github.com/waynegm/OpendTect-Plugins) as an example of this approach.

An overview of the design principles and preferred coding style/practices employed by **dGB** in the development of OpendTect are described in [dGB's coding guide](http://doc.opendtect.org/7.0.0/doc/Programmer/Default.htm#6_principles_and_best_practices_in_opendtect_coding.htm)

## Contacts and More Information

- [dGB Earth Sciences](https://dgbes.com/index.php/contact)
- [OpendTect Documentation](https://dgbes.com/index.php/support#documentation)
- [OpendTect Programmer's Manual](http://doc.opendtect.org/7.0.0/doc/Programmer/Default.htm)
- [OpendTect developers Google Group](https://dgbes.com/index.php/support/faq-developers-google-group)

[header-img]: doc/pics/opendtect_header.png
[example-img]: doc/pics/supported-functionality.jpg
