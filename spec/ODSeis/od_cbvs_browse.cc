/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "cbvsinfo.h"
#include "cbvsreadmgr.h"
#include "commandlineparser.h"
#include "conn.h"
#include "datachar.h"
#include "ioman.h"
#include "iostrm.h"
#include "file.h"
#include "filepath.h"
#include "moddepmgr.h"
#include "strmprov.h"
#include "ptrman.h"
#include "od_iostream.h"
#include "seiscbvs.h"
#include "seistrc.h"

#include <ctype.h>
#include <math.h>

#include <iostream>

#include "prog.h"


static void getInt( int& i )
{
    char buf[80];

    char* ptr;
    do
    {
	ptr = buf;
	std::cin.getline( ptr, 80 );
	while ( *ptr && iswspace(*ptr) ) ptr++;
    }
    while ( ! *ptr );

    char* endptr = ptr + 1;
    while ( *endptr && !iswspace(*endptr) ) endptr++;
    *endptr = '\0';
    i = toInt( ptr );
}


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::BatchProgCtxt )
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "Network" );

    if ( argc < 2 )
    {
	od_cerr() << "Usage: " << argv[0] << " cbvs_file" << od_endl;
	return 1;
    }

    PIM().loadAuto( false );
    CommandLineParser clp( argc, argv );
    const uiRetVal uirv = IOM().setDataSource( clp );
    if ( !uirv.isOK() )
	return 1;

    OD::ModDeps().ensureLoaded( "Seis" );
    PIM().loadAuto( true );

    FilePath fp( argv[1] );
    if ( !fp.isAbsolute() )
	fp.insert( File::getCurrentPath() );
    const BufferString fname = fp.fullPath();
    if ( !File::exists(fname) )
    {
	od_cerr() << fname << " does not exist" << od_endl;
        return 1;
    }

    od_cout() << "Browsing '" << fname << "'\n" << od_endl;

    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead( new StreamConn(fname,Conn::Read) ) )
	{ od_cout() << tri->errMsg() << od_endl;  return 1; }

    od_cout() << "\n";
    const CBVSReadMgr& mgr = *tri->readMgr();
    mgr.dumpInfo( od_cout(), true );
    const CBVSInfo& info = mgr.info();
    const int singinl = info.geom_.start.inl() == info.geom_.stop.inl()
			? info.geom_.start.inl() : -999;

    SeisTrc trc; BinID bid( singinl, 0 );
    BinID step( abs(info.geom_.step.inl()), abs(info.geom_.step.crl()) );
    StepInterval<int> samps;
    const int nrcomps = info.compinfo_.size();
    while ( true )
    {
	if ( singinl == -999 )
	{
	    od_cout() << "\nExamine In-line (" << info.geom_.start.inl()
		<< "-" << info.geom_.stop.inl();
	    if ( step.inl() > 1 )
		od_cout() << " [" << step.inl() << "]";
	    int stopinl = info.geom_.start.inl() == 0 ? -1 : 0;
	    od_cout() << ", " << stopinl << " to stop): ";
	    getInt( bid.inl() );
	    if ( bid.inl() == stopinl ) break;
	}

	if ( info.geom_.fullyrectandreg )
	{
	    if ( bid.inl()<info.geom_.start.inl() ||
		 bid.inl()>info.geom_.stop.inl() )
	    {
		od_cout() << "Invalid inline" << od_endl;
		continue;
	    }
	}
	else
	{
	    const int ldidx = info.geom_.cubedata.indexOf( bid.inl() );
	    if ( ldidx < 0 )
	    {
		od_cout() << "This inline is not present in the cube"
			  << od_endl;
		continue;
	    }
	    const PosInfo::LineData& inlinf = *info.geom_.cubedata[ldidx];
	    od_cout() << "Xline range available: ";
	    for ( int idx=0; idx<inlinf.segments_.size(); idx++ )
	    {
		od_cout() << inlinf.segments_[idx].start << " - "
		     << inlinf.segments_[idx].stop;
		if ( idx < inlinf.segments_.size()-1 )
		    od_cout() << " and ";
	    }
	    od_cout() << od_endl;
	}

	if ( singinl == -999 )
	    od_cout() << "X-line (0 for new in-line): ";
	else
	    od_cout() << "X-line or trace number (0 to stop): ";
	getInt( bid.crl() );
	if ( bid.crl() == 0 )
	{
	    if ( singinl == -999 )
		continue;
	    else
		break;
	}

	if ( !tri->goTo( bid ) )
	    { od_cout() << "Cannot find this position" << od_endl; continue; }
	if ( !tri->read(trc) )
	    { od_cout() << "Cannot read trace!" << od_endl; continue; }

	if ( !mIsZero(trc.info().pick,mDefEps)
		&& !mIsUdf(trc.info().pick) )
	    od_cout() << "Pick position: " << trc.info().pick << od_endl;
	if ( !mIsZero(trc.info().refnr,mDefEps)
		&& !mIsUdf(trc.info().refnr) )
	    od_cout() << "Reference number: " << trc.info().refnr
		      << od_endl;
	if ( !mIsZero(trc.info().offset,mDefEps)
		&& !mIsUdf(trc.info().offset) )
	    od_cout() << "Offset: " << trc.info().offset << od_endl;
	if ( !mIsZero(trc.info().azimuth,mDefEps)
		&& !mIsUdf(trc.info().azimuth) )
	    od_cout() << "Azimuth: " << (Math::toDegrees(trc.info().azimuth))
		      << od_endl;
	if ( !mIsZero(trc.info().coord.x,0.1) )
	{
	    od_cout() << "Coordinate: " << trc.info().coord.toPrettyString();
	    BinID b = info.geom_.b2c.transformBack( trc.info().coord );
	    if ( b != trc.info().binID() )
		od_cout() << " --> " << b.toString();
	    od_cout() << od_endl;
	}

	while ( true )
	{
	    od_cout() << "Print samples ( 1 - " << trc.size() << " )."
		      << od_endl;
	    od_cout() << "From (0 to stop): ";
	    getInt( samps.start );
	    if ( samps.start < 1 ) break;

	    od_cout() << "To: "; getInt( samps.stop );
	    od_cout() << "Step: "; getInt( samps.step );
	    if ( samps.step < 1 ) samps.step = 1;
	    if ( samps.start < 1 ) samps.start = 1;
	    if ( samps.stop > trc.size() ) samps.stop = trc.size();
	    od_cout() << od_endl;
	    for ( int isamp=samps.start; isamp<=samps.stop; isamp+=samps.step )
	    {
		od_cout() << isamp << '\t';
		for ( int icomp=0; icomp<nrcomps; icomp++ )
		    od_cout() << trc.get( isamp-1, icomp )
			      << (icomp == nrcomps-1 ? '\n' : '\t');
	    }
	    od_cout() << od_endl;
	}
    }

    return 0;
}
