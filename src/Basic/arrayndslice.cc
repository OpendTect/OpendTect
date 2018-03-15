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


bool ArrayNDSliceBase::setPos( DimIdxType dim, IdxType pos )
{
    const NrDimsType ndim = position_.size();
    if ( dim<0 || dim>=ndim || pos<0 || pos>=getDimSize(dim) )
	return false;

    position_[dim] = pos;
    isinited_ = false;

    return true;
}


ArrayNDInfo::IdxType ArrayNDSliceBase::getPos( DimIdxType dim ) const
{
    return position_[dim];
}


ArrayNDSliceBase::SzType ArrayNDSliceBase::getDimSize( DimIdxType dim ) const
{
    return sourceinfo_.getSize( dim );
}


void ArrayNDSliceBase::setDimMap( DimIdxType localdim, DimIdxType remotedim )
{
    vardim_[localdim] = remotedim;
    position_[remotedim] = -1;
    isinited_ = false;
}


bool ArrayNDSliceBase::init()
{
    const NrDimsType nrowndims = vardim_.size();
    const NrDimsType ndim = position_.size();

    TypeSet<NrDimsType> unkdims;
    for ( DimIdxType idx=0; idx<ndim; idx++ )
    {
	if ( position_[idx] == -1 )
	    unkdims += idx;
    }

    if ( unkdims.size() != nrowndims )
	return false;

    for ( DimIdxType idx=0; idx<nrowndims; idx++ )
    {
	if ( vardim_[idx]==-1 )
	{
	    if ( unkdims.size() )
	    {
		vardim_[idx] = unkdims[0];
		unkdims.removeSingle( 0 );
	    }
	    else
		return false;
	}
	else
	{
	    if ( !unkdims.isPresent(vardim_[idx]) )
		return false;
	    unkdims -= vardim_[idx];
	}
    }

    if ( unkdims.size() )
	return false;

    bool ismemorder = true;
    for ( DimIdxType idx=0; idx<nrowndims; idx++ )
    {
	if ( vardim_[idx]+nrowndims-idx!=ndim )
	{
	    ismemorder = false;
	    break;
	}
    }

    if ( !ismemorder )
    {
	bool hasonlyoneslice = true;
	for ( DimIdxType dimidx=0; dimidx<ndim; dimidx++ )
	    if ( position_[dimidx]!=-1 && getDimSize(dimidx)!=1 )
		{ hasonlyoneslice = false; break; }

	if ( hasonlyoneslice )
	    ismemorder = true;
    }

    if ( !ismemorder )
    {
	offset_ = -1;
    }
    else
    {
	mAllocVarLenArr( IdxType, localpos, nrowndims );
	OD::memZero( localpos, nrowndims * sizeof(IdxType) );

	mAllocVarLenArr( IdxType, tpos, ndim );
	getSourcePos( localpos, tpos );
	offset_ = sourceinfo_.getOffset(tpos);
    }

    for ( DimIdxType idx=0; idx<nrowndims; idx++ )
	info_.setSize( idx, sourceinfo_.getSize(vardim_[idx]) );

    isinited_ = true;
    return true;
}


void ArrayNDSliceBase::getSourcePos( NDPos localpos,
				     IdxType* arraypos ) const
{
    const NrDimsType ndim = (NrDimsType)position_.size();
    const NrDimsType nrowndims = (NrDimsType)vardim_.size();

    OD::memCopy( arraypos, position_.arr(), ndim*sizeof(IdxType) );

    for ( DimIdxType idx=0; idx<nrowndims; idx++ )
	arraypos[vardim_[idx]] = localpos[idx];
}
