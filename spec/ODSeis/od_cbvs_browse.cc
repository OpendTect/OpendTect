/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
 * RCS      : $Id$
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "seistrc.h"
#include "seiscbvs.h"
#include "cbvsinfo.h"
#include "cbvsreadmgr.h"
#include "conn.h"
#include "iostrm.h"
#include "file.h"
#include "filepath.h"
#include "datachar.h"
#include "strmprov.h"
#include "ptrman.h"
#include <iostream>
#include <math.h>
#include <ctype.h>

#include "prog.h"


static void getInt( int& i )
{
    char buf[80];

    char* ptr;
    do
    {
	ptr = buf;
	std::cin.getline( ptr, 80 );
	while ( *ptr && isspace(*ptr) ) ptr++;
    }
    while ( ! *ptr );

    char* endptr = ptr + 1;
    while ( *endptr && !isspace(*endptr) ) endptr++;
    *endptr = '\0';
    i = toInt( ptr );
}


int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
	std::cerr << "Usage: " << argv[0] << " cbvs_file" << std::endl;
	ExitProgram( 1 );
    }

    FilePath fp( argv[1] );
    if ( !fp.isAbsolute() )
	fp.insert( File::getCurrentPath() );
    const BufferString fname = fp.fullPath();
    if ( !File::exists(fname) )
    {
        std::cerr << fname << " does not exist" << std::endl;
        ExitProgram( 1 );
    }

    std::cerr << "Browsing '" << fname << "'\n" << std::endl;

    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead( new StreamConn(fname,Conn::Read) ) )
	{ std::cerr << tri->errMsg() << std::endl;  ExitProgram( 1 ); }

    std::cerr << "\n";
    const CBVSReadMgr& mgr = *tri->readMgr();
    mgr.dumpInfo( std::cerr, true );
    const CBVSInfo& info = mgr.info();
    const int singinl = info.geom_.start.inl == info.geom_.stop.inl
			? info.geom_.start.inl : -999;

    SeisTrc trc; BinID bid( singinl, 0 );
    BinID step( abs(info.geom_.step.inl), abs(info.geom_.step.crl) );
    StepInterval<int> samps;
    const int nrcomps = info.compinfo_.size();
    while ( true )
    {
	if ( singinl == -999 )
	{
	    std::cerr << "\nExamine In-line (" << info.geom_.start.inl
		<< "-" << info.geom_.stop.inl;
	    if ( step.inl > 1 )
		std::cerr << " [" << step.inl << "]";
	    int stopinl = info.geom_.start.inl == 0 ? -1 : 0;
	    std::cerr << ", " << stopinl << " to stop): ";
	    getInt( bid.inl );
	    if ( bid.inl == stopinl ) break;
	}

	if ( info.geom_.fullyrectandreg )
	{
	    if ( bid.inl < info.geom_.start.inl || bid.inl > info.geom_.stop.inl )
	    {
		std::cerr << "Invalid inline" << std::endl;
		continue;
	    }
	}
	else
	{
	    const int ldidx = info.geom_.cubedata.indexOf( bid.inl );
	    if ( ldidx < 0 )
	    {
		std::cerr << "This inline is not present in the cube"
		    	  << std::endl;
		continue;
	    }
	    const PosInfo::LineData& inlinf = *info.geom_.cubedata[ldidx];
	    std::cerr << "Xline range available: ";
	    for ( int idx=0; idx<inlinf.segments_.size(); idx++ )
	    {
		std::cerr << inlinf.segments_[idx].start << " - "
		     << inlinf.segments_[idx].stop;
		if ( idx < inlinf.segments_.size()-1 )
		    std::cerr << " and ";
	    }
	    std::cerr << std::endl;
	}

	if ( singinl == -999 )
	    std::cerr << "X-line (0 for new in-line): ";
	else
	    std::cerr << "X-line or trace number (0 to stop): ";
	getInt( bid.crl );
	if ( bid.crl == 0 )
	{
	    if ( singinl == -999 )
		continue;
	    else
		break;
	}

	if ( !tri->goTo( bid ) )
	    { std::cerr << "Cannot find this position" << std::endl; continue; }
	if ( !tri->read(trc) )
	    { std::cerr << "Cannot read trace!" << std::endl; continue; }

	if ( !mIsZero(trc.info().pick,mDefEps)
		&& !mIsUdf(trc.info().pick) )
	    std::cerr << "Pick position: " << trc.info().pick << std::endl;
	if ( !mIsZero(trc.info().refnr,mDefEps)
		&& !mIsUdf(trc.info().refnr) )
	    std::cerr << "Reference number: " << trc.info().refnr
		      << std::endl;
	if ( !mIsZero(trc.info().offset,mDefEps)
		&& !mIsUdf(trc.info().offset) )
	    std::cerr << "Offset: " << trc.info().offset << std::endl;
	if ( !mIsZero(trc.info().azimuth,mDefEps)
		&& !mIsUdf(trc.info().azimuth) )
	    std::cerr << "Azimuth: " << (trc.info().azimuth*57.29577951308232)
		      << std::endl;
	if ( !mIsZero(trc.info().coord.x,0.1) )
	{
	    BufferString str; trc.info().coord.fill(str.buf());
	    std::cerr << "Coordinate: " << str;
	    BinID b = info.geom_.b2c.transformBack( trc.info().coord );
	    if ( b != trc.info().binid )
	    {
		b.fill( str.buf() );
		std::cerr << " --> " << str;
	    }
	    std::cerr << std::endl;
	}

	while ( true )
	{
	    std::cerr << "Print samples ( 1 - " << trc.size() << " )."
		      << std::endl;
	    std::cerr << "From (0 to stop): ";
	    getInt( samps.start );
	    if ( samps.start < 1 ) break;

	    std::cerr << "To: "; getInt( samps.stop );
	    std::cerr << "Step: "; getInt( samps.step );
	    if ( samps.step < 1 ) samps.step = 1;
	    if ( samps.start < 1 ) samps.start = 1;
	    if ( samps.stop > trc.size() ) samps.stop = trc.size();
	    std::cerr << std::endl;
	    for ( int isamp=samps.start; isamp<=samps.stop; isamp+=samps.step )
	    {
		std::cerr << isamp << '\t';
		for ( int icomp=0; icomp<nrcomps; icomp++ )
		    std::cout << trc.get( isamp-1, icomp )
			      << (icomp == nrcomps-1 ? '\n' : '\t');
	    }
	    std::cerr << std::endl;
	}
    }

    ExitProgram( 0 ); return 0;
}
