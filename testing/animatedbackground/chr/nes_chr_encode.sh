#!/bin/sh
rm -f output.chr
python3 nes_chr_encode.py --color0 5a8cff --color1 efefef --color2 845e21 --color3 525252 chr.png output.chr
