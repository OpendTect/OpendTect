/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril 
 * DATE     : 2-12-2005
-*/

static const char* rcsID = "$Id: import_vel.cc,v 1.3 2010/10/14 09:58:06 cvsbert Exp $";

#include "prog.h"
#include "batchprog.h"

#include "arrayndimpl.h"
#include "interpol2d.h"
#include "iopar.h"
#include "position.h"
#include "progressmeter.h"
#include "ranges.h"
#include "scaler.h"
#include "separstr.h"
#include "sets.h"
#include "strmprov.h"

#include <math.h>


#define mErrRet(s) { strm << s << std::endl; return false; }


static Coord getCoord( const TypeSet<int>& cdps, const TypeSet<Coord>& crds,
		       int cdp )
{
    int startidx = -1;
    for ( int idx=0; idx<cdps.size(); idx++ )
    {
	if ( cdp >= cdps[idx] )
	{
	    startidx = idx;
	    break;
	}
    }

    if ( startidx==cdps.size()-1 )
	startidx--;

    LinScaler xscaler( cdps[startidx], crds[startidx].x, cdps[startidx+1], crds[startidx+1].x );
    LinScaler yscaler( cdps[startidx], crds[startidx].y, cdps[startidx+1], crds[startidx+1].y );

    return Coord( xscaler.scale(cdp), yscaler.scale(cdp) );
}



bool BatchProgram::go( std::ostream& strm )
{ 
    const char* filenm = pars().find( "Filename" );

    FileMultiString fms = pars().find( "File CDP Range" );
    const StepInterval<int> cdprg(toInt(fms[0]),toInt(fms[1]),toInt(fms[2]));
    fms = pars().find( "File Time Range" );
    const StepInterval<int> timerg(toInt(fms[0]),toInt(fms[1]),toInt(fms[2]));

    Array2DImpl<float> velarr( cdprg.nrSteps()+1, timerg.nrSteps()+1 );

    int line, cdp;
    float sp, x, y, time, vel;
    StreamData sdi = StreamProvider(filenm).makeIStream();
    if ( !sdi.istrm ) mErrRet( "Cannot open input file" )

    strm << "\nReading input velocity file: " << filenm << std::endl;
    TypeSet<int> cdps; TypeSet<Coord> xyvals;
    while ( sdi.istrm->good() )
    {
	*sdi.istrm >> line >> cdp >> sp >> x >> y >> time >> vel;

	const int cdpidx = cdprg.getIndex( cdp );
	const int tidx = timerg.nearestIndex( time );
	velarr.set( cdpidx, tidx, vel );

	if ( cdps.indexOf(cdp) < 0 )
	{
	    cdps += cdp;
	    xyvals += Coord( x, y );
	}
    }


    TextStreamProgressMeter pm( strm );
    const char* outfilenm = pars().find( "Output Filename" );
    StreamData sdo = StreamProvider(outfilenm).makeOStream();
    if ( !sdo.ostrm ) mErrRet( "Cannot create output file" );
    fms = pars().find( "Ouput CDP Range" );
    const StepInterval<int> cdprgo(toInt(fms[0]),toInt(fms[1]),toInt(fms[2]));

    fms = pars().find( "Ouput Time Range" );
    const StepInterval<int> timergo(toInt(fms[0]),toInt(fms[1]),toInt(fms[2]));

    strm << "\nTotal number of traces to process: " << cdprgo.nrSteps()+1
	 << std::endl << std::endl;
    for ( int cdp=cdprgo.start; cdp<=cdprgo.stop; cdp+=cdprgo.step )
    {
	const float cdpidx = cdprg.getfIndex( cdp );
	int cdp0 = (int)cdpidx; int cdp1 = cdp0 + 1;
	if ( cdp1 >= velarr.info().getSize(0) )
	{ cdp0--; cdp1--; }

	const Coord crd = getCoord( cdps, xyvals, cdp );
	*sdo.ostrm << cdp << '\t' << crd.x << '\t' << crd.y; 
	for ( int tm=timergo.start; tm<=timergo.stop; tm+=timergo.step )
	{
	    const float tidx = timerg.getfIndex( tm );
	    int t0 = (int)tidx; int t1 = t0 + 1;
	    if ( t1 >= velarr.info().getSize(1) )
	    { t0--; t1--; }

	    const float vel00 = velarr.get( cdp0, t0 );
	    const float vel01 = velarr.get( cdp1, t0 );
	    const float vel10 = velarr.get( cdp0, t1 );
	    const float vel11 = velarr.get( cdp1, t1 );
	    const float vel = Interpolate::linearReg2D( vel00, vel10,
		    					vel01, vel11,
							cdpidx-cdp0, tidx-t0 );
	    *sdo.ostrm << '\t' << vel;
	}

	*sdo.ostrm << std::endl;
	++pm;
    }

    pm.setFinished();


    strm << "\nUse this info to import from 'simple file':" << std::endl;
    strm << "Traces start with a position: Yes" << std::endl;
    strm << "Trace number included: Yes" << std::endl;
    strm << "File start contains sampling info: No" << std::endl;
    strm << "Sampling info: " << timergo.start << '\t'
			      << timergo.step << '\t'
			      << timergo.nrSteps()+1 << std::endl;

    return true;
}
