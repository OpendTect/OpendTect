/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "explicitmarchingcubes.h"

#include "task.h"
#include "marchingcubes.h"
#include "odmemory.h"
#include "positionlist.h"
#include "samplingdata.h"

#define mX	0
#define mY	1
#define mZ	2

#define mBucketSize 16

class ExplicitMarchingCubesSurfaceUpdater : public ParallelTask
{ mODTextTranslationClass(ExplicitMarchingCubesSurfaceUpdater);
public:
    ExplicitMarchingCubesSurfaceUpdater( ExplicitMarchingCubesSurface& is,
	bool updatecoords )
	: surface_( is )
	, totalnr_( is.getSurface()->models_.totalSize() )
	, xrg_( 0 )
	, yrg_( 0 )
	, zrg_( 0 )
	, updatecoords_( updatecoords )
	, modelslocker_(is.getSurface()->modelslock_)
    {
	MarchingCubeLookupTable::get(); // just to have the table ready
    }

    ~ExplicitMarchingCubesSurfaceUpdater()
    {
	delete xrg_; delete yrg_; delete zrg_;
    }

    void setUpdateCoords( bool yn ) { updatecoords_ = yn; }

    void	setLimits(const Interval<int>& xrg,
	const Interval<int>& yrg,
	const Interval<int>& zrg)
    {
	if ( xrg_ ) delete xrg_; xrg_ = new Interval<int>( xrg );
	if ( yrg_ ) delete yrg_; yrg_ = new Interval<int>( yrg );
	if ( zrg_ ) delete zrg_; zrg_ = new Interval<int>( zrg );

	idxstocompute_.erase();

	int start[3], stop[3];
	start[mX] = xrg.start; start[mY] = yrg.start; start[mZ] = zrg.start;
	stop[mX] = xrg.stop; stop[mY] = yrg.stop; stop[mZ] = zrg.stop;

	surface_.getSurface()->models_.getIndicesInRange( start, stop,
	    idxstocompute_);
	totalnr_ = idxstocompute_.size()/3;
    }

protected:

    od_int64 nrIterations() const override { return totalnr_; }
    uiString uiMessage() const override
    {
	return updatecoords_ ? tr("Triangulation: updating coordinates")
	    : tr("Triangulation: updating indices");
    }
    uiString uiNrDoneText() const override { return tr("Positions updated"); }

    bool doWork( od_int64 start, od_int64 stop, int thread ) override
    {
	const int* tableidxs = idxstocompute_.arr();
	const bool usetable = xrg_;
	int idxs[3];

	const MultiDimStorage<MarchingCubesModel>& models =
	    surface_.getSurface()->models_;

	for ( int idx=mCast(int,start); idx<=stop && shouldContinue();
	    idx++, addToNrDone(1) )
	{
	    if ( usetable )
		OD::sysMemCopy( idxs, tableidxs+idx*3, sizeof(int)*3 );
	    else if ( idx==start )
	    {
		if ( !models.getIndex(mCast(int,start),idxs) )
		    return false;
	    }
	    else
	    {
		if ( !models.next( idxs ) )
		    return false;
	    }

	    if ( updatecoords_ )
	    {
		if ( !surface_.updateCoordinates( idxs ) )
		{
		    pErrMsg("Hugh");
		    return false;
		}
	    }
	    else
	    {
		int pos[3];
		if ( !models.getPos( idxs, pos ) )
		{
		    pErrMsg("Hugh");
		    return false;
		}

		if ( !surface_.updateIndices( pos ) )
		{
		    pErrMsg("Hugh");
		    return false;
		}
	    }
	}

	return true;
    }

    int minThreadSize() const override { return 10000; }

    Interval<int>*			xrg_;
    Interval<int>*			yrg_;
    Interval<int>*			zrg_;

    TypeSet<int>			idxstocompute_;
    //Only used if ranges

    int					totalnr_;
    ExplicitMarchingCubesSurface&	surface_;
    bool				updatecoords_;
    Threads::Locker			modelslocker_;

};



