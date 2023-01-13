/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gmtarray2dinterpol.h"

#include "arraynd.h"
#include "file.h"
#include "filepath.h"
#include "gmtpar.h"
#include "math2.h"
#include "iopar.h"
#include "oddirs.h"
#include "string2.h"
#include "strmprov.h"
#include "oscommand.h"

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


uiString GMTArray2DInterpol::uiMessage() const
{
    return msg_;
}


bool GMTArray2DInterpol::doPrepare( int nrthreads )
{
    workingdir_.set( GetProcFileName(nullptr) );
    mTryAlloc( nodes_, bool[nrcells_] );
    getNodesToFill( 0, nodes_, nullptr );
    defundefpath_ = FilePath(GetDataDir(),"Misc","defundefinfo.grd").fullPath();
    BufferString rgstr( "-R0/" );
    rgstr.add( nrrows_ - 1 ).add( "/0/" ).add( nrcols_ - 1 );
    const BufferString garg( "-G", defundefpath_ );

    OS::MachineCommand xyzmc( "xyz2grd" );
    xyzmc.addArg( rgstr ).addArg( garg ).addArg( "-I1" );
    OS::MachineCommand mc = GMTPar::getWrappedComm( xyzmc );
    delete sdmask_;
    sdmask_ = new od_ostream( mc, workingdir_ );
    if ( !sdmask_ || !sdmask_->isOK() )
	return false;

    OS::MachineCommand finalmc;
    if ( !fillCommand(finalmc) )
	return false;

    sd_ = new od_ostream( finalmc, workingdir_ );
    if ( !sd_ || !sd_->isOK() )
	return false;

    return true;
}


bool GMTArray2DInterpol::doWork( od_int64 start, od_int64 stop, int threadid )
{
    nrdone_ = 0;
    for ( int ridx=mCast(int,start); ridx<stop; ridx++ )
    {
	if ( !sd_ || !sdmask_ )
	    break;

	for ( int cidx=0; cidx<nrcols_; cidx++ )
	{
	    if ( !sd_ || !sdmask_ )
		break;

	    nodes_[nrcols_*ridx+cidx]
		? *sdmask_ << ridx << " " << cidx << " " << 1
		: *sdmask_ << ridx << " " << cidx << " " << "NaN";
	    *sdmask_ << "\n";
	    if ( !arr_->info().validPos(ridx, cidx) )
		continue;

	    if ( mIsUdf(arr_->get(ridx, cidx)) )
		continue;

	    addToNrDone( 1 );
	    *sd_ << ridx << " " << cidx << " " << arr_->get(ridx, cidx)
						     << "\n";
	}

	nrdone_++;
    }

    deleteAndNullPtr( sd_ );
    deleteAndNullPtr( sdmask_ );

    BufferString path( path_ );
    path_ = FilePath( GetDataDir() ).add( "Misc" )
				    .add( "result.grd" ).fullPath();


    OS::MachineCommand grdmc( "grdmath" );
    grdmc.addArg( path ).addArg( defundefpath_ ).addArg( "OR" )
	 .addArg( "=" ).addArg( path_ );
    OS::MachineCommand mc = GMTPar::getWrappedComm( grdmc );
    if ( !mc.execute() )
	return false;

    File::remove( defundefpath_ );
    File::remove( path );

    return true;
}


