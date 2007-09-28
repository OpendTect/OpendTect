/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2006
-*/

static const char* rcsID = "$Id: explicitmarchingcubes.cc,v 1.10 2007-09-28 20:56:43 cvskris Exp $";

#include "explicitmarchingcubes.h"

#include "basictask.h"
#include "marchingcubes.h"
#include "positionlist.h"
#include "samplingdata.h"

#define mX	0
#define mY	1
#define mZ	2

#define mBucketSize 16	


class ExplicitMarchingCubesSurfaceUpdater : public ParallelTask
{
public:
    ExplicitMarchingCubesSurfaceUpdater( ExplicitMarchingCubesSurface& is,
	    bool updatecoords )
	: surface_( is )
	, totalnr_( is.getSurface()->models_.totalSize() )
	, xrg_( 0 )
	, yrg_( 0 )
	, zrg_( 0 )
        , updatecoords_( updatecoords )
    {
	is.getSurface()->modelslock_.readLock();
    }

    ~ExplicitMarchingCubesSurfaceUpdater()
    {
	surface_.getSurface()->modelslock_.readUnLock();

	delete xrg_;
	delete yrg_;
	delete zrg_;
    }
    
    void	setLimits(const Interval<int>& xrg,
	    		  const Interval<int>& yrg,
			  const Interval<int>& zrg)
    {
	if ( xrg_ ) delete xrg_; xrg_ = new Interval<int>( xrg );
	if ( yrg_ ) delete yrg_; yrg_ = new Interval<int>( yrg );
	if ( zrg_ ) delete zrg_; zrg_ = new Interval<int>( zrg );

	totalnr_ = 0;

	int idxs[] = { 0, 0, 0 };
	if ( !surface_.getSurface()->models_.isValidPos(idxs) )
	    return;
	do 
	{
	    int pos[3];
	    surface_.getSurface()->models_.getPos( idxs, pos );
	    if ( xrg_->includes(pos[mX]) &&
		 yrg_->includes(pos[mY]) &&
		 zrg_->includes(pos[mZ]) )
		totalnr_ ++;
	} while ( surface_.getSurface()->models_.next( idxs ) );
    }


    int	nrTimes() const { return totalnr_; }
    bool doWork( int start, int stop, int )
    {
	int idx = start;
	int idxs[3];
	
	if ( !surface_.getSurface()->models_.getIndex(idx,idxs) )
	{
	    pErrMsg("Hugh");
	    return false;
	}

	while ( idx<=stop )
	{
	    if ( idx!=start && !surface_.getSurface()->models_.next( idxs ) )
		return false;

	    int pos[3];
	    if ( !surface_.getSurface()->models_.getPos( idxs, pos ) )
	    {
		pErrMsg("Hugh");
		return false;
	    }

	    if ( !xrg_ || xrg_->includes(pos[mX]) && yrg_->includes(pos[mY]) &&
		 zrg_->includes(pos[mZ]) )
	    {

		if ( updatecoords_ )
		{
		    int res[3];
		    if ( !surface_.updateCoordinates( pos ) )
		    {
			pErrMsg("Hugh");
			return false;
		    }
		}
		else if ( !surface_.updateIndices( pos ) )
		{
		    pErrMsg("Hugh");
		    return false;
		}

		idx++;
	    }
	}

	return true;
    }

    int		minThreadSize() const { return 1; }

protected:
    Interval<int>*			xrg_;
    Interval<int>*			yrg_;
    Interval<int>*			zrg_;

    int					totalnr_;
    ExplicitMarchingCubesSurface&	surface_;
    bool				updatecoords_;
};



ExplicitMarchingCubesSurface::ExplicitMarchingCubesSurface(
	MarchingCubesSurface* surf)
    : surface_( surf )
    , ibuckets_( 3, 1 )
    , coordindices_( 3, 3 )
    , scale0_( 0 )
    , scale1_( 0 )
    , scale2_( 0 )
{
    changedbucketranges_[mX] = 0;
    changedbucketranges_[mY] = 0;
    changedbucketranges_[mZ] = 0;
    if ( surface_ ) surface_->ref();
}


ExplicitMarchingCubesSurface::~ExplicitMarchingCubesSurface()
{
    setSurface( 0 );
    delete scale0_;
    delete scale1_;
    delete scale2_;
    delete changedbucketranges_[mX];
    delete changedbucketranges_[mY];
    delete changedbucketranges_[mZ];
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
    removeAll();

    if ( surface_ )
    {
	surface_->ref();
	surface_->change.notify(
		mCB(this,ExplicitMarchingCubesSurface,surfaceChange));
    }
}


