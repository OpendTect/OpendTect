/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2007
________________________________________________________________________

-*/


#include "arrayndslice.h"
#include "varlenarray.h"


ArrayNDSliceBase::ArrayNDSliceBase( ArrayNDInfo* localinfo,
				    const ArrayNDInfo& sourceinfo )
    : info_( *localinfo )
    , sourceinfo_( sourceinfo )
    , position_( sourceinfo.nrDims(), -1 )
    , vardim_( localinfo->nrDims(), -1 )
    , offset_( -1 )
    , isinited_( false )
{
}


ArrayNDSliceBase::~ArrayNDSliceBase()
{
    delete &info_;
}


bool ArrayNDSliceBase::setPos( dim_idx_type dim, idx_type pos )
{
    const nr_dims_type ndim = (nr_dims_type)position_.size();
    if ( dim<0 || dim>=ndim || pos<0 || pos>=getDimSize(dim) )
	return false;

    position_[dim] = pos;
    isinited_ = false;

    return true;
}


ArrayNDInfo::idx_type ArrayNDSliceBase::getPos( dim_idx_type dim ) const
{
    return position_[dim];
}


ArrayNDSliceBase::size_type ArrayNDSliceBase::getDimSize(
					dim_idx_type dim ) const
{
    return sourceinfo_.getSize( dim );
}


void ArrayNDSliceBase::setDimMap( dim_idx_type localdim, dim_idx_type remotedim)
{
    vardim_[localdim] = remotedim;
    position_[remotedim] = -1;
    isinited_ = false;
}


bool ArrayNDSliceBase::init()
{
    const nr_dims_type nrowndims = (nr_dims_type)vardim_.size();
    const nr_dims_type ndim = (nr_dims_type)position_.size();

    TypeSet<nr_dims_type> unknowndims;
    for ( dim_idx_type idx=0; idx<ndim; idx++ )
    {
	if ( position_[idx] == -1 )
	    unknowndims += idx;
    }

    if ( unknowndims.size() != nrowndims )
	return false;

    for ( dim_idx_type idx=0; idx<nrowndims; idx++ )
    {
	if ( vardim_[idx]==-1 )
	{
	    if ( unknowndims.size() )
	    {
		vardim_[idx] = unknowndims[0];
		unknowndims.removeSingle( 0 );
	    }
	    else
		return false;
	}
	else
	{
	    if ( !unknowndims.isPresent(vardim_[idx]) )
		return false;
	    unknowndims -= vardim_[idx];
	}
    }

    if ( !unknowndims.isEmpty() )
	return false;

    bool ismemorder = true;
    for ( dim_idx_type idx=0; idx<nrowndims; idx++ )
    {
	if ( vardim_[idx]+nrowndims-idx != ndim )
	    { ismemorder = false; break; }
    }

    if ( !ismemorder )
    {
	bool hasonlyoneslice = true;
	for ( dim_idx_type dimidx=0; dimidx<ndim; dimidx++ )
	    if ( position_[dimidx]!=-1 && getDimSize(dimidx)!=1 )
		{ hasonlyoneslice = false; break; }

	if ( hasonlyoneslice )
	    ismemorder = true;
    }

    if ( !ismemorder )
	offset_ = -1;
    else
    {
	mAllocVarLenArr( idx_type, localpos, nrowndims );
	OD::memZero( localpos, nrowndims * sizeof(idx_type) );

	mAllocVarLenArr( idx_type, tpos, ndim );
	getSourcePos( localpos, tpos );
	offset_ = sourceinfo_.getOffset(tpos);
    }

    for ( dim_idx_type idx=0; idx<nrowndims; idx++ )
	info_.setSize( idx, sourceinfo_.getSize((dim_idx_type)vardim_[idx]) );

    isinited_ = true;
    return true;
}


void ArrayNDSliceBase::getSourcePos( NDPos localpos,
				     idx_type* arraypos ) const
{
    const nr_dims_type ndim = (nr_dims_type)position_.size();
    const nr_dims_type nrowndims = (nr_dims_type)vardim_.size();

    OD::memCopy( arraypos, position_.arr(), ndim*sizeof(idx_type) );

    for ( dim_idx_type idx=0; idx<nrowndims; idx++ )
	arraypos[vardim_[idx]] = localpos[idx];
}
