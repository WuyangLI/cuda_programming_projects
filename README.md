# Apply Bilateral Gauss Filter with NPP
apply Bilateral Gauss Filter to an RGB image of a classic oil painting using NPP library

## Project Description

apply Bilateral Gauss Filter to an RGB image of a classic oil painting using NPP library

<div style="display: flex;">
    <img src="./data/Emma_Lady_Hamilton_by_George_Romney_rbg.png" alt="Image 1" style="float: center; width: 45%; margin-right: 5%;" />
    <img src="./data/bilateral_gauss_filtered_output.png" alt="Image 2" style="float: center; width: 45%; margin-right: 5%;" />
</div>

## Code Organization

```data/```
This folder holds all example data in any format. The image is from wikipedia [link](https://upload.wikimedia.org/wikipedia/commons/e/e2/Emma%2C_Lady_Hamilton_by_George_Romney.jpg)

The original file is a 4 channel jpg image. 

It's converted to a 3-channel png format image `Emma_Lady_Hamilton_by_George_Romney_rbg.png` and gray-scale pgm format file `data/Emma_Lady_Hamilton_by_George_Romney.pgm`.

```src/```
The source contains two files: `main.cpp` and `ImageIO.h`.

`ImageIO.h` contains methods `loadImage` and `saveImage` that reads a 3 channel RGB image from a file and save 3 channel RGB image to a file respectively.

```README.md```
apply Bilateral Gauss Filter to an RGB image of a classic oil painting using NPP library


```Makefile```
```
make clean build
```

```run.sh```
```
./run.sh
```

