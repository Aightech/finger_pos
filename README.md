# finger_pos

This module is a C++ program that reads video frames from multiple cameras and detects blobs (groups of pixels with similar color or intensity) using OpenCV library functions. It uses trackbars to set thresholds for the HSV values and displays the frames with the detected blobs. It also records the video frames from the cameras and saves them as images. The program uses the LSL (Lab Streaming Layer) C library to stream the detected blob data.

### Submodules dependencies
#### libraries
- [built_lsl](lib/built_lsl/README.md)
- [camera](lib/camera/README.md) 
#### Tools
- [built_lsl](tool_lib/built_lsl/README.md)


# Building source code

To build the project run:
```bash
cd finger_pos
mkdir build && cd build
cmake .. && make
```

# Demonstration app

When the project have been built, you can run:
```bash
./finger_pos -h
```
to get the demonstration app usage.

# Example
Open the ![main.cpp](cpp:src/main.cpp) file to get an example how to use the lib.

# Contributing
See [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.