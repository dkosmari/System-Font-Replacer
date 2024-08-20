#!/usr/bin/env -S fontforge -quiet

import sys
from pathlib import Path

if len(sys.argv) < 3:
    print("""
Required arguments:  firstfont.ttf  secondfont.ttf  [outputfont.ttf]

If "outputfont.ttf" is not specified, it will be set to "firstfont+secondfont(PUA).ttf".
""")
    sys.exit(1)


first_name  = Path(sys.argv[1])
second_name   = Path(sys.argv[2])
output_name = Path("{}+{}(PUA).ttf".format(first_name.stem, second_name.stem))
if len(sys.argv) == 4:
    output_name = Path(sys.argv[3])

font1 = fontforge.open(str(first_name))

# first, make sure the Private Use Area is clear
font1.selection.select(("ranges",), 0xE000, 0xE099)
font1.clear()

font2 = fontforge.open(str(second_name))

# adjust font1 size so it matches font2
font1.em = font2.em

# copy only the PUA glyphs from font2
font2.selection.select(("ranges",), 0xE000, 0xE099)
font1.copy()
font1.paste()

# export to the output
font1.generate(str(output_name))
