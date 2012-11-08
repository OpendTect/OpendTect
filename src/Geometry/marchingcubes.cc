/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "marchingcubes.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "datainterp.h"
#include "executor.h"
#include <iostream>
#include "position.h"
#include "threadwork.h"

#define mX 0
#define mY 1
#define mZ 2
#define mWriteChunkSize 100

#define  m2d  360
#define  m3d  442
const unsigned char MarchingCubesModel::cUdfAxisPos = 255;
const unsigned char MarchingCubesModel::cMaxAxisPos = 254;
const unsigned char MarchingCubesModel::cAxisSpacing = 255;

const double  mInitValue = 1e+10;  


class MarchingCubesSurfaceWriter: public Executor
{
public:
    	MarchingCubesSurfaceWriter( std::ostream& strm, 
		const MarchingCubesSurface& s, bool binary )
	    : Executor("MarchingCubes surface writer")
	    , surface_( s )
	    , strm_( strm )
    	    , binary_( binary )
    	    , nrdone_( 0 )
	    {
	    	totalnr_= s.models_.totalSize();
	    	idx_[0] = -1;
	    	idx_[1] = -1;
	    	idx_[2] = -1;
	    }

    od_int64    totalNr() const { return totalnr_; }
    od_int64	nrDone() const { return nrdone_; }
    int     	nextStep()
		{
		    if ( !nrdone_ )
			writeInt32( totalnr_, '\n' );

		    const MultiDimStorage<MarchingCubesModel>& models =
			surface_.models_;
		    for ( int idx=0; idx<mWriteChunkSize; idx++ )
		    {
			if ( !models.next( idx_ ) )
			    return Finished();
			
			int pos[3];
			if ( !models.getPos( idx_, pos ) )
			    return ErrorOccurred();
			
			writeInt32( pos[mX], '\t' );
			writeInt32( pos[mY], '\t' );
			writeInt32( pos[mZ], '\t' );
			
			if ( !models.getRef(idx_,0).writeTo(strm_,binary_))
			    return ErrorOccurred();

			nrdone_++;
		    }
		    
		    return MoreToDo();
		}

	void    writeInt32( int val, char post )
		{
		    if ( binary_ )
			strm_.write((const char*)&val,sizeof(val));
		    else
			strm_ << val << post;
		}

protected:
    int                 	idx_[3];
    bool                	binary_;
    int                 	nrdone_;
    int                 	totalnr_;
    const MarchingCubesSurface& surface_;
    std::ostream&		strm_;
};



class MarchingCubesSurfaceReader : public Executor
{
public:
MarchingCubesSurfaceReader( std::istream& strm, MarchingCubesSurface& s,
			    const DataInterpreter<od_int32>* dt )
    : Executor("MarchingCubes surface writer")
    , surface_( s )
    , strm_( strm )
    , dt_( dt )
    , nrdone_( 0 )
    , totalnr_( -1 )
{}

od_int64 totalNr() const { return totalnr_; }
od_int64 nrDone() const { return nrdone_; }
int nextStep()
{
    if ( !nrdone_ )
    {
	totalnr_ = readInt32();
	if ( !totalnr_ )
	    return Finished();
    }

    if ( !strm_ )
	return ErrorOccurred();

    for ( int idx=0; idx<mWriteChunkSize; idx++ )
    {
	int pos[3];
	pos[mX] = readInt32();
	pos[mY] = readInt32();
	pos[mZ] = readInt32();

	MarchingCubesModel model;
	if ( !model.readFrom( strm_, dt_ ) )
	    return ErrorOccurred();

	surface_.modelslock_.writeLock();
	surface_.models_.add<MarchingCubesModel*,const int*, int*>
	    ( &model, pos, 0 );
	surface_.modelslock_.writeUnLock();
	nrdone_++;
	if ( nrdone_==totalnr_ )
	    return Finished();
    }
    
    return MoreToDo();
}


int readInt32()
{
    if ( dt_ )
    {
	const int sz = dt_->nrBytes();
	ArrPtrMan<char> buf = new char [sz];
	strm_.read(buf,sz);
	return dt_->get(buf,0);
    }

    int res;
    strm_ >> res;
    return res;
}


protected:
    int                 	nrdone_;
    int                 	totalnr_;
    MarchingCubesSurface&	surface_;
    std::istream&		strm_;
    const DataInterpreter<od_int32>* dt_;
};


