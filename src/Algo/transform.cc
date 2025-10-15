/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "transform.h"

#include "arraynd.h"


// GenericTransformND

GenericTransformND::GenericTransformND()
{}


GenericTransformND::~GenericTransformND()
{
    delete info_;
    deepErase( transforms_ );
    deepEraseArr( transforms1dstarts_ );
}


bool GenericTransformND::setInputInfo( const ArrayNDInfo& ni )
{
    delete info_;
    info_ = ni.clone();
    setNeedSetup();
    return true;
}


const ArrayNDInfo& GenericTransformND::getInputInfo() const
{ return *info_; }


bool GenericTransformND::setDir( bool forward )
{
    forward_ = forward;
    setNeedSetup();
    return true;
}


#define mSetInputData( inputdata ) \
    if ( transforms_.size() ) \
	transforms_[0]->setInputData( inputdata )


#define mSetOutputData( outputdata ) \
for ( int idx=0; idx<transforms_.size(); idx++ ) \
{ \
    if ( idx ) transforms_[idx]->setInputData( outputdata ); \
    transforms_[idx]->setOutputData( outputdata ); \
}


void GenericTransformND::setInput( const float_complex* id )
{
    if ( !id )
	return;

    cinput_ = id;
    rinput_ = nullptr;

    mSetInputData( id );
}


void GenericTransformND::setInput( const float* id )
{
    if ( !id )
	return;

    rinput_ = id;
    cinput_ = nullptr;

    mSetInputData( id );
}



void GenericTransformND::setOutput( float_complex* od )
{
    if ( !od )
	return;

    coutput_ = od;
    routput_ = nullptr;

    mSetOutputData( od );
}


void GenericTransformND::setOutput( float* od )
{
    if ( !od )
	return;

    routput_ = od;
    coutput_ = nullptr;

    mSetOutputData( od );
}



void GenericTransformND::setSampling( int sampling )
{ sampling_ = sampling; }


void GenericTransformND::setScope( int nr, int batchsampling )
{
    nr_ = nr;
    batchsampling_ = batchsampling;
    batchstarts_ = nullptr;
    setNeedSetup();
}


void GenericTransformND::setScope( int nr, const int* batchstarts )
{
    nr_ = nr;
    batchstarts_ = batchstarts;
    setNeedSetup();
}


void GenericTransformND::setNeedSetup( bool yn )
{
    needsetup_ = true;
}


od_int64 GenericTransformND::nrDone() const
{
    return curdim_;
}


od_int64 GenericTransformND::totalNr() const
{
    return info_ ? info_->getNDim() : SequentialTask::totalNr();
}


bool GenericTransformND::run( bool parallel )
{
    parallel_ = parallel;
    return execute();
}


bool GenericTransformND::doPrepare( od_ostream* strm )
{
    if ( needsetup_ && !setup() )
	return false;

    curdim_ = 0;
    return SequentialTask::doPrepare( strm );
}


int GenericTransformND::nextStep()
{
    if ( !transforms_[curdim_]->run(parallel_) )
	return ErrorOccurred();

    curdim_++;
    return curdim_>=info_->getNDim() ? Finished() : MoreToDo();
}


bool GenericTransformND::doFinish( bool success, od_ostream* strm )
{
    return SequentialTask::doFinish( success, strm );
}


