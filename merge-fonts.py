#!/usr/bin/env -S fontforge -quiet

import sys

if len(sys.argv) != 4:
    print("""
Required arguments:  inputfont.ttf  CafeX.ttf  outputfont.ttf

where 'CafeX.ttf' is one of: 'CafeCn.ttf'
                             'CafeKr.ttf'
                             'CafeStd.ttf'
                             'CafeTw.ttf'
""")
    sys.exit(1)


input_name  = sys.argv[1]
cafe_name   = sys.argv[2]
output_name = sys.argv[3]

font = fontforge.open(input_name)

# first, make sure te Private Use Area is clear
font.selection.select(("ranges",), 0xE000, 0xE099)
font.clear()

# merge from the Cafe font into the input
font.mergeFonts(cafe_name)

# export a .ttf with the output name
font.generate(output_name)