bool GMTArray2DInterpol::doFinish( bool success )
{
    deleteAndNullPtr( sd_ );
    deleteAndNullPtr( sdmask_ );

    OS::MachineCommand xyzmc( "grd2xyz" );
    xyzmc.addArg( path_ );
    OS::MachineCommand mc = GMTPar::getWrappedComm( xyzmc );
    StreamData sd = StreamProvider::createCmdIStream( mc, workingdir_ );
    if ( !sd.usable() )
	return false;

    nrdone_ = 0;
    char rowstr[10], colstr[10], valstr[20];
    StringView fsrowstr( rowstr ), fscolstr( colstr ), fsvalstr( valstr );
    for ( int ridx=0; ridx<nrrows_; ridx++ )
    {
	if ( !*sd.iStrm() )
	    break;

	for ( int cidx=0; cidx<nrcols_; cidx++ )
	{
	    if ( !*sd.iStrm() )
		break;

	    if ( !arr_->info().validPos(ridx, cidx) )
		continue;

	    rowstr[0] = colstr[0] = valstr[0] = '\0';
	    *sd.iStrm() >> rowstr >> colstr >> valstr;
	    if ( fsrowstr == sKeyGMTUdf || fscolstr == sKeyGMTUdf
					|| fsvalstr == sKeyGMTUdf )
		continue;

	    const int row = fsrowstr.toInt();
	    const int col = fscolstr.toInt();
	    const float val = fsvalstr.toFloat();
	    if ( !mIsUdf(row) && !mIsUdf(col) && !mIsUdf(val) )
		arr_->set( row, col, val );
	}

	nrdone_++;
    }

    sd.close();
    if ( nrdone_ == 0 )
    {
	msg_ = tr("Invalid positions found");
	return false;
    }

    File::remove( path_ );

    return true;
}


//GMTSurfaceGrid
GMTSurfaceGrid::GMTSurfaceGrid()
    : tension_( -1 )
{
    msg_ = tr("Continuous curvature");
}


const char* GMTSurfaceGrid::sType()
{ return "Continuous curvature(GMT)"; }


Array2DInterpol* GMTSurfaceGrid::create()
{
    return new GMTSurfaceGrid;
}


void GMTSurfaceGrid::setTension( float tension )
{
    tension_ = tension;
}


uiString GMTSurfaceGrid::infoMsg() const
{
    return tr("The selected algorithm will work only after loading GMT plugin");
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


bool GMTSurfaceGrid::fillCommand( OS::MachineCommand& mc )
{
    if ( tension_ < 0 )
    {
	msg_ = tr("Tension parameter missing");
	return false;
    }

    path_ = FilePath( GetDataDir() ).add( "Misc" ).add( "info.grd" ).fullPath();
    BufferString rgstr( "-R0/" );
    rgstr.add( nrrows_ - 1 ).add( "/0/" ).add( nrcols_ - 1 );
    BufferString tensionstr( "-T" ); tensionstr.add( tension_ );

    OS::MachineCommand locmc( "surface" );
    locmc.addArg( "-I1" ).addArg( tensionstr )
	 .addArg( BufferString(path_).insertAt(0,"-G") ).addArg( rgstr );

    mc = GMTPar::getWrappedComm( locmc );

    return !mc.isBad();
}


//GMTNearNeighborGrid
GMTNearNeighborGrid::GMTNearNeighborGrid()
    : radius_( -1 )
{
    msg_ = tr("Nearest neighbor");
}


const char* GMTNearNeighborGrid::sType()
{ return "Nearest neighbor(GMT)"; }


Array2DInterpol* GMTNearNeighborGrid::create()
{
    return new GMTNearNeighborGrid;
}


void GMTNearNeighborGrid::setRadius( float radius )
{
    radius_ = radius;
}


uiString GMTNearNeighborGrid::infoMsg() const
{
    return tr("The selected algorithm will work only after loading GMT plugin");
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


bool GMTNearNeighborGrid::fillCommand( OS::MachineCommand& mc )
{
    if ( radius_ < 0 )
    {
	msg_ = tr("Search radius parameter missing");
	return false;
    }

    path_ = FilePath( GetDataDir() ).add( "Misc" ).add( "info.grd" ).fullPath();
    BufferString rgstr( "-R0/" );
    rgstr.add( nrrows_ - 1 ).add( "/0/" ).add( nrcols_ - 1 );
    BufferString radiusstr( "-S" ); radiusstr.add( radius_ );

    OS::MachineCommand locmc( "nearneighbor" );
    locmc.addArg( "-I1" ).addArg( rgstr )
	 .addArg( radiusstr ).addArg( "-N4/2" )
	 .addArg( BufferString(path_).insertAt(0,"-G") ).addArg( rgstr );

    mc = GMTPar::getWrappedComm( locmc );

    return !mc.isBad();
}
