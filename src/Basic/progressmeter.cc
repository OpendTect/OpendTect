/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id";

#include "progressmeter.h"
#include "timefun.h"
#include <iostream.h>
#include <values.h>

static const char dispchars[] = ".:=|*#>}ABCDEFGHIJKLMNOPQRSTUVWXYZ";


ProgressMeter::ProgressMeter( ostream& out, unsigned long dist_,
			      unsigned short rowlen_ )
	: strm(&out)
	, rowlen(rowlen_)
	, dist(dist_)
	, auxnr(ULONG_MAX)
{
    reset();
    dist = dist_;
}


void ProgressMeter::reset()
{
    progress = 0;
    zeropoint = 0;
    oldtime = Time_getMilliSeconds();
    inited = false;
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

    unsigned long relprogress = progress - zeropoint;
    if ( !(relprogress % dist) )
    {
	*strm << (relprogress%(10*dist) ? dispchars[idist]:dispchars[idist+1]);
	strm->flush();
    }

    if ( relprogress == dist*rowlen ) 
    {
	zeropoint = progress;

	*strm << ' ';
	if ( auxnr != ULONG_MAX && abs(progress-auxnr) > 1 )
	    *strm << auxnr << '/';
  	*strm << progress;

        int newtime = Time_getMilliSeconds();
	float tdiff = newtime - oldtime;
	int permsec = (int)((1.e6 * dist * rowlen) / tdiff + .5);
        *strm<< " (" << permsec * .001 << "/s)" << endl;
	
	if ( tdiff >= 0 && tdiff < 5000 )
	    { idist++; dist *= 10; }
	else if ( tdiff > 60000 )
	{
	    idist--;
	    if ( idist < 0 ) idist = 0;
	    else	     dist /= 10;
	}

        oldtime = newtime; 
    }
    return progress;
}
