/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "arrayndimpl.h"
#include "typeset.h"


// ArrayNDInfo
bool ArrayNDInfo::setSize( int /*dim*/, int /*sz*/ )
{ return false; }


bool ArrayNDInfo::isOK() const
{
    for ( int idx=getNDim()-1; idx>=0; idx-- )
    {
	if ( getSize(idx)<0 )
	    return false;
    }

    return true;
}


od_uint64 ArrayNDInfo::getTotalSz() const
{
    return calcTotalSz();
}


od_uint64 ArrayNDInfo::getOffset( const int* pos ) const
{
    const int ndim = getNDim();
    od_uint64 unitsize = 1;
    od_uint64 res = 0;

    for ( int idx=ndim-1; idx>=0; idx-- )
    {
	res += unitsize*pos[idx];
	unitsize *= getSize(idx);
    }

    return res;
}


bool ArrayNDInfo::validPos( const int* pos ) const
{
    const int ndim = getNDim();
    for ( int idx=ndim-1; idx>=0; idx-- )
    {
	if ( !validDimPos(idx,pos[idx]) )
	    return false;
    }

    return true;
}


bool ArrayNDInfo::validDimPos( int dim, int pos ) const
{
    return pos>=0 && pos<getSize(dim);
}


bool ArrayNDInfo::getArrayPos( od_uint64 mempos, int* pos ) const
{
    const int ndim = getNDim();
    TypeSet<int> dimdevisor( ndim, 1 );

    int product = 1;

    for ( int idx=ndim-1; idx>=0; idx-- )
    {
	const int size = getSize( idx );
	dimdevisor[idx] = product;
	product *= size;
    }

    pos[0] = int(mempos)/dimdevisor[0];
    if ( pos[0]>=getSize(0) )
	return false;

    mempos = mempos%dimdevisor[0];
    for ( int idx=1; idx<ndim; idx++ )
    {
	pos[idx] = int(mempos)/dimdevisor[idx];
	mempos = mempos%dimdevisor[idx];
    }

    return true;
}


od_uint64 ArrayNDInfo::calcTotalSz() const
{
    const int ndim = getNDim();
    od_uint64 res = 1;

    for ( int idx=0; idx<ndim; idx++ )
	res *= getSize( idx );

    return res;
}


// Array2DInfo
od_uint64 Array2DInfo::getOffset( int p0, int p1 ) const
{
    const int pos[2] = { p0, p1 };
    return ArrayNDInfo::getOffset( pos );
}


bool Array2DInfo::validPos( int p0, int p1 ) const
{
    const int pos[2] = { p0, p1 };
    return ArrayNDInfo::validPos( pos );
}


// Array3DInfo
od_uint64 Array3DInfo::getOffset( int p0, int p1, int p2 ) const
{
    const int pos[3] = { p0, p1, p2 };
    return ArrayNDInfo::getOffset( pos );
}


bool Array3DInfo::validPos( int p0, int p1, int p2 ) const
{
    const int pos[3] = { p0, p1, p2 };
    return ArrayNDInfo::validPos( pos );
}


// Array4DInfo
od_uint64 Array4DInfo::getOffset( int p0, int p1, int p2, int p3 ) const
{
    const int pos[4] = { p0, p1, p2, p3 };
    return ArrayNDInfo::getOffset( const_cast<NDPos>(pos) );
}


bool Array4DInfo::validPos( int p0, int p1, int p2, int p3 ) const
{
    const int pos[4] = { p0, p1, p2, p3 };
    return ArrayNDInfo::validPos( const_cast<NDPos>(pos) );
}


// Array1DInfoImpl
Array1DInfoImpl::Array1DInfoImpl( int nsz )
    : dimsz_(nsz)
{
}


Array1DInfoImpl::Array1DInfoImpl( const Array1DInfo& nsz)
    : dimsz_( nsz.getSize(0) )
{}


bool Array1DInfoImpl::setSize( int dim, int nsz )
{
    if( dim != 0 )
	return false;

    dimsz_ = nsz;
    return true;
}


Array2DInfoImpl::Array2DInfoImpl( int sz0, int sz1 )
{
    dimsz_[0] = sz0; dimsz_[1] = sz1;
    cachedtotalsz_ = calcTotalSz();
}


Array2DInfoImpl::Array2DInfoImpl( const Array2DInfo& nsz )
{
    dimsz_[0] = nsz.getSize(0);
    dimsz_[1] = nsz.getSize(1);
    cachedtotalsz_ = calcTotalSz();
}


bool Array2DInfoImpl::setSize( int dim, int nsz )
{
    if( dim > 1 || dim < 0 )
	return false;

    dimsz_[dim] = nsz;
    cachedtotalsz_ = calcTotalSz();
    return true;
}


// Array3DInfoImpl
Array3DInfoImpl::Array3DInfoImpl( int sz0, int sz1, int sz2 )
{
    dimsz_[0] = sz0; dimsz_[1] = sz1; dimsz_[2] = sz2;
    cachedtotalsz_ = calcTotalSz();
}


Array3DInfoImpl::Array3DInfoImpl( const Array3DInfo& nsz)
{
    dimsz_[0] = nsz.getSize(0);
    dimsz_[1] = nsz.getSize(1);
    dimsz_[2] = nsz.getSize(2);
    cachedtotalsz_ = calcTotalSz();
}


bool Array3DInfoImpl::setSize(int dim, int nsz)
{
    if( dim > 2 || dim < 0 )
	return false;

    dimsz_[dim] = nsz;
    cachedtotalsz_ = calcTotalSz();
    return true;
}


// Array4DInfoImpl
Array4DInfoImpl::Array4DInfoImpl( int sz0, int sz1,
				  int sz2, int sz3 )
{
    dimsz_[0] = sz0; dimsz_[1] = sz1; dimsz_[2] = sz2; dimsz_[3] = sz3;
    cachedtotalsz_ = calcTotalSz();
}


