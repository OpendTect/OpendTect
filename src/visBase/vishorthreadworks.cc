/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vishorthreadworks.h"

#include "arraynd.h"
#include "binidsurface.h"
#include "simpnumer.h"
#include "sorting.h"
#include "survinfo.h"
#include "thread.h"
#include "vishorizonsection.h"
#include "vishorizonsectiontile.h"
#include "vishorizonsectiondef.h"
#include "zaxistransform.h"


namespace visBase
{

class HorTileSampleCache
{
public:

				HorTileSampleCache()	{}
				~HorTileSampleCache()	{}

    const float*		depths		= nullptr;
    int				depthrows	= 0;
    int				depthcols	= 0;
    int				originrow	= 0;
    int				origincol	= 0;
    int				rowstep		= 1;
    int				colstep		= 1;
    double			ax		= 0;
    double			bx		= 0;
    double			cx		= 0;
    double			ay		= 0;
    double			by		= 0;
    double			cy		= 0;
    TypeSet<StepInterval<int>>	colranges;
    StepInterval<int>		rrg;
    StepInterval<int>		crg;
    const ZAxisTransform*	zat		= nullptr;
    bool			ready		= false;
};

static void buildHorTileSampleCache( const Geometry::BinIDSurface& geo,
	const StepInterval<int>& rrg, const StepInterval<int>& crg,
	const ZAxisTransform* zat, HorTileSampleCache& cache )
{
    cache.ready = false;
    cache.rrg = rrg;
    cache.crg = crg;
    cache.zat = zat;
    cache.depths = nullptr;
    cache.depthrows = 0;
    cache.depthcols = 0;
    cache.colranges.setEmpty();

    const StepInterval<int> grow = geo.rowRange();
    const StepInterval<int> gcol = geo.colRange();
    cache.originrow = grow.start_;
    cache.origincol = gcol.start_;
    cache.rowstep = grow.step_ ? grow.step_ : 1;
    cache.colstep = gcol.step_ ? gcol.step_ : 1;

    const Array2D<float>* arr = geo.getArray();
    if ( arr )
    {
	cache.depths = arr->getData();
	cache.depthrows = arr->info().getSize( 0 );
	cache.depthcols = arr->info().getSize( 1 );
    }

    const Pos::IdxPair2Coord::DirTransform& xt =
	SI().binID2Coord().getTransform( true );
    const Pos::IdxPair2Coord::DirTransform& yt =
	SI().binID2Coord().getTransform( false );
    cache.ax = xt.a;
    cache.bx = xt.b;
    cache.cx = xt.c;
    cache.ay = yt.a;
    cache.by = yt.b;
    cache.cy = yt.c;

    const int nrrows = rrg.nrSteps() + 1;
    if ( nrrows>0 && cache.colranges.setSize(nrrows) )
    {
	for ( int idx=0; idx<nrrows; idx++ )
	    cache.colranges[idx] = geo.colRange( rrg.atIndex(idx) );
    }

    cache.ready = true;
}


static float rawZ( const HorTileSampleCache& cache,
		   const Geometry::BinIDSurface& geo, int row, int col )
{
    const int drow = row - cache.originrow;
    const int dcol = col - cache.origincol;
    if ( drow<0 || dcol<0 || !cache.rowstep || !cache.colstep ||
	 drow%cache.rowstep || dcol%cache.colstep )
	return mUdf(float);

    const int ri = drow / cache.rowstep;
    const int ci = dcol / cache.colstep;
    if ( ri>=cache.depthrows || ci>=cache.depthcols )
	return mUdf(float);

    if ( cache.depths )
	return cache.depths[(od_int64)ri*cache.depthcols + ci];

    const Array2D<float>* arr = geo.getArray();
    return arr ? arr->get( ri, ci ) : mUdf(float);
}


static bool fillTilePositions( const HorTileSampleCache& cache,
	const Geometry::BinIDSurface& geo, const RowCol& origin,
	int nrsidecoords, TypeSet<Coord3>& positions )
{
    bool hasdata = false;
    const int npos = nrsidecoords * nrsidecoords;
    if ( npos<=0 || !positions.setSize(npos, Coord3::udf()) )
	return false;

    for ( int rowidx=0; rowidx<nrsidecoords; rowidx++ )
    {
	const int row = origin.row() + rowidx*cache.rrg.step_;
	const bool rowok = cache.rrg.includes( row, false );
	StepInterval<int> colrg( mUdf(int), mUdf(int), cache.crg.step_ );
	if ( rowok )
	{
	    const int rel = row - cache.rrg.start_;
	    const int cidx = cache.rrg.step_ ? rel/cache.rrg.step_ : -1;
	    if ( cidx>=0 && rel%cache.rrg.step_==0 &&
		 cache.colranges.validIdx(cidx) )
		colrg = cache.colranges[cidx];
	    else
		colrg = geo.colRange( row );

	    colrg.start_ = mMAX( colrg.start_, cache.crg.start_ );
	    colrg.stop_ = mMIN( colrg.stop_, cache.crg.stop_ );
	    colrg.step_ = cache.crg.step_;
	}

	const double xrow = cache.ax + cache.bx*row;
	const double yrow = cache.ay + cache.by*row;
	Coord3* rowpos = &positions[rowidx*nrsidecoords];
	for ( int colidx=0; colidx<nrsidecoords; colidx++ )
	{
	    const int col = origin.col() + colidx*cache.crg.step_;
	    if ( !rowok || !colrg.includes(col,false) )
		continue;

	    Coord3& pos = rowpos[colidx];
	    pos.x_ = xrow + cache.cx*col;
	    pos.y_ = yrow + cache.cy*col;
	    if ( !pos.coord().isDefined() )
		continue;

	    float zval = rawZ( cache, geo, row, col );
	    if ( cache.zat && !mIsUdf(zval) )
		zval = cache.zat->transformTrc( TrcKey(BinID(row,col)), zval );

	    pos.z_ = zval;
	    hasdata = true;
	}
    }

    return hasdata;
}


HorizonTileRenderPreparer::HorizonTileRenderPreparer(
    HorizonSection& hrsection, const osg::CullStack* cs, char res )
    : hrsectiontiles_( hrsection.tiles_.getData() )
    , hrsection_( hrsection )
    , nrtiles_( hrsection.tiles_.info().getTotalSz() )
    , nrcoltiles_( hrsection.tiles_.info().getSize(1) )
    , resolution_( res )
    , tkzs_( cs )
{
}


HorizonTileRenderPreparer::~HorizonTileRenderPreparer()
{
    delete [] permutation_;
}


uiString HorizonTileRenderPreparer::uiNrDoneText() const
{
    return tr("Parts completed");
}


uiString HorizonTileRenderPreparer::uiMessage() const
{
    return tr("Updating Horizon Display");
}


bool HorizonTileRenderPreparer:: doPrepare( int nrthreads )
{
    barrier_.setNrThreads( nrthreads );
    nrthreadsfinishedwithres_ = 0;

    delete [] permutation_;
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
    for ( od_int64 idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = sCast(int,permutation_[idx]);
	HorizonSectionTile* tile = hrsectiontiles_[realidx];
	if ( tile )
	{
	    tile->updateAutoResolution( tkzs_ );
	    const char res = tile->getActualResolution();
	    if ( res != cNoneResolution )
		tile->tesselateResolution( res, true );
	}

	addToNrDone( 1 );
    }

    barrier_.waitForAll();
    if ( !shouldContinue() )
	return false;

    for ( od_int64 idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = sCast(int,permutation_[idx]);
	if ( hrsectiontiles_[realidx] )
	    hrsectiontiles_[realidx]->ensureGlueTesselated();

	addToNrDone( 1 );
    }

    return true;
}


bool HorizonTileRenderPreparer::doFinish( bool sucess )
{
    return sucess;
}


HorizonTileResolutionTesselator::HorizonTileResolutionTesselator(
    const HorizonSection* hrsection, char res )
    : horsection_(hrsection)
    , resolution_(res)
{
    if ( hrsection )
	nrtiles_ =  hrsection->tiles_.info().getTotalSz();

    setName( "Horizon resolution tessellation" );
}


HorizonTileResolutionTesselator::~HorizonTileResolutionTesselator()
{
    deepErase( hrtiles_ );
    delete cache_;
}


uiString HorizonTileResolutionTesselator::uiNrDoneText() const
{
    return tr("Parts completed");
}


uiString HorizonTileResolutionTesselator::uiMessage() const
{
    return tr("Tessellating horizon");
}


bool HorizonTileResolutionTesselator:: doPrepare( int nrthreads )
{
    if ( !createTiles() )
	return false;

    delete cache_;
    cache_ = new HorTileSampleCache;
    if ( horsection_ && horsection_->geometry_ )
	buildHorTileSampleCache( *horsection_->geometry_,
		horsection_->displayedRowRange(),
		horsection_->displayedColRange(),
		horsection_->getZAxisTransform(), *cache_ );

    return true;
}


bool HorizonTileResolutionTesselator::doWork( od_int64 start, od_int64 stop,int)
{
    if ( !horsection_ || !horsection_->geometry_ )
	return false;

    if ( !cache_ || !cache_->ready )
	return false;

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	if ( !hrtiles_[idx] )
	     continue;

	const RowCol& origin = hrtiles_[idx]->origin_;
	TypeSet<Coord3> positions;
	fillTilePositions( *cache_, *horsection_->geometry_, origin,
		horsection_->nrcoordspertileside_, positions );

	hrtiles_[idx]->setPositions( positions );
	hrtiles_[idx]->tesselateResolution( resolution_, false );
	addToNrDone( 1 );
    }

