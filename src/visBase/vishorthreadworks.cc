/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vishorthreadworks.h"
#include "vishorizonsection.h"
#include "vishorizonsectiontile.h"
#include "vishorizonsectiondef.h"

#include "binidsurface.h"
#include "position.h"
#include "simpnumer.h"
#include "sorting.h"
#include "thread.h"
#include "zaxistransform.h"



namespace visBase
{

HorizonTileRenderPreparer::HorizonTileRenderPreparer(
    HorizonSection& hrsection, const osg::CullStack* cs, char res )
    : permutation_( 0 )
    , hrsectiontiles_( hrsection.tiles_.getData() )
    , hrsection_( hrsection )
    , nrtiles_( hrsection.tiles_.info().getTotalSz() )
    , nrcoltiles_( hrsection.tiles_.info().getSize(1) )
    , resolution_( res )
    , tkzs_( cs )
{
}


bool HorizonTileRenderPreparer:: doPrepare( int nrthreads )
{
    barrier_.setNrThreads( nrthreads );
    nrthreadsfinishedwithres_ = 0;

    deleteAndNullArrPtr( permutation_ );
    mTryAlloc( permutation_, od_int64[nrtiles_] );
    if ( !permutation_ )
	return false;

    for ( int idx=0; idx<nrtiles_; idx++ )
	permutation_[idx] = idx;

    OD::shuffle( permutation_, permutation_+nrtiles_ );

    return true;
}


bool HorizonTileRenderPreparer::doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	if ( hrsectiontiles_[realidx] )
	    hrsectiontiles_[realidx]->updateAutoResolution( tkzs_ );
    }

    barrier_.waitForAll();

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	HorizonSectionTile* tile = hrsectiontiles_[realidx];
	if ( tile )
	{
	    char res = tile->getActualResolution();
	    if ( res==cNoneResolution )
		res = resolution_;
	    else
		tile->tesselateResolution( res, true );
	}

	addToNrDone( 1 );
    }

    barrier_.waitForAll();
    if ( !shouldContinue() )
	return false;

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	if ( hrsectiontiles_[realidx] )
	    hrsectiontiles_[realidx]->ensureGlueTesselated();

	addToNrDone( 1 );
    }

    return true;
}


HorizonTileResolutionTesselator::HorizonTileResolutionTesselator(
    const HorizonSection* hrsection, char res )
    : horsection_( hrsection )
    , nrtiles_( 0 )
    , resolution_( res )
{
    if ( hrsection )
	nrtiles_ =  hrsection->tiles_.info().getTotalSz();

    setName( "Horizon resolution tessellation" );
}


HorizonTileResolutionTesselator::~HorizonTileResolutionTesselator()
{
    for ( int idx=0; idx<hrtiles_.size(); idx++ )
    {
	if ( hrtiles_[idx] )
	    delete hrtiles_[idx];
    }
    hrtiles_.setEmpty();
}

bool HorizonTileResolutionTesselator:: doPrepare( int nrthreads )
{
    return createTiles();
}


bool HorizonTileResolutionTesselator::doWork( od_int64 start, od_int64 stop,int)
{
    if ( !horsection_ || !horsection_->geometry_ )
	return false;

    const StepInterval<int> rrg = horsection_->displayedRowRange();
    const StepInterval<int> crg = horsection_->displayedColRange();

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	if ( !hrtiles_[idx] )
	     continue;

	const RowCol& origin = hrtiles_[idx]->origin_;
	TypeSet<Coord3> positions;
	const int nrsidecoords = horsection_->nrcoordspertileside_;
	positions.setCapacity( nrsidecoords*nrsidecoords, false );
	for ( int rowidx=0; rowidx<nrsidecoords ; rowidx++ )
	{
	    const int row = origin.row() + rowidx*rrg.step_;
	    const bool rowok = rrg.includes(row, false);
	    const StepInterval<int> geocolrg =
		horsection_->geometry_->colRange( row );
	    const StepInterval<int> colrg(
			mMAX(geocolrg.start_,crg.start_),
			mMIN(geocolrg.stop_,crg.stop_), crg.step_ );

	    for ( int colidx=0; colidx<nrsidecoords; colidx++ )
	    {
		const int col = origin.col() + colidx*colrg.step_;
		Coord3 pos = rowok && colrg.includes(col, false)
		    ? horsection_->geometry_->getKnot(RowCol(row,col),false)
		    : Coord3::udf();
		if ( horsection_->zaxistransform_ )
                    pos.z_ = horsection_->zaxistransform_->transform( pos );
		positions += pos;
	    }
	}

	hrtiles_[idx]->setPositions( positions );
	hrtiles_[idx]->tesselateResolution( resolution_, false );
	addToNrDone( 1 );
    }

    return true;
}


bool HorizonTileResolutionTesselator::createTiles()
{
    if ( !horsection_ )
	return false;

    const StepInterval<int> rrg = horsection_->displayedRowRange();
    const StepInterval<int> crg = horsection_->displayedColRange();
    if ( rrg.width(false)<0 || crg.width(false)<0 )
	return false;

    const int nrrows = nrBlocks( rrg.nrSteps()+1,
				 horsection_->nrcoordspertileside_, 1 );
    const int nrcols = nrBlocks( crg.nrSteps()+1,
				 horsection_->nrcoordspertileside_, 1 );

    for ( int tilerowidx=0; tilerowidx<nrrows; tilerowidx++ )
    {
	for ( int tilecolidx=0; tilecolidx<nrcols; tilecolidx++ )
	{
	    const RowCol step(rrg.step_,crg.step_);
	    const RowCol tileorigin(horsection_->origin_.row() +
		tilerowidx*horsection_->tilesidesize_*step.row(),
		horsection_->origin_.col() +
		tilecolidx*horsection_->tilesidesize_*step.col() );

	    HorizonSectionTile* tile =
		new HorizonSectionTile( *horsection_, tileorigin );
	    tile->setResolution( resolution_ );
	    hrtiles_ += tile;
	}
    }

    return true;
}


