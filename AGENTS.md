# Agent Project Configuration

## Project Overview
* **Name:** OpendTect, main branch (v8.1)
* **Description:** Open source system for interpretation and processing of 2D, 3D and 4D seismic data
* **Tech Stack:** C++17, Qt6.9, OpenSceneGraph

## Build and Environment Rules
* If a `.pixi` folder exists in the project root, reference `.agents/using_pixi.md` for instructions on using `pixi`:
  * To configure and build OpendTect.
  * To test and use the ODBind Python binding.
  
## Code Layout
* `src/<Mod>/CMakeLists.txt` defines one module per dir.
  * Module headers live in `include/<Mod>/`.
  * Modules are added with `OD_INIT_MODULE()`.
  * Inter-module deps go in `OD_MODULE_DEPS`.
  * The `OD_MODULES` list order in the root `CMakeLists.txt` is dependency order (Basic/Algo/General → ... → uiODMain).
* `plugins/<Name>/` are loadable plugins (`OD_IS_PLUGIN yes`), e.g. ODBind (Python bindings), uiSEGY, CRS. 
* `spec/<Name>/`, e.g. ODSeis, ODvisBase, ODuiBase, are standalone programs.
* Main entrypoint: `src/uiODMain/od_main.cc`; console variant `od_main_console`.
* **Sources are enumerated, never globbed.** Any new `.cc`/`.h` must be added to the owning module's `CMakeLists.txt` (`OD_MODULE_SOURCES`, `OD_MODULE_INCFILES`) or it silently won't be built.

## Style
* Use OD::String methods instead of `strcmp(`.
* Use Math::Sqrt instead of `sqrt(`.
* Use Math::Atan2() instead of `atan2(`.
* Use `mAllocLargeVarLenArr` instead of `ArrPtrMan<Type> var = new`.
* Source files must have Unix line endings and end with a newline (LineEndTest/FileEndTest). A PHP tab-lint test runs if `php` is installed.
* Prefer `OD::String`/`BufferString` over `std::string`.
* See dGB coding guide: https://doc.opendtect.org/7.0.0/doc/Programmer/Default.htm