ExplicitMarchingCubesSurface::ExplicitMarchingCubesSurface(
	MarchingCubesSurface* surf)
    : surface_( surf )
    , ibuckets_( 3, 1 )
    , coordindices_( 3, 3 )
    , lastversionupdate_( -1 )
{
    changedbucketranges_[mX] = 0;
    changedbucketranges_[mY] = 0;
    changedbucketranges_[mZ] = 0;
    if ( surface_ ) surface_->ref();
}

#define mRemoveBucketRanges \
    delete changedbucketranges_[mX]; \
    delete changedbucketranges_[mY]; \
    delete changedbucketranges_[mZ]; \
    changedbucketranges_[mX] = 0; \
    changedbucketranges_[mY] = 0; \
    changedbucketranges_[mZ] = 0

ExplicitMarchingCubesSurface::~ExplicitMarchingCubesSurface()
{
    setSurface( 0 );
    mRemoveBucketRanges;
}


void ExplicitMarchingCubesSurface::setSurface( MarchingCubesSurface* ns )
{
    if ( surface_ )
    {
	surface_->change.remove(
		mCB(this,ExplicitMarchingCubesSurface,surfaceChange));
	surface_->unRef();
    }

    surface_ = ns;
    removeAll( true );

    if ( surface_ )
    {
	surface_->ref();
	surface_->change.notify(
		mCB(this,ExplicitMarchingCubesSurface,surfaceChange));
	addVersion();
    }
}

#define mGetWriteLock4CoordIndices(nm) \
    Threads::Locker nm( coordindiceslock_, Threads::Locker::WriteLock )

void ExplicitMarchingCubesSurface::removeAll( bool deep )
{
    mGetWriteLock4CoordIndices(ilckr);
    if ( coordlist_ && deep )
    {
	int idxs[] = { 0, 0, 0 };
	if ( !coordindices_.isValidPos( idxs ) )
	    return;

	do
	{
	    const int* ci = &coordindices_.getRef( idxs, 0 );
	    for ( int dim=0; dim<3; dim++ )
	    {
		if ( ci[dim]==-1 )
		    continue;

		coordlist_->remove( ci[dim] );
	    }
	} while ( coordindices_.next( idxs ) );
    }
    coordindices_.setEmpty();
    ilckr.unlockNow();

    mGetIndexedShapeWriteLocker4Geometries();
    ibuckets_.setEmpty();
    deepErase( geometries_ );
}


bool ExplicitMarchingCubesSurface::allBucketsHaveChanged() const
{
    for ( int idx=0; idx<3; idx++ )
    {
	Interval<int> range;
	if ( !surface_->models_.getRange( idx, range ) )
	    return false;

	range.start = getBucketPos( range.start );
	range.stop = getBucketPos( range.stop );

	if ( range!=*changedbucketranges_[idx] )
	    return false;
    }

    return true;
}


bool ExplicitMarchingCubesSurface::needsUpdate() const
{ return !getVersion() || getVersion()>lastversionupdate_; }


bool ExplicitMarchingCubesSurface::update( bool forceall, TaskRunner* tr )
{
    if ( !forceall && changedbucketranges_[mX] && !allBucketsHaveChanged() )
    {
	if ( update( *changedbucketranges_[mX],
		     *changedbucketranges_[mY],
		     *changedbucketranges_[mZ], tr ) )
	{
	    mRemoveBucketRanges;
	    lastversionupdate_ = getVersion();
	    return true;
	}
    }

    removeAll( true );

    if ( !surface_ || surface_->models_.isEmpty() )
	return true;

    if ( !surface_->models_.size() )
	return true;

    ExplicitMarchingCubesSurfaceUpdater updater( *this, true );

    if ( !TaskRunner::execute( tr, updater ) )
	return false;

    updater.setUpdateCoords( false );

    if ( TaskRunner::execute( tr, updater ) )
    {
	mRemoveBucketRanges;
	lastversionupdate_ = getVersion();
	return true;
    }

    return false;
}


