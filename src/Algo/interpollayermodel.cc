/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "interpollayermodel.h"

#include "iopar.h"
#include "keystrs.h"
#include "math2.h"

// InterpolationLayerModel
mImplFactory( InterpolationLayerModel, InterpolationLayerModel::factory )

InterpolationLayerModel::InterpolationLayerModel()
    : zsamp_(StepInterval<float>::udf())
{}


InterpolationLayerModel::InterpolationLayerModel(
					const InterpolationLayerModel& oth )
    : zsamp_(oth.zsamp_)
{
    for ( int idx=0; idx<oth.tkss_.size(); idx++ )
	tkss_.add( new TrcKeySampling( *oth.tkss_[idx] ) );
}


InterpolationLayerModel::~InterpolationLayerModel()
{
    deepErase( tkss_ );
}


bool InterpolationLayerModel::isOK( const TrcKey* tk ) const
{
    if ( !tk )
	return true;

    for ( int idx=0; idx<tkss_.size(); idx++ )
    {
	if ( tkss_[idx]->isDefined() && tkss_[idx]->includes(*tk) )
	    return true;
    }

    return false;
}

bool InterpolationLayerModel::hasSampling() const
{
    return !tkss_.isEmpty() && tkss_[0]->isDefined();
}


bool InterpolationLayerModel::prepare( const TrcKeyZSampling& tkzs,
				       TaskRunner* )
{
    deepErase( tkss_ );
    tkss_.add( new TrcKeySampling( tkzs.hsamp_ ) );
    zsamp_ = tkzs.zsamp_;

    return true;
}


void InterpolationLayerModel::addSampling( const TrcKeySampling& tks )
{
    if ( ( hasSampling() && !tkss_[0]->includes(tks) ) || !hasSampling() )
	tkss_.add( new TrcKeySampling(tks) );
}


od_int64 InterpolationLayerModel::getMemoryUsage(
				  const TrcKeySampling& hsamp ) const
{
    return hsamp.totalNr()* nrLayers() * sizeof(float);
}


float InterpolationLayerModel::getInterpolatedZ( const TrcKey& tk,
						 float layeridx ) const
{
    const float fidx0 = Math::Floor( layeridx );
    const float fidx1 = Math::Ceil( layeridx );
    const int idx0 = mNINT32( fidx0 );
    const int idx1 = mNINT32( fidx1 );
    const float z0 = getZ( tk, idx0 );
    if ( idx1==idx0 ) return z0;

    const float z1 = getZ( tk, idx1 );
    return z0 + (z1-z0)*(layeridx-idx0);
}


const char* InterpolationLayerModel::sKeyModelType()
{ return sKey::Type(); }

void InterpolationLayerModel::fillPar( IOPar& par ) const
{
    par.set( sKeyModelType(), factoryKeyword() );
}



// ZSliceInterpolationModel
ZSliceInterpolationModel::ZSliceInterpolationModel()
    :InterpolationLayerModel()
{}


ZSliceInterpolationModel::ZSliceInterpolationModel(
					const ZSliceInterpolationModel & oth )
    : InterpolationLayerModel( oth )
{}

InterpolationLayerModel* ZSliceInterpolationModel::clone() const
{ return new ZSliceInterpolationModel( *this ); }

bool ZSliceInterpolationModel::isOK( const TrcKey* tk ) const
{
    return InterpolationLayerModel::isOK(tk) && !zsamp_.isUdf();
}

float ZSliceInterpolationModel::getLayerIndex( const TrcKey& tk, float z ) const
{ return isOK(&tk) ? zsamp_.getfIndex( z ) : mUdf(float); }

int ZSliceInterpolationModel::nrLayers() const
{ return zsamp_.isUdf() ? mUdf(int) : zsamp_.nrSteps()+1; }

float ZSliceInterpolationModel::getZ( const TrcKey& tk, int layer ) const
{
    return isOK(&tk) ? zsamp_.atIndex( layer ) : mUdf(float);
}
