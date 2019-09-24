# Average codequester

Learning app written in Objective-C++ with use of OpenCV library. Calculates an average face of poeple working at codequest.

## Prerequisites

Project requires installation of [git lfs](https://git-lfs.github.com), because it contains large files (e.g. opencv.framework compiled with additional contributions which is more than 500MB).
You can install `git lfs` with Homebrew by running:
```
brew install git-lfs
```

## Requirements

- Xcode version minimum 10.3
- iPhone with iOS version minimum 11.0

## Resources

- [OpenCV](https://github.com/opencv/opencv)
- [OpenCV extra modules](https://github.com/opencv/opencv_contrib)
- [Course Computer Vision for Faces](https://courses.learnopencv.com/p/computer-vision-for-faces)
- [Average face tutorial](https://www.learnopencv.com/average-face-opencv-c-python-tutorial)
- [About Delaunay triangulation](https://www.learnopencv.com/delaunay-triangulation-and-voronoi-diagram-using-opencv-c-python/)
- [Warp affine](https://docs.opencv.org/3.1.0/d4/d61/tutorial_warp_affine.html)
- [Local binary patterns](https://en.wikipedia.org/wiki/Local_binary_pattern)

## Architecture

App is simple and is written in MVC architecture with the use of a storyboard.

## Disclaimer

File `faceBlendCommon.hpp` is a part of [Computer Vision for Faces](https://courses.learnopencv.com/p/computer-vision-for-faces) by Satya Mallick, modified only for cleaner code purpose by Hanna Dutkiewicz.
The permission to use methods from this file were granted to Hanna Dutkiewicz for educational and learning purpose during meetups etc.
