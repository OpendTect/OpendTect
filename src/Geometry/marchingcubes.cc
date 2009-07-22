/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2006
-*/

static const char* rcsID = "$Id: marchingcubes.cc,v 1.25 2009-07-22 16:01:33 cvsbert Exp $";

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

#define  m2DiagSpacing  360
#define  m3DiagSpacing  442
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
	axispos_[axis] = axispos; \
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
	return false; //How about on the edge?

	model_ = 0;
	submodel_ = 0;
	if ( !use2 )
	    axispos_[mZ] = cUdfAxisPos;
	else
	    mCalcCoord( 001, mZ );

	if ( !use1 )
	    axispos_[mY] = cUdfAxisPos;
	else
	    mCalcCoord( 010, mY );

	if ( !use0 )
	    axispos_[mX] = cUdfAxisPos;
	else
	    mCalcCoord( 100, mX );

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
	strm >> res; model_ = res;
	strm >> res; submodel_ = res;
	strm >> res; axispos_[mX] = res;
	strm >> res; axispos_[mY] = res;
	strm >> res; axispos_[mZ] = res;
    }

    return strm;
}


MarchingCubesSurface::MarchingCubesSurface()
    : models_( 3, 1 )
    , change( this )
{}


MarchingCubesSurface::~MarchingCubesSurface()
{
    removeAll();
}


bool MarchingCubesSurface::setVolumeData( int xorigin, int yorigin, int zorigin,
		const Array3D<float>& arr, float threshold, TaskRunner* tr )
{
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

    const int nriters = stop-start+1;
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


class MarchingCubes2ImplicitDistGen : public ParallelTask
{
public:
    MarchingCubes2ImplicitDistGen( MarchingCubes2Implicit& mc2i )
	: mc2i_( mc2i )
	, totalnr_( mc2i.surface_.models_.totalSize() )
    {
	mc2i_.result_.setAll( mUdf(int) );
    }

    od_int64	nrIterations() const { return totalnr_; }

protected:
    bool doWork( od_int64 start, od_int64 stop, int )
    {
	const int nrtimes = stop-start+1;
	int surfaceidxs[3];
	if ( !mc2i_.surface_.models_.getIndex( start, surfaceidxs ) )
	    return false;

	if ( !mc2i_.surface_.models_.isValidPos( surfaceidxs ) )
	    return false;

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

		int neighborpos[] ={modelpos[mX],modelpos[mY],modelpos[mZ]};
		neighborpos[dim] += 1;

		int neighbordist = MarchingCubesModel::cAxisSpacing-dist;
		mc2i_.setValue(neighborpos[mX],neighborpos[mY],
			       neighborpos[mZ],
			       neighborsign ? neighbordist : -neighbordist );

		//if ( dist ) continue;

		neighborpos[dim] -= 2;
		int neighboridxs[3];
		if ( mc2i_.surface_.models_.findFirst( neighborpos,
						 neighboridxs ) )
		    continue;
		
		neighbordist = MarchingCubesModel::cAxisSpacing+dist;
		mc2i_.setValue(neighborpos[mX],neighborpos[mY],
			       neighborpos[mZ],
			       originsign ? neighbordist : -neighbordist );
	    }
	    
	    if ( isset )
	    {
		mc2i_.setValue( modelpos[mX], modelpos[mY], modelpos[mZ],
			originsign ? mindist : -mindist );
	    }

	    if ( idx!=nrtimes-1 && !mc2i_.surface_.models_.next( surfaceidxs ) )
		return false;
	}
	
	return true;
    }

    int				totalnr_;
    MarchingCubes2Implicit&	mc2i_;
};


class MarchingCuebs2ImplicitFloodFiller : public SequentialTask
{
public:
    MarchingCuebs2ImplicitFloodFiller( MarchingCubes2Implicit& mc2i,
	   				int arrposx, int arrposy, int arrposz )
	: mc2i_( mc2i )
	, arrposx_( arrposx )
	, arrposy_( arrposy )
	, arrposz_( arrposz )
    {}

