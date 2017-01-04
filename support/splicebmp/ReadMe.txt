========================================================================
    CONSOLE APPLICATION : splicebmp Project Overview
========================================================================

////////////////////////////////////////////////////////////////////////
// Version:
////////////////////////////////////////////////////////////////////////

    This is release 002 of splicebmp.  When run, splicebmp will report
    its version number.  This may not be visable due to the console
    window immediately closing.  In this case, run it in a command
    line.

////////////////////////////////////////////////////////////////////////
// Usage:
////////////////////////////////////////////////////////////////////////

    splicebmp

    No command line parameters are required.
    
    "powder.gba" is assumed to be the name of the original POWDER ROM.
    "dungeon16.bmp" is assumed to be the name of the bitmap to replace
    the POWDER bitmap.
    "powder_c.gba" is the name of the generated spliced ROM.

    If an error occurs, an appropriate message is output.  However, if
    it is run from the Windows desktop the console application may
    close before the message can be read.  Run it from the command
    line to learn what the errors may be.

////////////////////////////////////////////////////////////////////////
// Description:
////////////////////////////////////////////////////////////////////////

    splicebmp is a tool for splicing custom bitmaps into the POWDER
    executable.  As POWDER is a Gameboy Advance ROM, it has no way of
    reading external files.  Thus, the ROM itself has to be hacked to
    replace the bitmaps.  This is somewhat complicated by the need to
    ensure the palettes match, and the fact the bitmap is diced into
    tiles inside the ROM.

////////////////////////////////////////////////////////////////////////
// Where to get:
////////////////////////////////////////////////////////////////////////

    splicebmp can be retrieved at:
    
	http://www.zincland.com/powder

    This is also the place to look for the most recent art pack.  With
    a new release, you may need to get a new art pack to get the new
    items (or new resolution bitmap).  Occasionaly, you may need to
    get a new splicebmp pack.

////////////////////////////////////////////////////////////////////////
// Important Points:
////////////////////////////////////////////////////////////////////////

    * splicebmp.exe requires the MSVC 7 runtime dlls.  These are
      provided with the Windows version of POWDER.  If you get an
      error about missing .dlls, try copying the .dlls from the
      Windows POWDER installation into the directory where splicebmp
      is run from.

    * dungeon16.bmp must be a 256 colour Windows bitmap

      Other types of bitmaps and other file formats will not be
      accepted.

    * Colour index 0 is transparent

      The default palette colour for this is black.  However, the
      palette colour is ignored.  The background of all tiles should
      be this index, however.

    * The GameboyAdvance has a 15 bit colour palette

      While each of RGB can have one of 256 different values in most
      modern systems, the GameboyAdvance only has 15 bit colours.  The
      provided colours will be quantized to this range.  This means
      you have only 32 levels of pure red, for example.  (In
      practice you have even less!  See the next point)

    * Bright Green is actually Black.  splicebmp automagically will
      remap bright green, (0, 255, 0), to black, (0, 0, 0), so that
      one can have true black despite index 0 being black.  This
      accomadates programs like gfx2gba which aggressively merge
      palette colours.

    * You may add additional colours.  You may change palette colours.
      Note, however, that the pre-existing palette colours will be
      preserved so that other internal bitmaps continue to work.
      Thus, you cannot create 255 entirely new colours.  It is
      undefined what will happen to the colours that overflow this
      list.

    * The resolution of dungeon16.bmp should not be changed.  Changing
      it will result in a crash and/or a corrupted ROM.

    * The GameboyAdvance is *dark*

      Even with the Afterburner installed, the GBA's screen has a very
      different gamma from your monitor.  What looks garish and high
      contrast on your monitor often comes out muted on the GBA.  The
      GBA SP has significantly better colour.

      To give an example: when you bring up the minimap in POWDER,
      unexplored area is drawn as dark brown.  On the GBA, this is
      black.  On the GBA SP, this is barely recognizeable as dark
      brown.

    * Overriding the overlay icons is currently not supported.  Thus
      there isn't a way to change what your avatar looks like or how
      he is dressed.  
    
////////////////////////////////////////////////////////////////////////
// Questions:
////////////////////////////////////////////////////////////////////////

    If you have any questions about the operation of this program, or
    POWDER in general, please contact:

	jmlait@zincland.com

////////////////////////////////////////////////////////////////////////
// Warranty:
////////////////////////////////////////////////////////////////////////

    There is none.  Use at your own risk.