bool GenericTransformND::setup()
{
    if ( !info_ )
	return false;

    deepEraseArr( transforms1dstarts_ );
    deepErase( transforms_ );
    nr1dtransforms_.erase();

    TypeSet<int> starts;
    for ( int idx=0; idx<nr_; idx++ )
	starts += batchstarts_ ? batchstarts_[idx] : batchsampling_ * idx;

    const int ndim = info_->getNDim();
    if ( ndim==1 )
    {
	Transform1D* trans = createTransform();
	trans->setSize( info_->getSize(0) );
	trans->setSampling( sampling_ );
	trans->setDir( forward_ );
	transforms_ += trans;

	int* offsets = new int[nr_];
	OD::memCopy( offsets, starts.arr(), sizeof(int)*starts.size() );

	transforms1dstarts_ += offsets;
	nr1dtransforms_ += nr_;
    }
    else
    {
	ArrayNDInfoImpl curarrsz( ndim-1 );
	mAllocVarLenArr(int,globalarrpos,ndim);
	OD::memZero( mVarLenArr(globalarrpos), ndim*sizeof(int) );
	for ( int dim=0; dim<ndim; dim++ )
	{
	    globalarrpos[dim] = 0;
	    for ( int idy=0; idy<ndim; idy++ )
	    {
		if ( idy==dim )
		    continue;

		const int curdim = idy>dim ? idy-1 : idy;
		curarrsz.setSize( curdim, info_->getSize(idy) );
	    }

	    ArrayNDIter iter( curarrsz );
	    TypeSet<int> offsets;

	    do
	    {
		for ( int idy=0; idy<ndim; idy++ )
		{
		    if ( idy==dim )
			continue;

		    const int curdim = idy>dim ? idy-1 : idy;
		    globalarrpos[idy] = iter.getPos()[curdim];
		}

		const od_int64 offset =
			info_->getOffset( mVarLenArr(globalarrpos) );
		//Multiply by sampling_ ?
		offsets += mCast( int, offset );
	    } while ( iter.next() );


	    int* offsetptr = new int[offsets.size()*nr_];
	    int* ptr = offsetptr;

	    for ( int idx=0; idx<nr_; idx++ )
	    {
		for ( int idy=0; idy<offsets.size(); idy++, ptr++ )
		    *ptr = offsets[idy]+starts[idx];
	    }

	    mAllocVarLenArr(int,nextarrpos,ndim);
	    OD::memCopy( mVarLenArr(nextarrpos), mVarLenArr(globalarrpos),
			 ndim*sizeof(int) );
	    nextarrpos[dim] = 1;

	    //Compute sampling for 1D transform
	    const od_int64 offset =
			info_->getOffset( mVarLenArr(globalarrpos) );
	    const od_int64 nextoffset =
			info_->getOffset( mVarLenArr(nextarrpos) );
	    const int sampling = mCast( int, sampling_*(nextoffset-offset) );

	    Transform1D* trans = createTransform();
	    trans->setSize( info_->getSize(dim) );
	    trans->setSampling( sampling );
	    trans->setDir(forward_);
	    trans->setScope( offsets.size()*nr_, offsetptr );

	    if ( !trans->init() )
	    {
		delete [] offsetptr;
		delete trans;
		return false;
	    }

	    transforms1dstarts_ += offsetptr;
	    nr1dtransforms_ += offsets.size()*nr_;
	    transforms_ += trans;
	}
    }

    mSetInputData( rinput_ );
    mSetInputData( cinput_ );

    mSetOutputData( routput_ );
    mSetOutputData( coutput_ );

    needsetup_ = false;

    return true;
}


// GenericTransformND::Transform1D

GenericTransformND::Transform1D::Transform1D()
{}


void GenericTransformND::Transform1D::setInputData(const float_complex* id )
{
    if ( !id )
	return;

    cinput_ = id;
    rinput_ = nullptr;
}


void GenericTransformND::Transform1D::setInputData( const float* id )
{
    if ( !id )
	return;

    rinput_ = id;
    cinput_ = nullptr;
}


void GenericTransformND::Transform1D::setOutputData( float_complex* od )
{
    if ( !od )
	return;

    coutput_ = od;
    routput_ = nullptr;
}


void GenericTransformND::Transform1D::setOutputData( float* od )
{
    if ( !od )
	return;

    routput_ = od;
    coutput_ = nullptr;
}


void GenericTransformND::Transform1D::setSize( int sz )
{ sz_ = sz; }


void GenericTransformND::Transform1D::setDir( bool forward )
{ forward_ = forward; }


void GenericTransformND::Transform1D::setSampling( int sampling )
{ sampling_ = sampling; }


void GenericTransformND::Transform1D::setScope( int nr, int batchsampling )
{
    nr_ = nr;
    batchsampling_ = batchsampling;
    batchstarts_ = nullptr;
}


void GenericTransformND::Transform1D::setScope( int nr, const int* batchstarts )
{
    nr_ = nr;
    batchstarts_ = batchstarts;
}
