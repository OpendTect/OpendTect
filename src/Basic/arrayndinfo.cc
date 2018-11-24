/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : 9-3-1999
-*/


#include "arraynd.h"
#include "typeset.h"


bool ArrayNDInfo::isOK() const
{
    for ( dim_idx_type idx=nrDims()-1; idx>=0; idx-- )
    {
	if ( getSize(idx)<0 )
	    return false;
    }

    return true;
}


ArrayNDInfo::total_size_type ArrayNDInfo::totalSize() const
{
    return calcTotalSz();
}


ArrayNDInfo::offset_type ArrayNDInfo::getOffset( NDPos pos ) const
{
    total_size_type unitsize = 1;
    total_size_type res = 0;

    for ( dim_idx_type idx=nrDims()-1; idx>=0; idx-- )
    {
	res += unitsize*pos[idx];
	unitsize *= getSize(idx);
    }

    return res;
}


bool ArrayNDInfo::validPos( NDPos pos ) const
{
    for ( dim_idx_type idx=nrDims()-1; idx>=0; idx-- )
    {
	if ( !validDimPos(idx,pos[idx]) )
	    return false;
    }

    return true;
}


bool ArrayNDInfo::validDimPos( dim_idx_type dim, idx_type pos ) const
{
    return pos>=0 && pos<getSize(dim);
}


bool ArrayNDInfo::getArrayPos( offset_type mempos, idx_type* pos ) const
{
    const nr_dims_type ndim = nrDims();
    TypeSet<total_size_type> dimdevisor( ndim, 1 );

    total_size_type product = 1;
    for ( dim_idx_type idx=ndim-1; idx>=0; idx-- )
    {
	const idx_type size = getSize( idx );
	dimdevisor[idx] = product;
	product *= size;
    }

    pos[0] = (idx_type)(mempos / dimdevisor[0]);
    if ( pos[0] >= getSize(0) )
	return false;

    mempos = mempos % dimdevisor[0];
    for ( dim_idx_type idx=1; idx<ndim; idx++ )
    {
	pos[idx] = (idx_type)( mempos / dimdevisor[idx] );
	mempos = mempos % dimdevisor[idx];
    }

    return true;
}


ArrayNDInfo::total_size_type ArrayNDInfo::calcTotalSz() const
{
    const nr_dims_type ndim = nrDims();
    total_size_type res = 1;

    for ( dim_idx_type idx=0; idx<ndim; idx++ )
	res *= getSize( idx );

    return res;
}


ArrayNDInfo::offset_type Array2DInfo::getOffset( idx_type p0,
						 idx_type p1 ) const
{
    const idx_type pos[2] = { p0, p1 };
    return ArrayNDInfo::getOffset( const_cast<NDPos>(pos) );
}


bool Array2DInfo::validPos( idx_type p0, idx_type p1 ) const
{
    const idx_type pos[2] = { p0, p1 };
    return ArrayNDInfo::validPos( const_cast<NDPos>(pos) );
}


ArrayNDInfo::offset_type Array3DInfo::getOffset( idx_type p0, idx_type p1,
						idx_type p2 ) const
{
    const idx_type pos[3] = { p0, p1, p2 };
    return ArrayNDInfo::getOffset( const_cast<NDPos>(pos) );
}


bool Array3DInfo::validPos( idx_type p0, idx_type p1, idx_type p2 ) const
{
    const idx_type pos[3] = { p0, p1, p2 };
    return ArrayNDInfo::validPos( const_cast<NDPos>(pos) );
}


ArrayNDInfo::offset_type Array4DInfo::getOffset( idx_type p0, idx_type p1,
						idx_type p2, idx_type p3 ) const
{
    const idx_type pos[4] = { p0, p1, p2, p3 };
    return ArrayNDInfo::getOffset( const_cast<NDPos>(pos) );
}


bool Array4DInfo::validPos( idx_type p0, idx_type p1,
			    idx_type p2, idx_type p3 ) const
{
    const idx_type pos[4] = { p0, p1, p2, p3 };
    return ArrayNDInfo::validPos( const_cast<NDPos>(pos) );
}



Array1DInfoImpl::Array1DInfoImpl( size_type nsz )
    : dimsz_(nsz)
{
}


Array1DInfoImpl::Array1DInfoImpl( const Array1DInfo& oth )
    : dimsz_( oth.getSize(0) )
{
}


bool Array1DInfoImpl::setSize( dim_idx_type dim, size_type nsz )
{
    if( dim != 0 )
	return false;
    dimsz_ = nsz;
    return true;
}


Array2DInfoImpl::Array2DInfoImpl( size_type sz0, size_type sz1 )
{
    dimsz_[0] = sz0; dimsz_[1] = sz1;
    cachedtotalsz_ = calcTotalSz();
}


Array2DInfoImpl::Array2DInfoImpl( const Array2DInfo& oth )
{
    dimsz_[0] = oth.getSize(0);
    dimsz_[1] = oth.getSize(1);

    cachedtotalsz_ = calcTotalSz();
}


bool Array2DInfoImpl::setSize( dim_idx_type dim, size_type nsz )
{
    if( dim > 1 || dim < 0 )
	return false;
    dimsz_[dim] = nsz;
    cachedtotalsz_ = calcTotalSz();
    return true;
}


