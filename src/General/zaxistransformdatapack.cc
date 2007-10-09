/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2007
 RCS:		$Id: zaxistransformdatapack.cc,v 1.1 2007-10-09 12:12:28 cvsnanne Exp $
________________________________________________________________________

-*/

#include "zaxistransformdatapack.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "flatposdata.h"
#include "cubesampling.h"
#include "iopar.h"
#include "keystrs.h"
#include "zaxistransform.h"
#include "zaxistransformer.h"


ZAxisTransformDataPack::ZAxisTransformDataPack( const FlatDataPack& fdp,
       						ZAxisTransformer& zt )
    : FlatDataPack( fdp.category() )
    , inputdp_(fdp)
    , transformer_(zt)
    , array3d_(0)
    , array2dsl_(0)
{
    setName( fdp.name() );
    posdata_.setRange( true, fdp.posData().range(true) );
    posdata_.setRange( false, fdp.posData().range(false) );
}


ZAxisTransformDataPack::~ZAxisTransformDataPack()
{
}


void ZAxisTransformDataPack::dumpInfo( IOPar& iop ) const
{
    DataPack::dumpInfo( iop );
    FlatDataPack::dumpInfo( iop );
    iop.set( sKey::Type, "Flat ZAxis Transformed" );
}


#define mInl 0
#define mCrl 1
#define mZ   2

bool ZAxisTransformDataPack::transform()
{
    const CubeSampling& cs = transformer_.getOutputRange();

    if ( !transformer_.execute() )
	return false;

    array3d_ = transformer_.getOutput( true );
    if ( !array3d_ )
	return false;

    int unuseddim, dim0, dim1;
    if ( cs.nrInl() < 2 )
    {
	unuseddim = mInl;
	dim0 = mCrl;
	dim1 = mZ;
    }
    else if ( cs.nrCrl() < 2 )
    {
	unuseddim = mCrl;
	dim0 = mInl;
	dim1 = mZ;
    }
    else
    {
	unuseddim = mZ;
	dim0 = mInl;
	dim1 = mCrl;
    }

    array2dsl_ = new Array2DSlice<float>( *array3d_ );
    array2dsl_->setPos( unuseddim, 0 );
    array2dsl_->setDimMap( 0, dim0 );
    array2dsl_->setDimMap( 1, dim1 );
    array2dsl_->init();

    // setPosData ?
    return true;
}


Array2D<float>& ZAxisTransformDataPack::data()
{
    return *array2dsl_;
}
