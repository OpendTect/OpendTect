/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "gmtarray2dinterpol.h"

#include "arraynd.h"
#include "file.h"
#include "filepath.h"
#include "math2.h"
#include "iopar.h"
#include "oddirs.h"
#include "string2.h"
#include "strmdata.h"
#include "strmprov.h"

#include <iostream>

static const char* sKeyGMTUdf = "NaN";

static const char* sKeyTension()			{ return "Tension"; }
static const char* sKeyRadius()				{ return "Radius"; }

GMTArray2DInterpol::GMTArray2DInterpol()
    : nrdone_(0)
    , nodes_(0)
{}


GMTArray2DInterpol::~GMTArray2DInterpol()
{
    delete nodes_;
}


od_int64 GMTArray2DInterpol::nrIterations() const
{
    return nrrows_ + 1;
}


const char* GMTArray2DInterpol::message() const
{
    return msg_.buf();
}


bool GMTArray2DInterpol::doPrepare( int nrthreads )
{
    mTryAlloc( nodes_, bool[nrcells_] );
    getNodesToFill( 0, nodes_, 0 );
    defundefpath_ = FilePath(GetDataDir(),"Misc","defundefinfo.grd").fullPath();
    BufferString gmtcmd( "@xyz2grd" );
    gmtcmd.add( " -R0/" ).add( nrrows_ - 1 ).add( "/0/" ).add( nrcols_ - 1 )
			 .add( " -G").add( defundefpath_ ).add( " -I1" );
    sdmask_ = StreamProvider( gmtcmd ).makeOStream();
    if ( !sdmask_.usable() )
	return false;

    if ( !mkCommand(gmtcmd) )
	return false;

    sd_ = StreamProvider( gmtcmd ).makeOStream();
    if ( !sd_.usable() )
	return false;

    return true;
}


bool GMTArray2DInterpol::doWork( od_int64 start, od_int64 stop, int threadid )
{
    nrdone_ = 0;
    for ( int ridx=mCast(int,start); ridx<stop; ridx++ )
    {
	if ( !*sd_.ostrm || !*sdmask_.ostrm )
	    break;

	for ( int cidx=0; cidx<nrcols_; cidx++ )
	{
	    if ( !*sd_.ostrm || !*sdmask_.ostrm )
		break;

	    nodes_[nrcols_*ridx+cidx]
		? *sdmask_.ostrm << ridx << " " << cidx << " " << 1
		: *sdmask_.ostrm << ridx << " " << cidx << " " << "NaN";
	    *sdmask_.ostrm << std::endl;
	    if ( !arr_->info().validPos(ridx, cidx) )
		continue;

	    if ( mIsUdf(arr_->get(ridx, cidx)) )
		continue;

	    addToNrDone( 1 );
	    *sd_.ostrm << ridx << " " << cidx << " " << arr_->get(ridx, cidx)
						     << std::endl;
	}

	nrdone_++;
    }

    sd_.close();
    sdmask_.close();

    BufferString path( path_ );
    path_ = FilePath( GetDataDir() ).add( "Misc" )
				    .add( "result.grd" ).fullPath();
    BufferString cmd( "@grdmath " );
    cmd.add( path ).add( " " ).add( defundefpath_ )
       .add( " OR = " ).add( path_ );
    StreamProvider strmprov( cmd );
    if ( !strmprov.executeCommand() )
	return false;

    File::remove( defundefpath_ );
    File::remove( path );

    return true;
}


