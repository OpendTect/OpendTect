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
    - [Windows](#windows)
    - [MacOS](#macos)
    - [Linux](#linux)
- [Contributing](#contributing)
- [Contacts](#contacts)

## Repository Structure
The repository contains a number of release branches and 3 development branches. The current stable release branch is [6.4.4](https://github.com/OpendTect/OpendTect/tree/od6.4.4)

The development branches are:

| BRANCH | DESCRIPTION |
| -------------| ----------------- |
| [master](https://github.com/OpendTect/OpendTect/tree/master) | This is the bleeding edge where migration of OpendTect to new versions of its major dependencies, Qt and OpenSceneGraph, is tested and major new functionality is added.  |
| [od6.4](https://github.com/OpendTect/OpendTect/tree/od6.4)  | This is the main development branch  for the next stable release series 6.6. No new features are currently being added to this branch as it is being prepared for release. |
| [od6.5](https://github.com/OpendTect/OpendTect/tree/od6.5)  |  This branch is used for developing new incremental features against the 6.4 series codebase for future integration into the software.  |

## License

OpendTect is released under a triple licensing scheme:

- [Free - GNU GPLv3 or higher](http://www.gnu.org/copyleft/gpl.html)
- [Commercial - Pro License Agreement](http://www.opendtect.org/backendscripts/eula.php?format=pdf)
- [Academic License Agreement](https://dgbes.com/images/PDF/Proforma_OpendTect_academic_v5f.pdf)

## Building the Software
To build the software you need to also download and install/build a few dependencies which probably are not installed in your system. The version of dependencies varies between the branches. The Qt dependencies are available in binary installers, the others have to be built from source.

| BRANCH | DEPENDENCIES |
| -------------| ----------------- |
| od6.4.4, od6.4, od6.5 | [Qt 5.9.6](http://download.qt.io/archive/qt/5.9/5.9.6/qt-opensource-linux-x64-5.9.6.run), [OpenSceneGraph 3.6.3](http://download.qt.io/archive/qt/5.9/5.9.6/qt-opensource-linux-x64-5.9.6.run), [osgQt 3.5.7](https://github.com/openscenegraph/osgQt/archive/3.5.7.tar.gz) |
| master | [Qt 5.12.3](http://download.qt.io/archive/qt/5.12/5.12.3/qt-opensource-linux-x64-5.12.3.run), [OpenSceneGraph 3.6.3](http://download.qt.io/archive/qt/5.9/5.9.6/qt-opensource-linux-x64-5.9.6.run), [osgQt 3.5.7](https://github.com/openscenegraph/osgQt/archive/3.5.7.tar.gz) |

### Windows
### MacOS
### Linux

## Contributing
Any contributions you make are **greatly appreciated**.

1. Fork the Project
2. Create your local Feature branch (`git checkout -b feature/AmazingFeature`)
3. Make/Test your changes in the local Feature branch
4. Commit your Changes to the local Feature branch (`git commit -m 'Add some AmazingFeature'`)
5. Merge/rebase against the upstream OpendTect repository (`git fetch upstream/`
6. Push the Branch to your fork (`git push origin feature/AmazingFeature`)
7. Open a Pull Request

[dGB's coding guide](http://doc.opendtect.org/6.4.0/doc/Programmer/plugins.html#intro) provides a starting point for developing with the OpendTect codebase.

## Contacts
[dGB Earth Sciences](https://dgbes.com/index.php/contact)


[header-img]: doc/pics/opendtect_header.png
[example-img]: doc/pics/supported-functionality.jpg
