# System Font Replacer

This is an Aroma plugin to safely temporarily replace the Wii U's system font.


## Usage

The plugin looks for replacement fonts in `sd:/wiiu/fonts/` with specific names:

  - `CafeCn.ttf`: replaces the Simplified Chinese font.

  - `CafeKr.ttf`: replaces the Korean font.

  - `CafeStd.ttf`: replaces the generic font (used in Western and Japanese texts).

  - `CafeTw.ttf`: replaces the Traditional Chinese font.

If any of these files is found, it will be loaded in place of the equivalent system font;
otherwise, the original system font will be loaded.

**No system file is modified by this plugin. All changes are done in RAM only.**


## Missing symbols

Some Wii U software make use of the system font's Private Use Area (PUA) block (from `U+E000`
to `U+E099`), to show symbols of gamepad buttons, sticks, etc. If the replacement font
doesn't have the correct symbols in that block, text might be rendered incorrectly.

To get correct text rendering you have to edit the new font, to add the correct symbols to
that area. In this repository you can find a Python script, that uses
[FontForge](https://fontforge.org/) to do that automatically.

1. Start by getting a copy of the original system fonts on your Wii U, from
   `/storage_mlc/sys/title/0005001b/10042400/content` using
   [ftpiiu](https://github.com/wiiu-env/ftpiiu_plugin).

2. Assuming the font you want to use on your Wii U is called `myfont.ttf`, and you want to
   replace `CafeStd.ttf`, you can execute the script like this:

       ./merge-fonts.py  myfont.ttf  path/to/original/CafeStd.ttf  CafeStd.ttf

   or

       fontforge  merge-fonts.py myfont.ttf  path/to/original/CafeStd.ttf  CafeStd.ttf

   Note: the order is important, the first font has priority, the second font is only used
   to fill the missing symbols (except for the PUA block, it is always taken from the
   second font.) The third font is the name of the output file.

3. Copy the generated `CafeStd.ttf` to your SD card, into `sd:/wiiu/fonts/`.


**Be aware of copyright restrictions.** It's better to not distribute merged fonts, unless
you're sure the font's license permits modified copies to be distributed.
