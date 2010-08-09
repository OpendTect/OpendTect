/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : August 2010
-*/

static const char* rcsID = "$Id: odimage.cc,v 1.1 2010-08-09 14:51:13 cvskris Exp $";

#include "odimage.h"

namespace OD
{


bool RGBImage::hasAlpha() const
{
    const char nc = nrComponents();
    return nc==2||nc==4;
}


void RGBImage::fill( unsigned char* res ) const
{
    const int xsize = getSize( true );
    const int ysize = getSize( false );
    const bool hasalpha = hasAlpha();

    for ( int idx=0; idx<xsize; idx++ )
    {
	for ( int idy=0; idy<ysize; idy++ )
	{
	    const Color col = get( idx, idy );
	    (*res++) = col.r();
	    (*res++) = col.g();
	    (*res++) = col.b();
	    if ( hasalpha )
		(*res++) = col.t();
	}
    }
}

};