Array3DInfoImpl::Array3DInfoImpl( size_type sz0, size_type sz1, size_type sz2 )
{
    dimsz_[0] = sz0; dimsz_[1] = sz1; dimsz_[2] = sz2;
    cachedtotalsz_ = calcTotalSz();
}


Array3DInfoImpl::Array3DInfoImpl( const Array3DInfo& oth )
{
    dimsz_[0] = oth.getSize(0);
    dimsz_[1] = oth.getSize(1);
    dimsz_[2] = oth.getSize(2);
    cachedtotalsz_ = calcTotalSz();
}


bool Array3DInfoImpl::setSize( dim_idx_type dim, size_type nsz )
{
    if( dim > 2 || dim < 0 )
	return false;
    dimsz_[dim] = nsz;
    cachedtotalsz_ = calcTotalSz();
    return true;
}


Array4DInfoImpl::Array4DInfoImpl( size_type sz0, size_type sz1,
				  size_type sz2, size_type sz3 )
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


bool Array4DInfoImpl::setSize( dim_idx_type dim, size_type nsz )
{
    if( dim > 3 || dim < 0 )
	return false;
    dimsz_[dim] = nsz;
    cachedtotalsz_ = calcTotalSz();
    return true;
}


ArrayNDInfo* ArrayNDInfoImpl::clone() const
{
    if ( ndim_==1 )
	return new Array1DInfoImpl( dimsz_[0]);
    if ( ndim_==2 )
	return new Array2DInfoImpl( dimsz_[0], dimsz_[1] );
    if ( ndim_==3 )
	return new Array3DInfoImpl( dimsz_[0], dimsz_[1], dimsz_[2] );
    if ( ndim_==4 )
	return new Array4DInfoImpl( dimsz_[0], dimsz_[1], dimsz_[2], dimsz_[3]);

    return new ArrayNDInfoImpl( *this );
}


ArrayNDInfo* ArrayNDInfoImpl::create( nr_dims_type ndim )
{
    if ( ndim==1 )
	return new Array1DInfoImpl;
    if ( ndim==2 )
	return new Array2DInfoImpl;
    if ( ndim==3 )
	return new Array3DInfoImpl;
    if ( ndim==4 )
	return new Array4DInfoImpl;

    return new ArrayNDInfoImpl(ndim);
}


ArrayNDInfoImpl::ArrayNDInfoImpl( nr_dims_type ndim )
	: ndim_( ndim )
	, dimsz_( ndim<1 ? 0 : new size_type[ndim_] )
{
    cachedtotalsz_ = 0;
    for ( dim_idx_type idx=0; idx<ndim_; idx++ )
	dimsz_[idx] = 0;
}


ArrayNDInfoImpl::ArrayNDInfoImpl( const ArrayNDInfo& nsz )
	: ArrayNDInfo( nsz )
	, ndim_(nsz.nrDims())
	, dimsz_( ndim_<1 ? 0 : new size_type[ndim_] )
{
    for ( dim_idx_type idx=0; idx<ndim_; idx++ )
	setSize( idx, nsz.getSize(idx) );
}


// To avoid using default copy constructor created by compiler.
ArrayNDInfoImpl::ArrayNDInfoImpl( const ArrayNDInfoImpl& nsz )
	: ArrayNDInfo(nsz)
	, ndim_(nsz.nrDims())
	, dimsz_(ndim_<1 ? 0 : new size_type[ndim_])
{
    for ( dim_idx_type idx=0; idx<ndim_; idx++ )
	setSize( idx, nsz.getSize(idx) );
}


ArrayNDInfoImpl::~ArrayNDInfoImpl()
{
    delete [] dimsz_;
}


ArrayNDInfo::nr_dims_type ArrayNDInfoImpl::nrDims() const
{
    return ndim_;
}


bool ArrayNDInfoImpl::setSize( dim_idx_type dim, size_type newsz )
{
    if ( dim >= ndim_ || dim < 0 )
	return false;

    dimsz_[dim] = newsz;
    cachedtotalsz_ = calcTotalSz();
    return true;
}


ArrayNDInfo::size_type ArrayNDInfoImpl::getSize( dim_idx_type dim ) const
{
    return dim >= ndim_ || dim < 0 ? 0 : dimsz_[dim];
}


ArrayNDIter::ArrayNDIter( const ArrayNDInfo& sz )
    : sz_ ( sz )
    , position_( new size_type[sz.nrDims()] )
{
    if ( sz.totalSize() < 1 )
	{ pErrMsg( "Not a valid array for iteration" ); }
    reset();
}


ArrayNDIter::~ArrayNDIter()
{
    delete [] position_;
}


bool ArrayNDIter::next()
{
    return inc( sz_.nrDims() - 1 );
}


void ArrayNDIter::reset()
{
    const nr_dims_type ndim = sz_.nrDims();

    for ( dim_idx_type idx=0; idx<ndim; idx++ )
	position_[idx] = 0;
}


ArrayNDIter::idx_type ArrayNDIter::operator[]( dim_idx_type dim ) const
{
    return position_[dim];
}


bool ArrayNDIter::inc( dim_idx_type dim )
{
    position_[dim]++;

    if ( position_[dim] >= sz_.getSize(dim) )
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
