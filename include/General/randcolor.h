#ifndef randcolor_h
#define randcolor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		November 2006
 RCS:		$Id: randcolor.h,v 1.1 2006-11-06 16:13:46 cvsnanne Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "statrand.h"

static const int ncols = 10;
static Color drawcols[] = {
    Color( 220,  50,  50 ), // red
    Color(  50,  50, 220 ), // blue
    Color(  50, 200,  50 ), // green
    Color(  50, 200, 200 ), // cyan
    Color( 255, 210,   0 ), // gold
    Color( 220,   0, 220 ), // magenta
    Color( 140, 130,  80 ), // khaki
    Color( 100, 160,   0 ), // orange
    Color( 140,  35,  80 ), // dark violet red
    Color( 204, 133,  61 ), // peru
};


static const Color& getRandomColor()
{
    static int lastidx = -1;
    Stats::RandGen::init();
    int newidx = Stats::RandGen::getIndex( ncols );
    if ( newidx == lastidx )
    {
	newidx++;
	if ( newidx == ncols ) newidx = 0;
    }
    lastidx = newidx;
    return drawcols[lastidx];
}

#endif
