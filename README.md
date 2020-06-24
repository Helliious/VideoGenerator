# VideoGenerator
Video generator in YUV format.

This File contains a program that creates a video which moves a square
around the screen with threads. The square bounces from the screen walls
and every time it bounces, it changes its color and its direction.


Ubuntu 18.04:
Compiling and executing the program:

* Compile command: gcc movingsquare_th.c -lpthread
* Executing command: ./a.out

The video output will be in the file: "file"

To run the video your player must be able to read YUV4MPEG 2 format.
A video player capable of this for example is mplayer.

How to install mplayer on Ubuntu 18.04:
https://zoomadmin.com/HowToInstall/UbuntuPackage/mplayer

After installation run the video with the following command in terminal:
mplayer file
