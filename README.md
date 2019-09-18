# Minetest NMPRS

This is an modified version of the mingw-gcc branch on [this repository](https://github.com/twm1994/minetest_nmpr).
 Tested on OMNeT++ 5.4.1, using MinGW GCC toolchain.

## Dependencies and tools

 - [JThread 1.3.3](http://research.edm.uhasselt.be/jori/page/CS/Jthread.html) (Threading)
 - [Irrlicht-1.8.4](http://irrlicht.sourceforge.net/) (3D engine)
 - [CMake](https://cmake.org) (Library building tool)
 - [MinGW](http://www.mingw.org/) / [GNU Make](https://www.gnu.org/software/make/) (Library building tool)
 - [Code::Block](http://www.codeblocks.org/) (Library building tool)

## Change log

These changes are necessary to fix build errors
 - In **socket.h**: add `#define` related to using windows socket utility
 - In **loadstatus.h**: add `s{w,n}printf` definition

## How to build

 - Download JThread and Irrlicht source file
 - Use CMake to build JThread makefile
 - Run the makefile (Use msys on MinGW or GNU Make)
 - Use Code::Block to build Irrlicht win32-gcc version .a and .dll files
 - Make sure WS2_32 and gdi32 libraries are in the **Libraries** option of the OMNet++ project **Property**
 - Add **Irrlicht.dll** and **libjthread.dll** to the output folder that contains the .exe file after building the project

## License

### To be updated
