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

mImplFactory( InterpolationLayerModel, InterpolationLayerModel::factory )

const char* InterpolationLayerModel::sKeyModelType()
{ return "Interpolation Layer Model"; }

void InterpolationLayerModel::fillPar( IOPar& par ) const
{ par.set( sKeyModelType(), factoryKeyword() ); }


void ZSliceInterpolationModel::setCubeSampling( const CubeSampling& cs )
{ cs_ = cs; }

int ZSliceInterpolationModel::nrLayers() const
{ return cs_.nrZ(); }

bool ZSliceInterpolationModel::hasPosition( const BinID& bid ) const
{ return cs_.hrg.includes( bid ); }

float ZSliceInterpolationModel::getZ( const BinID& bid, int layer ) const
{ return hasPosition( bid ) ? cs_.zrg.atIndex( layer ) : mUdf(float); }

void ZSliceInterpolationModel::getAllZ( const BinID& bid,
					TypeSet<float>& zvals ) const
{
    for ( int idx=0; idx<cs_.nrZ(); idx++ )
	zvals += cs_.zrg.atIndex( idx );
}

float ZSliceInterpolationModel::getLayerIndex( const BinID& bid, float z ) const
{ return hasPosition(bid) ? cs_.zrg.getfIndex( z ) : mUdf(float); }
