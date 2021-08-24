/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : October 2007
-*/


#include "explfaultsticksurface.h"

#include "arrayndinfo.h"
#include "trckeyzsampling.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "delaunay.h"
#include "executor.h"
#include "faultsticksurface.h"
#include "geometry.h"
#include "positionlist.h"
#include "posvecdataset.h"
#include "simpnumer.h"
#include "sorting.h"
#include "sortedtable.h"
#include "survinfo.h"
#include "task.h"
#include "trigonometry.h"
#include "varlenarray.h"

namespace Geometry {

#define mCoordsPerTriangle 3

class ExplFaultStickTexturePositionExtracter : public ParallelTask
{
public:
ExplFaultStickTexturePositionExtracter( ExplFaultStickSurface& efss,
					DataPointSet& dpset,
					int id=-1 )
    : explsurf_( efss )
    , dpset_( dpset )
    , sz_( efss.getTextureSize() )
    , id_(id)
{
    const DataColDef texture_i( explsurf_.sKeyTextureI() );
    const DataColDef texture_j( explsurf_.sKeyTextureJ() );
    const DataColDef iddef( sKey::ID() );

    i_column_ = dpset_.dataSet().findColDef(texture_i,PosVecDataSet::NameExact);
    j_column_ = dpset_.dataSet().findColDef(texture_j,PosVecDataSet::NameExact);
    id_column_ = dpset_.dataSet().findColDef(iddef,PosVecDataSet::NameExact);
}


od_int64 nrIterations() const	{ return explsurf_.getTextureSize().col(); }
int minThreadSize() const { return 100; }

bool doWork( od_int64 start, od_int64 stop, int )
{
    const TypeSet<int>& texturecols = explsurf_.texturecolcoords_;
    for ( int stickpos=mCast(int,start); stickpos<=stop; stickpos++ )
    {
	int stickidx = -1;
	int panelidx = -1;
	for ( int idy=0; idy<texturecols.size()-1; idy++ )
	{
	    if ( texturecols[idy]==stickpos )
	    {
		stickidx = idy;
		panelidx = -1;
		break;
	    }
	    else if ( texturecols[idy]<stickpos && texturecols[idy+1]>stickpos )
	    {
		panelidx = idy;
		break;
	    }
	    else if ( texturecols[texturecols.size()-1]==stickpos )
	    {
		stickidx = texturecols.size()-1;
		panelidx = -1;
		break;
	    }
	}

	if ( stickidx==-1 && panelidx==-1 )
	    return false;

	for( int knotpos=0; knotpos<explsurf_.getTextureSize().row(); knotpos++)
	{
	    Coord3 pos = Coord3::udf();
	    bool found = false;

	    if ( stickidx!=-1 )
		found = processPixelOnStick( stickidx, knotpos, pos );
	    else if ( panelidx!=-1 )
		found = processPixelOnPanel( panelidx, stickpos, knotpos, pos );

	    if ( !found || !pos.isDefined() )
		continue;

	    DataPointSet::Pos dpsetpos( pos );
	    DataPointSet::DataRow datarow( dpsetpos, 1 );
	    datarow.data_.setSize( dpset_.nrCols(), mUdf(float) );
	    datarow.data_[i_column_-dpset_.nrFixedCols()] =
					          mCast( float, knotpos );
	    datarow.data_[j_column_-dpset_.nrFixedCols()] =
						  mCast( float, stickpos );
	    if ( id_ >= 0 )
		datarow.data_[id_column_-dpset_.nrFixedCols()] =
						  mCast( float, id_ );
	    dpsetlock_.lock();
	    dpset_.addRow( datarow );
	    dpsetlock_.unLock();
	}

	addToNrDone( 1 );
    }

    return true;
}


bool doFinish( bool success )
{
    dpset_.dataChanged();
    return success;
}


bool processPixelOnStick( int stickidx, int knotpos, Coord3& pos )
{
    if ( !explsurf_.textureknotcoords_.validIdx(stickidx) )
	return false;

    const TypeSet<int>& knotcoords = *explsurf_.textureknotcoords_[stickidx];
    const int nrknots = knotcoords.size();
    if ( knotpos<knotcoords[0] || knotpos>knotcoords[nrknots-1] )
	return false;

    Geometry::PrimitiveSet* geomidxps =
	explsurf_.sticks_[stickidx]->getCoordsPrimitiveSet();

    if ( !geomidxps || !explsurf_.coordlist_ )
	return false;

    if ( nrknots==1 )
    {
	if ( knotpos==knotcoords[0] )
	{
	    pos = explsurf_.coordlist_->get( geomidxps->get(0) );
	    return true;
	}
	else
	    return false;
    }

    for ( int idx=0; idx<nrknots-1; idx++ )
    {
	if ( knotpos==knotcoords[idx] )
	{
	    pos = explsurf_.coordlist_->get( geomidxps->get(idx) );
	    return true;
	}
	else if ( knotpos>knotcoords[idx] && knotpos<knotcoords[idx+1] )
	{
	    const Coord3 p0 = explsurf_.coordlist_->get( geomidxps->get(idx) );
	    const Coord3 p1 = explsurf_.coordlist_->get( geomidxps->get(idx+1));
	    pos = p0+(p1-p0)*(knotpos-knotcoords[idx])/
					(knotcoords[idx+1]-knotcoords[idx]);
	    return true;
	}
    }

    if ( knotpos==knotcoords[nrknots-1] )
    {
	pos = explsurf_.coordlist_->get( geomidxps->get(nrknots-1) );
	return true;
    }

    return true;
}


bool processPixelOnPanel( int panelidx, int stickpos, int knotpos, Coord3& pos )
{
    IndexedGeometry* triangles = explsurf_.paneltriangles_[panelidx];
    if ( !triangles )
	return false;

    Geometry::PrimitiveSet* triangleidxps = triangles->getCoordsPrimitiveSet();

    if( !triangleidxps->size() )
	return false;

    const int lpos = explsurf_.texturecolcoords_[panelidx];
    const int rpos = explsurf_.texturecolcoords_[panelidx+1];
    if ( stickpos<lpos || stickpos>rpos )
	return false;

    Geometry::PrimitiveSet* lgeomidxps =
	explsurf_.sticks_[panelidx]->getCoordsPrimitiveSet();

    Geometry::PrimitiveSet* rgeomidxps =
	explsurf_.sticks_[panelidx+1]->getCoordsPrimitiveSet();


    if ( !lgeomidxps->size() || !rgeomidxps->size() )
	return false;

    const TypeSet<int>& lknots = *explsurf_.textureknotcoords_[panelidx];
    const TypeSet<int>& rknots = *explsurf_.textureknotcoords_[panelidx+1];
    if ( (knotpos<lknots[0] && knotpos<rknots[0]) ||
	 (knotpos>lknots[lknots.size()-1] && knotpos>rknots[rknots.size()-1]) )
	return false;

    const Coord checkpos( stickpos, knotpos );
    const int lendid = lgeomidxps->size()-1;
    const int rendid = rgeomidxps->size()-1;

    for ( int idx=0; idx<triangleidxps->size()/mCoordsPerTriangle; idx++ )
    {
	const int v0 = triangleidxps->get( mCoordsPerTriangle*idx );
	const int v1 = triangleidxps->get( mCoordsPerTriangle*idx+1 );
	const int v2 = triangleidxps->get( mCoordsPerTriangle*idx+2 );

	const int lp0 = lgeomidxps->indexOf(v0);
	const int lp1 = lgeomidxps->indexOf(v1);
	const int lp2 = lgeomidxps->indexOf(v2);
	const int rp0 = rgeomidxps->indexOf(v0);
	const int rp1 = rgeomidxps->indexOf(v1);
	const int rp2 = rgeomidxps->indexOf(v2);

	#define mGetPos(knots,i) (i >= knots.size() ? knots.last() : knots[i])

	Coord texture0, texture1, texture2;

	if ( lp0!=-1 && lp1!=-1 && rp2!=-1 )
	{
	    texture0 = Coord( lpos, mGetPos(lknots,lp0) );
	    texture1 = Coord( lpos, mGetPos(lknots,lp1) );
	    texture2 = Coord( rpos, mGetPos(rknots,rp2) );
	}
	else if ( lp0!=-1 && lp2!=-1 && rp1!=-1 )
	{
	    texture0 = Coord( lpos, mGetPos(lknots,lp0) );
	    texture1 = Coord( rpos, mGetPos(rknots,rp1) );
	    texture2 = Coord( lpos, mGetPos(lknots,lp2) );
	}
	else if ( lp1!=-1 && lp2!=-1 && rp0!=-1 )
	{
	    texture0 = Coord( rpos, mGetPos(rknots,rp0) );
	    texture1 = Coord( lpos, mGetPos(lknots,lp1) );
	    texture2 = Coord( lpos, mGetPos(lknots,lp2) );
	}
	else if ( rp0!=-1 && rp1!=-1 && lp2!=-1 )
	{
	    texture0 = Coord( rpos, mGetPos(rknots,rp0) );
	    texture1 = Coord( rpos, mGetPos(rknots,rp1) );
	    texture2 = Coord( lpos, mGetPos(lknots,lp2) );
	}
	else if ( rp0!=-1 && rp2!=-1 && lp1!=-1 )
	{
	    texture0 = Coord( rpos, mGetPos(rknots,rp0) );
	    texture1 = Coord( lpos, mGetPos(lknots,lp1) );
	    texture2 = Coord( rpos, mGetPos(rknots,rp2) );
	}
	else if ( rp1!=-1 && rp2!=-1 && lp0!=-1 )
	{
	    texture0 = Coord( lpos, mGetPos(lknots,lp0) );
	    texture1 = Coord( rpos, mGetPos(rknots,rp1) );
	    texture2 = Coord( rpos, mGetPos(rknots,rp2) );
	}

	bool intriangle = pointInTriangle2D( checkpos, texture0, texture1,
					     texture2, 1e-5 );
	if ( !intriangle )
	{
	    const bool v01onend =
		(v0==lgeomidxps->get(0) && v1==rgeomidxps->get(0)) ||
		(v0==rgeomidxps->get(0) && v1==lgeomidxps->get(0)) ||
		(v0==lgeomidxps->get(lendid) && v1==rgeomidxps->get(rendid)) ||
		(v0==rgeomidxps->get(rendid) && v1==lgeomidxps->get(lendid) );
	    const bool v12onend =
		(v1==lgeomidxps->get(0) && v2==rgeomidxps->get(0) )||
		(v1==rgeomidxps->get(0) && v2==lgeomidxps->get(0)) ||
		(v1==lgeomidxps->get(lendid) && v2==rgeomidxps->get(rendid) ) ||
		(v1==rgeomidxps->get(rendid) && v2==lgeomidxps->get(lendid) );
	    const bool v20onend =
		(v2==lgeomidxps->get(0) && v0==rgeomidxps->get(0))||
		(v2==rgeomidxps->get(0) && v0==lgeomidxps->get(0)) ||
		(v2==lgeomidxps->get(lendid) && v0==rgeomidxps->get(rendid)) ||
		(v2==rgeomidxps->get(rendid) && v0==lgeomidxps->get(lendid) );

	    if ( v01onend )
		intriangle = pointOnEdge2D( checkpos, texture0, texture1, 1 );
	    else if ( v12onend )
		intriangle = pointOnEdge2D( checkpos, texture1, texture2, 1 );
	    else if ( v20onend )
		intriangle = pointOnEdge2D( checkpos, texture2, texture0, 1 );
	}

	if ( intriangle )
	{
	    const Coord d0 = texture1-texture0;
	    const Coord d1 = checkpos-texture0;
	    const Coord d2 = texture1-texture2;
	    const double denm = d1.x*d2.y-d1.y*d2.x;
	    bool iszero = mIsZero( denm, 1e-8 );
	    const double fchkpt0 = iszero ? 0 : (d0.x*d2.y-d0.y*d2.x)/denm;
	    const double factor12 = iszero ? 0 : (d1.x*d0.y-d1.y*d0.x)/denm;

	    const Coord3 p0 = explsurf_.coordlist_->get( v0 );
	    const Coord3 p1 = explsurf_.coordlist_->get( v1 );
	    const Coord3 p2 = explsurf_.coordlist_->get( v2 );
	    const Coord3 intsectptxy = p1+(p2-p1)*factor12;
	    iszero = mIsZero( fchkpt0, 1e-8 );
	    pos = iszero ? p0 : p0+(intsectptxy- p0)/fchkpt0;
	    return true;
	}
    }

    return false;
}

