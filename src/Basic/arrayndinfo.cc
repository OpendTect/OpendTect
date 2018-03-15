/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : 9-3-1999
-*/


#include "arraynd.h"
#include "typeset.h"


bool ArrayNDInfo::isOK() const
{
    for ( DimIdxType idx=nrDims()-1; idx>=0; idx-- )
    {
	if ( getSize(idx)<0 )
	    return false;
    }

    return true;
}


ArrayNDInfo::TotalSzType ArrayNDInfo::totalSize() const
{
    return calcTotalSz();
}


ArrayNDInfo::OffsetType ArrayNDInfo::getOffset( NDPos pos ) const
{
    TotalSzType unitsize = 1;
    TotalSzType res = 0;

    for ( DimSzType idx=nrDims()-1; idx>=0; idx-- )
    {
	res += unitsize*pos[idx];
	unitsize *= getSize(idx);
    }

    return res;
}


bool ArrayNDInfo::validPos( NDPos pos ) const
{
    for ( DimIdxType idx=nrDims()-1; idx>=0; idx-- )
    {
	if ( !validDimPos(idx,pos[idx]) )
	    return false;
    }

    return true;
}


bool ArrayNDInfo::validDimPos( DimIdxType dim, IdxType pos ) const
{
    return pos>=0 && pos<getSize(dim);
}


bool ArrayNDInfo::getArrayPos( OffsetType mempos, IdxType* pos ) const
{
    const DimSzType ndim = nrDims();
    TypeSet<TotalSzType> dimdevisor( ndim, 1 );

    TotalSzType product = 1;
    for ( DimIdxType idx=ndim-1; idx>=0; idx-- )
    {
	const IdxType size = getSize( idx );
	dimdevisor[idx] = product;
	product *= size;
    }

    pos[0] = (IdxType)(mempos / dimdevisor[0]);
    if ( pos[0] >= getSize(0) )
	return false;

    mempos = mempos % dimdevisor[0];
    for ( DimIdxType idx=1; idx<ndim; idx++ )
    {
	pos[idx] = (IdxType)( mempos / dimdevisor[idx] );
	mempos = mempos % dimdevisor[idx];
    }

    return true;
}


ArrayNDInfo::TotalSzType ArrayNDInfo::calcTotalSz() const
{
    const DimSzType ndim = nrDims();
    TotalSzType res = 1;

    for ( DimIdxType idx=0; idx<ndim; idx++ )
	res *= getSize( idx );

    return res;
}


ArrayNDInfo::OffsetType Array2DInfo::getOffset( IdxType p0, IdxType p1 ) const
{
    const IdxType pos[2] = { p0, p1 };
    return ArrayNDInfo::getOffset( const_cast<NDPos>(pos) );
}


bool Array2DInfo::validPos( IdxType p0, IdxType p1 ) const
{
    const IdxType pos[2] = { p0, p1 };
    return ArrayNDInfo::validPos( const_cast<NDPos>(pos) );
}


ArrayNDInfo::OffsetType Array3DInfo::getOffset( IdxType p0, IdxType p1,
						IdxType p2 ) const
{
    const IdxType pos[3] = { p0, p1, p2 };
    return ArrayNDInfo::getOffset( const_cast<NDPos>(pos) );
}


bool Array3DInfo::validPos( IdxType p0, IdxType p1, IdxType p2 ) const
{
    const IdxType pos[3] = { p0, p1, p2 };
    return ArrayNDInfo::validPos( const_cast<NDPos>(pos) );
}


Array1DInfoImpl::Array1DInfoImpl( SzType nsz )
    : dimsz_(nsz)
{
}


Array1DInfoImpl::Array1DInfoImpl( const Array1DInfo& oth )
    : dimsz_( oth.getSize(0) )
{
}


bool Array1DInfoImpl::setSize( DimIdxType dim, SzType nsz )
{
    if( dim != 0 )
	return false;
    dimsz_ = nsz;
    return true;
}


Array2DInfoImpl::Array2DInfoImpl( SzType sz0, SzType sz1 )
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


bool Array2DInfoImpl::setSize( DimIdxType dim, SzType nsz )
{
    if( dim > 1 || dim < 0 )
	return false;
    dimsz_[dim] = nsz;
    cachedtotalsz_ = calcTotalSz();
    return true;
}


Array3DInfoImpl::Array3DInfoImpl( SzType sz0, SzType sz1, SzType sz2 )
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


bool Array3DInfoImpl::setSize( DimIdxType dim, SzType nsz )
{
    if( dim > 2 || dim < 0 )
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

    return new ArrayNDInfoImpl( *this );
}


ArrayNDInfo* ArrayNDInfoImpl::create( DimSzType ndim )
{
    if ( ndim==1 )
	return new Array1DInfoImpl;
    if ( ndim==2 )
	return new Array2DInfoImpl;
    if ( ndim==3 )
	return new Array3DInfoImpl;

    return new ArrayNDInfoImpl(ndim);
}


ArrayNDInfoImpl::ArrayNDInfoImpl( DimSzType ndim )
	: ndim_( ndim )
	, dimsz_( ndim<1 ? 0 : new SzType[ndim_] )
{
    cachedtotalsz_ = 0;
    for ( DimIdxType idx=0; idx<ndim_; idx++ )
	dimsz_[idx] = 0;
}


ArrayNDInfoImpl::ArrayNDInfoImpl( const ArrayNDInfo& nsz )
	: ArrayNDInfo( nsz )
	, ndim_(nsz.nrDims())
	, dimsz_( ndim_<1 ? 0 : new SzType[ndim_] )
{
    for ( DimIdxType idx=0; idx<ndim_; idx++ )
	setSize( idx, nsz.getSize(idx) );
}


// To avoid using default copy constructor created by compiler.
ArrayNDInfoImpl::ArrayNDInfoImpl( const ArrayNDInfoImpl& nsz )
	: ArrayNDInfo(nsz)
	, ndim_(nsz.nrDims())
	, dimsz_(ndim_<1 ? 0 : new SzType[ndim_])
{
    for ( DimIdxType idx=0; idx<ndim_; idx++ )
	setSize( idx, nsz.getSize(idx) );
}


ArrayNDInfoImpl::~ArrayNDInfoImpl()
{
    delete [] dimsz_;
}


ArrayNDInfo::DimSzType ArrayNDInfoImpl::nrDims() const
{
    return ndim_;
}


bool ArrayNDInfoImpl::setSize( DimIdxType dim, SzType newsz )
{
    if ( dim >= ndim_ || dim < 0 )
	return false;

    dimsz_[dim] = newsz;
    cachedtotalsz_ = calcTotalSz();
    return true;
}


ArrayNDInfo::SzType ArrayNDInfoImpl::getSize( DimIdxType dim ) const
{
    return dim >= ndim_ || dim < 0 ? 0 : dimsz_[dim];
}


ArrayNDIter::ArrayNDIter( const ArrayNDInfo& sz )
    : sz_ ( sz )
    , position_( new SzType[sz.nrDims()] )
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
    const DimSzType ndim = sz_.nrDims();

    for ( DimIdxType idx=0; idx<ndim; idx++ )
	position_[idx] = 0;
}


ArrayNDIter::IdxType ArrayNDIter::operator[]( DimIdxType dim ) const
{
    return position_[dim];
}


bool ArrayNDIter::inc( DimIdxType dim )
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
