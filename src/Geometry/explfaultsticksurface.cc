/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : J.C. Glas
 * DATE     : October 2007
-*/

static const char* rcsID = "$Id: explfaultsticksurface.cc,v 1.17 2008-05-29 13:54:09 cvskris Exp $";

#include "explfaultsticksurface.h"

#include "arrayndinfo.h"
#include "task.h"
#include "faultsticksurface.h"
#include "positionlist.h"
#include "sorting.h"

namespace Geometry {


class ExplFaultStickTexturePositionExtracter : public ParallelTask
{
public:
ExplFaultStickTexturePositionExtracter( ExplFaultStickSurface& efss )
    : explsurf_( efss )
    , sz_( efss.getTextureSize() )
{}

    
int totalNr() const
{
    const RowCol& sz = explsurf_.getTextureSize();
    return sz.row * sz.col;
}


int minThreadSize() const { return 100; }

#define mStick	0
#define mKnot	1

bool doWork( int start, int stop, int )
{
    const RowCol& sz = explsurf_.getTextureSize();
    const Array2DInfoImpl ainfo( sz.row, sz.col );

    int curpos[2];
    if ( !ainfo.getArrayPos( start, curpos ) )
	return false;

    ArrayNDIter iter( ainfo );
    iter.setPos( curpos );

    const TypeSet<int>& texturerows = explsurf_.texturerows_;

    for ( int idx=start; idx<=stop; idx++, reportNrDone() )
    {
	int panelidx = -1;
	int stickidx = -1;
	for ( int idy=0; idy<texturerows.size()-1; idy++ )
	{
	    if ( texturerows[idy]==curpos[mStick] )
	    {
		panelidx = -1;
		stickidx = idy;
		break;
	    }

	    if ( texturerows[idy+1]>curpos[mStick] )
	    {
		panelidx = idy;
		break;
	    }
	}

	if ( stickidx!=-1 )
	    processPixelOnStick( stickidx, curpos[mKnot] );
	else if ( panelidx!=-1 )
	    processPixelOnPanel( panelidx, curpos[mStick], curpos[mKnot] );
	else
	    return false;

	if ( idx<stop && !iter.next() )
	    return false;
    }

    return true;
}


bool processPixelOnStick( int i, int j )
{
    return true;
}


bool processPixelOnPanel( int panelidx, int i, int j )
{
    return true;
}

    ExplFaultStickSurface&	explsurf_;
    RowCol			sz_;

};


#undef mStick
#undef mKnot


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
	for ( int idx=0; idx<explsurf_.paneltriangles_.size()-1; idx++ )
	{
	    if ( explsurf_.paneltriangles_[idx] &&
		 !explsurf_.paneltriangles_[idx]->isEmpty() )
		continue;

	    if ( explsurf_.panellines_[idx] &&
		 !explsurf_.panellines_[idx]->isEmpty() )
		continue;

	    updatelist_ += idx;
	}
    }
}


int totalNr() const { return updatelist_.size(); }

int minThreadSize() const { return 1; }


bool doWork( int start, int stop, int )
{
    for ( int idx=start; idx<=stop && idx<updatelist_.size();
	  idx++, reportNrDone() )
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


ExplFaultStickSurface::ExplFaultStickSurface( FaultStickSurface* surf,
					      float zscale )
    : surface_( 0 )
    , displaysticks_( true )
    , displaypanels_( true )
    , scalefacs_( 1, 1, zscale )
    , needsupdate_( true )
    , maximumtexturesize_( 1024, 1024 )
    , texturesize_( mUdf(int), mUdf(int) )
    , textrurepot_( true )
{
    paneltriangles_.allowNull( true );
    panellines_.allowNull( true );
    setSurface( surf );
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
	surface_->movementnotifier.remove(
			mCB(this,ExplFaultStickSurface,surfaceMovement) );
    }

    removeAll();
    surface_ = fss;

    if ( surface_ )
    {
	surface_->nrpositionnotifier.notify(
			mCB(this,ExplFaultStickSurface,surfaceChange) );
	surface_->movementnotifier.notify(
			mCB(this,ExplFaultStickSurface,surfaceMovement) );

	if ( coordlist_ )
	{
	    insertAll();
	}
    }
}