void ExplicitMarchingCubesSurface::removeAll()
{
    coordindiceslock_.writeLock();
    if ( coordlist_ )
    {
	int idxs[] = { 0, 0, 0 };
	if ( !coordindices_.isValidPos( idxs ) )
	{
	    coordindiceslock_.writeUnLock();
	    return;
	}
	
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

    coordindices_.empty();
    coordindiceslock_.writeUnLock();

    geometrieslock_.writeLock();
    ibuckets_.empty();
    deepErase( geometries_ );
    geometrieslock_.writeUnLock();
}


bool ExplicitMarchingCubesSurface::update( bool forceall )
{
    if ( !forceall && changedbucketranges_[mX] )
    {
	if ( update(
		Interval<int>(changedbucketranges_[mX]->start*mBucketSize,
		    	     (changedbucketranges_[mX]->stop+1)*mBucketSize-1),
		Interval<int>(changedbucketranges_[mX]->start*mBucketSize,
		    	     (changedbucketranges_[mX]->stop+1)*mBucketSize-1),
		Interval<int>(changedbucketranges_[mX]->start*mBucketSize,
		    	     (changedbucketranges_[mX]->stop+1)*mBucketSize-1)))
	{
	    delete changedbucketranges_[mX];
	    delete changedbucketranges_[mY];
	    delete changedbucketranges_[mZ];
	    changedbucketranges_[mX] = 0;
	    changedbucketranges_[mY] = 0;
	    changedbucketranges_[mZ] = 0;
	}
    }

    removeAll();

    if ( !surface_ )
	return true;

    if ( !surface_->models_.size() )
	return true;

    if ( !surface_ || surface_->models_.isEmpty() )
	return true;

    PtrMan<ExplicitMarchingCubesSurfaceUpdater> updater = new
	ExplicitMarchingCubesSurfaceUpdater( *this, true );
    if ( !updater->execute() )
	return false;

    updater = 0; //deletes old & unlocks
    updater = new ExplicitMarchingCubesSurfaceUpdater( *this, false );
    return updater->execute();
}


bool ExplicitMarchingCubesSurface::update(
	    const Interval<int>& xrg,
	    const Interval<int>& yrg,
	    const Interval<int>& zrg )
{
    //TODO remove all influenced buckets && all coordinates in ranges
    PtrMan<ExplicitMarchingCubesSurfaceUpdater> updater = new
	ExplicitMarchingCubesSurfaceUpdater( *this, true );
    updater->setLimits( xrg, yrg, zrg );
    if ( !updater->execute() )
	return false;

    updater = 0; //deletes old & unlocks
    updater = new ExplicitMarchingCubesSurfaceUpdater( *this, false );
    updater->setLimits( xrg, yrg, zrg );
    return updater->execute();
}


#define mSetScale( dim ) \
    if ( mIsZero( scale##dim.start, mDefEps ) && \
	 mIsEqual( scale##dim.step, 1, mDefEps ) ) \
    { \
	if ( scale##dim##_ ) delete scale##dim##_; \
	scale##dim##_ = 0; \
    } \
    else \
    { \
	if ( !scale##dim##_ ) scale##dim##_ = \
		new SamplingData<float>( scale##dim ); \
	else *scale##dim##_ = scale##dim; \
    }

void ExplicitMarchingCubesSurface::setAxisScales(
	const SamplingData<float>& scale0,
	const SamplingData<float>& scale1,
	const SamplingData<float>& scale2 )
{
    mSetScale( 0 );
    mSetScale( 1 );
    mSetScale( 2 );
}


#undef mSetScale

const SamplingData<float>&
ExplicitMarchingCubesSurface::getAxisScale( int dim ) const
{
    const SamplingData<float>* scale = 0;
    if ( dim==mX )
	scale = scale0_;
    else if ( dim==mY )
	scale = scale1_;
    else
	scale = scale2_;

    static const SamplingData<float> dummy( 0, 1 );
    return scale ? *scale : dummy;
}


#define mEndTriangleStrip \
    const int bsz = bucket->coordindices_.size(); \
    if ( bsz<3 ) \
	bucket->coordindices_.erase(); \
    else if ( bucket->coordindices_[bsz-1]!=-1 ) \
    { \
	if ( bucket->coordindices_[bsz-2]==-1 ) \
	    bucket->coordindices_.remove( bsz-1 ); \
	else if ( bucket->coordindices_[bsz-3]==-1 ) \
	{ \
	    bucket->coordindices_.remove( bsz-1 ); \
	    bucket->coordindices_.remove( bsz-2 ); \
	} \
	else \
	    bucket->coordindices_ += -1; \
    } \
    if ( normallist_ ) \
	triangles.erase()



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

    const int indicesbucket[] = { pos[mX]/mBucketSize, pos[mY]/mBucketSize,
	    			  pos[mZ]/mBucketSize };

    Geometry::IndexedGeometry* bucket = 0;
    int bucketidx[3];
    geometrieslock_.readLock();
    if ( ibuckets_.findFirst( indicesbucket, bucketidx ) )
    {
	bucket = ibuckets_.getRef( bucketidx, 0 );
	geometrieslock_.readUnLock();
    }
    else
    {
	if ( !geometrieslock_.convToWriteLock() &&
	      ibuckets_.findFirst( indicesbucket, bucketidx ) )
	{
	    bucket = ibuckets_.getRef( bucketidx, 0 );
	}
	else
	{
	    bucket = new Geometry::IndexedGeometry(
		    Geometry::IndexedGeometry::TriangleStrip,
		    Geometry::IndexedGeometry::PerFace, 0, normallist_ );

	    ibuckets_.add( &bucket, indicesbucket );
	    geometries_ += bucket;
	}

	geometrieslock_.writeUnLock();
    }

    const char* tableindices = table.indices_[submodel];

    Threads::MutexLocker lock( bucket->lock_ );

    int arrpos = 0;
    bool gotonextstrip = false;
    TypeSet<Coord3> triangles;
    for ( int idx=0; idx<nrtableindices; idx++ )
    {
	const char neighbor = tableindices[arrpos++];
	if ( neighbor==-1 )
	{
	    gotonextstrip = false;
	    mEndTriangleStrip;
	    continue;
	}

	const char axis = tableindices[arrpos++];

	if ( gotonextstrip )
	    continue;

	int neighborpos[3];
	switch ( neighbor )
	{
	    case 0:
		memcpy( neighborpos, pos, sizeof(int)*3 );
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
	    mEndTriangleStrip;
	    gotonextstrip = true;
	    continue;
	}

	const int index = indices[axis];
	if ( index==-1 )
	{
	    mEndTriangleStrip;
	    gotonextstrip = true;
	    continue;
	}

	bucket->coordindices_ += index;
	if ( normallist_ )
	{
	    triangles += coordlist_->get( index );

	    const int nrtri = triangles.size();
	    const bool reverse = (nrtri % 2)!=righthandednormals_;

	    if ( nrtri>=3 )
	    {
		const Coord3 v0 =
		    triangles[nrtri-(reverse?3:1)]-triangles[nrtri-2];
		const Coord3 v1 =
		    triangles[nrtri-(reverse?1:3)]-triangles[nrtri-2];
		Coord3 normal = v0.cross( v1 );
		if ( !normal.sqAbs() )
		    normal = Coord3( 1, 0, 0 );

		const int normidx = normallist_->add( normal );
		bucket->normalindices_ += normidx;
	    }
	}
    }

    mEndTriangleStrip;
    return true;
}


#undef mEndTriangleStrip


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
    ranges[mX].start /= mBucketSize; ranges[mX].stop /= mBucketSize;
    ranges[mY].start /= mBucketSize; ranges[mY].stop /= mBucketSize;
    ranges[mZ].start /= mBucketSize; ranges[mZ].stop /= mBucketSize;

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
}


bool ExplicitMarchingCubesSurface::getCoordIndices( const int* pos, int* res )
{
    if ( !surface_ )
	return false;

    int cidxs[3];
    coordindiceslock_.readLock();
    if ( !coordindices_.findFirst( pos, cidxs ) )
    {
	coordindiceslock_.readUnLock();
	return false;
    }

    int* indices = &coordindices_.getRef( cidxs, 0 );

    memcpy( res, indices, sizeof(int)*3 );
    coordindiceslock_.readUnLock();
    return true;
}


bool ExplicitMarchingCubesSurface::updateCoordinate( const int* pos, int* res )
{
    int modelidxs[3];
    if ( !surface_->models_.findFirst( pos, modelidxs ) )
	return false;

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

	    if ( scale0_ ) coord[mX] = scale0_->atIndex( coord[mX] );
	    if ( scale1_ ) coord[mY] = scale1_->atIndex( coord[mY] );
	    if ( scale2_ ) coord[mZ] = scale2_->atIndex( coord[mZ] );

	    if ( res[dim]!=-1 )
		coordlist_->set( res[dim], coord );
	    else
		res[dim] = coordlist_->add( coord );
	}
    }

    return true;
}


bool ExplicitMarchingCubesSurface::updateCoordinates( const int* pos )
						
{
    if ( !surface_ )
	return false;

    int cidxs[3];
    coordindiceslock_.readLock();
    if ( !coordindices_.findFirst( pos, cidxs ) )
    {
	if ( coordindiceslock_.convToWriteLock() ||
	     !coordindices_.findFirst( pos, cidxs ) )
	{
	    int indices[] = { -1, -1, -1 };
	    if ( !updateCoordinate( pos, indices ) )
	    {
		coordindiceslock_.writeUnLock();
		return false;
	    }

	    coordindices_.add( indices, pos );
	}

	coordindiceslock_.writeUnLock();
	return true;
    }

    int* indices = &coordindices_.getRef( cidxs, 0 );
    const int res = updateCoordinate( pos, indices );
    coordindiceslock_.readUnLock();

    return res;
}


#define mSetIndices( idx, size ) \
    triangles_[idx].nrindices_ += size; \
    triangles_[idx].indices_ += (char[])

MarchingCubeLookupTable::MarchingCubeLookupTable()
{
    triangles_[0].nrindices_ += 0;
    triangles_[255].nrindices_ += 0;

    #include "marchingcubeslist.h"
}


MarchingCubeLookupTable::~MarchingCubeLookupTable()
{ }


const MarchingCubeLookupTable& MarchingCubeLookupTable::get()
{
    static MarchingCubeLookupTable* table = new MarchingCubeLookupTable;
    return *table;
}
