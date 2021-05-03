/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/


#include "vishortilescreatorandupdator.h"
#include "vishorizonsection.h"
#include "vishorizonsectiontile.h"
#include "vishordatahandler.h"
#include "vishorizonsectiondef.h"
#include "vishorthreadworks.h"
#include "vishorizontexturehandler.h"
#include "simpnumer.h"
#include "mousecursor.h"

#include "binidsurface.h"
#include "thread.h"
#include "ranges.h"
#include "task.h"

#include <osg/Switch>
#include <osgGeo/LayeredTexture>


namespace visBase
{

#define cFullResolution 0

HorTilesCreatorAndUpdator::HorTilesCreatorAndUpdator(HorizonSection* horsection)
:   horsection_( horsection )
{}

HorTilesCreatorAndUpdator::~HorTilesCreatorAndUpdator()
{}


void HorTilesCreatorAndUpdator::updateTiles( const TypeSet<GeomPosID>* gpids,
				      TaskRunner* tr )
{
    if (!horsection_) return;
    mDefineRCRange( horsection_,-> );

    if ( rrg.width(false)<0 || crg.width(false)<0 )
	return;

    horsection_->tesselationlock_ = true;
    updateTileArray( rrg, crg );

    const int nrrowsz = horsection_->tiles_.info().getSize(0);
    const int nrcolsz = horsection_->tiles_.info().getSize(1);

    ObjectSet<HorizonSectionTile> oldupdatetiles;
    TypeSet<RowCol> fullupdatetiles;
    TypeSet<RowCol> tileindexes;

    for ( int idx=(*gpids).size()-1; idx>=0; idx-- )
    {
	const RowCol absrc = RowCol::fromInt64( (*gpids)[idx] );
	RowCol rc = absrc - horsection_->origin_;
	const int tilesidesize = horsection_->tilesidesize_;

	rc.row() /= rrg.step; rc.col() /= crg.step;
	int tilerowidx = rc.row()/tilesidesize;
	int tilerow = rc.row()%tilesidesize;
	if ( tilerowidx==nrrowsz && !tilerow )
	{
	    tilerowidx--;
	    tilerow =  tilesidesize;
	}

	int tilecolidx = rc.col()/tilesidesize;
	int tilecol = rc.col()%tilesidesize;
	if ( tilecolidx==nrcolsz && !tilecol )
	{
	    tilecolidx--;
	    tilecol =  tilesidesize;
	}

	/*If we already set work area and the position is out of the area,
	  we will skip the position. */
	if ( tilerowidx>=nrrowsz || tilecolidx>=nrcolsz )
	    continue;

	const Coord3 pos = horsection_->geometry_->getKnot(absrc,false);

	bool addoldtile = false;
	if ( !horsection_->tiles_.info().validPos(tilerowidx,tilecolidx) )
	    continue;

	HorizonSectionTile* tile = horsection_->tiles_.get(
						    tilerowidx, tilecolidx );
	if ( !tile )
	{
	    const RowCol step( rrg.step, crg.step );
	    const RowCol tileorigin( horsection_->origin_.row() +
		tilerowidx*horsection_->tilesidesize_*step.row(),
		horsection_->origin_.col() +
		tilecolidx*horsection_->tilesidesize_*step.col() );

	    if ( fullupdatetiles.indexOf(tileorigin) !=-1 )
		continue;

	    fullupdatetiles += tileorigin;
	    tileindexes += RowCol(tilerowidx,tilecolidx);
	}
	else if ( fullupdatetiles.indexOf(tile->origin_)==-1 )
	{
	    tile->setPos( tilerow, tilecol, pos );
	    if ( horsection_->desiredresolution_!=-1 )
	    {
		addoldtile = true;
		if ( oldupdatetiles.indexOf(tile)==-1 )
		    oldupdatetiles += tile;
	    }
	}

	for ( int rowidx=-1; rowidx<=1; rowidx++ ) //Update neighbors
	{
	    const int nbrow = tilerowidx+rowidx;
	    if ( nbrow<0 || nbrow>=nrrowsz ) continue;

	    for ( int colidx=-1; colidx<=1; colidx++ )
	    {
		const int nbcol = tilecolidx+colidx;
		if ( (!rowidx && !colidx) || nbcol<0 || nbcol>=nrcolsz )
		    continue;

		HorizonSectionTile* nbtile = horsection_->tiles_.get(
		    nbrow, nbcol );
		if ( !nbtile || fullupdatetiles.indexOf(nbtile->origin_) !=-1 )
		    continue;

		nbtile->setPos( tilerow-rowidx*tilesidesize,
			tilecol-colidx*tilesidesize, pos );

		if ( !addoldtile ||
		    rowidx+colidx>=0 ||
		    horsection_->desiredresolution_==-1  ||
		    oldupdatetiles.indexOf(nbtile)!=-1 )
		    continue;

		if ( (!tilecol && !rowidx && colidx==-1) ||
			(!tilerow && rowidx==-1 &&
			 ((!tilecol && colidx==-1) || !colidx)) )
		    oldupdatetiles += nbtile;
	    }
	}
    }

    horsection_->setUpdateVar( horsection_->forceupdate_,  false );

    HorizonSectionTilePosSetup postask( fullupdatetiles, tileindexes,
	horsection_, rrg, crg );
    postask.setTesselationResolution( cFullResolution );

    TaskRunner::execute( tr, postask );

    for ( int idx = 0; idx< fullupdatetiles.size(); idx++ )
    {
	const int ridx = tileindexes[idx].row();
	const int cidx = tileindexes[idx].col();
	HorizonSectionTile* tile = horsection_->tiles_.get( ridx, cidx );
	if ( !tile || !fullupdatetiles.isPresent(tile->origin_) )
	    continue;
	setNeighbors( tile, ridx, cidx );
	horsection_->osghorizon_->addChild( tile->osgswitchnode_ );
	tile->addTileGlueTesselator();
	tile->setResolution( cFullResolution );
    }


    //Only for fixed resolutions, which won't be tessellated at render.
    if ( oldupdatetiles.size() )
    {
	TypeSet<Threads::Work> work;
	for ( int idx=0; idx<oldupdatetiles.size(); idx++ )
	{
	    TileTesselator* tt = new TileTesselator(
		oldupdatetiles[idx], horsection_->desiredresolution_ );
	    work += Threads::Work( *tt, true );
	}

	Threads::WorkManager::twm().addWork( work,
	       Threads::WorkManager::cDefaultQueueID() );
    }

    horsection_->tesselationlock_ = true;
    horsection_->setUpdateVar( horsection_->forceupdate_, true );
}


void HorTilesCreatorAndUpdator::updateTileArray( const StepInterval<int>& rrg,
				      const StepInterval<int>& crg )
{
    const int rowsteps = horsection_->tilesidesize_ * rrg.step;
    const int colsteps = horsection_->tilesidesize_ * crg.step;
    const int oldrowsize = horsection_->tiles_.info().getSize(0);
    const int oldcolsize = horsection_->tiles_.info().getSize(1);
    int newrowsize = oldrowsize;
    int newcolsize = oldcolsize;
    int nrnewrowsbefore = 0;
    int nrnewcolsbefore = 0;

    int diff = horsection_->origin_.row() - rrg.start;
    if ( diff>0 )
    {
	nrnewrowsbefore = diff/rowsteps + ( diff%rowsteps ? 1 : 0 );
	newrowsize += nrnewrowsbefore;
    }

    diff = rrg.stop - ( horsection_->origin_.row()+oldrowsize*rowsteps );
    if ( diff>0 ) newrowsize += diff/rowsteps + ( diff%rowsteps ? 1 : 0 );

    diff = horsection_->origin_.col() - crg.start;
    if ( diff>0 )
    {
	nrnewcolsbefore = diff/colsteps + ( diff%colsteps ? 1 : 0 );
	newcolsize += nrnewcolsbefore;
    }

    diff = crg.stop - ( horsection_->origin_.col()+oldcolsize*colsteps );
    if ( diff>0 ) newcolsize += diff/colsteps + ( diff%colsteps ? 1 : 0 );

    if ( newrowsize==oldrowsize && newcolsize==oldcolsize )
	return;

    Array2DImpl<HorizonSectionTile*> newtiles( newrowsize, newcolsize );
    newtiles.setAll( 0 );

    for ( int rowidx=0; rowidx<oldrowsize; rowidx++ )
    {
	const int targetrow = rowidx+nrnewrowsbefore;
	for ( int colidx=0; colidx<oldcolsize; colidx++ )
	{
	    const int targetcol = colidx+nrnewcolsbefore;
	    newtiles.set( targetrow, targetcol,
		horsection_->tiles_.get(rowidx,colidx) );
	}
    }

    horsection_->writeLock();
    horsection_->tiles_.copyFrom( newtiles );
    horsection_->origin_.row() -= nrnewrowsbefore*rowsteps;
    horsection_->origin_.col() -= nrnewcolsbefore*colsteps;
    horsection_->writeUnLock();
}


HorizonSectionTile* HorTilesCreatorAndUpdator::createOneTile( int tilerowidx,
							  int tilecolidx )
{
    mDefineRCRange( horsection_,-> );

    const RowCol step( rrg.step, crg.step );
    const RowCol tileorigin( horsection_->origin_.row() +
		 tilerowidx*horsection_->tilesidesize_*step.row(),
		 horsection_->origin_.col() +
		 tilecolidx*horsection_->tilesidesize_*step.col() );

    HorizonSectionTile* tile = new HorizonSectionTile(*horsection_, tileorigin);

    tile->setResolution( horsection_->desiredresolution_ );

    horsection_->writeLock();
    horsection_->tiles_.set( tilerowidx, tilecolidx, tile );
    horsection_->osghorizon_->addChild( tile->osgswitchnode_ );
    horsection_->writeUnLock();


    return tile;
}


void HorTilesCreatorAndUpdator::setNeighbors( HorizonSectionTile* tile,
    int tilerowidx, int tilecolidx )
{
    for ( int rowidx=-1; rowidx<=1; rowidx++ )
    {
	const int neighborrow = tilerowidx+rowidx;
	if (neighborrow<0 || neighborrow>=horsection_->tiles_.info().getSize(0))
	    continue;

	for ( int colidx=-1; colidx<=1; colidx++ )
	{
	    if ( !colidx && !rowidx )
		continue;

	    const int neighborcol = tilecolidx+colidx;
	    if ( neighborcol<0 ||
		 neighborcol>=horsection_->tiles_.info().getSize(1) )
		continue;
	    HorizonSectionTile* neighbor =
		horsection_->tiles_.get(neighborrow,neighborcol);

	    if ( !neighbor ) continue;

	    char pos;
	    if ( colidx==-1 )
		pos = rowidx==-1 ? LEFTUPTILE :
		( !rowidx ? LEFTTILE : LEFTBOTTOMTILE );
	    else if ( colidx==0 )
		pos = rowidx==-1 ? UPTILE :
		( !rowidx ? THISTILE : BOTTOMTILE );
	    else
		pos = rowidx==-1 ? RIGHTUPTILE :
		( !rowidx ? RIGHTTILE : RIGHTBOTTOMTILE );

	    tile->setNeighbor( pos, neighbor );

	    if ( colidx==1 )
		pos = rowidx==1 ? LEFTUPTILE :
		( !rowidx ? LEFTTILE : LEFTBOTTOMTILE);
	    else if ( colidx==0 )
		pos = rowidx==1 ? UPTILE :
		( !rowidx ? THISTILE : BOTTOMTILE);
	    else
		pos = rowidx==1 ? RIGHTUPTILE :
		( !rowidx ? RIGHTTILE : RIGHTBOTTOMTILE);

	    neighbor->setNeighbor( pos, tile );
	}
    }
}


void HorTilesCreatorAndUpdator::createAllTiles( TaskRunner* tr )
{
    mDefineRCRange( horsection_,-> );

    if ( rrg.width(false)<0 || crg.width(false)<0 )
	return;

    horsection_->tesselationlock_ = true;
    horsection_->origin_ = RowCol( rrg.start, crg.start );
    const int nrrows = nrBlocks( rrg.nrSteps()+1,
				 horsection_->nrcoordspertileside_, 1 );
    const int nrcols = nrBlocks( crg.nrSteps()+1,
				 horsection_->nrcoordspertileside_, 1 );

    horsection_->writeLock();
    if ( !horsection_->tiles_.setSize( nrrows, nrcols ) )
    {
	horsection_->tesselationlock_ = false;
	horsection_->writeUnLock();
	return;
    }

    TypeSet<RowCol> createtiles;
    TypeSet<RowCol> tileindexes;

    horsection_->tiles_.setAll( 0 );
    horsection_->writeUnLock();

    ObjectSet<HorizonSectionTile> newtiles;
    for ( int tilerowidx=0; tilerowidx<nrrows; tilerowidx++ )
    {
	for ( int tilecolidx=0; tilecolidx<nrcols; tilecolidx++ )
	{
	    const RowCol step(rrg.step,crg.step);
	    const RowCol tileorigin(horsection_->origin_.row() +
		tilerowidx*horsection_->tilesidesize_*step.row(),
		horsection_->origin_.col() +
		tilecolidx*horsection_->tilesidesize_*step.col());
	    createtiles += tileorigin;
	    tileindexes += RowCol( tilerowidx, tilecolidx );
	}
    }

    horsection_->setUpdateVar( horsection_->forceupdate_,  false );

    HorizonSectionTilePosSetup postask( createtiles, tileindexes,
	horsection_, rrg, crg );
    TaskRunner::execute(tr,postask);

    HorizonSectionTile** tileptrs = horsection_->tiles_.getData();
    int tidx = 0;
    for ( int tilerowidx = 0; tilerowidx<nrrows; tilerowidx++ )
    {
	for ( int tilecolidx = 0; tilecolidx<nrcols; tilecolidx++ )
	{
	    HorizonSectionTile* tile = tileptrs[tidx];
	    if ( tile )
	    {
		setNeighbors( tile, tilerowidx, tilecolidx );
		horsection_->osghorizon_->addChild(tile->osgswitchnode_ );
	    }
	    tidx++;
	}
    }

    horsection_->setUpdateVar( horsection_->forceupdate_,  true );
    horsection_->tesselationlock_ = false;

}


void HorTilesCreatorAndUpdator::updateTilesAutoResolution(
						  const osg::CullStack* cs )
{
    HorizonTileRenderPreparer task( *horsection_, cs,
				    horsection_->desiredresolution_ );
    task.execute();

    const int tilesz = horsection_->tiles_.info().getTotalSz();
    if ( !tilesz ) return;

    HorizonSectionTile** tileptrs = horsection_->tiles_.getData();

    spinlock_.lock();
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( tileptrs[idx] )
	{
	    tileptrs[idx]->applyTesselation(
		tileptrs[idx]->getActualResolution() );
	}
    }
    spinlock_.unLock();

}