void ExplFaultStickSurface::setZScale( float zscale )
{
    scalefacs_.z = zscale;
    for ( int idx=sticks_.size()-2; idx>=0; idx-- )
	emptyPanel( idx );
}


void ExplFaultStickSurface::removeAll()
{
    for ( int idx=sticks_.size()-1; idx>=0; idx-- )
    {
	removePanel( idx );
	removeStick( idx );
    }
}


void ExplFaultStickSurface::insertAll()
{
    if ( !surface_ || surface_->isEmpty() )
	return;

    for ( int idx=0; idx<=surface_->rowRange().nrSteps(); idx++ )
    {
	insertStick( idx );
	insertPanel( idx );
    }
}


bool ExplFaultStickSurface::update( bool forceall, TaskRunner* tr )
{
    if ( forceall )
    {
	removeAll();
	insertAll();
    }

    //First update the sticks since they are needed for the panels
    PtrMan<ExplFaultStickSurfaceUpdater> updater =
	new ExplFaultStickSurfaceUpdater( *this, true );

    if ( (tr && !tr->execute( *updater ) ) || !updater->execute() )
	return false;

    if ( !updateTextureSize() )
	return false;

    //Now do panels
    updater = new ExplFaultStickSurfaceUpdater( *this, false );

    if ( (tr && !tr->execute( *updater ) ) || !updater->execute() )
	return false;

    needsupdate_ = false;
    return true;
}


void ExplFaultStickSurface::display( bool ynsticks, bool ynpanels )
{
    if ( displaysticks_!=ynsticks )
    {
	for ( int idx=0; idx<sticks_.size(); idx++ )
	{
	    if ( ynsticks )
		addToGeometries( sticks_[idx] );
	    else
		removeFromGeometries( sticks_[idx] );
	}
    }

    displaysticks_ = ynsticks;

    if ( displaypanels_!=ynpanels )
    {
	for ( int idx=0; idx<paneltriangles_.size(); idx++ )
	{
	    if ( ynpanels )
	    {
		addToGeometries( paneltriangles_[idx] );
		addToGeometries( panellines_[idx] );
	    }
	    else
	    {
		removeFromGeometries( paneltriangles_[idx] );
		removeFromGeometries( panellines_[idx] );
	    }
	}
    }

    displaypanels_ = ynpanels;
}


void ExplFaultStickSurface::setMaximumTextureSize( const RowCol& rc )
{ maximumtexturesize_=rc; }


void ExplFaultStickSurface::setTexturePowerOfTwo( bool yn )
{ textrurepot_ = yn; }


const RowCol& ExplFaultStickSurface::getTextureSize() const
{ return texturesize_; }


DataPack::ID ExplFaultStickSurface::getDataPointSet() const
{ return DataPack::cNoID; }


void ExplFaultStickSurface::addToGeometries( IndexedGeometry* ig )
{
    if ( !ig ) return;
    geometrieslock_.writeLock();
    if ( geometries_.indexOf( ig )!=-1 )
    {
	pErrMsg("Adding more than once");
    }
    ig->ischanged_ = true;
    geometries_ += ig;
    geometrieslock_.writeUnLock();
}


void ExplFaultStickSurface::removeFromGeometries( const IndexedGeometry* ig )
{
    if ( !ig ) return;
    geometrieslock_.writeLock();
    const int idx = geometries_.indexOf( ig );

    if ( idx!=-1 )
	geometries_.removeFast( idx );

    geometrieslock_.writeUnLock();
}


void ExplFaultStickSurface::emptyStick( int stickidx )
{
    if ( !sticks_.validIdx(stickidx) )
	return;

    sticks_[stickidx]->removeAll();

    needsupdate_ = true;
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

    sticks_[stickidx]->ischanged_ = true;
}