    int nextStep()
    {
	mc2i_.resultlock_.readLock();
	const int prevvalue = mc2i_.result_.get( arrposx_, arrposy_, arrposz_ );
	mc2i_.resultlock_.readUnLock();

	if ( !prevvalue )
	    return Finished();

	const int neighborval = prevvalue>0
	    ? prevvalue+MarchingCubesModel::cAxisSpacing
	    : prevvalue-MarchingCubesModel::cAxisSpacing;

	for ( int idx=-1; idx<=1; idx++ )
	{
	    if ( !idx ) continue;
	    const int nxarrpos = arrposx_+idx;
	    if ( nxarrpos<0 || nxarrpos>=mc2i_.result_.info().getSize(mX) )
		continue;

	    mc2i_.setValue( nxarrpos+mc2i_.originx_, arrposy_+mc2i_.originy_, 
			    arrposz_+mc2i_.originz_, neighborval );
	}

	for ( int idy=-1; idy<=1; idy++ )
	{
	    if ( !idy ) continue;
	    const int nyarrpos = arrposy_+idy;
	    if ( nyarrpos<0 || nyarrpos>=mc2i_.result_.info().getSize(mY) )
		continue;

	    mc2i_.setValue( arrposx_+mc2i_.originx_, nyarrpos+mc2i_.originy_, 
			    arrposz_+mc2i_.originz_, neighborval );
	}

	for ( int idz=-1; idz<=1; idz++ )
	{
	    if ( !idz ) continue;
	    const int nzarrpos = arrposz_+idz;
	    if ( nzarrpos<0 || nzarrpos>=mc2i_.result_.info().getSize(mZ) )
		continue;

	    mc2i_.setValue( arrposx_+mc2i_.originx_, arrposy_+mc2i_.originy_, 
			    nzarrpos+mc2i_.originz_, neighborval );
	}
	

	for ( int idx=-1; idx<=1; idx++ )   
	{
	    for ( int idy=-1; idy<=1; idy++ )
	    {
		for ( int idz=-1; idz<=1; idz++ )
		{
		    if ( (!idx && !idy) || (!idx && !idz) || 
			 (!idy && !idz) )
			continue;
		    
		    const int nxarrpos = arrposx_+idx;
		    const int nyarrpos = arrposy_+idy;
		    const int nzarrpos = arrposz_+idz;
		    if ( nxarrpos<0 || nyarrpos<0 || nzarrpos<0 ||
			 nxarrpos>=mc2i_.result_.info().getSize(mX) ||
			 nyarrpos>=mc2i_.result_.info().getSize(mY) ||
			 nzarrpos>=mc2i_.result_.info().getSize(mZ) )
			continue;

		    const int nv=mc2i_.result_.get(nxarrpos,nyarrpos,nzarrpos);
 		    if ( Values::isUdf(nv) || !nv ) 
 			continue;
 
		    bool nbsign = nv>0;
 		    if ( !idx || !idy || !idz )    
 		    {
 			mc2i_.setValue( nxarrpos+mc2i_.originx_, 
				nyarrpos+mc2i_.originy_, 
				nzarrpos+mc2i_.originz_, 
				nbsign ? prevvalue + m2DiagSpacing :
				prevvalue - m2DiagSpacing );
 		    }
 		    else
 		    {
 			mc2i_.setValue( nxarrpos+mc2i_.originx_, 
				nyarrpos+mc2i_.originy_, 
				nzarrpos+mc2i_.originz_, 
				nbsign ? prevvalue + m3DiagSpacing:
				prevvalue - m3DiagSpacing );
		    }
		}
	    }
	}

	return Finished();
    }

    void setIndices( int idx, int idy, int idz )
    {
	arrposx_ = idx;
	arrposy_ = idy;
	arrposz_ = idz;
    }

protected:
    MarchingCubes2Implicit&	mc2i_;
    int				arrposx_;
    int				arrposy_;
    int				arrposz_;
};


MarchingCubes2Implicit::MarchingCubes2Implicit( 
		const MarchingCubesSurface& surface,
	        Array3D<int>& arr, int originx, int originy, int originz )
    : surface_( surface )
    , result_( arr )
    , originx_( originx )
    , originy_( originy )
    , originz_( originz )
{ }


MarchingCubes2Implicit::~MarchingCubes2Implicit()
{
    deepErase( newfloodfillers_ );
    deepErase( activefloodfillers_ );
    deepErase( oldfloodfillers_ );
}


bool MarchingCubes2Implicit::compute()
{
    MarchingCubes2ImplicitDistGen distget( *this );
    if ( !distget.execute() )
	return false;

    return floodFill();
}


bool MarchingCubes2Implicit::floodFill()
{
    Threads::ThreadWorkManager workman;

    resultlock_.writeLock();
    while ( newfloodfillers_.size() )
    {
	deepErase( oldfloodfillers_ );
	oldfloodfillers_ = activefloodfillers_;
	activefloodfillers_ = newfloodfillers_;
	newfloodfillers_.erase();
	resultlock_.writeUnLock();

	workman.addWork( (ObjectSet<SequentialTask>&) activefloodfillers_ );

	resultlock_.writeLock();
    }

    resultlock_.writeUnLock();

    return true;
}


void  MarchingCubes2Implicit::setValue( int xpos,int ypos,int zpos,int newval )
{
    const Interval<int> xrange(originx_,originx_+result_.info().getSize(mX)-1);
    const Interval<int> yrange(originy_,originy_+result_.info().getSize(mY)-1);
    const Interval<int> zrange(originz_,originz_+result_.info().getSize(mZ)-1);
    
    if ( !xrange.includes(xpos) || !yrange.includes(ypos) ||
	 !zrange.includes(zpos) )
    {
	return;
    }

    const int idx = xpos-originx_;
    const int idy = ypos-originy_;
    const int idz = zpos-originz_;

    resultlock_.readLock();
    const bool shouldset = shouldSetResult( newval, result_.get(idx,idy,idz) );

    if ( !shouldset )
    {
	resultlock_.readUnLock();
	return;
    }

    if ( !resultlock_.convReadToWriteLock() &&
	 !shouldSetResult( newval, result_.get(idx,idy,idz) ) )
    {
	resultlock_.writeUnLock();
	return;
    }

    result_.set( idx, idy, idz, newval );

    MarchingCuebs2ImplicitFloodFiller* floodfiller = 0;
    if ( oldfloodfillers_.size() )
    {
	floodfiller = oldfloodfillers_.remove( oldfloodfillers_.size()-1 );
	floodfiller->setIndices( idx, idy, idz );
    }
    else
	floodfiller = new MarchingCuebs2ImplicitFloodFiller(*this,idx,idy,idz);

    newfloodfillers_ += floodfiller;

    resultlock_.writeUnLock();
}


bool MarchingCubes2Implicit::shouldSetResult( int newval, int prevvalue )
{
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


