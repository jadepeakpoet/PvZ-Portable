# Font rasterizer

`stb_truetype.h` is unmodified stb_truetype 1.26 from the fixed upstream commit
`f0569113c93ad095470c54bf34a17b36646bbbb5`:
https://github.com/nothings/stb/blob/f0569113c93ad095470c54bf34a17b36646bbbb5/stb_truetype.h

The upstream dual MIT/public-domain license is included at the end of the header.
No operating-system font files are redistributed. TrainerFont looks for installed
fonts, or an explicitly provided trusted `trainer-font.ttf`, `.otf`, or `.ttc` in
the resource directory. stb_truetype is not designed for hostile font files.

The font cache is per font instance, bounded to 2048 glyphs, and uses the game's
MemoryImage/Graphics API for both software and GL rendering. Font instances must
be destroyed while their SexyAppBase is still alive, like other game images.
