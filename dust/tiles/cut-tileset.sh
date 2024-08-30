#!/bin/bash

magick $1 -trim -crop $2x$3-3-3@\!  +repage +adjoin tile_%02d.png
