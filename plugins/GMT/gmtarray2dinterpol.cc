/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: gmtarray2dinterpol.cc,v 1.2 2010-08-25 07:11:11 cvsnageswara Exp $";

#include "gmtarray2dinterpol.h"

#include "arraynd.h"
#include "file.h"
#include "filepath.h"
#include "math2.h"
#include "oddirs.h"
#include "string2.h"
#include "strmdata.h"
#include "strmprov.h"

#include <iostream>

static const char* sKeyGMTUdf = "NaN";

GMTArray2DInterpol::GMTArray2DInterpol()
    : nrdone_(0)
{}


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
    if ( filltype_ != Array2DInterpol::Full )
    {
	msg_ = "Selected algorithm will work only on full survey";
	return false;
    }

    BufferString gmtcmd;
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
    for ( int ridx=start; ridx<stop; ridx++ )
    {
	if ( !*sd_.ostrm )
	    break;

	for ( int cidx=0; cidx<nrcols_; cidx++ )
	{
	    if ( !*sd_.ostrm )
		break;

	    if ( !arr_->info().validPos(ridx, cidx) )
		continue;

	    if ( mIsUdf(arr_->get(ridx,cidx)) )
		continue;

	    *sd_.ostrm << ridx << " " << cidx << " " << arr_->get(ridx,cidx)
						     << std::endl;

	}

	nrdone_++;
    }

    sd_.close();

    return true;
}


bool GMTArray2DInterpol::doFinish( bool success )
{
    BufferString cmd = "@grd2xyz ";
    cmd.add( tmpfnm_ );

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
	    if ( !strcmp(rowstr,sKeyGMTUdf) || !strcmp(colstr,sKeyGMTUdf)
	      				    || !strcmp(valstr,sKeyGMTUdf) )
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

    File::remove( tmpfnm_ );

    return true;
}


//GMTSurfaceGrid
GMTSurfaceGrid::GMTSurfaceGrid()
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


void GMTSurfaceGrid::setPar( const IOPar& iop )
{
    iopar_  = iop;
}


bool GMTSurfaceGrid::mkCommand( BufferString& cmd )
{
    if ( iopar_.isEmpty() )
    {
	msg_ = "Tension parameter missing";
	return false;
    }

    float tension;
    iopar_.get( "Tension", tension );
    tmpfnm_ = FilePath::getTempName( "grd" );
    cmd = "@surface -I1 ";
    cmd.add( "-T" ).add( tension )
       .add( " -G" ).add( tmpfnm_ )
       .add( " -R0/" ).add( nrrows_ - 1 ).add( "/0/" ).add( nrcols_ - 1 );

    return true;
}


//GMTNearNeighborGrid
GMTNearNeighborGrid::GMTNearNeighborGrid()
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


void GMTNearNeighborGrid::setPar( const IOPar& iop )
{
    iopar_  = iop;
}


bool GMTNearNeighborGrid::mkCommand( BufferString& cmd )
{
    if ( iopar_.isEmpty() )
    {
	msg_ = "Search radius parameter missing";
	return false;
    }

    float radius;
    iopar_.get( "Radius", radius );
    tmpfnm_ = FilePath::getTempName( "grd" );
    cmd = "@nearneighbor -I1 ";
    cmd.add( " -R0/" ).add( nrrows_ - 1 ).add( "/0/" ).add( nrcols_ - 1 )
       .add( " -S" ).add( radius )
       .add( " -N4/2" )
       .add( " -G" ).add( tmpfnm_ );

    return true;
}
