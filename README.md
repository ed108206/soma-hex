### Fast CSV Viewer (handles millions of rows)

![CMake Build](https://github.com/ed108206/soma-hex/actions/workflows/cmake-multi-platform.yml/badge.svg)

This project is a Win32 desktop application that includes a fast CSV viewer with no row limits, a custom splitter control, and a tree view.
It's written in C++ using plain Win32 APIs (no MFC), with a focus on speed and simplicity.

## Features

- **Custom splitter control**  
  A lightweight splitter implemented from scratch allows you to resize panels within the main window.

- **Fast CSV loading**  
  Large CSV files are memory-mapped and parsed quickly.  
  The app can handle millions of rows without freezing the UI.

- **Tree view with drag & drop**  
  A side panel shows a tree view with categories.  
  Items can be dragged and dropped to reorganize them.

- **Multiple views**  
  - **Data view (CEView):** shows CSV rows in a virtual list view.  
  - **Extra tree (CExtraTree):** manages hierarchical items.  

## UI Layout

Here's a simple ASCII diagram of the main window layout:


```text
+---------------------------------------------------+
|                  Main Window                      |
|                                                   |
|  +-----------+-------------------------------+    |
|  | ExtraTree |          CEView               |    |
|  | (Tree)    |   (CSV Data / ListView)       |    |
|  +-----------+-------------------------------+    |
|  |              (Logs / Bottom)              |    |
+---------------------------------------------------+
```

- Left panel: **ExtraTree** (tree view with drag & drop).  
- Right panel: **EView** (CSV data)   
- Bottom panel: **Logs** (not implemented)
- Panels are resizable thanks to the custom **SplitterE**.


## Build

You can build the project in two ways:

- **Makefile** (GNU toolchain)  
- **CMakeLists.txt** (cross-platform build system)

> **Note:** The project has been tested with GNU compilers (MinGW).  
> It may also compile with Microsoft Visual C++ (MSVC), but has not been tested.

### Example commands

```bash
Using Makefile:
make

Using CMake:
mkdir build
cd build
cmake ..
make
```

## CSV Generator (generator.cpp)

There's also a small helper program included: **generator.cpp**.  
Its only purpose is to create large CSV files for testing the viewer.<br>

Compile:<br>
g++ -O2 -std=c++20 ext/generator.cpp -o generator

## Usage

```bash
./generator 1000000 10 mydata.csv
```
- Running this command generates a file named mydata.csv containing 1,000,000 rows and 10 columns, with an approximate size of 430 MB.

This makes it easy to stress-test the app with big datasets and see how the splitter and views behave under load.

