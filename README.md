# LogToGraph

## How to Build :

You need to use cMake.
For the 3 Os (Win, Linux, MacOs), the cMake usage is exactly the same, 

1) Choose a build directory. (called here my_build_directory for instance) and
2) Choose a Build Mode : "Release" / "MinSizeRel" / "RelWithDebInfo" / "Debug" (called here BuildMode for instance)
3) Run cMake in console : (the first for generate cmake build files, the second for build the binary)
```cpp
cmake -B my_build_directory -DCMAKE_BUILD_TYPE=BuildMode
cmake --build my_build_directory --config BuildMode
```

Some cMake version need Build mode define via the directive CMAKE_BUILD_TYPE or via --Config when we launch the build. 
This is why i put the boths possibilities

By the way you need before, to make sure, you have needed dependencies.

### On Windows :

You need to have the opengl library installed

### On Linux :

You need many lib : (X11, xrandr, xinerama, xcursor, mesa)

If you are on debian you can run :  

```cpp
sudo apt-get update 
sudo apt-get install libgl1-mesa-dev libx11-dev libxi-dev libxrandr-dev libxinerama-dev libxcursor-dev
```

### On MacOs :

you need many lib : opengl and cocoa framework
