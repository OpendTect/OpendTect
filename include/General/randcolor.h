#ifndef randcolor_h
#define randcolor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		November 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "color.h"
#include "statrand.h"

inline Color getRandomColor( bool withtransp=false )
{
    Stats::randGen().init();
    return Color( (unsigned char) Stats::randGen().getIndex(255),
	          (unsigned char) Stats::randGen().getIndex(255),
		  (unsigned char) Stats::randGen().getIndex(255),
		  (unsigned char)
		     (withtransp ? Stats::randGen().getIndex(255) : 0) );
}


inline Color getRandStdDrawColor()
{
    static int curidx = -1;
    if ( curidx == -1 )
    {
	Stats::randGen().init();
	curidx = Stats::randGen().getIndex( Color::nrStdDrawColors() );
    }
    else
    {
	curidx++;
	if ( curidx == Color::nrStdDrawColors() )
	    curidx = 0;
    }

    return Color::stdDrawColor( curidx );
}

#endif