bool ExplicitMarchingCubesSurface::update(
	    const Interval<int>& xbucketrg,
	    const Interval<int>& ybucketrg,
	    const Interval<int>& zbucketrg,
            TaskRunner* tr )
{
    removeBuckets( xbucketrg, ybucketrg, zbucketrg );

    Interval<int> xrg = Interval<int>( xbucketrg.start*mBucketSize,
				       (xbucketrg.stop+1)*mBucketSize-1 );
    Interval<int> yrg = Interval<int>( ybucketrg.start*mBucketSize,
	                               (ybucketrg.stop+1)*mBucketSize-1 );
    Interval<int> zrg = Interval<int>( zbucketrg.start*mBucketSize,
	                               (zbucketrg.stop+1)*mBucketSize-1 );

    ExplicitMarchingCubesSurfaceUpdater updater( *this, true );
    updater.setLimits( xrg, yrg, zrg );

    if ( !TaskRunner::execute( tr, updater ) )
	return false;

    updater.setUpdateCoords( false );

    return TaskRunner::execute( tr, updater );
}


void ExplicitMarchingCubesSurface::removeBuckets(
					const Interval<int>& xbucketrg,
					const Interval<int>& ybucketrg,
					const Interval<int>& zbucketrg )
{
    for ( int idx=xbucketrg.start; idx<xbucketrg.stop+1; idx++ )
    {
	for ( int idy=ybucketrg.start; idy<ybucketrg.stop+1; idy++ )
	{
	    for ( int idz=zbucketrg.start; idz<zbucketrg.stop+1; idz++ )
	    {
		int pos[3] = { idx, idy, idz };
		int bucketidx[3];
		if ( !ibuckets_.findFirst( pos, bucketidx ) )
		    continue;

		Geometry::IndexedGeometry* bucket=ibuckets_.getRef(bucketidx,0);
		geometries_ -= bucket;
		ibuckets_.remove( bucketidx );
		delete bucket;
	    }
	}
    }
}


