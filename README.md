[![OpendTect header logo][header-img]](https://dgbes.com/software/opendtect)
[![Example][example-img]](https://dgbes.com/software/supported-functionality)

OpendTect is a free, open source seismic interpretation system and software development platform. The system supports all tools needed for visualizing, analyzing and interpreting 2D, 3D and 4D seismic and Ground Penetrating Radar (GPR) data. The software is written in C++ and the same codebase compiles and runs on [Windows, macOS and Linux](https://dgbes.com/software/supported-platforms). It also has a mature plugin programming interface that allows third parties to [develop plugins](https://dgbes.com/library#documentation) to add functionality to the system without touching the OpendTect source code. A binary installer for OpendTect can be downloaded from the [dGB download page](https://dgbes.com/software/download).

This source code is released under the [GPLv3 or higher](https://www.gnu.org/licenses/gpl-3.0.html) license. Commercial and Academic licenses are offered by [dGB Earth Sciences](https://dgbes.com) for [OpendTect Pro](https://dgbes.com/software/opendtect-pro), an extension of OpendTect that adds special functions for professional users and the potential to [rent or purchase commercial plugins](https://prostore.dgbes.com/) offering access to [unique seismic interpretation workflows](https://dgbes.com/software/commercial-plugins). The differences in functionality of the open source OpendTect, commercial OpendTect Pro and commercial plugins are compared [here](https://dgbes.com/software/supported-functionality).

OpendTect is a flexible and powerful R&D software platform that you can extend for seismic data analysis. [dGB Earth Sciences](https://www.dgbes.com/services#custom-software-development) is also available to undertake industry funded (single or multi-client) projects to enhance OpendTect itself or create advanced plugin functionality.

OpendTect is used worldwide by thousands of open source users, thousands of academic users and hundreds of commercial users.

## Table of Contents

- [Repository Structure](#repository-structure)
- [License](#license)
- [Building the Software](#building-the-software)
  - [Build Using Pixi](#build-using-pixi)
  - [Build Manually](#build-manual)
    - [Requirements](#requirements)
    - [Dependencies](#dependencies)
      - [QT](#qt)
      - [OSG](#osg)
      - [OPENSSL](#openssl)
      - [PROJ](#proj)
      - [SQLite3](#sqlite3)
      - [HDF5](#hdf5)
    - [Windows](#windows)
    - [Linux](#linux)
- [Contributing](#contributing)
- [Contacts and More Information](#contacts-and-more-information)

## Repository Structure

The repository contains release branches and 2 development branches. The release branch for OpendTect 2026 is [od8.0_rel](https://github.com/OpendTect/OpendTect/tree/od8.0_rel).

The development branches are:

| BRANCH | DESCRIPTION |
| -------------| ----------------- |
| [main](https://github.com/OpendTect/OpendTect/tree/main) | This is the bleeding edge where migration of OpendTect to new versions of its major dependencies, Qt and OpenSceneGraph, is tested and major new functionality is added.  |
| [od8.0](https://github.com/OpendTect/OpendTect/tree/od8.0)  | This is the development branch for the OpendTect 2026 releases. |

## License

OpendTect is released under the [GPLv3 or higher](https://www.gnu.org/licenses/gpl-3.0.html) license.

## Building the Software

### Build Using Pixi

The [Pixi](https://pixi.prefix.dev/latest/) package manager can be used to obtain all required dependencies and build the OpendTect development (main) branch. Everything is kept within the source code folder to avoid cluttering your system. A fully functional build of OpendTect is created in the **inst** folder. OpendTect can be built from source and run with just 3 commands. Some dependency versions installed by Pixi are lower than the versions used for the official builds.

**Note for Windows**: you will need to install the MSVC toolset 17.44 (64-bit). The free community edition is sufficient.

#### Install Pixi

<details>
  <summary><b>Windows</b></summary>

  <kbd>[**Download installer**](https://github.com/prefix-dev/pixi/releases/latest/download/pixi-x86_64-pc-windows-msvc.msi)</kbd>

  Or run:

```powershell
powershell -ExecutionPolicy ByPass -c "irm -useb https://pixi.sh/install.ps1 | iex"
```

</details>
<details>
  <summary><b>Linux</b></summary>

```bash
  curl -fsSL https://pixi.sh/install.sh | sh
```

</details>

#### Git Clone the OpendTect Main Branch
Assuming **git** is installed, open a shell window and enter:
```bash
git clone -b main --depth 1 https://github.com/OpendTect/OpendTect.git
```
This will create a folder called **OpendTect** with the source code.
#### Configure
To configure the build, change to the source code folder and enter:
```bash
pixi run config
```
#### Build
To build in Release mode, open a shell, change to the source code folder (unless already there) and enter:
```bash
pixi run build
```
Or to build in Debug mode, open a shell, change to the source code folder (unless already there) and enter:
```bash
pixi run build-dev
```
#### Run
To run the Release build, open a shell, change to the source code folder (unless already there) and enter:
```bash
pixi run release
```
Or to run the Debug build, open a shell, change to the source code folder (unless already there) and enter:
```bash
pixi run debug
```

### Build Manually
#### Requirements

- A C++ compiler and compilation tool chain:
  - Windows: MSVC toolset 17.44 (64-bit). The free community edition is sufficient.
  - Linux: gcc 64 bit version 13.3.1 or higher
  - macOS: SDK 14 (deployment version >= 12.0)
- CMake version 3.24 or higher (3.27 or higher recommended)
- The C++17 standard is enabled by default on all platforms.

#### Dependencies
To build the software you need to also download and install/build a few dependencies which probably are not installed in your system. The version of dependencies varies between the branches. The Qt dependencies are available in binary installers, the others have to be built from source.

| BRANCH | DEPENDENCIES |
| -------------| ----------------- |
| main | [QT 6.9.3](https://download.qt.io/archive/qt/6.9/6.9.3/), [OSG 3.6.5](https://github.com/openscenegraph/OpenSceneGraph/archive/OpenSceneGraph-3.6.5.tar.gz), [OPENSSL 3.5.9](https://www.openssl.org/source/openssl-3.5.9.tar.gz), [PROJ 9.8.1 (optional)](https://download.osgeo.org/proj/proj-9.8.1.tar.gz), [SQLite3 3.53.4 (required when PROJ is enabled)](https://www.sqlite.org/download.html), [HDF5 2.2.0 (optional)](https://www.hdfgroup.org/downloads/hdf5) |
| od8.0_rel, od8.0 | [QT 6.8.3](https://download.qt.io/archive/qt/6.8/6.8.3/), [OSG 3.6.5](https://github.com/openscenegraph/OpenSceneGraph/archive/OpenSceneGraph-3.6.5.tar.gz), [PROJ 9.8.1 (optional)](https://download.osgeo.org/proj/proj-9.8.1.tar.gz), [SQLite3 3.51.1 (required when PROJ is enabled)](https://www.sqlite.org/download.html), [HDF5 1.14.6 (optional)](https://www.hdfgroup.org/downloads/hdf5) |

##### QT
For the Qt install the following components must be selected depending on your build platform:

- 'MSVC 2022 64-bit' (Windows), 'Desktop' (Linux/macOS)
- Additional Libraries: Qt Charts, Qt Positioning, Qt WebChannel
- Extensions: Qt WebEngine (MSVC 2022 x64/Desktop)
- Optionally debug information files (recommended on Windows)
- Optionally Qt source code is available for all the versions in use:
  - [Qt 6.9.3](https://dgbearthsciences.sharefile.com/public/share/web-sef14c254e42342d5bfdb1a36ae955420) : main
  - [Qt 6.8.3](https://dgbearthsciences.sharefile.com/public/share/web-sad64fa4ce9974762a877a682d9d1a679) : od8.0, od8.0_rel

##### OSG
Configure using CMake, compile and install.

##### OPENSSL
Configure, compile and install. OpenSSL 3 is required, including the SSL and Crypto components.

##### PROJ
Configure using CMake, compile and install. OpendTect can be built and run without PROJ. Set `OD_NO_PROJ=ON` to disable it. Enabling PROJ makes building SQLite3 mandatory.

##### SQLite3
Retrieve from their download site or the OpendTect SDK. OpendTect can be built and run without SQLite3. Building SQLite3 is mandatory when PROJ is enabled.

##### HDF5
The link to HDF5 requires to provide the path to an existing HDF5 installation. All versions above 1.10.3 are supported, but using the current API 2.2 is preferred. Installation is best done using their binary installations (on Windows especially), or from the package manager on Linux. Windows developers however need to recompile the sources since no debug binary libraries can be downloaded. Only the C components of the HDF5 installation are required. The current Pixi setup does not work with HDF5 2.1.0 and Qt 6.9.3. There is a compatibility issue between these versions.

#### Windows
Configure CMake ensuring to set the following variables:

- QT_ROOT= set this to the Qt install location for the appropriate version of Qt for the OpendTect version
- OSG_ROOT="OpenSceneGraph install location"
- OPENSSL_ROOT_DIR="OpenSSL install location"
- PROJ_ROOT="PROJ install location", or OD_NO_PROJ=ON to build without PROJ
- SQLite3_ROOT="SQLite3 install location" (required when PROJ is enabled)
- HDF5_ROOT="HDF5 install location" (optional)

Start Visual Studio with the MSVC toolset 17.44, open the OpendTect solution and build.

#### Linux
Configure CMake ensuring to set the following variables:

- QT_ROOT= set this to the Qt install location for the appropriate version of Qt for the OpendTect version
- OSG_ROOT="OpenSceneGraph install location"
- OPENSSL_ROOT_DIR="OpenSSL install location"
- PROJ_ROOT="PROJ install location", or OD_NO_PROJ=ON to build without PROJ
- SQLite3_ROOT="SQLite3 install location" (required when PROJ is enabled)
- HDF5_ROOT="HDF5 install location" (optional)
- ZLIB_ROOT="ZLIB install location" (optional)
- OpenGL_GL_PREFERENCE=LEGACY

Run make in the toplevel folder (i.e. where this README.md file is located)

## Contributing
[//]: # (PROBABLY WANT TO EXPAND THIS WITH GUIDANCE ON WHAT TYPE OF CONTRIBUTIONS WILL BE WELCOME AND WHERE IN THE CODEBASE)
You can contribute to the enhancement of OpendTect either by:

- providing bug fixes or enhancements to the OpendTect source code following the usual GitHub Fork-Pull Request process.
- or independently by developing and releasing open source plugins from your own GitHub or equivalent repository. See the [wmplugins repository](https://github.com/waynegm/OpendTect-Plugins) as an example of this approach.

An overview of the design principles and preferred coding style/practices employed by **dGB** in the development of OpendTect are described in [dGB's coding guide](https://doc.opendtect.org/2026/doc/Programmer/Default.htm#6_principles_and_best_practices_in_opendtect_coding.htm)

## Contacts and More Information

- [dGB Earth Sciences](https://dgbes.com/about#contact)
- [OpendTect Documentation](https://dgbes.com/library#documentation)
- [OpendTect Programmer's Manual](https://doc.opendtect.org/2026/doc/Programmer/Default.htm)
- [OpendTect developers Google Group](https://dgbes.com/index.php/support/faq-developers-google-group)

[header-img]: doc/pics/opendtect_header.png
[example-img]: doc/pics/supported-functionality.jpg