bool GMTArray2DInterpol::doFinish( bool success )
{
    BufferString cmd = "@grd2xyz ";
    cmd.add( path_ );

    sd_ = StreamProvider( cmd ).makeIStream( true, false );
    if ( !sd_.usable() )
	return false;

    nrdone_ = 0;
    for ( int ridx=0; ridx<nrrows_; ridx++ )
    {
	if ( !*sd_.istrm )
	    break;

	for ( int cidx=0; cidx<nrcols_; cidx++ )
	{
	    if ( !*sd_.istrm )
		break;

	    if ( !arr_->info().validPos(ridx, cidx) )
		continue;

	    char rowstr[10], colstr[10], valstr[20];
	    *sd_.istrm >> rowstr >> colstr >> valstr;
	    if ( !strcmp(rowstr, sKeyGMTUdf) || !strcmp(colstr, sKeyGMTUdf)
	      				     || !strcmp(valstr, sKeyGMTUdf) )
		continue;

	    int row, col; float val;
	    if ( !getFromString(row, rowstr) || !getFromString(col, colstr)
					     || !getFromString(val, valstr) )
		continue;

	    arr_->set( row, col, val );
	}

	nrdone_++;
    }

    sd_.close();
    if ( nrdone_ == 0 )
    {
	msg_ = "Invalid positions found";
	return false;
    }

    File::remove( path_ );

    return true;
}


//GMTSurfaceGrid
GMTSurfaceGrid::GMTSurfaceGrid()
    : tension_( -1 )
{
    msg_ = "Continuous curvature";
}


const char* GMTSurfaceGrid::sType()
{ return "Continuous curvature(GMT)"; }


void GMTSurfaceGrid::initClass()
{
    Array2DInterpol::factory().addCreator( create, sType() );
}


Array2DInterpol* GMTSurfaceGrid::create()
{
    return new GMTSurfaceGrid;
}


void GMTSurfaceGrid::setTension( float tension )
{
    tension_ = tension;
}


const char* GMTSurfaceGrid::infoMsg() const
{
    return "The selected algorithm will work only after loading GMT plugin";
}


bool GMTSurfaceGrid::fillPar( IOPar& iop ) const
{
    Array2DInterpol::fillPar( iop );
    iop.set( sKeyTension(), tension_ );
    return true;
}


bool GMTSurfaceGrid::usePar( const IOPar& par )
{
    Array2DInterpol::usePar( par );
    par.get( sKeyTension(), tension_ );
    return true;
}


bool GMTSurfaceGrid::mkCommand( BufferString& cmd )
{
    if ( tension_ < 0 )
    {
	msg_ = "Tension parameter missing";
	return false;
    }

    path_ = FilePath( GetDataDir() ).add( "Misc" ).add( "info.grd" ).fullPath();
    cmd = "@surface -I1 ";
    cmd.add( "-T" ).add( tension_ )
       .add( " -G" ).add( path_ )
       .add( " -R0/" ).add( nrrows_ - 1 ).add( "/0/" ).add( nrcols_ - 1 );

    return true;
}


//GMTNearNeighborGrid
GMTNearNeighborGrid::GMTNearNeighborGrid()
    : radius_( -1 )
{
    msg_ = "Nearest neighbor";
}


const char* GMTNearNeighborGrid::sType()
{ return "Nearest neighbor(GMT)"; }


void GMTNearNeighborGrid::initClass()
{
    Array2DInterpol::factory().addCreator( create, sType() );
}


Array2DInterpol* GMTNearNeighborGrid::create()
{
    return new GMTNearNeighborGrid;
}


void GMTNearNeighborGrid::setRadius( float radius )
{
    radius_ = radius;
}


const char* GMTNearNeighborGrid::infoMsg() const
{
    return "The selected algorithm will work only after loading GMT plugin";
}


bool GMTNearNeighborGrid::fillPar( IOPar& iop ) const
{
    Array2DInterpol::fillPar( iop );
    iop.set( sKeyRadius(), radius_ );
    return true;
}


bool GMTNearNeighborGrid::usePar( const IOPar& par )
{
    Array2DInterpol::usePar( par );
    par.get( sKeyRadius(), radius_ );
    return true;
}


bool GMTNearNeighborGrid::mkCommand( BufferString& cmd )
{
    if ( radius_ < 0 )
    {
	msg_ = "Search radius parameter missing";
	return false;
    }

    path_ = FilePath( GetDataDir() ).add( "Misc" ).add( "info.grd" ).fullPath();
    cmd = "@nearneighbor -I1 ";
    cmd.add( " -R0/" ).add( nrrows_ - 1 ).add( "/0/" ).add( nrcols_ - 1 )
       .add( " -S" ).add( radius_ )
       .add( " -N4/2" )
       .add( " -G" ).add( path_ );

    return true;
}
