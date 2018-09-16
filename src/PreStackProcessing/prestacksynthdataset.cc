/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        nageswara
Date:	       Sep 2016
________________________________________________________________________

-*/

#include "prestacksynthdataset.h"

#include "prestackgather.h"
#include "seisbufadapters.h"
#include "synthseisgenerator.h"


SynthSeis::PreStackDataSet::PreStackDataSet( const GenParams& gp,
					     GatherSetDataPack& dp )
    : DataSet(gp,dp)
    , angledp_(0)
{
    auto& gathers = dp.getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
	gathers[idx]->setName( name() );
}


SynthSeis::PreStackDataSet::~PreStackDataSet()
{
}


DataPackMgr::ID	SynthSeis::PreStackDataSet::dpMgrID() const
{
    return DataPackMgr::SeisID();
}


GatherSetDataPack& SynthSeis::PreStackDataSet::preStackPack()
{
    return static_cast<GatherSetDataPack&>( *datapack_ );
}


const GatherSetDataPack& SynthSeis::PreStackDataSet::preStackPack() const
{
    return static_cast<const GatherSetDataPack&>( *datapack_ );
}


void SynthSeis::PreStackDataSet::convertAngleDataToDegrees( Gather* ag ) const
{
    Array2D<float>& agdata = ag->data();
    const int dim0sz = agdata.getSize(0);
    const int dim1sz = agdata.getSize(1);
    for ( int idx=0; idx<dim0sz; idx++ )
    {
	for ( int idy=0; idy<dim1sz; idy++ )
	{
	    const float radval = agdata.get( idx, idy );
	    if ( mIsUdf(radval) ) continue;
	    const float dval =	Math::toDegrees( radval );
	    agdata.set( idx, idy, dval );
	}
    }
}


void SynthSeis::PreStackDataSet::setAngleData( const GatherSet& ags )
{
    BufferString angledpnm( name().buf(), " (Angle Gather)" );
    angledp_ = new GatherSetDataPack( angledpnm, ags );
}


float SynthSeis::PreStackDataSet::offsetRangeStep() const
{
    float offsetstep = mUdf(float);
    const auto& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() )
    {
	const Gather& gather = *gathers[0];
	offsetstep = gather.getOffset(1)-gather.getOffset(0);
    }

    return offsetstep;
}


const Interval<float> SynthSeis::PreStackDataSet::offsetRange() const
{
    Interval<float> offrg( 0, 0 );
    const auto& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() )
    {
	const Gather& gather = *gathers[0];
	offrg.set(gather.getOffset(0),gather.getOffset( gather.size(true)-1));
    }
    return offrg;
}


bool SynthSeis::PreStackDataSet::hasOffset() const
{
    return offsetRange().width() > 0;
}


bool SynthSeis::PreStackDataSet::isNMOCorrected() const
{
    bool isnmo = true;
    raypars_.getYN( SynthSeis::GenBase::sKeyNMO(), isnmo );
    return isnmo;
}


const SeisTrc* SynthSeis::PreStackDataSet::getTrace(
			int seqnr, int offset ) const
{
    return preStackPack().getTrace( seqnr, offset );
}


SeisTrcBuf* SynthSeis::PreStackDataSet::getTrcBuf( float offset,
					const Interval<float>* stackrg ) const
{
    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    Interval<float> offrg = stackrg ? *stackrg : Interval<float>(offset,offset);
    preStackPack().fill( *tbuf, offrg );
    return tbuf;
}
