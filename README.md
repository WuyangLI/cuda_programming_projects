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
This folder holds all example data in any format. The image is from wikipedia (link)[https://upload.wikimedia.org/wikipedia/commons/e/e2/Emma%2C_Lady_Hamilton_by_George_Romney.jpg]
The original file is a 4 channel jpg image. It's converted to a 3-channel png image `Emma_Lady_Hamilton_by_George_Romney_rbg.png` and gray-scale pgm file.

```src/```
The source code should be placed here in a hierarchical fashion, as appropriate.

```README.md```
This file should hold the description of the project so that anyone cloning or deciding if they want to clone this repository can understand its purpose to help with their decision.


```Makefile```
```
make clean build
```

```run.sh```
```
./run.sh
```