bool ExplicitMarchingCubesSurface::updateIndices( const int* pos )
{
    if ( !surface_ )
	return true;

    const MarchingCubeLookupTable& tables = MarchingCubeLookupTable::get();

    unsigned char model,submodel;
    if ( !surface_->getModel( pos, model, submodel ) )
	return false;

    const MarchingCubeTriangleTable& table = tables.triangles_[model];
    const int nrtableindices = table.nrindices_[submodel];
    if ( !nrtableindices )
	return true;

    const int indicesbucket[] = { getBucketPos(pos[mX]),
				  getBucketPos(pos[mY]),
				  getBucketPos(pos[mZ]) };

    Geometry::IndexedGeometry* bucket = 0;
    int bucketidx[3];
    Threads::Locker lckr( geometrieslock_ );

    if ( ibuckets_.findFirst( indicesbucket, bucketidx ) )
    {
	bucket = ibuckets_.getRef( bucketidx, 0 );
    }
    else
    {
	if ( !lckr.convertToWriteLock() &&
	      ibuckets_.findFirst( indicesbucket, bucketidx ) )
	{
	    bucket = ibuckets_.getRef( bucketidx, 0 );
	}
	else
	{
	    bucket = new Geometry::IndexedGeometry(
		    Geometry::IndexedGeometry::Triangles,0, normallist_, 0,
		    Geometry::IndexedGeometry::IndexSet, true );

	    ibuckets_.add( &bucket, indicesbucket );
	    geometries_ += bucket;
	}

    }
    lckr.unlockNow();
    const char* tableindices = table.indices_[submodel];


    TypeSet<int> coordindices;
    TypeSet<Coord3> normals;

    coordindices.setCapacity( nrtableindices*16, false );
    normals.setCapacity( nrtableindices*16, false );

    int arrpos = 0;
    bool gotonextstrip = false;
    TypeSet<Coord3> triangles;
    for ( int idx=0; idx<nrtableindices; idx++ )
    {
	const char neighbor = tableindices[arrpos++];
	if ( neighbor==-1 )
	{
	    gotonextstrip = false;
	    continue;
	}

	const char axis = tableindices[arrpos++];

	if ( gotonextstrip )
	    continue;

	int neighborpos[3];
	switch ( neighbor )
	{
	    case 0:
		OD::memCopy( neighborpos, pos, sizeof(int)*3 );
		break;
	    case 1:
		neighborpos[0] = pos[0];
		neighborpos[1] = pos[1];
		neighborpos[2] = pos[2]+1;
		break;
	    case 2:
		neighborpos[0] = pos[0];
		neighborpos[1] = pos[1]+1;
		neighborpos[2] = pos[2];
		break;
	    case 3:
		neighborpos[0] = pos[0];
		neighborpos[1] = pos[1]+1;
		neighborpos[2] = pos[2]+1;
		break;
	    case 4:
		neighborpos[0] = pos[0]+1;
		neighborpos[1] = pos[1];
		neighborpos[2] = pos[2];
		break;
	    case 5:
		neighborpos[0] = pos[0]+1;
		neighborpos[1] = pos[1];
		neighborpos[2] = pos[2]+1;
		break;
	    case 6:
		neighborpos[0] = pos[0]+1;
		neighborpos[1] = pos[1]+1;
		neighborpos[2] = pos[2];
		break;
	    case 7:
		neighborpos[0] = pos[0]+1;
		neighborpos[1] = pos[1]+1;
		neighborpos[2] = pos[2]+1;
		break;
	}

	int indices[3];
	if ( !getCoordIndices( neighborpos, indices ) )
	{
	    gotonextstrip = true;
	    continue;
	}

	const int index = indices[axis];
	if ( index==-1 )
	{
	    gotonextstrip = true;
	    continue;
	}

	coordindices += index;
	normals += Coord3(0,0,0);

	if ( normallist_ )
	{
	    triangles += coordlist_->get( index );

	    const int nrtri = triangles.size();
	    const bool reverse = ((bool)(nrtri % 2));

	    if ( nrtri>=3 )
	    {
		const int c0 = nrtri-(reverse?3:1);
		const int c1 = nrtri-2;
		const int c2 = nrtri-(reverse?1:3);
		const Coord3 v0 = triangles[c0]-triangles[c1];
		const Coord3 v1 = triangles[c2]-triangles[c1];

		Coord3 normal = v0.cross( v1 );
		const double normalsqlen = normal.sqAbs();
		if ( !normalsqlen )
		    normal = Coord3( 1, 0, 0 );
		else
		    normal /= Math::Sqrt( normalsqlen );

		const int nrindices = coordindices.size();

		const int i0 = coordindices[nrindices-1];
		const int i1 = coordindices[nrindices-2];
		const int i2 = coordindices[nrindices-3];

		//Should be locked
		normallist_->set( i0, normal );
		normallist_->set( i1, normal );
		normallist_->set( i2, normal );
	    }
	}
    }

    Threads::Locker lock( bucket->lock_ );
    bucket->appendCoordIndices( coordindices );

    return true;
}


void ExplicitMarchingCubesSurface::surfaceChange(CallBacker*)
{
    Interval<int> ranges[3];
    if ( surface_->allchanged_ )
    {
	surface_->models_.getRange( mX, ranges[mX] );
	surface_->models_.getRange( mY, ranges[mY] );
	surface_->models_.getRange( mZ, ranges[mZ] );
    }
    else
    {
	ranges[mX] = surface_->changepos_[mX];
	ranges[mY] = surface_->changepos_[mY];
	ranges[mZ] = surface_->changepos_[mZ];
    }

    //convert to bucket-ranges
    ranges[mX] = Interval<int>( getBucketPos( ranges[mX].start),
				getBucketPos( ranges[mX].stop ) );
    ranges[mY] = Interval<int>( getBucketPos( ranges[mY].start),
				getBucketPos( ranges[mY].stop ) );
    ranges[mZ] = Interval<int>( getBucketPos( ranges[mZ].start),
				getBucketPos( ranges[mZ].stop ) );

    if ( !changedbucketranges_[mX] )
    {
	changedbucketranges_[mX] = new Interval<int>( ranges[mX] );
	changedbucketranges_[mY] = new Interval<int>( ranges[mY] );
	changedbucketranges_[mZ] = new Interval<int>( ranges[mZ] );
    }
    else
    {
	changedbucketranges_[mX]->include( ranges[mX] );
	changedbucketranges_[mY]->include( ranges[mY] );
	changedbucketranges_[mZ]->include( ranges[mZ] );
    }

    addVersion();
}


