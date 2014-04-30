 /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/
static const char* rcsID mUsedVar = "$Id$";


#include "vishorthreadworks.h"
#include "vishorizonsection.h"
#include "vishorizonsectiontile.h"
#include "vishorizonsectiondef.h"

#include "zaxistransform.h"
#include "binidsurface.h"
#include "position.h"


using namespace visBase;


HorizonTileRenderPreparer::HorizonTileRenderPreparer( 
    HorizonSection& hrsection, const osg::CullStack* cs, char res )
    : hrsection_( hrsection )
    , cs_( cs )
    , hrsectiontiles_( hrsection.tiles_.getData() )
    , nrtiles_( hrsection.tiles_.info().getTotalSz() )
    , nrcoltiles_( hrsection.tiles_.info().getSize(1) )
    , resolution_( res )						
    , permutation_( 0 )    
{

}


bool HorizonTileRenderPreparer:: doPrepare( int nrthreads )
{
    barrier_.setNrThreads( nrthreads );
    nrthreadsfinishedwithres_ = 0;

    delete [] permutation_;
    permutation_ = 0;
    mTryAlloc( permutation_, od_int64[nrtiles_] );

    for ( int idx=0; idx<nrtiles_; idx++ )
	permutation_[idx] = idx;

    std::random_shuffle( permutation_, permutation_+nrtiles_ );

    return true;
}


bool HorizonTileRenderPreparer::doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	if ( hrsectiontiles_[realidx] ) 
	    hrsectiontiles_[realidx]->updateAutoResolution( cs_ );
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
	    tile->updateNormals( res );
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


TileTesselator::TileTesselator( HorizonSectionTile* tile, char res )
    : tile_( tile ), res_( res )
{}


int TileTesselator::nextStep()
{
    tile_->updateNormals( res_ );
    tile_->tesselateResolution( res_, false );
    return SequentialTask::Finished();
}


TileGlueTesselator::TileGlueTesselator( HorizonSectionTile* tile )
    : tile_( tile )
{}


int TileGlueTesselator::nextStep()
{
    tile_->ensureGlueTesselated();
    return SequentialTask::Finished();
}



HorizonSectionTilePosSetup::HorizonSectionTilePosSetup( 
    ObjectSet<HorizonSectionTile> tiles, HorizonSection* horsection,
    StepInterval<int>rrg,StepInterval<int>crg )
    : hrtiles_( tiles )
    , rrg_( rrg )
    , crg_( crg )
    , geo_( 0 )
    , horsection_( horsection )
{

    if ( horsection_ )
    {
	zaxistransform_ = horsection_->getZAxisTransform();
	nrcrdspertileside_ = horsection_->nrcoordspertileside_;
	lowestresidx_ = horsection_->lowestresidx_;
	geo_ = new Geometry::BinIDSurface( *horsection_->geometry_ );
    }
    
    if ( zaxistransform_ ) zaxistransform_->ref();
    setName( BufferString( "Creating horizon surface..." ) );
}


HorizonSectionTilePosSetup::~HorizonSectionTilePosSetup()
{
    if ( zaxistransform_ ) zaxistransform_->unRef();
    if ( geo_ )
	delete geo_;
    
}


bool HorizonSectionTilePosSetup::doWork( od_int64 start, od_int64 stop, 
					 int threadid )
{
    if ( !geo_ )
	return false;
    
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const RowCol& origin = hrtiles_[idx]->origin_;
	TypeSet<Coord3> positions;
	positions.setCapacity( (nrcrdspertileside_)*(nrcrdspertileside_) );
	for ( int rowidx=0; rowidx<nrcrdspertileside_ ; rowidx++ )
	{
	    const int row = origin.row() + rowidx*rrg_.step;
	    const bool rowok = rrg_.includes(row, false);
	    const StepInterval<int> colrg( 
		mMAX(geo_->colRange(row).start, crg_.start),
		mMIN(geo_->colRange(row).stop, crg_.stop), crg_.step );

	    for ( int colidx=0; colidx<nrcrdspertileside_ ; colidx++ )
	    {
		const int col = origin.col() + colidx*colrg.step;
		Coord3 pos = rowok && colrg.includes(col, false)
		    ? geo_->getKnot(RowCol(row,col),false) 
		    : Coord3::udf();
		if ( zaxistransform_ ) 
		    pos.z = zaxistransform_->transform( pos );		
		positions += pos;
	    }
	}

	hrtiles_[idx]->setPositions( positions );
	hrtiles_[idx]->updateNormals( lowestresidx_ );
	hrtiles_[idx]->tesselateResolution( lowestresidx_, false );
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