void ExplFaultStickSurface::removeStick( int stickidx )
{
    if ( !sticks_.validIdx(stickidx) )
	return;

    removeFromGeometries( sticks_[stickidx] );
    delete sticks_.remove( stickidx );
    needsupdate_ = true;
}


void ExplFaultStickSurface::insertStick( int stickidx )
{
    if ( stickidx<0 || stickidx>sticks_.size() )
	return;

    sticks_.insertAt(
	new IndexedGeometry(IndexedGeometry::Lines,
	    IndexedGeometry::PerFace,coordlist_,0,0),
	stickidx);

    if ( displaysticks_ ) addToGeometries( sticks_[stickidx] );

    needsupdate_ = true;
}


#define mSqDist( coordidx1, coordidx2 ) \
    coordlist_->get(coordidx1).scaleBy(scalefacs_).sqDistTo( \
    coordlist_->get(coordidx2).scaleBy(scalefacs_) ) 


void ExplFaultStickSurface::emptyPanel( int panelidx )
{
    if ( !paneltriangles_.validIdx(panelidx) )
	return;

    if ( paneltriangles_[panelidx] ) paneltriangles_[panelidx]->removeAll();
    if ( panellines_[panelidx] ) panellines_[panelidx]->removeAll();

    needsupdate_ = true;
}


#define mSqDistArr( a, b ) sqdists[(a)*rsize+(b)]

#define mAddConnection( il, ir ) \
{ \
    lconn += il; \
    rconn += ir; \
    mSqDistArr( il, ir ) = mUdf(float); \
}

#define mAddTriangle( a, b, c ) \
{									\
    triangles->coordindices_ += (a);					\
    triangles->coordindices_ += (b);					\
    triangles->coordindices_ += (c);					\
    triangles->coordindices_ += -1;					\
    Coord3 vec1 = coordlist_->get( a );					\
    const Coord3 vec2 = coordlist_->get( b )-vec1;			\
    vec1 = coordlist_->get( c ) - vec1;					\
    const Coord3 normal = righthandednormals_				\
	? vec1.cross( vec2 )						\
	: vec2.cross( vec1 );						\
    triangles->normalindices_ += normallist_->add( normal );		\
}

void ExplFaultStickSurface::fillPanel( int panelidx )
{
    if ( !coordlist_ || !normallist_ || !paneltriangles_.validIdx(panelidx) )
	return;

    IndexedGeometry* triangles = paneltriangles_[panelidx];
    IndexedGeometry* lines = panellines_[panelidx];

    if ( triangles && !triangles->isEmpty() )
	triangles->removeAll();

    if ( lines && !lines->isEmpty() )
	lines->removeAll();
    
    const TypeSet<int>& lknots = sticks_[panelidx]->coordindices_;
    const TypeSet<int>& rknots = sticks_[panelidx+1]->coordindices_;

    const int lsize = lknots.size(); 
    const int rsize = rknots.size();

    if ( !lsize || !rsize )
	return;

    if ( lsize==1 && rsize==1 )
    {
	if ( !lines )
	{
	    lines = new IndexedGeometry( IndexedGeometry::Lines,
		    IndexedGeometry::PerFace, 0, normallist_, 0 );
	    panellines_.replace( panelidx, lines );
	    if ( displaypanels_ ) addToGeometries( lines );
	}

	lines->coordindices_ += lknots[0];
	lines->coordindices_ += rknots[0];

	return;
    }

    if ( !triangles )
    {
	triangles = new IndexedGeometry( IndexedGeometry::TriangleStrip,
					 IndexedGeometry::PerFace,
					 0, normallist_, 0 );
	paneltriangles_.replace( panelidx, triangles );
	if ( displaypanels_ ) addToGeometries( triangles );
    }

    float sqdists[lsize*rsize];
    for ( int idx=0; idx<lsize; idx++ )
    {
	for ( int idy=0; idy<rsize; idy++ )
	{
	    const float sqdist = mSqDist( lknots[idx], rknots[idy] );
	    mSqDistArr( idx, idy ) =  sqdist;
	}
    }

    TypeSet<int> lconn,rconn;
    mAddConnection( 0, 0 );
    mAddConnection( lsize-1, rsize-1 );

    while ( true )
    {
	float minsqdist;
	int minl=-1, minr;
	for ( int idx=0; idx<lsize; idx++ )
	{
	    for ( int idy=0; idy<rsize; idy++ )
	    {
		const float sqdist = mSqDistArr( idx, idy );
		if ( mIsUdf(sqdist) )
		    continue;

		if ( minl!=-1 && sqdist>minsqdist )
		    continue;

		minl = idx;
		minr = idy;
		minsqdist = sqdist;
	    }
	}

	if ( minl==-1 )
	    break;

	mAddConnection( minl, minr );
	//Remove connections that are not possible anylonger
	for ( int idx=0; idx<lsize; idx++ )
	{
	    if ( idx==minl )
		continue;

	    for ( int idy=0; idy<rsize; idy++ )
	    {
		if ( idy==minr )
		    continue;

		if ( idx>minl == idy>minr )
		    continue;

		mSqDistArr( idx, idy ) = mUdf(float);
	    }
	}
    }

    for ( int lidx=0; lidx<lsize; lidx++ )
    {
	TypeSet<int> conns;
	for ( int idx=lconn.size()-1; idx>=0; idx-- )
	{
	    if ( lconn[idx]!=lidx )
		continue;

	    conns += rconn[idx];
	}

	sort_array( conns.arr(), conns.size() );

	if ( lidx )
	    mAddTriangle( lknots[lidx], rknots[conns[0]], lknots[lidx-1] );

	for ( int idx=1; idx<conns.size(); idx++ )
	    mAddTriangle(lknots[lidx],rknots[conns[idx]],rknots[conns[idx-1]]);
    }
}


