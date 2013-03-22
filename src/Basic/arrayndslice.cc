/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "arrayndslice.h"
#include "varlenarray.h"


ArrayNDSliceBase::ArrayNDSliceBase( ArrayNDInfo* localinfo,
				    const ArrayNDInfo& sourceinfo )
    : info_( *localinfo )
    , sourceinfo_( sourceinfo )
    , position_( sourceinfo.getNDim(), -1 )
    , vardim_( localinfo->getNDim(), -1 )
    , offset_( -1 )
{ }


ArrayNDSliceBase::~ArrayNDSliceBase()
{ delete &info_; }


bool ArrayNDSliceBase::setPos( int dim, int pos )
{
    const int ndim = position_.size();
    if ( dim<0 || dim>=ndim || pos<0 || pos>=getDimSize(dim) )
	return false;

    position_[dim] = pos;

    return true;
}


int ArrayNDSliceBase::getPos( int dim ) const
{
    return position_[dim];
}


int ArrayNDSliceBase::getDimSize( int dim ) const
{
    return sourceinfo_.getSize( dim );
}


void ArrayNDSliceBase::setDimMap( int localdim, int remotedim )
{
    vardim_[localdim] = remotedim;
    position_[remotedim] = -1;
}


bool ArrayNDSliceBase::init()
{ 
    const int nrowndims = vardim_.size();
    const int ndim = position_.size();

    TypeSet<int> unkdims;
    for ( int idx=0; idx<ndim; idx++ )
    {
	if ( position_[idx] == -1 )
	    unkdims += idx;
    }

    if ( unkdims.size() != nrowndims )
	return false;

    for ( int idx=0; idx<nrowndims; idx++ )
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
    for ( int idx=0; idx<nrowndims; idx++ )
    {
	if ( vardim_[idx]+nrowndims-idx!=ndim )
	{
	    ismemorder = false;
	    break;
	}
    }

    if ( !ismemorder )
    {
	offset_ = -1;
    }
    else
    {
	mAllocVarLenArr( int, localpos, nrowndims );
	memset( localpos, 0, nrowndims * sizeof(int) );

	mAllocVarLenArr( int, tpos, ndim );
	getSourcePos( localpos, tpos );
	offset_ = sourceinfo_.getOffset(tpos);
    }

    for ( int idx=0; idx<nrowndims; idx++ )
	info_.setSize( idx, sourceinfo_.getSize(vardim_[idx]) );

    return true;
}


void ArrayNDSliceBase::getSourcePos( const int* localpos, int* arraypos ) const
{
    const int ndim = position_.size();
    const int nrowndims = vardim_.size();

    memcpy( arraypos, position_.arr(), ndim*sizeof(int) );

    for ( int idx=0; idx<nrowndims; idx++ )
	arraypos[vardim_[idx]] = localpos[idx];
}
