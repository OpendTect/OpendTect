/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id";

#include "progressmeter.h"
#include "timefun.h"
#include <iostream>
#include <limits.h>
#include <stdlib.h>

static const char dispchars[] = ".:=|*#>}ABCDEFGHIJKLMNOPQRSTUVWXYZ";


ProgressMeter::ProgressMeter( ostream& out, unsigned long dist_,
			      unsigned short rowlen_,
			      bool df )
	: strm(out)
	, rowlen(rowlen_)
	, auxnr(ULONG_MAX)
	, destrfin(df)
{
    reset();
    dist = dist_;
}


ProgressMeter::~ProgressMeter()
{
    if ( destrfin ) finish();
}


void ProgressMeter::finish()
{
    if ( !finished )
	{ annotate(false); finished = true; }
}


void ProgressMeter::reset()
{
    progress = 0;
    oldtime = Time_getMilliSeconds();
    inited = false;
    finished = false;
    dist = 1; idist = 0;
    lastannotatedprogress = 0;
    nrdotsonline = 0;
}


void ProgressMeter::resetDist()
{
    dist = 1;
    idist = 0;
}


unsigned long ProgressMeter::operator++()
{
    return update( ULONG_MAX );
}


unsigned long ProgressMeter::update( unsigned long a )
{
    auxnr = a;
    progress++;
    if ( !inited )
    {
        oldtime = Time_getMilliSeconds();
	inited = true;
    }

    unsigned long relprogress = progress - lastannotatedprogress;
    if ( !(relprogress % dist) )
    {
	strm << (relprogress%(10*dist) ? dispchars[idist]:dispchars[idist+1]);
	strm.flush();
	nrdotsonline++;
    }

    if ( nrdotsonline == rowlen )
	annotate(true);

    return progress;
}
	

void ProgressMeter::annotate( bool withrate )
{
    // Show numbers
    strm << ' ';
    if ( auxnr != ULONG_MAX )
	strm << auxnr << '/';
    strm << progress;

    // Show rate
    int newtime = Time_getMilliSeconds();
    int tdiff = newtime - oldtime;
    if ( withrate && tdiff > 0 )
    {
	int nrdone = progress - lastannotatedprogress;
	int permsec = (int)(1.e6 * nrdone / tdiff + .5);
	strm << " (" << permsec * .001 << "/s)";
    }
    strm << endl;

    lastannotatedprogress = progress;
    oldtime = newtime; 
    nrdotsonline = 0;
    
    // Adjust display speed if necessary
    if ( tdiff > -1 && tdiff < 5000 )
    {
	idist++;
	dist *= 10;
    }
    else if ( tdiff > 60000 )
    {
	if ( idist )
	{
	    idist--;
	    dist /= 10;
	}
    }
}