MarchingCubesModel::MarchingCubesModel()
    : model_( 0 )
    , submodel_( 0 )
{
    axispos_[mX] = cUdfAxisPos;
    axispos_[mY] = cUdfAxisPos;
    axispos_[mZ] = cUdfAxisPos;
}


MarchingCubesModel::MarchingCubesModel( const MarchingCubesModel& templ )
    : model_( templ.model_ )
    , submodel_( templ.submodel_ )
{
    axispos_[mX] = templ.axispos_[mX];
    axispos_[mY] = templ.axispos_[mY];
    axispos_[mZ] = templ.axispos_[mZ];
}


MarchingCubesModel& MarchingCubesModel::operator=( const MarchingCubesModel& b )
{
    model_ = b.model_;
    submodel_ = b.submodel_;

    axispos_[mX] = b.axispos_[mX];
    axispos_[mY] = b.axispos_[mY];
    axispos_[mZ] = b.axispos_[mZ];

    return *this;
}


bool MarchingCubesModel::operator==( const MarchingCubesModel& mc ) const
{ return mc.model_==model_ && mc.submodel_==mc.submodel_; }


#define mCalcCoord( valnr, axis ) \
    if ( use##valnr && use000 && sign000!=sign##valnr ) \
    { \
	const float factor = diff000/(val##valnr-val000); \
	int axispos = (int) (factor*cAxisSpacing); \
	if ( axispos<0 ) axispos=0; \
	else if ( axispos>cMaxAxisPos ) \
	    axispos = cMaxAxisPos; \
	axispos_[axis] = mCast( unsigned char, axispos ); \
    } \
    else \
	axispos_[axis] = cUdfAxisPos; \


bool MarchingCubesModel::set( const Array3D<float>& arr, int i0, int i1, int i2,
		              float threshold )
{
    const bool use0 = i0!=arr.info().getSize( mX )-1;
    const bool use1 = i1!=arr.info().getSize( mY )-1;
    const bool use2 = i2!=arr.info().getSize( mZ )-1;

    const float val000 = arr.get( i0, i1, i2 );
    const float val001 = use2 ? arr.get( i0, i1, i2+1 ) : mUdf(float);
    const float val010 = use1 ? arr.get( i0, i1+1, i2 ) : mUdf(float);
    const float val011 = use1 && use2 ? arr.get( i0, i1+1, i2+1 ) : mUdf(float);
    const float val100 = use0 ? arr.get( i0+1, i1, i2 ) : mUdf(float);
    const float val101 = use0 && use2 ? arr.get( i0+1, i1, i2+1 ) : mUdf(float);
    const float val110 = use0 && use1 ? arr.get( i0+1, i1+1, i2 ) : mUdf(float);
    const float val111 = use0 && use1 && use2
		? arr.get( i0+1, i1+1, i2+1 ) : mUdf(float);

    const bool use000 = !mIsUdf(val000);
    const bool use001 = use2 && !mIsUdf(val001);
    const bool use010 = use1 && !mIsUdf(val010);
    const bool use011 = use1 && use2 && !mIsUdf(val011);
    const bool use100 = use0 && !mIsUdf(val100);
    const bool use101 = use0 && use2 && !mIsUdf(val101);
    const bool use110 = use0 && use1 && !mIsUdf(val110);
    const bool use111 = use0 && use1 && use2 && !mIsUdf(val111);

    /*
    if ( !use2 && use000 && use010 && use100 && use110 &&
	 mIsEqual(threshold,val000,eps) &&
	 mIsEqual(threshold,val010,eps) &&
	 mIsEqual(threshold,val100,eps) &&
	 mIsEqual(threshold,val110,eps))
    {
	model_ = 0;
	submodel = 0;
	axispos[mX] = cUdfAxisPos;
	axispos[mY] = cUdfAxisPos;
	axispos[mZ] = 0;
	return true;
    }
    */

    const float diff000 = threshold-val000;
    const bool sign000 = use000 && val000>threshold;
    const bool sign001 = use001 && val001>threshold;
    const bool sign010 = use010 && val010>threshold;
    const bool sign100 = use100 && val100>threshold;

    if ( !use2 && !use1 && !use0 )
    {
	model_ = 0;
	submodel_ = 0;
	axispos_[mZ] = cUdfAxisPos;
	axispos_[mY] = cUdfAxisPos;
	axispos_[mX] = cUdfAxisPos;
	return true;
    }

    if ( use000 && use001 && use010 && use011 &&
	 use100 && use101 && use110 && use111 )
    {
	model_ = determineModel( sign000, sign001, sign010,
		    val011>threshold, sign100, val101>threshold,
		    val110>threshold, val111>threshold );
    }
    else if ( use000 && sign000 )
	model_ = 255;
    else
	model_ = 0;

    mCalcCoord( 001, mZ );
    mCalcCoord( 010, mY );
    mCalcCoord( 100, mX );
    return true;
}


bool MarchingCubesModel::isEmpty() const
{
    if ( model_ && model_!=255 )
	return false;

    return axispos_[mX]==cUdfAxisPos &&
	   axispos_[mY]==cUdfAxisPos &&
	   axispos_[mZ]==cUdfAxisPos;
}


unsigned char MarchingCubesModel::determineModel( bool c000, bool c001,
	bool c010, bool c011, bool c100, bool c101, bool c110, bool c111 )
{
    return c000 + (c001 << 1) + (c010 << 2) + (c011 << 3) + (c100 << 4) +
		  (c101 << 5) + (c110 << 6) + (c111 << 7);
}


bool MarchingCubesModel::getCornerSign( unsigned char model, int corner )
{
    model = model >> corner;
    return model&1;
}


bool MarchingCubesModel::writeTo( std::ostream& strm,bool binary ) const
{
    if ( binary )
	strm.write( (const char*) &model_, 5 );
    else
	strm << (int) model_ << '\t' << (int) submodel_ << '\t' <<
	        (int) axispos_[mX] << '\t' << (int) axispos_[mY]<< '\t' <<
		(int) axispos_[mZ] <<'\n';
    return strm;
}


bool MarchingCubesModel::readFrom( std::istream& strm, bool binary )
{
    if ( binary )
	strm.read( (char*) &model_, 5 );
    else
    {
	int res;
	strm >> res; model_ = mCast( unsigned char, res );
	strm >> res; submodel_ = mCast( unsigned char, res );
	strm >> res; axispos_[mX] = mCast( unsigned char, res );
	strm >> res; axispos_[mY] = mCast( unsigned char, res );
	strm >> res; axispos_[mZ] = mCast( unsigned char, res );
    }

    return strm;
}


MarchingCubesSurface::MarchingCubesSurface()
    : models_( 3, 1 )
    , change( this )
    , impvoldata_( 0 )		    
{}


MarchingCubesSurface::~MarchingCubesSurface()
{
    removeAll();
    delete impvoldata_;
}


bool MarchingCubesSurface::setVolumeData( int xorigin, int yorigin, int zorigin,
		const Array3D<float>& arr, float threshold, TaskRunner* tr )
{
    if ( impvoldata_ )
	delete impvoldata_;
    mDeclareAndTryAlloc( Array3DImpl<float>*, nvoldata, 
	    Array3DImpl<float>(arr.info()) );
    if ( nvoldata )
	nvoldata->copyFrom(arr);
    impvoldata_ = nvoldata;

    const bool wasempty = models_.isEmpty();

    Implicit2MarchingCubes converter( xorigin, yorigin, zorigin, arr, threshold,
	    			      *this );
    const bool res = tr ? tr->execute( converter ) : converter.execute();

    if ( wasempty )
	allchanged_ = true;
    else
    {
	changepos_[mX].start = xorigin;
	changepos_[mX].stop = xorigin+arr.info().getSize(mX)-1;

	changepos_[mY].start = yorigin;
	changepos_[mY].stop = yorigin+arr.info().getSize(mY)-1;

	changepos_[mZ].start = zorigin;
	changepos_[mZ].stop = zorigin+arr.info().getSize(mZ)-1;
	allchanged_ = false;
    }

    change.trigger();

    return res;
}


void MarchingCubesSurface::removeAll()
{
    modelslock_.writeLock();
    models_.empty();
    modelslock_.writeUnLock();
}


bool MarchingCubesSurface::isEmpty() const
{
    return models_.isEmpty();
}


bool MarchingCubesSurface::getModel(const int* pos, unsigned char& model,
				    unsigned char& submodel) const
{
    int idxs[3];
    if ( !models_.findFirst( pos, idxs ) )
	return false;

    const MarchingCubesModel& cube = models_.getRef( idxs, 0 );
    model = cube.model_;
    submodel = cube.submodel_;
    return true;
}


Executor* MarchingCubesSurface::writeTo( std::ostream& strm, bool binary ) const
{
    return new ::MarchingCubesSurfaceWriter( strm, *this, binary );
}


Executor* MarchingCubesSurface::readFrom( std::istream& strm,
	const DataInterpreter<od_int32>* dt )
{
    return new ::MarchingCubesSurfaceReader( strm, *this, dt );
}


Implicit2MarchingCubes:: Implicit2MarchingCubes( int posx, int posy, int posz,
						 const Array3D<float>& arr, 
						 float threshold,
						 MarchingCubesSurface& mcs )
    : surface_( mcs )
    , threshold_( threshold )
    , array_( arr )			    
    , xorigin_( posx )
    , yorigin_( posy )
    , zorigin_( posz )		      
{}
    
Implicit2MarchingCubes::~Implicit2MarchingCubes() {}


od_int64 Implicit2MarchingCubes::nrIterations() const
{
    return array_.info().getTotalSz();
}   


bool Implicit2MarchingCubes::doWork( od_int64 start, od_int64 stop, int )
{
    int arraypos[3];
    array_.info().getArrayPos( start, arraypos );

    ArrayNDIter iterator( array_.info() );
    iterator.setPos( arraypos );

    const int nriters = mCast( int, stop-start+1 );
    for ( int idx=0; idx<nriters && shouldContinue();
	  idx++, iterator.next(), addToNrDone(1) )
    {
	const int pos[] = { iterator[mX]+xorigin_, iterator[mY]+yorigin_,
			    iterator[mZ]+zorigin_ };
	MarchingCubesModel model;
        if (model.set(array_,iterator[mX],iterator[mY],iterator[mZ],threshold_))
	{
	    if ( !model.isEmpty() )
	    {
		surface_.modelslock_.writeLock();
		surface_.models_.add<MarchingCubesModel*,const int*, int*>
		    ( &model, pos, 0 );
		surface_.modelslock_.writeUnLock();
	    }
	    else
	    {
		surface_.modelslock_.readLock();
		int idxs[3];
		if ( !surface_.models_.findFirst( pos, idxs ) )
		{
		    surface_.modelslock_.readUnLock();
		    continue;
		}

		if ( surface_.modelslock_.convReadToWriteLock() ||
		     surface_.models_.findFirst( pos, idxs ) )
		{
		    surface_.models_.remove( idxs );
		}

		surface_.modelslock_.writeUnLock();
	    }
	}
    }

    return true;
}


/*!Fills an array3d with data from an MarchingCubes surface. Does only 
   set the adjacent to the surface itself. */

class MarchingCubes2ImplicitDistGen : public ParallelTask
{
public:
    MarchingCubes2ImplicitDistGen( MarchingCubes2Implicit& mc2i,
	   			      bool nodistance )
	: mc2i_( mc2i )
	, totalnr_( mc2i.surface_.models_.totalSize() )
	, nodistance_( nodistance )
    {
	mc2i_.result_.setAll( mUdf(int) );
    }

    od_int64	nrIterations() const { return totalnr_; }

protected:
    bool doWork( od_int64 start, od_int64 stop, int )
    {
	const int nrtimes = mCast( int, stop-start+1 );
	int surfaceidxs[3];
	if ( !mc2i_.surface_.models_.getIndex( mCast(int,start), surfaceidxs ) )
	    return false;

	if ( !mc2i_.surface_.models_.isValidPos( surfaceidxs ) )
	    return false;

	Interval<int> dimranges[3];
	dimranges[0].start = mc2i_.originx_;
	dimranges[0].stop = mc2i_.originx_ + mc2i_.size_[mX]-1;
	dimranges[1].start = mc2i_.originy_;
	dimranges[1].stop = mc2i_.originy_ + mc2i_.size_[mY]-1;
	dimranges[2].start = mc2i_.originz_;
	dimranges[2].stop = mc2i_.originz_ + mc2i_.size_[mZ]-1;

	TypeSet<od_int64> offsets;
	TypeSet<int> vals;

	for ( int idx=0; idx<nrtimes; idx++, addToNrDone(1) )
	{
	    int modelpos[3];
	    if ( !mc2i_.surface_.models_.getPos( surfaceidxs, modelpos ) )
		return false;

	    const MarchingCubesModel& model =
		mc2i_.surface_.models_.getRef( surfaceidxs, 0 );
	    const bool originsign = model.getCornerSign( model.model_, 0 );
	    const bool neighborsign = !originsign;

	    int mindist;
	    bool isset = false;
	    for ( int dim=0; dim<3; dim++ )
	    {
		if ( model.axispos_[dim]==MarchingCubesModel::cUdfAxisPos )
		    continue;

		int dist = model.axispos_[dim];
		
		if ( !isset || dist<mindist )
		{
		    mindist = dist;
		    isset = true;
		}

		int neighbordist;
		int neighborpos[] ={modelpos[mX],modelpos[mY],modelpos[mZ]};
		neighborpos[dim] += 1;
		if ( dimranges[dim].includes( neighborpos[dim], false ) )
		{
		    neighbordist = MarchingCubesModel::cAxisSpacing-dist;
	  	    const od_int64 offset = mc2i_.result_.info().getOffset(
			    neighborpos[mX]-mc2i_.originx_,
			    neighborpos[mY]-mc2i_.originy_,
			    neighborpos[mZ]-mc2i_.originz_ );
		    const int val = nodistance_
			? (neighborsign ? 1 : -1)
			: (neighborsign ? neighbordist : -neighbordist);
		    if ( mc2i_.shouldSetValue( offset, val ) )
		    {
			vals += val;
			offsets += offset;
		    }
		}

		//if ( dist ) continue;

		neighborpos[dim] -= 2;
		if ( dimranges[dim].includes( neighborpos[dim], false ) )
		{
		    int neighboridxs[3];
		    if ( mc2i_.surface_.models_.findFirst( neighborpos,
						     neighboridxs ) )
			continue;
		    
		    neighbordist = MarchingCubesModel::cAxisSpacing+dist;
	  	    const od_int64 offset = mc2i_.result_.info().getOffset(
			    neighborpos[mX]-mc2i_.originx_,
			    neighborpos[mY]-mc2i_.originy_,
			    neighborpos[mZ]-mc2i_.originz_ );
		    const int val = nodistance_
			? (originsign ? 1 : -1)
			: (originsign ? neighbordist : -neighbordist);
		    if ( mc2i_.shouldSetValue( offset, val ) )
		    {
			vals += val;
			offsets += offset;
		    }
		}
	    }
	    
	    if ( isset )
	    {
		const od_int64 offset = mc2i_.result_.info().getOffset(
			modelpos[mX]-mc2i_.originx_,
			modelpos[mY]-mc2i_.originy_,
			modelpos[mZ]-mc2i_.originz_ );
		const int val = nodistance_
		    ? (originsign ? 1 : -1)
		    : (originsign ? mindist : -mindist);
		if ( mc2i_.shouldSetValue( offset, val ) )
		{
		    vals += val;
		    offsets += offset;
		}
	    }

	    if ( idx!=nrtimes-1 && !mc2i_.surface_.models_.next( surfaceidxs ) )
		return false;
	}

	mutex_.lock();

	for ( int idx=0; idx<offsets.size(); idx++ )
	    mc2i_.setValue( offsets[idx], vals[idx], true );

	mutex_.unLock();
	
	return true;
    }

    int				totalnr_;
    MarchingCubes2Implicit&	mc2i_;
    Threads::Mutex		mutex_;
    bool			nodistance_;
};


MarchingCubes2Implicit::MarchingCubes2Implicit( 
		const MarchingCubesSurface& surface,
	        Array3D<int>& arr, int originx, int originy, int originz,
       		bool nodistance	)
    : surface_( surface )
    , result_( arr )
    , originx_( originx )
    , originy_( originy )
    , originz_( originz )
    , newfloodfillers_( new bool[arr.info().getTotalSz()] )
    , nrdefined_( 0 )
    , nodistance_( nodistance )
{
    for ( int idx=0; idx<3; idx++ )
	size_[idx] = arr.info().getSize( idx );
}


MarchingCubes2Implicit::~MarchingCubes2Implicit()
{
    delete [] newfloodfillers_;
}


od_int64 MarchingCubes2Implicit::nrIterations() const
{ return result_.info().getTotalSz(); }


od_int64 MarchingCubes2Implicit::nrDone() const
{
    Threads::MutexLocker lock( barrier_.mutex() );

    return nrdefined_ - activefloodfillers_.size();
}


bool MarchingCubes2Implicit::doPrepare( int nrthreads )
{
    if ( !newfloodfillers_ )
	return false;

    //Set no position as seed
    MemSetter<bool> setter( newfloodfillers_, false, nrIterations() );
    setter.execute();

    nrdefined_ = 0;
    barrier_.setNrThreads( nrthreads );

    //Populate array with seeds
    MarchingCubes2ImplicitDistGen distget( *this, nodistance_ );
    if ( !distget.execute() )
	return false;

    return true;
}



bool MarchingCubes2Implicit::doWork( od_int64 start, od_int64 stop,
					int thread )
{
    while ( shouldContinue() )
    {
	//Get seeds from newfloodfillers_ and polulate activefloodfillers_
	TypeSet<od_int64> newfloodfillers;
	for ( int idx=mCast(int,start); idx<=stop; idx++ )
	{
	    if ( newfloodfillers_[idx] )
	    {
		newfloodfillers_[idx] = 0;
		newfloodfillers += idx;
	    }
	}

	if ( barrier_.waitForAll( false ) )		//Start single thread
	    activefloodfillers_.erase();		//
							//
	activefloodfillers_.append( newfloodfillers );	//
	barrier_.mutex().unLock();			//End single thread

	barrier_.waitForAll();

	//Divide the seeds in activefloodfillers_ between the threads and 
	//process them in paralell.
	const int nrseeds = activefloodfillers_.size();
	if ( !nrseeds )
	    break;

	const int nrperthread = nrseeds/barrier_.nrThreads();
	const int startseed = thread*nrperthread;
	const int stopseed = mMIN(startseed+nrperthread,nrseeds)-1;

	const int sz = stopseed-startseed+1;
	processSeeds( activefloodfillers_.arr()+startseed, sz );
    }

    return true;
}


#define mChkN( doffset, dval ) \
{ \
    const od_int64 offset = curoffset+doffset; \
    const int nv = resptr[offset]; \
    if ( nv && !Values::isUdf(nv) ) \
    { \
	const bool nbsign = nv>0; \
	const int val = nbsign \
		? prevvalue + dval \
		: prevvalue - dval; \
	if ( shouldSetValue( offset, val ) ) \
	{ \
	    newseeds += offset; \
	    vals += val; \
	} \
\
	if ( shouldSetValue( curoffset+doffset, dval ) ) \
	{ \
	    newseeds += offset; \
	    vals += dval; \
	} \
    } \
\
}


bool MarchingCubes2Implicit::processSeeds( const od_int64* offsets, int nr )
{
    TypeSet<od_int64> newseeds;
    TypeSet<int> vals;

    const int capactity = nr * (nodistance_ ? 6 : 26 );
    newseeds.setCapacity( capactity );
    vals.setCapacity( capactity );

    const int* resptr = result_.getData();

    //Process all seeds in the list, add new seeds to newseeds and vals
    for ( int offsetidx=0; offsetidx<nr; offsetidx++ )
    {
	od_int64 curoffset = offsets[offsetidx];
	int arrpos[3];
	result_.info().getArrayPos( curoffset, arrpos );

	const int prevvalue = resptr[curoffset];

	if ( !prevvalue )
	    continue;

	const int neighborval = nodistance_
	    ? (prevvalue>0 ? 1 : -1 )
	    : (prevvalue>0
		? prevvalue+MarchingCubesModel::cAxisSpacing
		: prevvalue-MarchingCubesModel::cAxisSpacing);

	const int doff[] = { size_[mY]*size_[mZ], size_[mZ], 1 };

	//First, go in a cross (6 connectivity)
	for ( int idx=-1; idx<=1; idx+=2 )
	{
	    for ( int dim=0; dim<3; dim++ )
	    {
		const int nxarrpos = arrpos[dim]+idx;
		if ( nxarrpos<0 || nxarrpos>=size_[dim] )
		    continue;

		const od_int64 offset = curoffset + doff[dim]*idx;
		if ( shouldSetValue( offset, neighborval ) )
		{
		    newseeds += offset;
		    vals += neighborval;
		}
	    }
	}

	if ( nodistance_ )
	    continue;

	//Secondly, go to 26 connectivity, but don't change any udf values
	//to defined. This may spread cause the fill to pass the iso-surface.
	
	const bool doprevy = arrpos[mY]>0;
	const bool donexty = arrpos[mY]<size_[mY]-1;
	const bool doprevz = arrpos[mZ]>0;
	const bool donextz = arrpos[mZ]<size_[mZ]-1;

	if ( arrpos[mX]>0 )
	{
	    if ( doprevy )
	    {
		if ( doprevz )
		    mChkN( -doff[mX]	-doff[mY]	-doff[mZ]	, m2d );

		mChkN(	   -doff[mX]	-doff[mY]			, m2d );

		if ( donextz )
		    mChkN( -doff[mX]	-doff[mY]	+doff[mZ]	, m2d );
	    }
	
	    if ( doprevz )
		mChkN(	   -doff[mX]			-doff[mZ]	, m2d );
	    if ( donextz )
		mChkN(	   -doff[mX]			+doff[mZ]	, m2d );

	    if ( donexty )
	    {
		if ( doprevz )
		    mChkN( -doff[mX]	+doff[mY]	-doff[mZ]	, m2d );

		mChkN(	   -doff[mX]	+doff[mY]			, m2d );

		if ( donextz )
		    mChkN( -doff[mX]	+doff[mY]	+doff[mZ]	, m2d );
	    }
	}

	if ( doprevy )
	{
	    if ( doprevz )
		mChkN( 			-doff[mY]	-doff[mZ]	, m2d );
	    if ( donextz )
		mChkN( 			-doff[mY]	+doff[mZ]	, m2d );
	}

	if ( donexty )
	{
	    if ( doprevz )
		mChkN( 			 doff[mY]	-doff[mZ]	, m2d );
	    if ( donextz )
		mChkN( 			 doff[mY]	+doff[mZ]	, m2d );
	}

	if ( arrpos[mX]<size_[mX]-1 )
	{
	    if ( doprevy )
	    {
		if ( doprevz )
		    mChkN(   doff[mX] 	-doff[mY]	-doff[mZ]	, m2d );

		mChkN(	     doff[mX] 	-doff[mY]			, m2d );

		if ( donextz )
		    mChkN(   doff[mX] 	-doff[mY]	+doff[mZ]	, m2d );
	    }

	    if ( doprevz )
		mChkN(	     doff[mX] 			-doff[mZ]	, m2d );
	    if ( donextz )
		mChkN(	     doff[mX] 			+doff[mZ]	, m2d );

	    if ( donexty )
	    {
		if ( doprevz )
		    mChkN(   doff[mX] 	+doff[mY]	-doff[mZ]	, m2d );

		mChkN(	     doff[mX] 	+doff[mY]			, m2d );

		if ( donextz )
		    mChkN(   doff[mX] 	+doff[mY]	+doff[mZ]	, m2d );
	    }
	}
    }

    //Populate the array with the new results. Checking is not needed for the
    //first thread, as nothing has changed since last thread.

    const bool docheck = barrier_.waitForAll( false );	//Start single thread
    for ( int idx=0; idx<newseeds.size(); idx++ )	//
	setValue( newseeds[idx], vals[idx], docheck );	//
							//
    barrier_.mutex().unLock();				//End single thread

    return true;
}


bool MarchingCubes2Implicit::shouldSetValue( od_int64 offset, int newval )
{
    const int prevvalue = result_.getData()[offset];
    if ( !mIsUdf( prevvalue ) )
    {
	const bool prevsign = prevvalue>0;
	const bool newsign = newval>0;
	
	if ( prevsign!=newsign )
	    return false;

	const int prevdist = abs(prevvalue);
	const int newdist = abs(newval);
	
	if ( prevdist<=newdist )
	    return false;
    }

    return true;
}


void MarchingCubes2Implicit::setValue( od_int64 offset, int newval,
					  bool check )
{
    if ( check && !shouldSetValue( offset, newval ) )
	return;

    int* resultptr = result_.getData() + offset;

    if ( mIsUdf(*resultptr) )
	nrdefined_++;

    *resultptr = newval;
    newfloodfillers_[offset] = true;
}


