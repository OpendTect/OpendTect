/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:	       July 2011
________________________________________________________________________

-*/

#include "syntheticdata.h"
#include "syntheticdataimpl.h"

#include "datapackbase.h"
#include "stratsynthgenparams.h"
#include "velocitycalc.h"

#include "array1dinterpol.h"
#include "angles.h"
#include "arrayndimpl.h"
#include "binidvalset.h"
#include "datapackbase.h"
#include "elasticpropsel.h"
#include "envvars.h"
#include "fourier.h"
#include "fftfilter.h"
#include "flatposdata.h"
#include "mathfunc.h"
#include "propertyref.h"
#include "raytracerrunner.h"
#include "separstr.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "timeser.h"
#include "waveletmanager.h"

#define sDefaultAngleRange Interval<float>( 0.0f, 30.0f )
#define sDefaultOffsetRange StepInterval<float>( 0.f, 6000.f, 100.f )

#include "raytrace1d.h"

static const char* sKeyDispPar()		{ return "Display Parameter"; }

//SynthFVSpecificDispPars
void SynthFVSpecificDispPars::fillPar( IOPar& par ) const
{
    IOPar disppar, vdmapperpar, wvamapperpar;
    vdmapperpar.set( FlatView::DataDispPars::sKeyColTab(), ctab_ );
    wvamapperpar.set( FlatView::DataDispPars::sKeyOverlap(), overlap_ );
    vdmapper_.fillPar( vdmapperpar );
    disppar.mergeComp( vdmapperpar, FlatView::DataDispPars::sKeyVD() );
    wvamapper_.fillPar( wvamapperpar );
    disppar.mergeComp( wvamapperpar, FlatView::DataDispPars::sKeyWVA() );
    par.mergeComp( disppar, sKeyDispPar() );
}


void SynthFVSpecificDispPars::usePar( const IOPar& par )
{
    PtrMan<IOPar> disppar = par.subselect( sKeyDispPar() );
    if ( !disppar )
	return;

    overlap_ = 1.0f;
    disppar->get( FlatView::DataDispPars::sKeyColTab(), ctab_ );
    disppar->get( FlatView::DataDispPars::sKeyOverlap(), overlap_ );
    PtrMan<IOPar> vdmapperpar =
	disppar->subselect( FlatView::DataDispPars::sKeyVD() );
    if ( !vdmapperpar ) // Older par file
    {
	vdmapper_.type_ = ColTab::MapperSetup::Fixed;
	wvamapper_.type_ = ColTab::MapperSetup::Fixed;
	disppar->get( sKey::Range(), vdmapper_.range_ );
	disppar->get( sKey::Range(), wvamapper_.range_ );
    }
    else
    {
	 if ( vdmapperpar )
	 {
	     vdmapper_.usePar( *vdmapperpar );
	     vdmapperpar->get( FlatView::DataDispPars::sKeyColTab(), ctab_ );
	 }
	 PtrMan<IOPar> wvamapperpar =
	     disppar->subselect( FlatView::DataDispPars::sKeyWVA() );
	 if ( wvamapperpar )
	 {
	     wvamapper_.usePar( *wvamapperpar );
	     wvamapperpar->get(FlatView::DataDispPars::sKeyOverlap(),overlap_);
	 }
    }
}


//SyntheticData
SyntheticData::SyntheticData( const SynthGenParams& sgp, DataPack& dp )
    : NamedObject(sgp.name_)
    , datapack_(&dp)
    , id_(-1)
{
}


SyntheticData::~SyntheticData()
{
    deepErase( d2tmodels_ );
    deepErase( zerooffsd2tmodels_ );
    removePack();
}


void SyntheticData::setName( const char* nm )
{
    NamedObject::setName( nm );
    datapack_->setName( nm );
}


void SyntheticData::removePack()
{
    datapack_ = 0;
}


float SyntheticData::getTime( float dpt, int seqnr ) const
{
    return zerooffsd2tmodels_.validIdx( seqnr )
	? zerooffsd2tmodels_[seqnr]->getTime( dpt ) : mUdf( float );
}


float SyntheticData::getDepth( float time, int seqnr ) const
{
    return zerooffsd2tmodels_.validIdx( seqnr )
	? zerooffsd2tmodels_[seqnr]->getDepth( time ) : mUdf( float );
}


bool SyntheticData::isAngleStack() const
{
    TypeSet<float> offsets;
    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
    return !isPS() && offsets.size()>1;
}


void SyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
    sgp.inpsynthnm_.setEmpty();
    sgp.raypars_ = raypars_;
    sgp.wvltnm_ = wvltnm_;
    sgp.name_ = name();
    sgp.synthtype_ = synthType();
}


void SyntheticData::useGenParams( const SynthGenParams& sgp )
{
    raypars_ = sgp.raypars_;
    wvltnm_ = sgp.wvltnm_;
    setName( sgp.name_ );
}


void SyntheticData::fillDispPar( IOPar& par ) const
{
    disppars_.fillPar( par );
}


void SyntheticData::useDispPar( const IOPar& par )
{
    disppars_.usePar( par );
}


//PostStackSyntheticData
PostStackSyntheticData::PostStackSyntheticData( const SynthGenParams& sgp,
						SeisTrcBufDataPack& dp)
    : SyntheticData(sgp,dp)
{
    useGenParams( sgp );
    DataPackMgr::ID pmid = DataPackMgr::FlatID();
    DPM( pmid ).add( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
}


PostStackSyntheticData::~PostStackSyntheticData()
{
}


const SeisTrc* PostStackSyntheticData::getTrace( int seqnr ) const
{ return postStackPack().trcBuf().get( seqnr ); }


SeisTrcBufDataPack& PostStackSyntheticData::postStackPack()
{
    return static_cast<SeisTrcBufDataPack&>( *datapack_ );
}


const SeisTrcBufDataPack& PostStackSyntheticData::postStackPack() const
{
    return static_cast<const SeisTrcBufDataPack&>( *datapack_ );
}


//PSBasedPostStackSyntheticData
PSBasedPostStackSyntheticData::PSBasedPostStackSyntheticData(
	const SynthGenParams& sgp, SeisTrcBufDataPack& sdp )
    : PostStackSyntheticData(sgp,sdp)
{
    useGenParams( sgp );
}


PSBasedPostStackSyntheticData::~PSBasedPostStackSyntheticData()
{}


void PSBasedPostStackSyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
    SyntheticData::fillGenParams( sgp );
    sgp.inpsynthnm_ = inpsynthnm_;
    sgp.anglerg_ = anglerg_;
}


void PSBasedPostStackSyntheticData::useGenParams( const SynthGenParams& sgp )
{
    SyntheticData::useGenParams( sgp );
    inpsynthnm_ = sgp.inpsynthnm_;
    anglerg_ = sgp.anglerg_;
}


//StratPropSyntheticData
StratPropSyntheticData::StratPropSyntheticData( const SynthGenParams& sgp,
						    SeisTrcBufDataPack& dp,
						    const PropertyRef& pr )
    : PostStackSyntheticData( sgp, dp )
    , prop_(pr)
{}
