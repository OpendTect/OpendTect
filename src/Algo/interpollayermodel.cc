/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "interpollayermodel.h"

#include "iopar.h"
#include "math2.h"

// InterpolationLayerModel
mImplFactory( InterpolationLayerModel, InterpolationLayerModel::factory )

InterpolationLayerModel::~InterpolationLayerModel()
{}

const char* InterpolationLayerModel::sKeyModelType()
{ return sKey::Type(); }

void InterpolationLayerModel::fillPar( IOPar& par ) const
{ par.set( sKeyModelType(), factoryKeyword() ); }


float InterpolationLayerModel::getInterpolatedZ( const BinID& bid,
						 float layeridx ) const
{
    const float fidx0 = Math::Floor( layeridx );
    const float fidx1 = Math::Ceil( layeridx );
    const int idx0 = mNINT32( fidx0 );
    const int idx1 = mNINT32( fidx1 );
    const float z0 = getZ( bid, idx0 );
    if ( idx1==idx0 ) return z0;

    const float z1 = getZ( bid, idx1 );
    return z0 + (z1-z0)*(layeridx-idx0);
}



// ZSliceInterpolationModel
void ZSliceInterpolationModel::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{ tkzs_ = cs; }

int ZSliceInterpolationModel::nrLayers() const
{ return tkzs_.nrZ(); }

bool ZSliceInterpolationModel::hasPosition( const BinID& bid ) const
{ return tkzs_.hsamp_.includes( bid ); }

float ZSliceInterpolationModel::getZ( const BinID& bid, int layer ) const
{ return hasPosition( bid ) ? tkzs_.zsamp_.atIndex( layer ) : mUdf(float); }


void ZSliceInterpolationModel::getAllZ( const BinID& bid,
					TypeSet<float>& zvals ) const
{
    for ( int idx=0; idx<tkzs_.nrZ(); idx++ )
	zvals += tkzs_.zsamp_.atIndex( idx );
}

float ZSliceInterpolationModel::getLayerIndex( const BinID& bid, float z ) const
{ return hasPosition(bid) ? tkzs_.zsamp_.getfIndex( z ) : mUdf(float); }

