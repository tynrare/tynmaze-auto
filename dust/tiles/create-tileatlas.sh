#!/bin/bash

magick \
    <(magick ceils/*.png +append png:-) \
    <(magick $(ls walls/*.png | sort -V) +append png:-) \
    <(magick floors/*.png +append png:-) \
    -append atlas_maze.png