Array4DInfoImpl::Array4DInfoImpl( const Array4DInfo& oth )
{
    dimsz_[0] = oth.getSize(0);
    dimsz_[1] = oth.getSize(1);
    dimsz_[2] = oth.getSize(2);
    dimsz_[3] = oth.getSize(3);
    cachedtotalsz_ = calcTotalSz();
}


bool Array4DInfoImpl::setSize( int dim, int nsz )
{
    if( dim > 3 || dim < 0 )
	return false;
    dimsz_[dim] = nsz;
    cachedtotalsz_ = calcTotalSz();
    return true;
}


// ArrayNDInfoImpl
ArrayNDInfo* ArrayNDInfoImpl::clone() const
{
    if ( ndim_==1 ) return new Array1DInfoImpl(dimsz_[0]);
    if ( ndim_==2 ) return new Array2DInfoImpl(dimsz_[0], dimsz_[1]);
    if ( ndim_==3 ) return new Array3DInfoImpl(dimsz_[0], dimsz_[1], dimsz_[2]);
    if ( ndim_==4 )
	return new Array4DInfoImpl( dimsz_[0], dimsz_[1], dimsz_[2], dimsz_[3]);

    return new ArrayNDInfoImpl(*this);
}


ArrayNDInfo* ArrayNDInfoImpl::create( int ndim )
{
    if ( ndim==1 ) return new Array1DInfoImpl;
    if ( ndim==2 ) return new Array2DInfoImpl;
    if ( ndim==3 ) return new Array3DInfoImpl;
    if ( ndim==4 ) return new Array4DInfoImpl;

    return new ArrayNDInfoImpl(ndim);
}


ArrayNDInfoImpl::ArrayNDInfoImpl( int ndim )
    : ndim_( ndim )
    , dimsz_( ndim<1 ? nullptr : new int[ndim_] )
{
    cachedtotalsz_ = 0;
    for ( int idx = 0; idx<ndim_; idx++ )
	dimsz_[idx] = 0;
}


ArrayNDInfoImpl::ArrayNDInfoImpl( const ArrayNDInfo& nsz )
    : ArrayNDInfo( nsz )
    , ndim_(nsz.getNDim())
    , dimsz_( ndim_<1 ? nullptr : new int[ndim_] )
{
    cachedtotalsz_ = 0;
    for ( int idx=0; idx<ndim_; idx++ )
	setSize( idx, nsz.getSize(idx) );
}


// To avoid using default copy constructor created by compiler.
ArrayNDInfoImpl::ArrayNDInfoImpl( const ArrayNDInfoImpl& nsz )
    : ArrayNDInfo(nsz)
    , ndim_(nsz.getNDim())
    , dimsz_(ndim_<1 ? nullptr : new int[ndim_])
{
    cachedtotalsz_ = 0;
    for ( int idx=0; idx<ndim_; idx++ )
	setSize( idx, nsz.getSize(idx) );
}


ArrayNDInfoImpl::~ArrayNDInfoImpl()
{
    delete [] dimsz_;
}


int ArrayNDInfoImpl::getNDim() const
{
    return ndim_;
}


bool ArrayNDInfoImpl::setSize( int dim, int newSz )
{
    if ( dim >= ndim_ || dim < 0 )
	return false;

    dimsz_[dim] = newSz;
    cachedtotalsz_ = calcTotalSz();
    return true;
}


int ArrayNDInfoImpl::getSize( int dim ) const
{
    return dim >= ndim_ || dim < 0 ? 0 : dimsz_[dim];
}


// ArrayNDIter
ArrayNDIter::ArrayNDIter( const ArrayNDInfo& sz )
    : position_(new int[sz.getNDim()])
    , sz_(sz)
{
    if ( !sz.getTotalSz() )
    {
	pErrMsg( "Not a valid array for iteration" );
    }
    reset();
}


ArrayNDIter::~ArrayNDIter()
{
    delete [] position_;
}


bool ArrayNDIter::next()
{
    return inc( sz_.getNDim() - 1 );
}


void ArrayNDIter::reset()
{
    const int ndim = sz_.getNDim();
    for ( int idx=0; idx<ndim; idx++ )
	position_[idx] = 0;
}


int ArrayNDIter::operator[](int dim) const
{
    return position_[dim];
}


bool ArrayNDIter::inc( int dim )
{
    position_[dim]++;

    if ( position_[dim] >= sz_.getSize(dim))
    {
	if ( dim )
	{
	    position_[dim] = 0;
	    return inc( dim-1 );
	}
	else
	    return false;
    }

    return true;
}


void* getArrayND( const ArrayNDInfo& info, const OD::DataRepType typ )
{
    if ( typ == OD::F32 || typ == OD::AutoDataRep )
	return ArrayNDImpl<float>::create( info );
    else if ( typ == OD::F64 )
	return ArrayNDImpl<double>::create( info );
    else if ( typ == OD::SI32 )
	return ArrayNDImpl<od_int32>::create( info );
    else if ( typ == OD::UI32 )
	return ArrayNDImpl<od_uint32>::create( info );
    else if ( typ == OD::SI16 )
	return ArrayNDImpl<od_int16>::create( info );
    else if ( typ == OD::UI16 )
	return ArrayNDImpl<od_uint16>::create( info );
    else if ( typ == OD::SI8 )
	return ArrayNDImpl<signed char>::create( info );
    else if ( typ == OD::UI8 )
	return ArrayNDImpl<unsigned char>::create( info );
    else if ( typ == OD::SI64 )
	return ArrayNDImpl<od_int64>::create( info );

    return nullptr;
}
