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
#include "trckey.h"
#include "seistrc.h"


SynthSeis::PreStackDataSet::PreStackDataSet( const GenParams& gp,
					     GatherSetDataPack& dp,
					     RayModelSet& rms )
    : DataSet(gp,dp,&rms)
    , angledp_(0)
{
    trccache_.setNullAllowed( true );
    auto& gathers = dp.getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
	gathers[idx]->setName( name() );
}


SynthSeis::PreStackDataSet::~PreStackDataSet()
{
    for ( auto trcset : trccache_ )
	if ( trcset )
	    for ( auto trc : *trcset )
		delete trc;
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


SynthSeis::DataSet::OffsetDef SynthSeis::PreStackDataSet::offsetDef() const
{
    return OffsetDef( offsetRange(), offsetStep() );
}


#define mNrTrcs(gath) gath->size( true )


int SynthSeis::PreStackDataSet::nrOffsets() const
{
    const auto& gathers = preStackPack().getGathers();
    return gathers.isEmpty() ? 0 : mNrTrcs( gathers.first() );
}


const SeisTrc* SynthSeis::PreStackDataSet::gtTrc( int seqnr, float offs ) const
{
    const auto offsdef = offsetDef();
    const auto ioffs = offsdef.nearestIndex( offs );
    return getTrc( seqnr, ioffs );
}



void SynthSeis::PreStackDataSet::ensureCacheReady() const
{
    if ( trccache_.isEmpty() )
    {
	const auto& gathers = preStackPack().getGathers();
	for ( int igath=0; igath<gathers.size(); igath++ )
	    trccache_ += 0;
    }
}


#define mCheckOffsNr() \
    if ( gathers.isEmpty() ) \
	return 0; \
    if ( offsnr < 0 ) \
	{ pErrMsg("offsnr < 0"); offsnr = 0; } \
    if ( offsnr >= mNrTrcs(gathers.get(0)) ) \
    { \
	pErrMsg( "offsnr >= gather nrtrcs" ); \
	offsnr = mNrTrcs(gathers.get(0))-1; \
    }


const SeisTrc* SynthSeis::PreStackDataSet::getCachedTrc(
						int seqnr, int offsnr ) const
{
    const auto& gathers = preStackPack().getGathers();
    mCheckOffsNr();

    ensureCacheReady();
    if ( !trccache_.validIdx(seqnr) )
	return 0;

    const auto* trcset = trccache_.get( seqnr );
    return trcset ? trcset->get( offsnr ) : 0;
}


const SeisTrc* SynthSeis::PreStackDataSet::getTrc( int seqnr, int offsnr ) const
{
    const SeisTrc* trc = getCachedTrc( seqnr, offsnr );
    if ( !trc )
	trc = createTrc( seqnr, offsnr );
    return trc;
}


SeisTrc* SynthSeis::PreStackDataSet::createTrc( int seqnr, int offsnr ) const
{
    const auto& gathers = preStackPack().getGathers();
    mCheckOffsNr();

    const SeisTrc* cachedtrc = getCachedTrc( seqnr, offsnr );
    if ( !cachedtrc )
	cachedtrc = addTrcToCache( seqnr, offsnr );

    return new SeisTrc( *cachedtrc );
}


const SeisTrc* SynthSeis::PreStackDataSet::addTrcToCache( int seqnr,
							  int offsnr ) const
{
    const auto nroffs = nrOffsets();
    SeisTrc* trc = preStackPack().createTrace( seqnr, offsnr );
    if ( !trc )
	trc = new SeisTrc( nroffs );

    ensureCacheReady();
    if ( !trccache_.validIdx(seqnr) )
	return 0;

    trc->info().setTrcKey( TrcKey::getSynth(seqnr + 1) );
    trc->info().offset_ = offsetDef().atIndex( offsnr );
    auto* trcset = trccache_.get( seqnr );
    if ( !trcset )
    {
	trcset = new TrcSet;
	trcset->setNullAllowed( true );
	for ( auto ioffs=0; ioffs<nroffs; ioffs++ )
	    *trcset += ioffs == offsnr ? trc : 0;
    }
    else
    {
	SeisTrc* curtrc = trcset->get( offsnr );
	if ( !curtrc )
	    trcset->replace( offsnr, trc );
	else
	{
	    pErrMsg("new trc added but one exists - on purpose?");
	    *curtrc = *trc;
	    delete trc;
	}
    }

    return trc;
}


void SynthSeis::PreStackDataSet::setAngleData( const GatherSet& ags )
{
    angledp_ = new GatherSetDataPack( ags );
    const BufferString angledpnm( name().buf(), " (Angle Gather)" );
    angledp_->setName( angledpnm );
}


float SynthSeis::PreStackDataSet::offsetStep() const
{
    float offsetstep = 100.f; // better default than mUdf(float)
    const auto& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() )
    {
	const Gather& gather = *gathers.first();
	offsetstep = gather.getOffset(1) - gather.getOffset(0);
    }

    return offsetstep;
}


Interval<float> SynthSeis::PreStackDataSet::offsetRange() const
{
    Interval<float> offrg( 0.f, 0.f );
    const auto& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() )
    {
	const Gather& gather = *gathers.first();
	offrg.set( gather.getOffset(0), gather.getOffset(gather.size(true)-1) );
    }
    return offrg;
}


bool SynthSeis::PreStackDataSet::isNMOCorrected() const
{
    bool isnmo = true;
    rayPars().getYN( SynthSeis::GenBase::sKeyNMO(), isnmo );
    return isnmo;
}


SeisTrcBuf* SynthSeis::PreStackDataSet::getTrcBuf( float offset,
					const Interval<float>* stackrg ) const
{
    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    Interval<float> offrg = stackrg ? *stackrg : Interval<float>(offset,offset);
    preStackPack().fill( *tbuf, offrg );
    return tbuf;
}
