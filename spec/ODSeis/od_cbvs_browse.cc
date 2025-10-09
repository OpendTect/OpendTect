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
#include "ioman.h"
#include "file.h"
#include "filepath.h"
#include "moddepmgr.h"
#include "ptrman.h"
#include "seiscbvs.h"
#include "seistrc.h"

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
    SetProgramArgs( argc, argv );
    mInitProg( OD::RunCtxt::BatchProgCtxt )
    OD::ModDeps().ensureLoaded( "Network" );

    if ( argc < 2 )
    {
	od_cerr() << "Usage: " << argv[0] << " cbvs_file" << od_endl;
	return 1;
    }

    const CommandLineParser clp( argc, argv );
    const uiRetVal uirv = IOMan::setDataSource( clp );
    if ( !uirv.isOK() )
	return 1;

    PIM().loadAuto( false );
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

    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::instance();
    if ( !tri->initRead( new StreamConn(fname,Conn::Read) ) )
	{ od_cout() << tri->errMsg() << od_endl;  return 1; }

    od_cout() << "\n";
    const CBVSReadMgr& mgr = *tri->readMgr();
    mgr.dumpInfo( od_cout(), true );
    const CBVSInfo& info = mgr.info();
    const int singinl = info.geom_.start_.inl() == info.geom_.stop_.inl()
			? info.geom_.start_.inl() : -999;

    SeisTrc trc; BinID bid( singinl, 0 );
    BinID step( abs(info.geom_.step_.inl()), abs(info.geom_.step_.crl()) );
    StepInterval<int> samps;
    const int nrcomps = info.compinfo_.size();
    while ( true )
    {
	if ( singinl == -999 )
	{
	    od_cout() << "\nExamine In-line (" << info.geom_.start_.inl()
		      << "-" << info.geom_.stop_.inl();
	    if ( step.inl() > 1 )
		od_cout() << " [" << step.inl() << "]";
	    int stopinl = info.geom_.start_.inl() == 0 ? -1 : 0;
	    od_cout() << ", " << stopinl << " to stop): ";
	    getInt( bid.inl() );
	    if ( bid.inl() == stopinl ) break;
	}

	if ( info.geom_.fullyrectandreg_ )
	{
	    if ( bid.inl()<info.geom_.start_.inl() ||
		 bid.inl()>info.geom_.stop_.inl() )
	    {
		od_cout() << "Invalid inline" << od_endl;
		continue;
	    }
	}
	else
	{
	    const int ldidx = info.geom_.cubedata_.indexOf( bid.inl() );
	    if ( ldidx < 0 )
	    {
		od_cout() << "This inline is not present in the cube"
			  << od_endl;
		continue;
	    }
	    const PosInfo::LineData& inlinf = *info.geom_.cubedata_[ldidx];
	    od_cout() << "Xline range available: ";
	    for ( int idx=0; idx<inlinf.segments_.size(); idx++ )
	    {
                od_cout() << inlinf.segments_[idx].start_ << " - "
                          << inlinf.segments_[idx].stop_;
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

	if ( !mIsZero(trc.info().pick_,mDefEps) && !mIsUdf(trc.info().pick_) )
	    od_cout() << "Pick position: " << trc.info().pick_ << od_endl;

	if ( !mIsZero(trc.info().refnr_,mDefEps) && !mIsUdf(trc.info().refnr_) )
	    od_cout() << "Reference number: " << trc.info().refnr_
		      << od_endl;
	if ( !mIsZero(trc.info().offset_,mDefEps)
	     && !mIsUdf(trc.info().offset_) )
	    od_cout() << "Offset: " << trc.info().offset_ << od_endl;
	if ( !mIsZero(trc.info().azimuth_,mDefEps)
	     && !mIsUdf(trc.info().azimuth_) )
	    od_cout() << "Azimuth: " << (Math::toDegrees(trc.info().azimuth_))
		      << od_endl;
	if ( !mIsZero(trc.info().coord_.x_,0.1) )
	{
	    od_cout() << "Coordinate: " << trc.info().coord_.toPrettyString();
	    BinID b = info.geom_.b2c_.transformBack( trc.info().coord_ );
	    if ( b != trc.info().binID() )
		od_cout() << " --> " << b.toString();
	    od_cout() << od_endl;
	}

	while ( true )
	{
	    od_cout() << "Print samples ( 1 - " << trc.size() << " )."
		      << od_endl;
	    od_cout() << "From (0 to stop): ";
	    getInt( samps.start_ );
	    if ( samps.start_ < 1 ) break;

	    od_cout() << "To: "; getInt( samps.stop_ );
	    od_cout() << "Step: "; getInt( samps.step_ );
	    if ( samps.step_ < 1 ) samps.step_ = 1;
	    if ( samps.start_ < 1 ) samps.start_ = 1;
	    if ( samps.stop_ > trc.size() ) samps.stop_ = trc.size();
	    od_cout() << od_endl;
	    for ( int isamp=samps.start_; isamp<=samps.stop_;
		  isamp+=samps.step_ )
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
