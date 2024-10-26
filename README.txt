# How to build
In the project root run:
```cmake -B build/```
This command will configure cmake in a new build subdirectory, cloning TGA and its submodules.
To build the project run:
```cmake --build build/```
This should build all dependencies, including the shaders in the `shaders/` subdirectory.
The compiled binary then resides in at `./build/gpupro`, its shaders are stored in `./build/shaders`.
