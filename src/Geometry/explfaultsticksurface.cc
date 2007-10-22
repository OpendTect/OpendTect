/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : J.C. Glas
 * DATE     : October 2007
-*/

static const char* rcsID = "$Id: explfaultsticksurface.cc,v 1.2 2007-10-22 11:33:51 cvsjaap Exp $";

#include "explfaultsticksurface.h"

#include "basictask.h"
#include "faultsticksurface.h"
#include "positionlist.h"

namespace Geometry {


class ExplFaultStickSurfaceUpdater : public ParallelTask
{
public:
    ExplFaultStickSurfaceUpdater( ExplFaultStickSurface& efss,
	    			  bool updatesticksnotpanels )
    : explsurf_( efss )
    , updatesticksnotpanels_( updatesticksnotpanels )
{
    if ( updatesticksnotpanels_ )
    {
	if ( explsurf_.displaysticks_ || explsurf_.displaypanels_ )
	{
	    for ( int idx=0; idx<explsurf_.sticks_.size(); idx++ )
	    {
		if ( explsurf_.sticks_[idx]->coordindices_.isEmpty() )
		    updatelist_ += idx;
	    }
	}
    }
    else if ( explsurf_.displaypanels_ )
    {
	for ( int idx=0; idx<explsurf_.panels_.size(); idx++ )
	{
	    if ( explsurf_.panels_[idx]->isEmpty() )
		updatelist_ += idx;
	}
    }
}


int nrTimes() const { return updatelist_.size(); }

int minThreadSize() const { return 1; }


bool doWork( int start, int stop, int )
{
    for ( int idx=start; idx<=stop && idx<updatelist_.size(); idx++ )
    {
	if ( updatesticksnotpanels_ )
	    explsurf_.fillStick( updatelist_[idx] );
	else
	    explsurf_.fillPanel( updatelist_[idx] );
    }
    return true;
}

protected:

