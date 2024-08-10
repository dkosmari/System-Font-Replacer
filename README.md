# System Font Replacer

This is an Aroma plugin to safely temporarily replace the Wii U's system font.


## Usage

The plugin looks for replacement fonts in `sd:/wiiu/fonts/` with specific names:

  - `CafeCn.ttf`: replaces the Simplified Chinese font.

  - `CafeKr.ttf`: replaces the Korean font.

  - `CafeStd.ttf`: replaces the generic font (used in Western and Japanese texts).

  - `CafeTw.ttf`: replaces the Traditional Chinese font.

If any of these files is found, it's loaded instead of the equivalent system font;
otherwise, the system font is loaded.

**No system file is modified by this plugin. All changes are done in RAM only.**


## Missing symbols

Some Wii U software make use of the system font's Private Use Area (PUA) block (from `U+E000`
to `U+E099`), to show symbols of gamepad buttons, sticks, etc. If the replacement font
doesn't have the correct symbols in that block, text might be rendered incorrectly.

To get correct text rendering you have to edit the new font, to add the correct symbols to
that area. In this repository you can find a Python script, that uses `fontforge` to do
that automatically.

Example usage:

    ./merge-fonts.py myfont.ttf path/to/original/CafeStd.ttf CafeStd.ttf

or

    fontforge merge-fonts.py myfont.ttf path/to/original/CafeStd.ttf CafeStd.ttf

The resulting `CafeStd.ttf` is now ready to be placed on the SD card, and it will contain
all the PUA symbols the Wii U software expects.

**Be mindful of copyright restrictions.** It's better to not distribute merged fonts
unless you're sure you have the rights to do so.