int ExplicitMarchingCubesSurface::getBucketPos( int pos ) const
{
    return ( pos<0 ? (pos+1)/mBucketSize-1 : pos/mBucketSize );
}


bool ExplicitMarchingCubesSurface::getCoordIndices( const int* pos, int* res )
{
    if ( !surface_ )
	return false;

    int cidxs[3];
    Threads::Locker lckr( coordindiceslock_ );
    if ( !coordindices_.findFirst( pos, cidxs ) )
    {
	return false;
    }

    int* indices = &coordindices_.getRef( cidxs, 0 );

    OD::memCopy( res, indices, sizeof(int)*3 );
    return true;
}


bool ExplicitMarchingCubesSurface::updateCoordinate( const int* pos,
						     const int* modelidxs,
						     int* res )
{
    const MarchingCubesModel& model = surface_->models_.getRef( modelidxs, 0 );

    for ( int dim=0; dim<3; dim++ )
    {
	if ( model.axispos_[dim]==MarchingCubesModel::cUdfAxisPos )
	{
	    if ( res[dim]!=-1 )
	    {
		coordlist_->remove( res[dim] );
		res[dim] = -1;
	    }
	}
	else
	{
	    Coord3 coord( pos[mX], pos[mY], pos[mZ] );
	    coord[dim] +=
	      ((float) model.axispos_[dim])/MarchingCubesModel::cAxisSpacing;

	    if ( res[dim]!=-1 )
		coordlist_->set( res[dim], coord );
	    else
		res[dim] = coordlist_->add( coord );
	}
    }

    return true;
}


bool ExplicitMarchingCubesSurface::updateCoordinates( const int* modelidxs )
{
    if ( !surface_ )
	return false;

    int pos[3];
    if ( !surface_->models_.getPos( modelidxs, pos ) )
	return false;


    int cidxs[3];
     Threads::Locker lckr( coordindiceslock_ );
    if ( !coordindices_.findFirst( pos, cidxs ) )
    {
	int indices[] = { -1, -1, -1 };
	if ( !updateCoordinate( pos, modelidxs, indices ) )
	{
	    return false;
	}

	if ( lckr.convertToWriteLock() ||
	     !coordindices_.findFirst( pos, cidxs ) )
	    coordindices_.add( indices, pos );

	return true;
    }

    int* indices = &coordindices_.getRef( cidxs, 0 );
    const int res = updateCoordinate( pos, modelidxs, indices );

    return res;
}


// MarchingCubeTriangleTable
MarchingCubeTriangleTable::MarchingCubeTriangleTable()
{}


MarchingCubeTriangleTable::~MarchingCubeTriangleTable()
{}



// MarchingCubeLookupTable
MarchingCubeLookupTable::MarchingCubeLookupTable()
{
    triangles_[0].nrindices_ += 0;
    triangles_[255].nrindices_ += 0;

    #include "marchingcubeslist.cc"
}


MarchingCubeLookupTable::~MarchingCubeLookupTable()
{ }


const MarchingCubeLookupTable& MarchingCubeLookupTable::get()
{
    mDefineStaticLocalObject( MarchingCubeLookupTable*, table,
                              (new MarchingCubeLookupTable) );
    return *table;
}
