/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        nageswara
Date:	       Sep 2016
________________________________________________________________________

-*/

#include "prestacksyntheticdata.h"

#include "prestackgather.h"
#include "seisbufadapters.h"
#include "synthseis.h"

namespace PreStack
{

PreStackSyntheticData::PreStackSyntheticData( const SynthGenParams& sgp,
					      GatherSetDataPack& dp)
    : SyntheticData(sgp,dp)
    , angledp_(0)
{
    useGenParams( sgp );
    DataPackMgr::ID pmid = DataPackMgr::SeisID();
    DPM( pmid ).add( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
    ObjectSet<Gather>& gathers = dp.getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
    {
	gathers[idx]->setName( name() );
    }
}


PreStackSyntheticData::~PreStackSyntheticData()
{}


GatherSetDataPack& PreStackSyntheticData::preStackPack()
{
    return static_cast<GatherSetDataPack&>( *datapack_ );
}


const GatherSetDataPack& PreStackSyntheticData::preStackPack() const
{
    return static_cast<const GatherSetDataPack&>( *datapack_ );
}


void PreStackSyntheticData::convertAngleDataToDegrees(
					Gather* ag ) const
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


void PreStackSyntheticData::setAngleData(
	const ObjectSet<Gather>& ags )
{
    BufferString angledpnm( name().buf(), " (Angle Gather)" );
    angledp_ = new GatherSetDataPack( angledpnm, ags );
    DPM( DataPackMgr::SeisID() ).add( angledp_ );
}


float PreStackSyntheticData::offsetRangeStep() const
{
    float offsetstep = mUdf(float);
    const ObjectSet<Gather>& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() )
    {
	const Gather& gather = *gathers[0];
	offsetstep = gather.getOffset(1)-gather.getOffset(0);
    }

    return offsetstep;
}


const Interval<float> PreStackSyntheticData::offsetRange() const
{
    Interval<float> offrg( 0, 0 );
    const ObjectSet<Gather>& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() )
    {
	const Gather& gather = *gathers[0];
	offrg.set(gather.getOffset(0),gather.getOffset( gather.size(true)-1));
    }
    return offrg;
}


bool PreStackSyntheticData::hasOffset() const
{ return offsetRange().width() > 0; }


bool PreStackSyntheticData::isNMOCorrected() const
{
    bool isnmo = true;
    raypars_.getYN( Seis::SynthGenBase::sKeyNMO(), isnmo );
    return isnmo;
}


const SeisTrc* PreStackSyntheticData::getTrace( int seqnr, int* offset ) const
{ return preStackPack().getTrace( seqnr, offset ? *offset : 0 ); }


SeisTrcBuf* PreStackSyntheticData::getTrcBuf( float offset,
					const Interval<float>* stackrg ) const
{
    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    Interval<float> offrg = stackrg ? *stackrg : Interval<float>(offset,offset);
    preStackPack().fill( *tbuf, offrg );
    return tbuf;
}

}; //namespace
