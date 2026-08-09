# Pixi Build,Testing and Run Instructions
This file provides instructions for building and testing the software using the Pixi package manager and the `pixi.toml` included in the project root.

## C++ Build
From the project root:
* Install dependencies using `pixi install`
* Configure build using `pixi run config`
* Build release build using `pixi run build`
* Build debug build using `pixi run build-dev`
* Execute release build using `pixi run release`
* Execute debug build using `pixi run debug`

## ODBind Python Binding
The ODBind plugin provides a Python binding to parts of OpendTect. The `odbind` environment defined in the project root `pixi.toml` file provides a basic Python environment for testing and evaluation of ODBind.

* Execute a Python script using ODBind and the odbind environment using `pixi run python [path/to/script.py]`
* Start a Python repl in the odbind environment using `pixi run python`
* Start a shell in the odbind environment using `pixi shell -e odbind`

### Unit Tests
* Unit tests for ODBind are defined in the `plugins/ODBind/python/odbind/tests` folder.
* Execute all tests using `pixi run odbind-tests`
* Execute a specific test file using `pixi run odbind-tests python/odbind/tests/[test_file.py]`
* Execute a specific test in a specific test file using `pixi run odbind-tests python/odbind/tests/[test_file.py::test_function_name]`

### Notebook Examples
* Example Python Jupyter text notebooks (requires jupytext module) are included in `plugins/ODBind/Examples/Python`.
* Launch Jupyter Lab using `pixi run jupyter`