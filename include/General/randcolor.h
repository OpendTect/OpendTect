#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		November 2006
________________________________________________________________________

-*/

#include "color.h"
#include "statrand.h"


inline OD::Color getRandomColor( bool withtransp=false )
{
    return OD::Color( (unsigned char) Stats::randGen().getIndex(255),
	          (unsigned char) Stats::randGen().getIndex(255),
		  (unsigned char) Stats::randGen().getIndex(255),
		  (unsigned char)
		     (withtransp ? Stats::randGen().getIndex(255) : 0) );
}


inline OD::Color getRandStdDrawColor()
{
    mDefineStaticLocalObject( int, curidx, = -1 );
    if ( curidx == -1 )
	curidx = Stats::randGen().getIndex( OD::Color::nrStdDrawColors() );
    else
    {
	curidx++;
	if ( curidx == OD::Color::nrStdDrawColors() )
	    curidx = 0;
    }

    return OD::Color::stdDrawColor( curidx );
}


inline OD::Color getRandomFillColor()
{
    return OD::Color( (unsigned char) (155 + Stats::randGen().getIndex(100)),
	    	      (unsigned char) (155 + Stats::randGen().getIndex(100)),
		      (unsigned char) (155 + Stats::randGen().getIndex(100)) );
}
