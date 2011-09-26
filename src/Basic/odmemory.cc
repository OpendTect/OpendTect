/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odmemory.cc,v 1.3 2011-09-26 15:11:25 cvskris Exp $";

#include "odmemory.h"

#ifdef lux
#include "malloc.h" 
#endif

#include "iopar.h" 
#include "string2.h" 

bool OD::canDumpMemInfo()
{
#ifdef lux
    return true;
#else
    return false;
#endif
}

void OD::dumpMemInfo( IOPar& res )
{
#ifdef lux
    struct mallinfo info = mallinfo();

    res.set( "Total heap size", getBytesString( info.arena ) );
    res.set( "Small block size", getBytesString( info.usmblks ) );
    res.set( "Free small blocks", getBytesString( info.fsmblks ) );
    res.set( "Ordinary block allocation",
	     getBytesString( info.uordblks+info.ordblks*8) );
    res.set( "Free ordinary Ordinary block", getBytesString( info.fordblks ) );
    res.set( "MMapped size", getBytesString( info.hblkhd ) );
    res.set( "Total used memory",
	    getBytesString(info.uordblks+info.ordblks*8+info.usmblks+
			   info.hblkhd ));
#endif
}