bool HorizonTileResolutionTesselator::getTileCoordinates( int idx,
    TypeSet<Coord3>& coords ) const
{
    if ( idx>=0 && idx<hrtiles_.size() )
	return hrtiles_[idx]->getResolutionCoordinates( coords );

    return false;
}


bool HorizonTileResolutionTesselator::getTileNormals(
    int idx, TypeSet<Coord3>& normals ) const
{
    if ( idx>=0 && idx<hrtiles_.size() )
	return hrtiles_[idx]->getResolutionNormals( normals );

    return false;
}


bool HorizonTileResolutionTesselator::getTilePrimitiveSet( int idx,
    TypeSet<int>& ps, GeometryType type ) const
{
    if ( idx>=0 && idx<hrtiles_.size() )
	return hrtiles_[idx]->getResolutionPrimitiveSet(resolution_,ps,type);

    return false;
}


TileTesselator::TileTesselator( HorizonSectionTile* tile, char res )
    : tile_( tile ), res_( res )
{}


od_int64 TileTesselator::totalNr() const
{
    return 1;
}


int TileTesselator::nextStep()
{
    if ( tile_ )
	tile_->tesselateResolution( res_, false );

    return SequentialTask::Finished();
}


TileGlueTesselator::TileGlueTesselator( HorizonSectionTile* tile )
    : tile_( tile )
{}


int TileGlueTesselator::nextStep()
{
    if ( tile_ )
        tile_->ensureGlueTesselated();
    return SequentialTask::Finished();
}


//HorizonSectionTilePosSetup
HorizonSectionTilePosSetup::HorizonSectionTilePosSetup(
    TypeSet<RowCol>& tiles, TypeSet<RowCol>& indexes,HorizonSection* horsection,
    StepInterval<int>rrg,StepInterval<int>crg )
    : geo_( 0 )
    , rrg_( rrg )
    , crg_( crg )
    , horsection_( horsection )
    , hortiles_( tiles )
    , indexes_( indexes )
{
    if ( horsection_ )
    {
	nrcrdspertileside_ = horsection_->nrcoordspertileside_;
	resolution_ = horsection_->lowestresidx_;
	geo_ = horsection_->geometry_;
    }

    setName( BufferString( "Creating horizon surface..." ) );
}


HorizonSectionTilePosSetup::~HorizonSectionTilePosSetup()
{
}


void HorizonSectionTilePosSetup::setTesselationResolution( char res )
{
    if ( horsection_ && res>=0 && res<horsection_->nrResolutions() )
	resolution_ = res;
}


od_int64 HorizonSectionTilePosSetup::nrIterations() const
{
    return hortiles_.size();
}


bool HorizonSectionTilePosSetup::doWork( od_int64 start, od_int64 stop, int )
{
    if ( !geo_ )
	return false;

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const RowCol& origin =	hortiles_[idx];
	if ( origin.isUdf() )
	     continue;

	TypeSet<Coord3> positions;
	bool hasdata = false;
	positions.setCapacity( (nrcrdspertileside_)*(nrcrdspertileside_),
			       false );
	for ( int rowidx=0; rowidx<nrcrdspertileside_ ; rowidx++ )
	{
	    const int row = origin.row() + rowidx*rrg_.step_;
	    const bool rowok = rrg_.includes(row, false);
	    const StepInterval<int> geocolrg = geo_->colRange( row );
	    const StepInterval<int> colrg(
			mMAX(geocolrg.start_,crg_.start_),
			mMIN(geocolrg.stop_,crg_.stop_), crg_.step_ );

	    for ( int colidx=0; colidx<nrcrdspertileside_ ; colidx++ )
	    {
		const int col = origin.col() + colidx*colrg.step_;
		Coord3 pos = rowok && colrg.includes(col, false)
		    ? geo_->getKnot(RowCol(row,col),false)
		    : Coord3::udf();

		if ( pos.isDefined() )
		{
		    hasdata = true;
		    if ( zaxistransform_ )
                        pos.z_ = zaxistransform_->transform( pos );
		}

		positions += pos;
	    }
	}

	HorizonSectionTile* tile = nullptr;
	if ( hasdata )
	{
	    Threads::Locker locker( lock_ );
	    tile = new HorizonSectionTile( *horsection_, origin );
	    tile->setPositions( positions );
	    tile->tesselateResolution( resolution_, false );
	    locker.unlockNow();
	}

	const RowCol tileindex = indexes_[idx];
	horsection_->writeLock();
	horsection_->tiles_.set( tileindex.row(), tileindex.col(), tile );
	horsection_->writeUnLock();
	addToNrDone( 1 );
    }

    return true;
}


bool HorizonSectionTilePosSetup::doFinish( bool sucess )
{
    if ( sucess && horsection_ )
    {
	horsection_->forceupdate_ =  true;
    }

    return sucess;
}

} // namespace visBase
