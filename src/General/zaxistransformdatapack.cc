/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: zaxistransformdatapack.cc,v 1.11 2010/06/28 04:09:54 cvsnanne Exp $";

#include "zaxistransformdatapack.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "arrayndwrapper.h"
#include "flatposdata.h"
#include "cubesampling.h"
#include "iopar.h"
#include "keystrs.h"
#include "zaxistransform.h"
#include "zaxistransformer.h"


ZAxisTransformDataPack::ZAxisTransformDataPack( const FlatDataPack& fdp,
						const CubeSampling& cs,
       						ZAxisTransform& zat )
    : FlatDataPack( fdp.category() )
    , inputdp_(fdp)
    , inputcs_(cs)
    , transform_(zat)
    , interpolate_(false)
    , array3d_(0)
    , array2dsl_(0)
    , outputcs_( 0 )
{
    DPM( DataPackMgr::FlatID() ).obtain( fdp.id() );
    setName( fdp.name() );
    posdata_.setRange( true, fdp.posData().range(true) );
    posdata_.setRange( false, fdp.posData().range(false) );

    transform_.ref();
}


ZAxisTransformDataPack::~ZAxisTransformDataPack()
{
    DPM( DataPackMgr::FlatID() ).release( inputdp_.id() );
    transform_.removeVolumeOfInterest( voiid_ );
    transform_.unRef();
    delete outputcs_;
}


void ZAxisTransformDataPack::setOutputCS( const CubeSampling& cs )
{
    if ( !outputcs_ ) outputcs_ = new CubeSampling( cs );
    else *outputcs_ = cs;
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

static void getDimensions( int& dim0, int& dim1, int& unuseddim,
			   const CubeSampling& cs )
{
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
}


bool ZAxisTransformDataPack::transform()
{
    int unuseddim, dim0, dim1;
    getDimensions( dim0, dim1, unuseddim, inputcs_ );

    ZAxisTransformer transformer( transform_, true );
    transformer.setInterpolate( interpolate_ );

    Array2D<float>& arr2d = const_cast<Array2D<float>&>( inputdp_.data() );
    Array3DWrapper<float> inputarr3d( arr2d );
    inputarr3d.setDimMap( 0, dim0 );
    inputarr3d.setDimMap( 1, dim1 );
    inputarr3d.init();
    transformer.setInput( inputarr3d, inputcs_ );

    CubeSampling outputcs = outputcs_ ? *outputcs_ : inputcs_;
    if ( !outputcs_ )
	outputcs.zrg.setFrom( transform_.getZInterval(false) );
    transformer.setOutputRange( outputcs );

    if ( !transformer.execute() )
	return false;

    transformer.removeVoiOnDelete( false );
    voiid_ = transformer.getVoiID();
    array3d_ = transformer.getOutput( true );
    if ( !array3d_ )
	return false;

    array2dsl_ = new Array2DSlice<float>( *array3d_ );
    array2dsl_->setPos( unuseddim, 0 );
    array2dsl_->setDimMap( 0, dim0 );
    array2dsl_->setDimMap( 1, dim1 );
    array2dsl_->init();

    const StepInterval<float> zrg = outputcs.zrg;
    posdata_.setRange( false,
	    	       StepInterval<double>(zrg.start,zrg.stop,zrg.step) );
    return true;
}


Array2D<float>& ZAxisTransformDataPack::data()
{
    return *array2dsl_;
}


const Array2D<float>& ZAxisTransformDataPack::data() const
{
    return *array2dsl_;
}


const char* ZAxisTransformDataPack::dimName( bool x1 ) const
{ return inputdp_.dimName( x1 ); }