    return true;
}


bool HorizonTileResolutionTesselator::doFinish( bool sucess )
{
    deleteAndNullPtr( cache_ );
    return sucess;
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
    : tile_( tile )
    , res_( res )
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
		TypeSet<RowCol>& tiles, TypeSet<RowCol>& indexes,
		HorizonSection* horsection,
		StepInterval<int>rrg, StepInterval<int>crg )
    : rrg_( rrg )
    , crg_( crg )
    , horsection_( horsection )
    , hortiles_( tiles )
    , indexes_( indexes )
{
    if ( horsection_ )
    {
	zaxistransform_ = horsection_->getZAxisTransform();
	nrcrdspertileside_ = horsection_->nrcoordspertileside_;
	resolution_ = horsection_->lowestresidx_;
	geo_ = horsection_->geometry_;
    }

    setName( "Creating horizon surface" );
}


HorizonSectionTilePosSetup::~HorizonSectionTilePosSetup()
{
    delete cache_;
}


uiString HorizonSectionTilePosSetup::uiNrDoneText() const
{
    return tr("Parts completed");
}


uiString HorizonSectionTilePosSetup::uiMessage() const
{
    return tr("Creating Horizon Display");
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


bool HorizonSectionTilePosSetup::doPrepare( int )
{
    if ( !geo_ )
	return false;

    delete cache_;
    cache_ = new HorTileSampleCache;
    buildHorTileSampleCache( *geo_, rrg_, crg_,
			zaxistransform_.ptr(), *cache_ );
    return cache_->ready;
}


bool HorizonSectionTilePosSetup::doWork( od_int64 start, od_int64 stop, int )
{
    if ( !geo_ || !cache_ || !cache_->ready )
	return false;

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const RowCol& origin =	hortiles_[idx];
	if ( origin.isUdf() )
	     continue;

	TypeSet<Coord3> positions;
	const bool hasdata = fillTilePositions( *cache_, *geo_, origin,
					nrcrdspertileside_, positions );

	HorizonSectionTile* tile = nullptr;
	if ( hasdata )
	{
	    tile = new HorizonSectionTile( *horsection_, origin );
	    tile->setPositions( positions );
	    tile->tesselateResolution( resolution_, false );
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
    deleteAndNullPtr( cache_ );
    if ( sucess && horsection_ )
	horsection_->forceupdate_ =  true;

    return sucess;
}

} // namespace visBase
