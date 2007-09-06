/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2006
-*/

static const char* rcsID = "$Id: explicitmarchingcubes.cc,v 1.7 2007-09-06 20:41:23 cvskris Exp $";

#include "explicitmarchingcubes.h"

#include "basictask.h"
#include "marchingcubes.h"
#include "positionlist.h"
#include "samplingdata.h"

#define mX	0
#define mY	1
#define mZ	2

#define mBucketSize 16	


class ExplicitMarchingCubesSurfaceBucket
{
public:
    			ExplicitMarchingCubesSurfaceBucket(CoordList* nl)
			    : normallist_( nl ) { if ( nl ) nl->ref(); }
			~ExplicitMarchingCubesSurfaceBucket()
			{
			    for ( int idx=normalindices_.size()-1; idx>=0;idx--)
			    {
				if ( normalindices_[idx]==-1 )
				    continue;

				normallist_->remove( normalindices_[idx] );
			    }

			    normallist_->unRef();
			}

    Threads::Mutex	lock_;
    TypeSet<int32>	coordindices_;

    CoordList*		normallist_;
    TypeSet<int32>	normalindices_;
};

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
		    int32 res[3];
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
    , coordlist_( 0 )
    , normallist_( 0 )
    , righthandednormals_( true )
{
    if ( surface_ ) surface_->ref();
}


ExplicitMarchingCubesSurface::~ExplicitMarchingCubesSurface()
{
    if ( surface_ ) surface_->unRef();

    setCoordList( 0, 0 );

    deepErase( ibucketsset_ );

    delete scale0_;
    delete scale1_;
    delete scale2_;
}


void ExplicitMarchingCubesSurface::setSurface( MarchingCubesSurface* ns )
{
    if ( surface_ )
	surface_->unRef();

    surface_ = ns;
    removeAll();

    if ( surface_ ) surface_->ref();
    deepErase( ibucketsset_ );
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

    ibucketslock_.writeLock();
    ibuckets_.empty();
    deepErase( ibucketsset_ );
    ibucketslock_.writeUnLock();
}


void ExplicitMarchingCubesSurface::setCoordList( CoordList* cl, CoordList* nl )
{
    removeAll();

    if ( coordlist_ ) coordlist_->unRef();
    coordlist_ = cl;
    if ( coordlist_ ) coordlist_->ref();

    if ( normallist_ ) normallist_->unRef();
    normallist_ = nl;
    if ( normallist_ ) normallist_->ref();
}


void ExplicitMarchingCubesSurface::setRightHandedNormals( bool yn )
{ righthandednormals_ = yn; }


bool ExplicitMarchingCubesSurface::update()
{
    removeAll();
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


int ExplicitMarchingCubesSurface::nrIndicesSets() const
{ return ibucketsset_.size(); }


int ExplicitMarchingCubesSurface::nrCoordIndices( int idx ) const
{ return ibucketsset_[idx]->coordindices_.size(); }


const int* ExplicitMarchingCubesSurface::getCoordIndices( int idx ) const
{ return ibucketsset_[idx]->coordindices_.arr(); }


int ExplicitMarchingCubesSurface::nrNormalIndices( int idx ) const
{ return ibucketsset_[idx]->normalindices_.size(); }


const int* ExplicitMarchingCubesSurface::getNormalIndices( int idx ) const
{ return ibucketsset_[idx]->normalindices_.arr(); }


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

    ExplicitMarchingCubesSurfaceBucket* bucket = 0;
    int bucketidx[3];
    ibucketslock_.readLock();
    if ( ibuckets_.findFirst( indicesbucket, bucketidx ) )
    {
	bucket = ibuckets_.getRef( bucketidx, 0 );
	ibucketslock_.readUnLock();
    }
    else
    {
	if ( !ibucketslock_.convToWriteLock() &&
	      ibuckets_.findFirst( indicesbucket, bucketidx ) )
	{
	    bucket = ibuckets_.getRef( bucketidx, 0 );
	}
	else
	{
	    bucket = new ExplicitMarchingCubesSurfaceBucket( normallist_ );
	    ibuckets_.add( &bucket, indicesbucket );
	    ibucketsset_ += bucket;
	}

	ibucketslock_.writeUnLock();
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

	int32 indices[3];
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

    int32* indices = &coordindices_.getRef( cidxs, 0 );

    memcpy( res, indices, sizeof(int32)*3 );
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
	    int32 indices[] = { -1, -1, -1 };
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

    int32* indices = &coordindices_.getRef( cidxs, 0 );
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