void ExplFaultStickSurface::removePanel( int panelidx )
{
    if ( !paneltriangles_.validIdx(panelidx) )
	return;

    removeFromGeometries( paneltriangles_[panelidx] );
    removeFromGeometries(  panellines_[panelidx] );
    delete paneltriangles_.remove( panelidx );
    delete panellines_.remove( panelidx );
    needsupdate_ = true;
}


void ExplFaultStickSurface::insertPanel( int panelidx )
{
    if ( panelidx<0 || panelidx>paneltriangles_.size() )
	return;

    paneltriangles_.insertAt( 0, panelidx );
    panellines_.insertAt( 0, panelidx );
    needsupdate_ = true;
}


void ExplFaultStickSurface::surfaceChange( CallBacker* cb )
{
    mCBCapsuleUnpack( const TypeSet<GeomPosID>*, pidlist, cb );
    for ( int idx=0; pidlist && idx<pidlist->size(); idx++ )
    {
	RowCol rc;
	rc.setSerialized( (*pidlist)[idx] );
	const int stickidx = rc.r();

	if ( rc.c()==FaultStickSurface::StickChange )
	{
	    emptyPanel( stickidx-1 );
	    emptyPanel( stickidx );
	    emptyStick( stickidx );
	}
	if ( rc.c()==FaultStickSurface::StickInsert )
	{
	    emptyPanel( stickidx-1 );
	    insertPanel( !stickidx ? 0 : stickidx-1 );
	    insertStick( stickidx );
	}
	if ( rc.c()==FaultStickSurface::StickRemove )
	{
	    emptyPanel( stickidx );
	    removePanel( !stickidx ? 0 : stickidx-1 );
	    removeStick( stickidx );
	}
    }

    addVersion();
}


void ExplFaultStickSurface::surfaceMovement( CallBacker* cb )
{
    mCBCapsuleUnpack( const TypeSet<GeomPosID>*, pidlist, cb );
    for ( int idx=0; pidlist && idx<pidlist->size(); idx++ )
    {
	RowCol rc;
	rc.setSerialized( (*pidlist)[idx] );
	const int stickidx = rc.r();

	emptyPanel( stickidx-1 );
	emptyPanel( stickidx );
	emptyStick( stickidx );
    }
}

}; // namespace Geometry