    ExplFaultStickSurface&	explsurf_;
    bool			updatesticksnotpanels_;
    TypeSet<int>		updatelist_;
};


ExplFaultStickSurface::ExplFaultStickSurface( FaultStickSurface* surf )
    : surface_( 0 )
    , displaysticks_( true )
    , displaypanels_( true )
{
    setSurface( surface_ );
}


ExplFaultStickSurface::~ExplFaultStickSurface()
{
    setSurface( 0 );
}


void ExplFaultStickSurface::setSurface( FaultStickSurface* fss )
{
    if ( surface_ )
    {
	surface_->nrpositionnotifier.remove(
			mCB(this,ExplFaultStickSurface,surfaceChange) );
	surface_->unRef();
    }

    removeAll();
    surface_ = fss;

    if ( surface_ )
    {
	surface_->ref();
	insertAll();
	update();
	surface_->nrpositionnotifier.notify(
			mCB(this,ExplFaultStickSurface,surfaceChange) );
    }
}


void ExplFaultStickSurface::removeAll()
{
    for ( int idx=sticks_.size()-1; idx>=0; idx-- )
    {
	if ( idx ) removePanel( idx-1 );
	removeStick( idx );
    }
}


void ExplFaultStickSurface::insertAll()
{
    for ( int idx=0; surface_ && idx<surface_->rowRange().nrSteps(); idx++ )
    {
	insertStick( idx );
	if ( idx ) insertPanel( idx-1 );
    }
}


void ExplFaultStickSurface::updateAll()
{
    removeAll();
    insertAll();
    update();
}


void ExplFaultStickSurface::update()
{
    ExplFaultStickSurfaceUpdater* updater; 
    updater = new ExplFaultStickSurfaceUpdater( *this, true );
    updater->execute();
    delete updater;
    updater = new ExplFaultStickSurfaceUpdater( *this, false );
    updater->execute();
    delete updater;
}


void ExplFaultStickSurface::display( bool ynsticks, bool ynpanels )
{
    for ( int idx=0; idx<sticks_.size(); idx++ )
    {
	if ( displaysticks_==ynsticks || sticks_[idx]->coordindices_.isEmpty() )
	    continue;

	if ( ynsticks )
	    addToGeometries( sticks_[idx] );
	else
	    removeFromGeometries( sticks_[idx] );
    }
    displaysticks_ = ynsticks;

    for ( int idx=0; idx<panels_.size(); idx++ )
    {
	if ( displaypanels_==ynpanels || panels_[idx]->isEmpty() )
	    continue;

	if ( ynpanels )
	    addToGeometries( *panels_[idx] );
	else
	    removeFromGeometries( (*panels_[idx])[0], panels_[idx]->size() );
    }
    displaypanels_ = ynpanels;

    update();
}


void ExplFaultStickSurface::addToGeometries( IndexedGeometry* piece )
{
    geometrieslock_.writeLock();
    geometries_ += piece;
    geometrieslock_.writeUnLock();
}


void ExplFaultStickSurface::addToGeometries( ObjectSet<IndexedGeometry>& pieces)
{
    geometrieslock_.writeLock();
    
    for ( int idx=0; idx<pieces.size(); idx++ )
	geometries_ += pieces[idx];

    geometrieslock_.writeUnLock();
}


void ExplFaultStickSurface::removeFromGeometries( const IndexedGeometry* first,
						  int total )
{
    geometrieslock_.writeLock();
    const int firstidx = geometries_.indexOf( first );
    const int lastidx = firstidx + total - 1;

    if ( firstidx>=0 && lastidx<geometries_.size() )
	geometries_.remove( firstidx, lastidx );

    geometrieslock_.writeUnLock();
}


void ExplFaultStickSurface::emptyStick( int stickidx )
{
    if ( sticks_.validIdx(stickidx) )
    {
	TypeSet<int>& knots = sticks_[stickidx]->coordindices_;

	if ( displaysticks_ && !knots.isEmpty() )
	    removeFromGeometries( sticks_[stickidx] );

	if ( coordlist_ )
	{
	    for ( int idx=0; idx<knots.size(); idx++ )
		coordlist_->remove( knots[idx] );
	}
	knots.erase(); 
    }
}


void ExplFaultStickSurface::fillStick( int stickidx )
{
    if ( !surface_ || !coordlist_ || !sticks_.validIdx(stickidx) )
	return;

    TypeSet<int>& knots = sticks_[stickidx]->coordindices_;
    if ( !knots.isEmpty() )
	return;

    const int sticknr = surface_->rowRange().atIndex( stickidx );
    const StepInterval<int> colrg = surface_->colRange( sticknr );
    
    for ( int knotnr=colrg.start; knotnr<=colrg.stop; knotnr+=colrg.step )
    {
	const Coord3 pos = surface_->getKnot( RowCol(sticknr,knotnr) );
	knots += coordlist_->add( pos );
    }

    if ( displaysticks_ )
	addToGeometries( sticks_[stickidx] );
}


void ExplFaultStickSurface::removeStick( int stickidx )
{
    if ( sticks_.validIdx(stickidx) )
    {
	emptyStick( stickidx );
	delete sticks_.remove( stickidx );
    }
}


void ExplFaultStickSurface::insertStick( int stickidx )
{
    if ( stickidx>=0 || stickidx<=sticks_.size() )
	sticks_.insertAt(new IndexedGeometry(IndexedGeometry::Lines), stickidx);
}


void ExplFaultStickSurface::emptyPanel( int panelidx )
{
    if ( panels_.validIdx(panelidx) )
    {
	ObjectSet<IndexedGeometry>& panel = *panels_[panelidx];

	if ( displaypanels_ && !panel.isEmpty() )
	    removeFromGeometries( panel[0], panel.size() );

	deepErase( panel );
    }
}


#define mSqDist( coordidx1, coordidx2 ) \
    coordlist_->get(coordidx1).sqDistTo( coordlist_->get(coordidx2) ) 

#define mNextKnotLeft( lindex, rindex ) \
    ( rindex>=rsize-1 ? true : \
    ( lindex>=lsize-1 ? false : \
			mSqDist( (*lknot)[lindex+1], (*rknot)[rindex] ) \
			 <= mSqDist( (*lknot)[lindex], (*rknot)[rindex+1] ) ))

#define mAddKnotIndex( rl, offset ) \
    if ( rl##idx+offset < rl##size ) \
	piece.coordindices_ += (*rl##knot)[rl##idx+offset];

void ExplFaultStickSurface::fillPanel( int panelidx )
{
    if ( !coordlist_ || !normallist_ || !panels_.validIdx(panelidx) )
	return;

    ObjectSet<IndexedGeometry>& panel = *panels_[panelidx];
    TypeSet<int>* lknot = &sticks_[panelidx]->coordindices_;
    TypeSet<int>* rknot = &sticks_[panelidx+1]->coordindices_;
    
    int lsize = lknot->size(); 
    int rsize = rknot->size();

    if ( !panel.isEmpty() || !lsize || !rsize )
	return;

    if ( lsize==1 && rsize==1 )
    {
	panel += new IndexedGeometry( IndexedGeometry::Lines );
	panel[0]->coordindices_ += (*lknot)[0];
	panel[0]->coordindices_ += (*rknot)[0];
	if ( displaypanels_ )
	    addToGeometries( panel );
	return;
    }

    const float sum1 = mSqDist( (*lknot)[0], (*rknot)[0] ) +
		       mSqDist( (*lknot)[lsize-1], (*rknot)[rsize-1] );
    const float sum2 = mSqDist( (*lknot)[0], (*rknot)[rsize-1] ) +
		       mSqDist( (*lknot)[lsize-1], (*rknot)[0] );
    TypeSet<int> lknotreversed;
    if ( sum1>sum2 )
    {
	lknot = &lknotreversed;
	for ( int idx=lsize-1; idx>=0; idx-- )
	    lknotreversed += (*lknot)[idx];
    }

    int lidx=0; int ridx=0;
    bool mirrored = false;
	
    while ( lidx<lsize-1 || ridx<rsize-1 ) 
    {
	if ( !mNextKnotLeft(lidx,ridx) )
	{
	    int tmpint;
	    TypeSet<int>* tmpset;
	    mSWAP( lknot, rknot, tmpset );
	    mSWAP( lsize, rsize, tmpint );
	    mSWAP( lidx, ridx, tmpint );
	    mirrored = !mirrored;
	}

	panel += new IndexedGeometry( IndexedGeometry::TriangleStrip,
				      IndexedGeometry::PerFace,0,normallist_ );
	IndexedGeometry& piece = *panel[panel.size()-1];
	bool finalknotleft = true;
	
	if ( mNextKnotLeft(lidx+1,ridx) )
	{
	    if ( mNextKnotLeft(lidx+2,ridx) )
	    {
		piece.type_ = IndexedGeometry::TriangleFan;
		mAddKnotIndex( r, 0 );
		mAddKnotIndex( l, 0 );
		mAddKnotIndex( l, 1 );
		mAddKnotIndex( l, 2 );
		mAddKnotIndex( l, 3 );
		lidx+=3; 
	    }
	    else
	    {
		mAddKnotIndex( l, 0 );
		mAddKnotIndex( l, 1 );
		mAddKnotIndex( r, 0 );
		mAddKnotIndex( l, 2 );
		mAddKnotIndex( r, 1 );
		lidx+=2; ridx++;
	    }
	    finalknotleft = false;
	    mirrored = !mirrored;
	}
	else if ( mNextKnotLeft(lidx+1,ridx+1) )
	{
	    mAddKnotIndex( l, 0 );
	    mAddKnotIndex( r, 0 );
	    mAddKnotIndex( l, 1 );
	    mAddKnotIndex( r, 1 );
	    mAddKnotIndex( l, 2 );
	    lidx+=2; ridx++;
	}
	else
	{
	    piece.type_ = IndexedGeometry::TriangleFan;
	    mAddKnotIndex( l, 1 );
	    mAddKnotIndex( l, 0 );
	    mAddKnotIndex( r, 0 );
	    mAddKnotIndex( r, 1 );
	    mAddKnotIndex( r, 2 );
	    lidx++; ridx+=2;
	}

	while ( lidx<lsize-1 || ridx<rsize-1 )
	{
	    if ( mNextKnotLeft(lidx,ridx) )
	    {
		mAddKnotIndex( l, 1 );
		lidx++;
		if ( finalknotleft ) break;
	    }
	    else
	    {
		mAddKnotIndex( r, 1 );
		ridx++;
		if ( !finalknotleft ) break;
	    }
	    
	    if ( piece.type_ == IndexedGeometry::TriangleStrip )
		finalknotleft = !finalknotleft;
	}

	calcNormals( piece, mirrored );
    }

    if ( displaysticks_ )
	addToGeometries( panel );
}


void ExplFaultStickSurface::removePanel( int panelidx )
{
    if ( panels_.validIdx(panelidx) )
    {
	emptyPanel( panelidx );
	delete panels_.remove( panelidx );
    }
}


void ExplFaultStickSurface::insertPanel( int panelidx )
{
    if ( panelidx>=0 || panelidx<=panels_.size() )
	panels_.insertAt( new ObjectSet<IndexedGeometry>, panelidx );
}


void ExplFaultStickSurface::calcNormals( IndexedGeometry& piece, bool mirrored )
{
    if ( !coordlist_ || !normallist_ || piece.type_==IndexedGeometry::Lines )
	return;

    for ( int idx=2; idx<piece.coordindices_.size(); idx++ )
    {
	Coord3 vec1 = coordlist_->get( piece.coordindices_[idx-1] );
	Coord3 vec2 = vec1 - coordlist_->get( piece.coordindices_[idx] );

	if ( piece.type_ == IndexedGeometry::TriangleStrip )
	{
	    vec1 -= coordlist_->get( piece.coordindices_[idx-2] );
	    if ( idx%2 == 1 )
		vec1 = -vec1;
	}
	else 
	    vec1 -= coordlist_->get( piece.coordindices_[0] );

        Coord3 normal = vec1.cross( vec2 );
	if ( mirrored != righthandednormals_ )
	    normal = -normal;

	if ( !normal.sqAbs() )
	    normal = Coord3( 1, 0, 0 );	    
	
	piece.normalindices_ += normallist_->add( normal );
    }
}


void ExplFaultStickSurface::surfaceChange( CallBacker* cb )
{
    mCBCapsuleUnpack( const TypeSet<GeomPosID>*, pidlist, cb );
    for ( int idx=0; pidlist && idx<pidlist->size(); idx++ )
    {
	RowCol rc;
	rc.setSerialized( (*pidlist)[idx] );
	const int stickidx = surface_->rowRange().getIndex( rc.r() );

	if ( rc.c()==FaultStickSurface::StickChange )
	{
	    emptyPanel( stickidx-1 );
	    emptyPanel( stickidx );
	    emptyStick( stickidx );
	}
	if ( rc.c()==FaultStickSurface::StickInsert )
	{
	    emptyPanel( stickidx-1 );
	    insertPanel( stickidx );
	    insertStick( stickidx );
	}
	if ( rc.c()==FaultStickSurface::StickRemove )
	{
	    emptyPanel( stickidx-1 );
	    removePanel( stickidx );
	    removeStick( stickidx );
	}
    }
    update();
}

}; // namespace Geometry