void HorTilesCreatorAndUpdator::updateTilesPrimitiveSets()
{
    const int tilesz = horsection_->tiles_.info().getTotalSz();

    if ( tilesz )
    {
	HorizonSectionTile** tileptrs = horsection_->tiles_.getData();
	Threads::MutexLocker updatemutex( updatelock_ );
	// below process takes some time
	for ( int idx=0; idx<tilesz; idx++ )
	{
	    if ( tileptrs[idx] )
		tileptrs[idx]->updatePrimitiveSets();
	}
    }

    horsection_->setUpdateVar( horsection_->forceupdate_, false );
}


void HorTilesCreatorAndUpdator::setFixedResolution( char res, TaskRunner* tr )
{
    const int tilesz = horsection_->tiles_.info().getTotalSz();
    if ( !tilesz ) return;

    HorizonSectionTile** tileptrs = horsection_->tiles_.getData();
    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->setResolution( res );

    if ( res==cNoneResolution )
	return;

    TypeSet<Threads::Work> work;
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( !tileptrs[idx] )
	    continue;

	tileptrs[idx]->setActualResolution( res );
	tileptrs[idx]->turnOnGlue( res==-1 );
	work += Threads::Work(
	    *new TileTesselator( tileptrs[idx], res ), true );
    }

    Threads::WorkManager::twm().addWork( work,
	Threads::WorkManager::cDefaultQueueID() );
}

} // namespace visBase