    Threads::Mutex		dpsetlock_;
    ExplFaultStickSurface&	explsurf_;
    DataPointSet&		dpset_;
    RowCol			sz_;
    int				id_;
    int				i_column_;
    int				j_column_;
    int				id_column_;
};


#undef mStick
#undef mKnot



class ExplFaultStickSurfaceUpdater : public Executor
{
public:
    ExplFaultStickSurfaceUpdater( ExplFaultStickSurface& efss,
				  bool updatesticksnotpanels )
    : Executor("Fault Surface Updater")
    , explsurf_( efss )
    , updatesticksnotpanels_( updatesticksnotpanels )
    , curidx_(0)
{}


od_int64 totalNr() const
{
    return updatesticksnotpanels_ ? explsurf_.sticks_.size()
	: explsurf_.paneltriangles_.size()-1;
}


od_int64 nrDone() const
{ return curidx_; }


int nextStep()
{
    if ( curidx_ >= totalNr() )
	return Finished();

    if ( updatesticksnotpanels_ )
	explsurf_.fillStick( curidx_++ );
    else
	explsurf_.fillPanel( curidx_++ );

    return MoreToDo();
}

protected:

    ExplFaultStickSurface&	explsurf_;
    bool			updatesticksnotpanels_;
    int				curidx_;
};


ExplFaultStickSurface::ExplFaultStickSurface( FaultStickSurface* surf,
					      float zscale )
    : surface_( 0 )
    , displaysticks_( true )
    , displaypanels_( true )
    , sceneidx_( -1 )
    , scalefacs_( 1, 1, mIsUdf(zscale) ? SI().zScale() : zscale )
    , needsupdate_( true )
    , needsupdatetexture_( false )
    , maximumtexturesize_( 1024 )
    , texturesize_( mUdf(int), mUdf(int) )
    , texturepot_( true )
    , texturesampling_( BinID( SI().inlStep(), SI().crlStep() ), SI().zStep() )
    , trialg_( ExplFaultStickSurface::None )
{
    paneltriangles_.allowNull( true );
    panellines_.allowNull( true );
    setSurface( surf );
}


ExplFaultStickSurface::~ExplFaultStickSurface()
{
    setSurface( 0 );
    deepErase( textureknotcoords_ );
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

    removeAll( true );
    surface_ = fss;

    if ( surface_ )
    {
	surface_->nrpositionnotifier.notify(
			mCB(this,ExplFaultStickSurface,surfaceChange) );
	surface_->movementnotifier.notify(
			mCB(this,ExplFaultStickSurface,surfaceMovement) );

	if ( coordlist_ )
	    insertAll();
    }
}


void ExplFaultStickSurface::setZScale( float zscale )
{
    scalefacs_.z = zscale;
    for ( int idx=sticks_.size()-2; idx>=0; idx-- )
	emptyPanel( idx );
}


void ExplFaultStickSurface::removeAll( bool deep )
{
    for ( int idx=sticks_.size()-1; idx>=0; idx-- )
    {
	removePanel( idx );
	removeStick( idx );
    }

    texturesize_ = RowCol( mUdf(int), mUdf(int) );
    textureknotcoords_.erase();
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


void ExplFaultStickSurface::triangulateAlg( TriProjection ta )
{
    if ( trialg_==ta )
	return;

    trialg_ = ta;
    if ( ta )
    {
	removeAll( true );
	insertAll();
	reTriangulateSurface();
    }
    else
	update( true, 0 );

    needsupdatetexture_ = true;
}


bool ExplFaultStickSurface::update( bool forceall, TaskRunner* tr )
{
    if ( forceall )
    {
	removeAll( true );
	insertAll();
    }

    //First update the sticks since they are needed for the panels
    PtrMan<ExplFaultStickSurfaceUpdater> updater =
	new ExplFaultStickSurfaceUpdater( *this, true );

    if ( !TaskRunner::execute( tr, *updater ) )
	return false;

    //Now do panels
    updater = new ExplFaultStickSurfaceUpdater( *this, false );

    if ( !TaskRunner::execute( tr, *updater ) )
	return false;

    needsupdate_ = false;
    return true;
}

bool ExplFaultStickSurface::reTriangulateSurface()
{
    if ( !surface_ || trialg_==ExplFaultStickSurface::None )
	return false;

    const int nrsticks = sticks_.size();
    for ( int idx=nrsticks-1; idx>=0; idx-- )
	removePanel( idx );

    const float zscale = SI().zScale();
    TypeSet<Coord> knots;
    for ( int idx=0; idx<nrsticks; idx++ )
    {
	const TypeSet<Coord3>& stick = *surface_->getStick(idx);
	for ( int idy=0; idy<stick.size(); idy++ )
	{
	    if ( !stick[idy].isDefined() )
		continue;

	    const BinID bid = SI().transform( stick[idy] );
	    const Coord pos( trialg_==ExplFaultStickSurface::Inline ? bid.crl()
				: bid.inl(),
			     trialg_==ExplFaultStickSurface::ZSlice ? bid.crl()
				: stick[idy].z*zscale );
	    if ( !knots.isPresent(pos) )
	    {
		knots += pos;
		coordlist_->add( stick[idy] );
	    }
	}
    }

    DAGTriangleTree tt;
    if ( !tt.setCoordList( &knots, OD::UsePtr ) )
	return false;

    DelaunayTriangulator triangulator( tt );
    triangulator.executeParallel( false );
    TypeSet<int> grid;
    tt.getCoordIndices( grid );
    IndexedGeometry* triangle = new IndexedGeometry(
	    IndexedGeometry::Triangles, 0, 0, texturecoordlist_ );
    if ( !paneltriangles_.size() )
	paneltriangles_ += triangle;
    else
	paneltriangles_.replace( 0, triangle );

    if ( displaypanels_ )
	addToGeometries( triangle );

    for ( int idx=0; idx<grid.size(); )
    {
	TypeSet<int> crdindices;
	crdindices.add(grid[idx++]);
	crdindices.add(grid[idx++]);
	crdindices.add(grid[idx++]);
	triangle->appendCoordIndices( crdindices, false );
    }

    needsupdate_ = false;
    return true;
}




void ExplFaultStickSurface::updateTextureCoords()
{
    for ( int stickidx=0; stickidx<sticks_.size(); stickidx++ )
    {
	IndexedGeometry* stick = sticks_[stickidx];
	if ( !stick )
	    continue;
	Geometry::PrimitiveSet* idxps = stick->getCoordsPrimitiveSet();
	TypeSet<int>& texknotcoords = *textureknotcoords_[stickidx];

	for ( int idx=0; idx<idxps->size() && idx<texknotcoords.size(); idx++ )
	{
	    const int ci = idxps->get(idx);
	    if ( ci == -1 )
		continue;

	    const float rowcoord =
		(texturecolcoords_[stickidx] + 0.5f)/texturesize_.col();

	    const float knotpos = (texknotcoords[idx]+0.5f)/texturesize_.row();

	    texturecoordlist_->set( ci, Coord3(knotpos,rowcoord,0) );
	}
    }
}


void ExplFaultStickSurface::display( bool ynsticks, bool ynpanels )
{
    if ( !ynsticks && !ynpanels && !geometries_.size() )
    {
	addToGeometries( new IndexedGeometry(IndexedGeometry::Lines,
			 coordlist_,normallist_,0) );
    }

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


int ExplFaultStickSurface::textureColSz( const int panelidx )
{
    IndexedGeometry* triangles = paneltriangles_.validIdx(panelidx) ?
	paneltriangles_[panelidx] : 0;

    if ( !triangles )
	return 0;

    Geometry::PrimitiveSet* triangleps = triangles->getCoordsPrimitiveSet();

    Geometry::PrimitiveSet* knots0ps=sticks_[panelidx]->getCoordsPrimitiveSet();

    Geometry::PrimitiveSet* knots1ps =
	sticks_[panelidx+1]->getCoordsPrimitiveSet();

    if( !knots0ps->size() && !knots1ps->size() )
	return 1;

    int res = 0;
    for ( int idx=0; idx<triangleps->size()/mCoordsPerTriangle; idx++ )
    {
	const int v0 = triangleps->get(mCoordsPerTriangle*idx);
	const int v1 = triangleps->get(mCoordsPerTriangle*idx+1);
	const int v2 = triangleps->get(mCoordsPerTriangle*idx+2);

	const bool v0onstick0 = knots0ps->indexOf(v0)!=-1;
	const bool v1onstick0 = knots0ps->indexOf(v1)!=-1;
	const bool v2onstick0 = knots0ps->indexOf(v2)!=-1;

	const bool lineonstick0 = (v0onstick0+v1onstick0+v2onstick0)>1;
	int checkpt=mUdf(int),lp0=mUdf(int),lp1=mUdf(int);
	if ( lineonstick0 )
	{
	    if ( !v0onstick0 )
	    { checkpt=v0; lp0=v1; lp1=v2; }
	    else if ( !v1onstick0 )
	    { checkpt=v1; lp0=v0; lp1=v2; }
	    else if ( !v2onstick0 )
	    { checkpt=v2; lp0=v0; lp1=v1; }
	}
	else
	{
	    if ( v0onstick0 )
	    { checkpt=v0; lp0=v1; lp1=v2; }
	    else if ( v1onstick0 )
	    { checkpt=v1; lp0=v0; lp1=v2; }
	    else if ( v2onstick0 )
	    { checkpt=v2; lp0=v0; lp1=v1; }
	}

	const int sz = point2LineSampleSz( coordlist_->get(checkpt),
		coordlist_->get(lp0), coordlist_->get(lp1) );

	if ( res<sz )
	    res = sz;
    }

    return res;
}


int ExplFaultStickSurface::point2LineSampleSz( const Coord3& point,
	const Coord3& linept0, const Coord3& linept1 )
{
    const BinID ptbid = SI().transform(point.coord());
    const BinID lp0bid = SI().transform(linept0.coord());
    const BinID lp1bid = SI().transform(linept1.coord());

    const Coord3 lp0relpos(
	    (float)(lp0bid.inl()-ptbid.inl())/texturesampling_.inl(),
	    (float)(lp0bid.crl()-ptbid.crl())/texturesampling_.crl(),
	    (linept0.z-point.z)/texturesampling_.val());

    const Coord3 lp1relpos(
	    (float)(lp1bid.inl()-ptbid.inl())/texturesampling_.inl(),
	    (float)(lp1bid.crl()-ptbid.crl())/texturesampling_.crl(),
	    (linept1.z-point.z)/texturesampling_.val());

    const Coord3 dir = lp0relpos-lp1relpos;
    const float u = (float) (-lp1relpos.dot(dir)/dir.sqAbs());
    const float nrsamples = (float) (lp1relpos+u*dir).abs();

    return mNINT32( nrsamples );
}


int ExplFaultStickSurface::sampleSize( const Coord3& p0, const Coord3& p1 )
{
    const BinID bid = SI().transform(p0.coord()) - SI().transform(p1.coord());
    const Coord3 sampl( (float)bid.inl()/texturesampling_.inl(),
			(float)bid.crl()/texturesampling_.crl(),
			(p0.z-p1.z)/texturesampling_.val() );
    const float nrsamples =  (float) sampl.abs();
    return mNINT32( nrsamples );
}



bool ExplFaultStickSurface::updateTextureSize()
{
    if ( sticks_.size()<2 )
	return false;

    int texturerowsz = mUdf(int);
    ObjectSet< TypeSet<int> > sticksegments;
    for ( int stickidx=0; stickidx<sticks_.size(); stickidx++ )
    {
	const int sticknr = surface_->rowRange().atIndex( stickidx );
	const StepInterval<int> colrg = surface_->colRange( sticknr );

	int sticktexturerowsz = 0;
	TypeSet<int>& segmentsizes = *new TypeSet<int>;
	sticksegments += &segmentsizes;

	if ( colrg.start==colrg.stop )
	{
	    sticktexturerowsz = 1;
	    segmentsizes += 1;
	}
	else
	{
	    for ( int knotnr=colrg.start; knotnr<=colrg.stop-colrg.step;
		  knotnr += colrg.step )
	    {
		const Coord3 pos0 = surface_->getKnot( RowCol(sticknr,knotnr) );
		const Coord3 pos1 =
		    surface_->getKnot( RowCol(sticknr,knotnr+colrg.step));

		const BinIDValue bid0(
			SI().transform(pos0.coord()), (float)pos0.z );
		const BinIDValue bid1(
			SI().transform(pos1.coord()), (float)pos1.z );
		const int inlsamples =
		    (bid0.inl()-bid1.inl())/texturesampling_.inl();
		const int crlsamples =
		    (bid0.crl()-bid1.crl())/texturesampling_.crl();
		const float zsamples =
		    (bid0.val()-bid1.val())/texturesampling_.val();

		const float nrsamples = Math::Sqrt( inlsamples*inlsamples +
			crlsamples*crlsamples + zsamples*zsamples );

		const int sz = mNINT32( nrsamples );
		sticktexturerowsz += sz;
		segmentsizes += sz;
	    }
	}

	if ( !stickidx || texturerowsz<sticktexturerowsz )
	    texturerowsz = sticktexturerowsz;
    }

    int texturecolsz = 0;
    TypeSet<int> coldists;
    for ( int panelidx=0; panelidx<sticks_.size()-1; panelidx++ )
    {
	const int colsz = textureColSz( panelidx );
	coldists += colsz;
	texturecolsz += colsz;
    }

    if ( !texturerowsz || !texturecolsz )
	return false;

    int po2rowsz = texturerowsz;
    int po2colsz = texturecolsz;
    if ( texturepot_ )
    {
	const bool df = !mIsUdf(maximumtexturesize_);
	po2rowsz = df ? getPow2Sz(texturerowsz,true,1,maximumtexturesize_) :
			nextPower( texturerowsz, 2 );
	po2colsz = df ? getPow2Sz(texturecolsz,true,1,maximumtexturesize_) :
			nextPower( texturecolsz, 2 );
    }

    texturesize_ = RowCol( po2rowsz, po2colsz );

    texturecolcoords_.erase();
    texturecolcoords_ +=0;
    int sum = 0;
    const float colfactor = (float)po2colsz/texturecolsz;
    for ( int idx=0; idx<coldists.size()-1; idx++ )
    {
	sum += coldists[idx];
	texturecolcoords_ += mNINT32( colfactor*sum );
    }

    texturecolcoords_ += po2colsz-1;
    const float rowfactor = (float)po2rowsz/texturerowsz;
    deepErase( textureknotcoords_ );
    for ( int idx=0; idx<sticksegments.size(); idx++ )
    {
	TypeSet<int>& segmentpositions = *new TypeSet<int>;
	textureknotcoords_ += &segmentpositions;

	if ( (*sticksegments[idx]).size()==1 && (*sticksegments[idx])[0]==1 )
	{
	    segmentpositions += po2rowsz/2;
	    continue;
	}

	int sticklength = 0;
	for ( int idy=0; idy<(*sticksegments[idx]).size(); idy++ )
	    sticklength += (*sticksegments[idx])[idy];

	const float st0 = (po2rowsz-sticklength*rowfactor)/2;
	int startpos = mNINT32( st0 );
	if ( startpos<0 )
	    startpos = 0;

	segmentpositions += startpos;

	int pos = 0;
	for ( int idy=0; idy<(*sticksegments[idx]).size()-1; idy++ )
	{
	    pos += (*sticksegments[idx])[idy];
	    segmentpositions += mNINT32( rowfactor*pos )+startpos;
	}

	segmentpositions += po2rowsz-1-startpos;
    }

    updateStickShifting();
    updateTextureCoords();
    deepErase( sticksegments );
    return true;
}


void ExplFaultStickSurface::updateStickShifting()
{
    const int sticksz = sticks_.size();
    TypeSet<float> prevavgdist( sticksz, mUdf(float) );
    TypeSet<float> avgdist( sticksz, mUdf(float) );
    TypeSet<float> nextavgdist( sticksz, mUdf(float) );
    TypeSet<int> curshift( sticksz, 0 );
    const int shiftunit = 2;

    float totaldist = 0;
    for ( int idx=0; idx<sticksz; idx++ )
    {
	prevavgdist[idx] = getAvgDistance( idx, curshift, -shiftunit );
	avgdist[idx] = getAvgDistance( idx, curshift, 0 );
	nextavgdist[idx] = getAvgDistance( idx, curshift, shiftunit );

	if ( !mIsUdf(avgdist[idx] ) )
	    totaldist += avgdist[idx];
	else
	{
	    pErrMsg("Hmm");
	    return;
	}
    }

    while ( true )
    {
	int largestprevshiftidx = -1;
	float largestprevshiftdiff = mUdf(float);
	int largestnextshiftidx = -1;
	float largestnextshiftdiff = mUdf(float);
	for ( int idx=0; idx<sticksz; idx++ )
	{
	    if ( !mIsUdf(prevavgdist[idx] ) )
	    {
		const float diff = avgdist[idx]-prevavgdist[idx];
		if ( largestprevshiftidx==-1 || largestprevshiftdiff<diff )
		{
		    largestprevshiftidx = idx;
		    largestprevshiftdiff = diff;
		}
	    }

	    if ( !mIsUdf(nextavgdist[idx] ) )
	    {
		const float diff = avgdist[idx]-nextavgdist[idx];
		if ( largestnextshiftidx==-1 || largestnextshiftdiff<diff )
		{
		    largestnextshiftidx = idx;
		    largestnextshiftdiff = diff;
		}
	    }
	}

	if ( largestprevshiftidx==-1 && largestnextshiftidx==-1 )
	    break;

	int shift = 0;
	int shiftstick = mUdf(int);
	if ( largestprevshiftidx==-1 || (largestnextshiftidx != -1 &&
	     largestnextshiftdiff>largestprevshiftdiff) )
	{
	    shift = shiftunit;
	    shiftstick = largestnextshiftidx;
	    curshift[shiftstick] += shiftunit;
	    prevavgdist[shiftstick] = avgdist[shiftstick];
	    avgdist[shiftstick] = nextavgdist[shiftstick];
	    nextavgdist[shiftstick] =
		getAvgDistance( shiftstick, curshift, shiftunit );
	}
	else if ( largestprevshiftidx!=-1 )
	{
	    shift = -shiftunit;
	    shiftstick = largestprevshiftidx;
	    curshift[shiftstick] -= shiftunit;
	    nextavgdist[shiftstick] = avgdist[shiftstick];
	    avgdist[shiftstick] = prevavgdist[shiftstick];
	    prevavgdist[shiftstick] =
		getAvgDistance( shiftstick, curshift, -shiftunit );
	}

	if ( shiftstick )
	{
	    prevavgdist[shiftstick-1] =
		getAvgDistance( shiftstick-1, curshift, -shiftunit);
	    avgdist[shiftstick-1] =
		getAvgDistance( shiftstick-1, curshift, 0 );
	    nextavgdist[shiftstick-1] =
		getAvgDistance( shiftstick-1, curshift, shiftunit );
	}

	if ( shiftstick!=sticksz-1 )
	{
	    prevavgdist[shiftstick+1] =
		getAvgDistance( shiftstick+1, curshift, -shiftunit);
	    avgdist[shiftstick+1] =
		getAvgDistance( shiftstick+1, curshift, 0 );
	    nextavgdist[shiftstick+1] =
		getAvgDistance( shiftstick+1, curshift, shiftunit );
	}

	float newsum = 0;
	for ( int idx=0; idx<sticksz; idx++ )
	    newsum += avgdist[idx];

	if ( newsum>=totaldist )
	{
	    curshift[shiftstick] -= shift;
	    break;
	}
	else
	    totaldist = newsum;
    }

    for ( int idx=0; idx<sticksz; idx++ )
       shiftStick( idx, curshift[idx] );
}


Coord3 ExplFaultStickSurface::getCoord( int stickidx, int texturerow ) const
{
    if ( stickidx<0 || stickidx>=sticks_.size() ||
	 texturerow<0 || texturerow>texturesize_.row() )
	return Coord3::udf();

    const int sticknr = surface_->rowRange().atIndex( stickidx );
    const StepInterval<int> colrg = surface_->colRange( sticknr );
    const TypeSet<int>& knotpos = *textureknotcoords_[stickidx];

    if ( texturerow<=knotpos[0] )
	return surface_->getKnot( RowCol(sticknr, colrg.start) );
    if ( texturerow>knotpos[knotpos.size()-1] )
	return surface_->getKnot( RowCol(sticknr, colrg.stop) );
    else
    {
	for ( int knot=0; knot<knotpos.size()-1; knot++ )
	{
	    const float t = (float)(texturerow-knotpos[knot])/
		(knotpos[knot+1]-knotpos[knot]);
	    if ( texturerow>=knotpos[knot] && texturerow<=knotpos[knot+1] )
	    {
		const Coord3 p0 = surface_->getKnot(
			RowCol(sticknr,colrg.start+knot*colrg.step) );
		const Coord3 p1 = surface_->getKnot(
			RowCol(sticknr,colrg.start+(knot+1)*colrg.step) );
		return p0+(p1-p0)*t;
	    }
	}
    }

    return Coord3::udf();
}


float ExplFaultStickSurface::getAvgDistance( int stickidx,
			    const TypeSet<int>& shift, int extrashift ) const
{
    const int sticknr = surface_->rowRange().atIndex( stickidx );
    const StepInterval<int> colrg = surface_->colRange( sticknr );
    const TypeSet<int>& knotpos = *textureknotcoords_[stickidx];

    float dist = 0;
    int nrposused = 0;

    const int firstrow = knotpos[0] + shift[stickidx] + extrashift;
    if ( firstrow<0 )
	return mUdf(float);

    const int lastrow =
	knotpos[knotpos.size()-1] + shift[stickidx] + extrashift;
    if ( lastrow>=texturesize_.row() )
	return mUdf(float);

    for ( int row=firstrow; row<=lastrow; row++ )
    {
	const Coord3 pos = getCoord( stickidx, row-shift[stickidx]-extrashift ).
	    scaleBy(scalefacs_ );

	if ( stickidx )
	{
	    const Coord3 prevpos =
	     getCoord( stickidx-1, row-shift[stickidx-1] ).scaleBy(scalefacs_);
	    if ( prevpos.isDefined() )
	    {
		dist += (float) prevpos.distTo( pos );
		nrposused++;
	    }
	}

	if ( stickidx<sticks_.size()-1 )
	{
	    const Coord3 nextpos =
	     getCoord( stickidx+1, row-shift[stickidx+1] ).scaleBy(scalefacs_);

	    if ( nextpos.isDefined() )
	    {
		dist += (float) nextpos.distTo( pos );
		nrposused++;
	    }
	}
    }

    if ( !nrposused )
	return mUdf(float);

    return dist/nrposused;
}


void ExplFaultStickSurface::shiftStick( int stickidx, int nrunits )
{
    if ( stickidx<0 || stickidx>textureknotcoords_.size()-1 )
	return;

    TypeSet<int>& knots = *textureknotcoords_[stickidx];
    if ( nrunits+knots[knots.size()-1]>texturesize_.row() )
	return;

    for ( int idx=0; idx<knots.size(); idx++ )
	knots[idx] += nrunits;
}


void ExplFaultStickSurface::needUpdateTexture( bool yn )
{
    if ( yn== needsupdatetexture_ )
	return;

    needsupdatetexture_ = yn;
    if ( needsupdatetexture_ )
    {
	updateTextureSize();
	needsupdatetexture_ = false;
    }
}


bool ExplFaultStickSurface::needsUpdateTexture() const
{ return needsupdatetexture_; }


void ExplFaultStickSurface::setMaximumTextureSize( int sz )
{ maximumtexturesize_ = sz; }


void ExplFaultStickSurface::setTexturePowerOfTwo( bool yn )
{ texturepot_ = yn; }


void ExplFaultStickSurface::setTextureSampling( const BinIDValue& bidv )
{ texturesampling_ = bidv; }


const RowCol& ExplFaultStickSurface::getTextureSize() const
{ return texturesize_; }


bool ExplFaultStickSurface::getTexturePositions( DataPointSet& dpset,
       TaskRunner* tr )
{ return getTexturePositions( dpset, -1, tr ); }

bool ExplFaultStickSurface::getTexturePositions( DataPointSet& dpset,
						 int id, TaskRunner* tr )
{
    const DataColDef texture_i( sKeyTextureI() );
    if ( dpset.dataSet().findColDef(texture_i,PosVecDataSet::NameExact)==-1 )
    {
	dpset.dataSet().add( new DataColDef(texture_i) );
	dpset.dataSet().add( new DataColDef(sKeyTextureJ()) );
    }

    if ( id >= 0 )
    {
	const DataColDef iddef( sKey::ID() );
	if ( dpset.dataSet().findColDef(iddef,PosVecDataSet::NameExact)==-1 )
	    dpset.dataSet().add( new DataColDef(iddef) );
    }

    if ( trialg_==ExplFaultStickSurface::None )
    {
	if ( !updateTextureSize() )
	    return false;

	PtrMan<ExplFaultStickTexturePositionExtracter> extractor =
	    new ExplFaultStickTexturePositionExtracter( *this, dpset, id );
	return TaskRunner::execute( tr, *extractor );
    }
    else
    {
	return setProjTexturePositions( dpset, id );
    }
}


bool ExplFaultStickSurface::setProjTexturePositions( DataPointSet& dps,
						     int id )
{
    //Refine needed for pos calculation

    const float zscale = mCast( float, SI().zDomain().userFactor() );

    TypeSet<Coord> knots;
    TypeSet<int> knotids;
    Interval<float> zrg;
    Interval<int> inlrg, crlrg;
    bool found=false;
    for ( int curid=-1; ; )
    {
	curid = coordlist_->nextID( curid );
	if ( curid<0 )
	    break;

	const Coord3& pos = coordlist_->get( curid );
	const BinID bid = SI().transform( pos );
	knots += Coord( trialg_==ExplFaultStickSurface::Inline ? bid.crl()
							       : bid.inl(),
			trialg_==ExplFaultStickSurface::ZSlice ? bid.crl()
							       : pos.z*zscale );
	knotids += curid;
	if ( !found )
	{
	    inlrg.start = inlrg.stop = bid.inl();
	    crlrg.start = crlrg.stop = bid.crl();
	    zrg.start = zrg.stop = (float) pos.z;
	    found = true;
	}
	else
	{
	    inlrg.include( bid.inl() );
	    crlrg.include( bid.crl() );
	    zrg.include( (float) pos.z );
	}
    }

    if ( !found )
	return false;

    DAGTriangleTree tt;
    tt.setCoordList( &knots, OD::UsePtr );
    DelaunayTriangulator triangulator( tt );
    triangulator.executeParallel( false );

    const int inlsamples = inlrg.width()/texturesampling_.inl();
    const int crlsamples = crlrg.width()/texturesampling_.crl();
    const float zsamples = zrg.width()/texturesampling_.val();
    texturesize_ = RowCol( trialg_==ExplFaultStickSurface::Inline ? crlsamples
							  : inlsamples,
			   trialg_==ExplFaultStickSurface::ZSlice ? crlsamples
							  : mNINT32(zsamples) );
    const int nrfc = dps.nrFixedCols();
    const int nrcs = dps.nrCols();
    const DataColDef ti( sKeyTextureI() );
    const DataColDef tj( sKeyTextureJ() );
    const DataColDef iddef( sKey::ID() );
    const int ic = dps.dataSet().findColDef(ti,PosVecDataSet::NameExact)-nrfc;
    const int jc = dps.dataSet().findColDef(tj,PosVecDataSet::NameExact)-nrfc;
    const int idc =
	    dps.dataSet().findColDef(iddef,PosVecDataSet::NameExact)-nrfc;

    for ( int row=0; row<texturesize_.row(); row++ )
    {
	BinID bid( trialg_==ExplFaultStickSurface::Inline ? -1 :
		texturesampling_.inl()*row+inlrg.start,
		trialg_!=ExplFaultStickSurface::Inline ? -1 :
		texturesampling_.crl()*row+crlrg.start );
	for ( int col=0; col<texturesize_.col(); col++ )
	{
	    float z = -1;
	    if ( trialg_==ExplFaultStickSurface::ZSlice )
		bid.crl() = texturesampling_.crl()*col+crlrg.start;
	    else
		z = texturesampling_.val()*col+zrg.start;
	    const Coord pt( trialg_==ExplFaultStickSurface::Inline ? bid.crl()
								   : bid.inl(),
			    trialg_==ExplFaultStickSurface::ZSlice ? bid.crl()
								   : z*zscale );
	    int dupid = -1;
	    TypeSet<int> vs;
	    if ( !tt.getTriangle(pt,dupid,vs) )
		continue;

	    Coord3 pos;
	    if ( dupid!=-1 )
		pos = coordlist_->get( knotids[dupid] );
	    else
	    {
		if ( vs[0]<0 || vs[1]<0 || vs[2]<0 )
		    continue;

		double w[3];
		interpolateOnTriangle2D( pt, knots[vs[0]], knots[vs[1]],
			knots[vs[2]], w[0], w[1], w[2] );
		pos = w[0] * coordlist_->get(knotids[vs[0]]) +
		      w[1] * coordlist_->get(knotids[vs[1]]) +
		      w[2] * coordlist_->get(knotids[vs[2]]);
	    }

	    DataPointSet::Pos dpsetpos( pos );
	    DataPointSet::DataRow datarow( dpsetpos, 1 );
	    datarow.data_.setSize( nrcs, mUdf(float) );
	    datarow.data_[ic] =  mCast( float, row );
	    datarow.data_[jc] =  mCast( float, col );
	    datarow.data_[idc] =  mCast( float, id );

	    dps.addRow( datarow );
	}
    }

    dps.dataChanged();
    return true;
}


void ExplFaultStickSurface::addToGeometries( IndexedGeometry* ig )
{
    if ( !ig || ig->isHidden() )
	return;

    mGetIndexedShapeWriteLocker4Geometries();
    if ( geometries_.isPresent( ig ) )
	{ pErrMsg("Adding more than once"); }

    ig->ischanged_ = true;
    geometries_ += ig;
}


void ExplFaultStickSurface::removeFromGeometries( const IndexedGeometry* ig )
{
    if ( !ig ) return;
    mGetIndexedShapeWriteLocker4Geometries();
    const int idx = geometries_.indexOf( ig );

    if ( idx!=-1 )
	geometries_.removeSingle( idx, false );
}


void ExplFaultStickSurface::emptyStick( int stickidx )
{
    if ( !sticks_.validIdx(stickidx) )
	return;

    sticks_[stickidx]->removeAll( false );

    needsupdate_ = true;
}


void ExplFaultStickSurface::fillStick( int stickidx )
{
    if ( !surface_ || !coordlist_ || !sticks_.validIdx(stickidx) )
	return;

    emptyStick( stickidx );

    TypeSet<int> knotsps;

    const int sticknr = surface_->rowRange().atIndex( stickidx );
    const StepInterval<int> colrg = surface_->colRange( sticknr );

    for ( int knotnr=colrg.start; knotnr<=colrg.stop; knotnr+=colrg.step )
    {
	const Coord3 pos = surface_->getKnot( RowCol(sticknr,knotnr) );
	int idx = coordlist_->add( pos );
	knotsps += idx;
    }

    sticks_[stickidx]->appendCoordIndices( knotsps );
    sticks_[stickidx]->ischanged_ = true;
}


void ExplFaultStickSurface::removeStick( int stickidx )
{
    if ( !sticks_.validIdx(stickidx) )
	return;

    removeFromGeometries( sticks_[stickidx] );
    delete sticks_.removeSingle( stickidx );
    needsupdate_ = true;
}


void ExplFaultStickSurface::insertStick( int stickidx )
{
    if ( stickidx<0 || stickidx>sticks_.size() )
	return;

    sticks_.insertAt(
	new IndexedGeometry(IndexedGeometry::Lines,
			    coordlist_,normallist_,0),
	stickidx);

    if ( displaysticks_ ) addToGeometries( sticks_[stickidx] );

    needsupdate_ = true;
}


#define mSqDist( coordidx1, coordidx2 ) \
    coordlist_->get(coordidx1).scaleBy(scalefacs_).sqDistTo( \
    coordlist_->get(coordidx2).scaleBy(scalefacs_) )


void ExplFaultStickSurface::emptyPanel( int panelidx )
{
    if ( paneltriangles_.validIdx(panelidx) && paneltriangles_[panelidx] )
	paneltriangles_[panelidx]->removeAll( false );

    if ( panellines_.validIdx(panelidx) && panellines_[panelidx] )
	panellines_[panelidx]->removeAll( false );

    needsupdate_ = true;
}


#define mSqDistArr( a, b ) sqdists[(a)*rsize+(b)]

#define mAddConnection( il, ir ) \
{ \
    lconn += il; \
    rconn += ir; \
    mSqDistArr( il, ir ) = mUdf(float); \
}

void ExplFaultStickSurface::addTriangle( IndexedGeometry* triangles,
					 int a, int b, int c )
{
    TypeSet<int> coordindices;
    coordindices += (a);
    coordindices += (b);
    coordindices += (c);
    triangles->appendCoordIndices( coordindices,false );
}


void ExplFaultStickSurface::fillPanel( int panelidx )
{
    if ( !coordlist_ || !paneltriangles_.validIdx(panelidx) )
	return;

    IndexedGeometry* triangles = paneltriangles_[panelidx];
    IndexedGeometry* lines = panellines_[panelidx];

    if ( triangles && !triangles->isEmpty() )
	triangles->removeAll( false );

    if ( lines && !lines->isEmpty() )
	lines->removeAll( false );

    Geometry::PrimitiveSet* lknotsps =
			sticks_[panelidx]->getCoordsPrimitiveSet();
    Geometry::PrimitiveSet* rknotsps =
			sticks_[panelidx+1]->getCoordsPrimitiveSet();
    const int lsize = lknotsps ? lknotsps->size() : 0;
    const int rsize = rknotsps ? rknotsps->size() : 0;
    if ( lsize==0 || rsize==0 )
	return;

    if ( lsize==1 && rsize==1 )
    {
	if ( !lines )
	{
	    lines = new IndexedGeometry( IndexedGeometry::Lines,
					 0, 0, texturecoordlist_);
	    panellines_.replace( panelidx, lines );
	    if ( displaypanels_ ) addToGeometries( lines );
	}

	Geometry::PrimitiveSet* linesps = lines->getCoordsPrimitiveSet();

	linesps->append( lknotsps->get(0) );
	linesps->append( rknotsps->get(0) );
	return;
    }

    if ( !triangles )
    {
	triangles = new IndexedGeometry( IndexedGeometry::Triangles,
					 0, 0, texturecoordlist_ );
	paneltriangles_.replace( panelidx, triangles );
	if ( displaypanels_ ) addToGeometries( triangles );
    }

    if ( lsize==1 || rsize==1 )
    {
	if ( lsize==1 )
	{
	    for ( int idx=1; idx<rsize; idx++ )
	    {
		addTriangle( triangles, lknotsps->get(0),
			     rknotsps->get(idx), rknotsps->get(idx-1) );
	    }
	}
	else
	{
	    for ( int idx=1; idx<lsize; idx++ )
	    {
		addTriangle( triangles, lknotsps->get(idx),
			     lknotsps->get(idx-1), rknotsps->get(0) );
	    }
	}
	return;
    }

    mAllocVarLenArr( float, sqdists, lsize*rsize );
    for ( int idx=0; idx<lsize; idx++ )
    {
	for ( int idy=0; idy<rsize; idy++ )
	{
	    const float sqdist = (float) mSqDist(
		lknotsps->get(idx), rknotsps->get(idy) );
	    mSqDistArr( idx, idy ) = sqdist;
	}
    }

    TypeSet<int> lconn,rconn;
    mAddConnection( 0, 0 );
    mAddConnection( lsize-1, rsize-1 );

    while ( true )
    {
	float minsqdist = mUdf(float);
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

		if ( (idx>minl) == (idy>minr) )
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
	    if ( lconn[idx]==lidx )
	    conns += rconn[idx];
	}

	if ( conns.isEmpty() )
	    continue;

	sort_array( conns.arr(), conns.size() );

	if ( lidx )
	    addTriangle( triangles,
	    lknotsps->get(lidx),rknotsps->get(conns[0]),lknotsps->get(lidx-1) );

	for ( int idx=1; idx<conns.size(); idx++ )
	    addTriangle(triangles, lknotsps->get(lidx),
	    rknotsps->get(conns[idx]),rknotsps->get(conns[idx-1]) );
    }
}


void ExplFaultStickSurface::removePanel( int panelidx )
{
    if ( paneltriangles_.validIdx(panelidx) )
    {
	removeFromGeometries( paneltriangles_[panelidx] );
	delete paneltriangles_.removeSingle( panelidx );
    }

    if ( panellines_.validIdx(panelidx) )
    {
	removeFromGeometries(  panellines_[panelidx] );
	delete panellines_.removeSingle( panelidx );
    }

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
	RowCol rc = RowCol::fromInt64( (*pidlist)[idx] );
	const int stickidx = rc.row();

	if ( rc.col()==FaultStickSurface::StickChange )
	{
	    emptyPanel( stickidx-1 );
	    emptyPanel( stickidx );
	    emptyStick( stickidx );
	}
	if ( rc.col()==FaultStickSurface::StickInsert )
	{
	    emptyPanel( stickidx-1 );
	    insertPanel( !stickidx ? 0 : stickidx-1 );
	    insertStick( stickidx );
	}
	if ( rc.col()==FaultStickSurface::StickRemove )
	{
	    emptyPanel( stickidx );
	    removePanel( !stickidx ? 0 : stickidx-1 );
	    removeStick( stickidx );
	}
	if ( rc.col()==FaultStickSurface::StickHide )
	{
	    if ( sticks_.validIdx(stickidx) )
	    {
		const int sticknr = surface_->rowRange().atIndex( stickidx );
		sticks_[stickidx]->hide(
				surface_->isStickHidden(sticknr,sceneidx_) );

		removeFromGeometries( sticks_[stickidx] );
		if ( displaysticks_ )
		    addToGeometries( sticks_[stickidx] );
	    }
	}
    }

    addVersion();
}


void ExplFaultStickSurface::surfaceMovement( CallBacker* cb )
{
    mCBCapsuleUnpack( const TypeSet<GeomPosID>*, pidlist, cb );
    for ( int idx=0; pidlist && idx<pidlist->size(); idx++ )
    {
	RowCol rc = RowCol::fromInt64( (*pidlist)[idx] );
	const int stickidx = rc.row();

	emptyPanel( stickidx-1 );
	emptyPanel( stickidx );
	emptyStick( stickidx );
    }

    addVersion();
}

} // namespace Geometry
