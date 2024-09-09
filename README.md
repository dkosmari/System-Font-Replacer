# System Font Replacer

This is an Aroma plugin to safely temporarily replace the Wii U's system font.

The perfect solution for completing your custom Wii U Menu theme, without having to modify
anything in your NAND.

**No system file is modified by this plugin. All changes are done in RAM only.**


## Usage

The plugin can load TrueType fonts from anywhere in the SD card, but it's a good idea to have
them in `SD:/wiiu/fonts/`; that's the default location the plugin looks for fonts.

1. Open up the WUPS menu (**L + ↓ + SELECT**) and enter the **System Font
   Replacer** menu.

2. Ensure the "Enabled" option is set to "*yes*", otherwise the plugin won't do anything.

3. Select the "Std Font" option, and press **A** to start editing it:

   - Press **→** to enter a folder;
   - Press **←** to leave a folder;
   - Press **↑** or **↓** to change the file;
   - Press **X** to reset back to the default value (`SD:/wiiu/fonts/`);
   - Press **A** to confirm, or **B** to cancel the change.

   Use it to navigate to the desired `.ttf` font you want to use, then press **A** to
   confirm it.

   Note that, if you select a directory (like the default, `SD:/wiiu/fonts`) the font will
   not be replaced, and the original system font is used instead.

4. Exit the plugin menu.

5. Reboot your Wii U.


## Freezes/Crashes and text glitches

Not every game/app has good a font rendering implementation. Some cannot handle more
"advanced" TTF fonts, and will either render it incorrectly, or outright crash.

Please do not report bugs about crashes that only occur with some fonts; that's a
limitation inside the game/app, and there's nothing this plugin can do to fix it.


## Large font files

Aroma plugins have very limited amount of memory to use. If the font file is too large, it
might use too much memory, and other plugins might stop working. Fonts up to 2.5 MiB in
size seem to work fine; I have not tested larger fonts.


## Missing symbols

Some Wii U software make use of the system font's Private Use Area (PUA) block (from
`U+E000` to `U+E099`), to show symbols for gamepad buttons, sticks, etc. If the
replacement font doesn't have the correct symbols in that block, the text might be
rendered incorrectly. To get correct text rendering you have to edit your custom font, to
add the correct symbols to that block.

In this repository you can find a Python script, [`merge-fonts.py`](merge-fonts.py), that
uses the [FontForge](https://fontforge.org/) program to do that automatically, copying the
PUA block (and any other missing symbol) from the original system font into your custom
font.

1. Start by getting a copy of the original system fonts on your Wii U, from
   `/storage_mlc/sys/title/0005001b/10042400/content` using
   [ftpiiu](https://github.com/wiiu-env/ftpiiu_plugin).

2. Assuming the font you want to use on your Wii U is called `myfont.ttf`, and you want to
   use it as the "Std Font" font (replacing `CafeStd.ttf`), you can execute the script
   like this:

       ./merge-fonts.py  myfont.ttf  path/to/original/CafeStd.ttf  myfont+CafeStd.ttf

   or

       fontforge  merge-fonts.py  myfont.ttf  path/to/original/CafeStd.ttf  myfont+CafeStd.ttf

   - The first font has priority, all symbols from it will be copied to the output.

   - The PUA block (if it exists) is removed from the output.

   - Every missing symbol (including the PUA block) in the output will be filled in with
     symbols from the second font.

   - The last argument is optional, it's the name of the output file. It's a good idea to
     use the names of both source fonts, so you remember what's in the output.

   There's also the [`copy-pua.py`](copy-pua.py) script, that only copies in the PUA
   symbols from the second font, and nothing else. The result is a much smaller output
   font, but it might still be missing some symbols; it depends on how "complete" your
   source font is.

3. Copy the output font to your SD card, into `SD:/wiiu/fonts/`, then configure the plugin
   to use it.


## "Use custom fonts only for Wii U Menu"

If you disable this option, the font will show up in any other software that requests the
system font. Some of them may crash, or might not render properly. To avoid surprises,
leave this option enabled ("*yes*"), so only the Wii U Menu is affected by the custom
font. If you trust your custom font to not crash the on-screen keyboard, the System
Settings, the Friends List, etc, you can disable this option ("*no*").


## Helper App

If you get a custom font in the form of a `.bps` patch, to be applied to one of the system
fonts, you can use the [System Font Replacer Helper](helper-app) to easily convert them.

Simply put your `.bps` patches in `SD:/wiiu/fonts/`, and run the Helper app. It will then
automatically convert all `.bps` patches in that folder into `.ttf` fonts.
