/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2001
-*/

static const char* rcsID = "$Id: transform.cc,v 1.13 2011/07/24 13:11:35 cvskris Exp $";

#include "transform.h"
#include "arraynd.h"


GenericTransformND::GenericTransformND()
    : forward_ ( true )
    , info_( 0 )
    , curdim_( -1 )
    , parallel_( true )
    , nr_( 1 )
    , sampling_( 1 )
    , batchsampling_( 1 )
    , batchstarts_( 0 )
    , cinput_( 0 )
    , rinput_( 0 )
    , coutput_( 0 )
    , routput_( 0 )
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
    curdim_ = -1;
    return true;
}


const ArrayNDInfo& GenericTransformND::getInputInfo() const
{ return *info_; }


bool GenericTransformND::setDir( bool forward )
{
    forward_ = forward;
    curdim_ = -1;
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
    if ( !id ) return;
    
    cinput_ = id;
    rinput_ = 0;

    mSetInputData( id );
}


void GenericTransformND::setInput( const float* id )
{
    if ( !id ) return;
    
    rinput_ = id;
    cinput_ = 0;

    mSetInputData( id );
}



void GenericTransformND::setOutput( float_complex* od )
{
    if ( !od ) return;
    
    coutput_ = od;
    routput_ = 0;

    mSetOutputData( od );
}


void GenericTransformND::setOutput( float* od )
{
    if ( !od ) return;
    
    routput_ = od;
    coutput_ = 0;

    mSetOutputData( od );
}



void GenericTransformND::setSampling( int sampling )
{ sampling_ = sampling; }


void GenericTransformND::setScope( int nr, int batchsampling )
{
    curdim_ = -1;
    nr_ = nr;
    batchsampling_ = batchsampling;
    batchstarts_ = 0;
}


void GenericTransformND::setScope( int nr, const int* batchstarts )
{
    curdim_ = -1;
    nr_ = nr;
    batchstarts_ = batchstarts;
}


bool GenericTransformND::run( bool parallel )
{
    parallel_ = parallel;
    return execute();
}


int GenericTransformND::nextStep()
{
    if ( curdim_<0 )
    {
	if ( !setup() )
	return ErrorOccurred();
    }

    if ( !transforms_[curdim_]->run( parallel_ ) )
	return ErrorOccurred();

    curdim_++;
    if ( curdim_>=info_->getNDim() )
    {
	curdim_ = 0;
	return Finished();
    }

    return MoreToDo();
}


bool GenericTransformND::setup()
{
    if ( !info_ ) return false;
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
	memcpy( offsets, starts.arr(), sizeof(int)*starts.size() );

	transforms1dstarts_ += offsets;
	nr1dtransforms_ += nr_;
    }
    else
    {
	ArrayNDInfoImpl curarrsz( ndim-1 );
	mAllocVarLenArr(int,globalarrpos,ndim);
	memset( globalarrpos, 0, ndim*sizeof(int) );
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

		const od_int64 offset = info_->getOffset( globalarrpos );
		//Multiply by sampling_ ?
		offsets += offset;
	    } while ( iter.next() );


	    int* offsetptr = new int[offsets.size()*nr_];
	    int* ptr = offsetptr;

	    for ( int idx=0; idx<nr_; idx++ )
	    {
		for ( int idy=0; idy<offsets.size(); idy++, ptr++ )
		    *ptr = offsets[idy]+starts[idx];
	    }

	    mAllocVarLenArr(int,nextarrpos,ndim);
	    memcpy( nextarrpos, globalarrpos, ndim*sizeof(int) );
	    nextarrpos[dim] = 1;

	    //Compute sampling for 1D transform
	    const od_int64 offset = info_->getOffset( globalarrpos );
	    const od_int64 nextoffset = info_->getOffset( nextarrpos );
	    const int sampling = sampling_*(nextoffset-offset);

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

    curdim_ = 0;

    return true;
}


GenericTransformND::Transform1D::Transform1D()
    : sz_( -1 )
    , sampling_( 1 )
    , batchsampling_( 1 )
    , batchstarts_( 0 )
    , cinput_( 0 )
    , rinput_( 0 )
    , coutput_( 0 )
    , routput_( 0 )
    , nr_( 1 )
    , forward_( true )
{}


void GenericTransformND::Transform1D::setInputData(const float_complex* id )
{
    if ( !id ) return;
    
    cinput_ = id;
    rinput_ = 0;
}


void GenericTransformND::Transform1D::setInputData( const float* id )
{
    if ( !id ) return;
    
    rinput_ = id;
    cinput_ = 0;
}


void GenericTransformND::Transform1D::setOutputData( float_complex* od )
{
    if ( !od ) return;
    
    coutput_ = od;
    routput_ = 0;
}


void GenericTransformND::Transform1D::setOutputData( float* od )
{
    if ( !od ) return;
    
    routput_ = od;
    coutput_ = 0;
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
    batchstarts_ = 0;
}


void GenericTransformND::Transform1D::setScope( int nr, const int* batchstarts )
{
    nr_ = nr;
    batchstarts_ = batchstarts;
}


