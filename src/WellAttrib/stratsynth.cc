/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
________________________________________________________________________

-*/


#include "stratsynth.h"
#include "syntheticdataimpl.h"
#include "stratsynthlevel.h"

#include "array1dinterpol.h"
#include "angles.h"
#include "arrayndalgo.h"
#include "attribsel.h"
#include "attribengman.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribprocessor.h"
#include "attribfactory.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "binidvalset.h"
#include "datapackbase.h"
#include "elasticpropsel.h"
#include "envvars.h"
#include "fourier.h"
#include "fftfilter.h"
#include "flatposdata.h"
#include "hilbertattrib.h"
#include "ioman.h"
#include "ioobj.h"
#include "mathfunc.h"
#include "prestackattrib.h"
#include "prestackgather.h"
#include "prestackanglecomputer.h"
#include "propertyref.h"
#include "raytracerrunner.h"
#include "separstr.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "unitofmeasure.h"
#include "timedepthmodel.h"
#include "timeser.h"
#include "transl.h"
#include "wavelet.h"
#include "waveletio.h"

static const char* sKeyIsPreStack()		{ return "Is Pre Stack"; }
static const char* sKeySynthType()		{ return "Synthetic Type"; }
static const char* sKeyWaveLetName()		{ return "Wavelet Name"; }
static const char* sKeyRayPar()			{ return "Ray Parameter"; }
static const char* sKeySynthPar()		{ return "Synth Parameter"; }
static const char* sKeyDispPar()		{ return "Display Parameter"; }
static const char* sKeyInput()			{ return "Input Synthetic"; }
static const char* sKeyAngleRange()		{ return "Angle Range"; }
const char* PostStackSyntheticData::sDataPackCategory()
{ return "Post-stack synthetics"; }

#define sDefaultAngleRange Interval<float>( 0.0f, 30.0f )
#define sDefaultOffsetRange StepInterval<float>( 0.f, 6000.f, 100.f )


mDefineEnumUtils(SynthGenParams,SynthType,"Synthetic Type")
{
    "Pre Stack",
    "Zero Offset Stack",
    "Strat Property",
    "Angle Stack",
    "AVO Gradient",
    "Attribute",
    nullptr
};


SynthGenParams::SynthGenParams( SynthType tp )
    : synthtype_(tp)
{
    setDefaultValues();
}


bool SynthGenParams::operator== ( const SynthGenParams& oth ) const
{
    bool hassameanglerg = true;
    bool hassameinput = true;
    bool hassameattrib = true;
    if ( oth.isPSBased() )
    {
	hassameanglerg = anglerg_ == oth.anglerg_;
	hassameinput = inpsynthnm_ == oth.inpsynthnm_;
    }
    else if ( oth.isAttribute() )
    {
	hassameinput = inpsynthnm_ == oth.inpsynthnm_;
	hassameattrib = attribtype_ == oth.attribtype_;
    }

    return isPreStack() == oth.isPreStack() && wvltnm_ == oth.wvltnm_ &&
	   raypars_ == oth.raypars_ && synthpars_ == oth.synthpars_ &&
	   hassameanglerg && hassameinput && hassameattrib;
}


bool SynthGenParams::operator!= ( const SynthGenParams& oth ) const
{
    return !(*this == oth);
}


bool SynthGenParams::isOK() const
{
    if ( isRawOutput() )
	return !wvltnm_.isEmpty();
    else if ( isPSBased() )
	return !inpsynthnm_.isEmpty() && !anglerg_.isUdf();
    else if ( isAttribute() )
	return !inpsynthnm_.isEmpty();

    return true;
}


void SynthGenParams::setDefaultValues()
{
    if ( isRawOutput() )
    {
	const BufferStringSet& facnms = RayTracer1D::factory().getNames();
	if ( !facnms.isEmpty() )
	{
	    const BufferString defnm( RayTracer1D::factory().getDefaultName() );
	    raypars_.set( sKey::Type(), defnm.isEmpty()
			   ? VrmsRayTracer1D::sFactoryKeyword() : defnm.str() );
	}
    }

    if ( isZeroOffset() )
	RayTracer1D::setIOParsToZeroOffset( raypars_ );
    else if ( synthtype_ == PreStack )
    {
	const StepInterval<float> offsetrg = sDefaultOffsetRange;
	TypeSet<float> offsets;
	for ( int idx=0; idx<offsetrg.nrSteps()+1; idx++ )
	    offsets += offsetrg.atIndex( idx );
	raypars_.set( RayTracer1D::sKeyOffset(), offsets );
    }

    if ( isRawOutput() )
    {
	const TranslatorGroup& wvlttrgrp = WaveletTranslatorGroup::theInst();
	const BufferString keystr( wvlttrgrp.getSurveyDefaultKey() );
	MultiID wvltkey;
	PtrMan<IOObj> wvltobj = SI().getPars().get( keystr, wvltkey )
			      ? IOM().get( wvltkey )
			      : IOM().getFirst( wvlttrgrp.ioCtxt() );
	if ( wvltobj )
	    wvltnm_ = wvltobj->name();
    }

    synthpars_.setEmpty();

    anglerg_ = synthtype_ == AngleStack || synthtype_ == AVOGradient
	     ? sDefaultAngleRange : Interval<float>::udf();
    createName( name_ );
}


const char* SynthGenParams::getWaveletNm() const
{
    return wvltnm_.buf();
}


bool SynthGenParams::hasOffsets() const
{
    TypeSet<float> offsets;
    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
    return offsets.size()>1;
}


void SynthGenParams::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name_ );
    par.set( sKeySynthType(), toString(synthtype_) );
    if ( isRawOutput() )
    {
	par.set( sKeyWaveLetName(), wvltnm_ );
	IOPar raypar, synthpar;
	raypar.mergeComp( raypars_, sKeyRayPar() );
	synthpar.mergeComp( synthpars_, sKeySynthPar() );
	par.merge( raypar );
	par.merge( synthpar );
    }
    else if ( needsInput() )
    {
	par.set( sKeyInput(), inpsynthnm_ );
	if ( synthtype_ == AngleStack || synthtype_ == AVOGradient )
	    par.set( sKeyAngleRange(), anglerg_ );
	else if ( synthtype_ == InstAttrib )
	    par.set( sKey::Attribute(),
			     Attrib::Instantaneous::toString( attribtype_) );
    }
}


void SynthGenParams::usePar( const IOPar& par )
{
    par.get( sKey::Name(), name_ );
    if ( par.hasKey(sKeyIsPreStack()) ) //Legacy
    {
	bool isps = false;
	par.getYN( sKeyIsPreStack(), isps );
	if ( !isps && hasOffsets() )
	    synthtype_ = AngleStack;
	else if ( !isps )
	    synthtype_ = ZeroOffset;
	else
	    synthtype_ = PreStack;
    }
    else
    {
	BufferString typestr;
	parseEnum( par, sKeySynthType(), synthtype_ );
    }

    raypars_.setEmpty();
    synthpars_.setEmpty();
    if ( isRawOutput() )
    {
	par.get( sKeyWaveLetName(), wvltnm_ );
	PtrMan<IOPar> raypar = par.subselect( sKeyRayPar() );
	if ( raypar )
	    cleanRayPar( *raypar, raypars_ );

	PtrMan<IOPar> synthpar = par.subselect( sKeySynthPar() );
	if ( synthpar )
	    synthpars_.merge( *synthpar.ptr() );
	else if ( raypar )
	    setSynthGenPar( *raypar, synthpars_ );

	MultiID wvltkey;
	if ( synthpars_.get(sKey::WaveletID(),wvltkey) )
	{
	    PtrMan<IOObj> wvltobj = IOM().get( wvltkey );
	    if ( wvltobj && wvltnm_ != wvltobj->name() )
	    {
		wvltnm_.set( wvltobj->name() );
		createName( name_ );
	    }
	}

	inpsynthnm_.setEmpty();
	anglerg_ = Interval<float>::udf();
    }
    else if ( needsInput() )
    {
	par.get( sKeyInput(), inpsynthnm_ );
	if ( synthtype_ == AngleStack || synthtype_ == AVOGradient )
	    par.get( sKeyAngleRange(), anglerg_ );
	else if ( synthtype_ == InstAttrib )
	{
	    BufferString attribstr;
	    par.get( sKey::Attribute(), attribstr );
	    Attrib::Instantaneous::parseEnum( attribstr, attribtype_ );
	}
    }
}


void SynthGenParams::createName( BufferString& nm ) const
{
    if ( isPSBased() )
    {
	nm = toString( synthtype_ );
	nm += " ["; nm += anglerg_.start; nm += ",";
	nm += anglerg_.stop; nm += "] degrees";
	return;
    }
    else if ( isAttribute() )
    {
	nm = Attrib::Instantaneous::toString( attribtype_ );
	nm += " ["; nm += inpsynthnm_; nm += "]";
	return;
    }
    else if ( !isRawOutput() )
	return;

    nm = wvltnm_;
    TypeSet<float> offset;
    raypars_.get( RayTracer1D::sKeyOffset(), offset );
    const int offsz = offset.size();
    if ( offsz )
    {
	nm += " ";
	nm += "Offset ";
	nm += ::toString( offset[0] );
	if ( offsz > 1 )
	{
	    nm += "-"; nm += offset[offsz-1];
	    bool nmocorrected = true;
	    if ( raypars_.getYN(Seis::SynthGenBase::sKeyNMO(),nmocorrected) &&
		 !nmocorrected )
		nm += " uncorrected";
	}
    }
}


void SynthGenParams::setWavelet( const char* wvltnm )
{
    if ( !isRawOutput() || wvltnm_ == wvltnm )
	return;

    wvltnm_.set( wvltnm );
    PtrMan<IOObj> wvltobj = Wavelet::getIOObj( wvltnm_.buf() );
    if ( !wvltobj )
	return;

    synthpars_.set( sKey::WaveletID(), wvltobj->key() );
}


void SynthGenParams::setWavelet( const Wavelet& wvlt )
{
    if ( !isRawOutput() || wvltnm_ == wvlt.name() )
	return;

    wvltnm_ = wvlt.name();
    PtrMan<IOObj> wvltobj = Wavelet::getIOObj( wvltnm_.buf() );
    if ( !wvltobj )
	return;

    synthpars_.set( sKey::WaveletID(), wvltobj->key() );
}


namespace Strat
{

static BufferStringSet fillSynthSeisKeys()
{
    BufferStringSet rmkeys;
    rmkeys.add( sKey::WaveletID() )
	  .add( Seis::SynthGenBase::sKeyFourier() )
	  .add( Seis::SynthGenBase::sKeyTimeRefs() )
	  .add( Seis::SynthGenBase::sKeyNMO() )
	  .add( Seis::SynthGenBase::sKeyMuteLength() )
	  .add( Seis::SynthGenBase::sKeyStretchLimit() )
	  .add( "Internal Multiples" )
	  .add( "Surface Reflection coef" );
    return rmkeys;
}

}

void SynthGenParams::cleanRayPar( const IOPar& iop, IOPar& raypar )
{
    raypar.merge( iop );
    const BufferStringSet rmkeys = Strat::fillSynthSeisKeys();
    for ( const auto* rmky : rmkeys )
	raypar.removeWithKey( rmky->buf() );
}


void SynthGenParams::setSynthGenPar( const IOPar& iop, IOPar& synthpar )
{
    const BufferStringSet rmkeys = Strat::fillSynthSeisKeys();
    for ( const auto* rmky : rmkeys )
    {
	BufferString valstr;
	if ( iop.get(rmky->buf(),valstr) && !valstr.isEmpty() )
	    synthpar.set( rmky->buf(), valstr );
    }
}



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



StratSynth::StratSynth( const Strat::LayerModelProvider& lmp, bool useed )
    : lmp_(lmp)
    , useed_(useed)
{
}


StratSynth::~StratSynth()
{
    clearSynthetics();
    setLevel( nullptr );
}


const Strat::LayerModel& StratSynth::layMod() const
{
    return lmp_.getEdited( useed_ );
}


void StratSynth::setWavelet( const Wavelet* wvlt )
{
    if ( !wvlt )
	return;

    delete wvlt_;
    wvlt_ = wvlt;
    genparams_.setWavelet( *wvlt_ );
}


void StratSynth::clearSynthetics( bool excludeprops )
{
    if ( excludeprops )
    {
	for ( int idx=synthetics_.size()-1; idx>=0; idx-- )
	{
	    if ( !synthetics_.get(idx)->isStratProp() )
		delete synthetics_.removeSingle( idx );
	}
    }
    else
	deepErase( synthetics_ );
}


#define mErrRet( msg, act )\
{\
    errmsg_ = toUiString("Can not generate synthetics %1 : %2\n") \
		    .arg( sgp.name_ ).arg( msg ); \
    act;\
}


const ReflectivityModelSet* StratSynth::getRefModels( const SynthGenParams& sgp)
{
    BufferStringSet synthnms;
    if ( sgp.isZeroOffset() )
	getSyntheticNames( synthnms, SynthGenParams::ZeroOffset );
    else if ( sgp.isPreStack() )
	getSyntheticNames( synthnms, SynthGenParams::PreStack );

    const IOPar& reflpars = sgp.raypars_;
    for ( const auto* synthnm : synthnms )
    {
	const SyntheticData* sd = getSynthetic( synthnm->buf() );
	if ( sd->getRefModels().hasSameParams(reflpars) )
	    return &sd->getRefModels();
    }

    return nullptr;
}


const Seis::SynthGenDataPack* StratSynth::getSynthGenRes(
						const SynthGenParams& sgp )
{
    if ( sgp.needsInput() )
    {
	const SyntheticData* sd = getSynthetic( sgp.inpsynthnm_ );
	return sd ? &sd->synthGenDP() : nullptr;
    }

    BufferStringSet synthnms;
    if ( sgp.isZeroOffset() || !sgp.isRawOutput() )
	getSyntheticNames( synthnms, SynthGenParams::ZeroOffset );

    if ( sgp.isPreStack() || !sgp.isRawOutput() )
    {
	BufferStringSet pssynthnms;
	getSyntheticNames( pssynthnms, SynthGenParams::PreStack );
	synthnms.append( pssynthnms );
    }

    for ( const auto* synthnm : synthnms )
    {
	const SyntheticData* sd = getSynthetic( synthnm->buf() );
	const Seis::SynthGenDataPack& syngendp = sd->synthGenDP();
	if ( syngendp.hasSameParams(sgp.raypars_,sgp.synthpars_) )
	    return &syngendp;
    }

    return nullptr;
}


const PreStack::GatherSetDataPack* StratSynth::getRelevantAngleData(
			    const Seis::SynthGenDataPack& synthgendp ) const
{
    for ( const auto* sd : synthetics_ )
    {
	if ( !sd->isPS() ||
	     !sd->synthGenDP().hasSameParams(synthgendp) )
	    continue;

	mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
	if ( presd )
	    return &presd->angleData();
    }

    return nullptr;
}


bool StratSynth::disableSynthetic( const char* nm )
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	SyntheticData* sd = synthetics_[idx];
	if ( sd->name() != nm )
	    continue;

	SynthGenParams sgp( sd->getGenParams() );
	if ( sgp.isPSBased() )
	{
	    sgp.inpsynthnm_.set( SynthGenParams::sKeyInvalidInputPS() );
	    sd->useGenParams( sgp );
	    return true;
	}
    }

    return false;
}


bool StratSynth::removeSynthetic( const char* nm )
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	if ( synthetics_[idx]->name() == nm )
	{
	    delete synthetics_.removeSingle( idx );
	    return true;
	}
    }

    return false;
}


SyntheticData* StratSynth::addSynthetic( const SynthGenParams& synthgen )
{
    SyntheticData* sd = generateSD( synthgen );
    if ( sd )
    {
	int propidx = 0;
	while ( propidx<synthetics_.size() )
	{
	    if ( synthetics_[propidx]->isStratProp() )
		break;
	    propidx++;
	}
	synthetics_.insertAt( sd, propidx );
    }

    return sd;
}



SyntheticData* StratSynth::replaceSynthetic( SynthID id )
{
    const SyntheticData* sd = getSynthetic( id );
    if ( !sd )
	return nullptr;

    const SynthGenParams synthgen( sd->getGenParams() );
    SyntheticData* newsd = generateSD( synthgen );
    if ( newsd )
    {
	const int sdidx = synthetics_.indexOf( sd );
	delete synthetics_.replace( sdidx, newsd );
    }

    return newsd;
}


SyntheticData* StratSynth::addDefaultSynthetic()
{
    genparams_ = SynthGenParams();
    SyntheticData* sd = addSynthetic( genparams_ );
    return sd;
}


int StratSynth::syntheticIdx( const char* nm ) const
{
    for ( int idx=0; idx<synthetics().size(); idx++ )
    {
	if( synthetics_[idx]->name() == nm )
	    return idx;
    }

    return -1;
}


SyntheticData* StratSynth::getSynthetic( const char* nm )
{
    for ( int idx=0; idx<synthetics().size(); idx ++ )
    {
	if ( synthetics_[idx]->name() == nm )
	    return synthetics_[idx];
    }
    return nullptr;
}


SyntheticData* StratSynth::getSynthetic( SynthID id )
{
    for ( auto* sd : synthetics() )
    {
	if ( sd->id_ == id )
	    return sd;
    }

    return nullptr;
}


SyntheticData* StratSynth::getSyntheticByIdx( int idx )
{
    return synthetics_.validIdx( idx ) ?  synthetics_[idx] : nullptr;
}


const SyntheticData* StratSynth::getSyntheticByIdx( int idx ) const
{
    return synthetics_.validIdx( idx ) ?  synthetics_[idx] : nullptr;
}


int StratSynth::syntheticIdx( const PropertyRef& pr ) const
{
    int idx = 0;
    for ( const auto* sd : synthetics_ )
    {
	idx++;
	if ( !sd->isStratProp() )
	    continue;

	mDynamicCastGet(const StratPropSyntheticData*,pssd,sd);
	if ( !pssd )
	    continue;

	if ( pr == pssd->propRef() )
	    return idx;
    }

    return -1;
}


void StratSynth::getSyntheticNames( BufferStringSet& nms ) const
{
    for ( const auto* synthetic : synthetics_ )
	nms.add( synthetic->name() );
}


void StratSynth::getSyntheticNames( BufferStringSet& nms, bool wantprest ) const
{
    nms.erase();
    for ( const auto* synthetic : synthetics_ )
    {
	if ( synthetic->isPS() == wantprest )
	    nms.add( synthetic->name() );
    }
}


void StratSynth::getSyntheticNames( BufferStringSet& nms,
				    SynthGenParams::SynthType synthtype ) const
{
    nms.erase();
    for ( const auto* synthetic : synthetics_ )
    {
	if ( synthetic->synthType() == synthtype )
	    nms.add( synthetic->name() );
    }
}


SyntheticData* StratSynth::getSynthetic( const	PropertyRef& pr )
{
    for ( auto* sd : synthetics_ )
    {
	if ( !sd->isStratProp() )
	    continue;

	mDynamicCastGet(StratPropSyntheticData*,pssd,sd);
	if ( !pssd ) continue;
	if ( &pr == &pssd->propRef() )
	    return sd;
    }
    return nullptr;
}


int StratSynth::nrSynthetics() const
{
    return synthetics_.size();
}


#define mSetBool( str, newval ) \
{ \
    mDynamicCastGet(Attrib::BoolParam*,param,psdesc->getValParam(str)) \
    param->setValue( newval ); \
}


#define mSetEnum( str, newval ) \
{ \
    mDynamicCastGet(Attrib::EnumParam*,param,psdesc->getValParam(str)) \
    param->setValue( newval ); \
}

#define mSetFloat( str, newval ) \
{ \
    Attrib::ValParam* param  = psdesc->getValParam( str ); \
    param->setValue( newval ); \
}


#define mSetString( str, newval ) \
{ \
    Attrib::ValParam* param = psdesc->getValParam( str ); \
    param->setValue( newval ); \
}


#define mCreateDesc() \
if ( !sd.isPS() ) return 0; \
mDynamicCastGet(const PreStackSyntheticData&,presd,sd); \
BufferString dpidstring( "#" ); \
SeparString fullidstr( toString(DataPackMgr::SeisID()), '.' ); \
const PreStack::GatherSetDataPack& gdp = presd.preStackPack(); \
fullidstr.add( toString(gdp.id()) ); \
dpidstring.add( fullidstr.buf() ); \
Attrib::Desc* psdesc = \
    Attrib::PF().createDescCopy(Attrib::PSAttrib::attribName()); \
mSetString(Attrib::StorageProvider::keyStr(),dpidstring.buf());


#define mSetProc() \
mSetBool(Attrib::PSAttrib::useangleStr(), true ); \
mSetFloat(Attrib::PSAttrib::angleStartStr(), sgp.anglerg_.start ); \
mSetFloat(Attrib::PSAttrib::angleStopStr(), sgp.anglerg_.stop ); \
mSetFloat(Attrib::PSAttrib::angleDPIDStr(),\
	&presd.angleData() ? presd.angleData().id() : -1 ); \
psdesc->setUserRef( sgp.name_ ); \
psdesc->updateParams(); \
PtrMan<Attrib::DescSet> descset = new Attrib::DescSet( false ); \
if ( !descset ) return nullptr; \
Attrib::DescID attribid = descset->addDesc( psdesc ); \
PtrMan<Attrib::EngineMan> aem = new Attrib::EngineMan; \
TypeSet<Attrib::SelSpec> attribspecs; \
Attrib::SelSpec sp( 0, attribid ); \
sp.set( *psdesc ); \
attribspecs += sp; \
aem->setAttribSet( descset ); \
aem->setAttribSpecs( attribspecs ); \
aem->setTrcKeyZSampling( tkzs ); \
aem->setGeomID( tkzs.hsamp_.getGeomID() ); \
BinIDValueSet bidvals( 0, false ); \
bidvals.setIs2D( true ); \
const ObjectSet<PreStack::Gather>& gathers = gdp.getGathers(); \
for ( int idx=0; idx<gathers.size(); idx++ ) \
    bidvals.add( gathers[idx]->getBinID() ); \
auto* dptrcbufs = new SeisTrcBuf( true ); \
Interval<float> zrg( tkzs.zsamp_ ); \
uiString errmsg; \
PtrMan<Attrib::Processor> proc = \
    aem->createTrcSelOutput( errmsg, bidvals, *dptrcbufs, 0, &zrg); \
if ( !proc || !proc->getProvider() ) \
    mErrRet( errmsg, return 0 ) ; \
proc->getProvider()->setDesiredVolume( tkzs ); \
proc->getProvider()->setPossibleVolume( tkzs ); \
mDynamicCastGet(Attrib::PSAttrib*,psattr,proc->getProvider()); \
if ( !psattr ) \
    mErrRet( proc->uiMessage(), return nullptr ) ;


#define mCreateSeisBuf( dpname ) \
if ( !TaskRunner::execute(taskr_,*proc) ) \
    mErrRet( proc->uiMessage(), return nullptr ) ; \
SeisTrcBufDataPack* dpname = \
    new SeisTrcBufDataPack( dptrcbufs, Seis::Line, \
			    SeisTrcInfo::TrcNr, \
			    PostStackSyntheticData::sDataPackCategory() ); \


mClass(WellAttrib) AttributeSyntheticCreator : public ParallelTask
{ mODTextTranslationClass(AttributeSyntheticCreator);

public:
AttributeSyntheticCreator( const PostStackSyntheticData& sd,
			   const BufferStringSet& attribs,
			   ObjectSet<SeisTrcBuf>& seisbufs )
    : ParallelTask( "Creating Attribute Synthetics" )
    , seistrcbufs_(seisbufs)
    , sd_(sd)
    , attribs_(attribs)
    , descset_(nullptr)
{
}


uiString uiMessage() const override
{
    return msg_;
}


uiString uiNrDoneText() const override
{
    return tr("Attributes done");
}


private:

void createInstAttributeSet()
{
    descset_ = new Attrib::DescSet( false );
    BufferString dpidstr( "#", sd_.datapackid_.buf() );
    Attrib::DescID did = descset_->getStoredID( dpidstr.buf(), 0, true );

    Attrib::Desc* imagdesc = Attrib::PF().createDescCopy(
						Attrib::Hilbert::attribName() );
    imagdesc->selectOutput( 0 );
    imagdesc->setInput(0, descset_->getDesc(did) );
    imagdesc->setHidden( true );
    BufferString usrref( dpidstr.buf(), "_imag" );
    imagdesc->setUserRef( usrref );
    descset_->addDesc( imagdesc );

    Attrib::Desc* psdesc = Attrib::PF().createDescCopy(
					 Attrib::Instantaneous::attribName());
    psdesc->setInput( 0, descset_->getDesc(did) );
    psdesc->setInput( 1, imagdesc );
    psdesc->setUserRef( "synthetic attributes" );
    descset_->addDesc( psdesc );
    descset_->updateInputs();
}


od_int64 nrIterations() const override
{ return sd_.postStackPack().trcBuf().size(); }


bool doPrepare( int /* nrthreads */ ) override
{
    msg_ = tr("Preparing Attributes");

    createInstAttributeSet();
    sd_.postStackPack().getTrcKeyZSampling( tkzs_ );

    seistrcbufs_.setEmpty();
    comps_.setEmpty();
    const SeisTrcBuf& trcs = sd_.postStackPack().trcBuf();
    int order = 1;
    for ( const auto* attrib : attribs_ )
    {
	auto* stb = new SeisTrcBuf( true );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    auto* trc = new SeisTrc;
	    trc->info() = trcs.get( idx )->info();
	    stb->add( trc );
	}

	seistrcbufs_ += stb;
	if ( Attrib::Instantaneous::parseEnumOutType(attrib->buf())==
					    Attrib::Instantaneous::Amplitude )
	    comps_ += 0;
	else
	{
	    comps_ += order;
	    order++;
	}
    }

    resetNrDone();
    msg_ = tr("Calculating");
    return true;
}


bool doWork( od_int64 start, od_int64 stop, int /* threadid */ )
{
    const Attrib::Desc& psdesc = *descset_->desc( descset_->size()-1 );
    PtrMan<Attrib::EngineMan> aem = new Attrib::EngineMan;
    TypeSet<Attrib::SelSpec> attribspecs;
    Attrib::DescID attribid = descset_->getID( descset_->size()-1 );
    Attrib::SelSpec sp( nullptr, attribid );
    sp.set( psdesc );
    attribspecs += sp;
    aem->setAttribSet( descset_ );
    aem->setAttribSpecs( attribspecs );
    aem->setTrcKeyZSampling( tkzs_ );
    aem->setGeomID( tkzs_.hsamp_.getGeomID() );
    BinIDValueSet bidvals( 0, false );
    bidvals.setIs2D( true );
    const SeisTrcBuf& trcs = sd_.postStackPack().trcBuf();
    for ( int idx=start; idx<=stop; idx++ )
	bidvals.add( trcs.get(idx)->info().binID() );

    PtrMan<SeisTrcBuf> dptrcbufs = new SeisTrcBuf( true );
    const Interval<float> zrg( tkzs_.zsamp_ );
    PtrMan<Attrib::Processor> proc = aem->createTrcSelOutput( msg_, bidvals,
							  *dptrcbufs, 0, &zrg);
    if ( !proc || !proc->getProvider() )
	return false;

    Attrib::Provider* prov = proc->getProvider();
    for ( const auto* attrib : attribs_ )
    {
	int icomp = Attrib::Instantaneous::parseEnumOutType( attrib->buf() );
	proc->addOutputInterest( icomp );
    }
    prov->setDesiredVolume( tkzs_ );
    prov->setPossibleVolume( tkzs_ );
    prov->doParallel( false );

    if ( !proc->execute() )
	return false;

    for ( int trcidx=0; trcidx<dptrcbufs->size(); trcidx++ )
    {
	const SeisTrc& intrc = *dptrcbufs->get( trcidx );
	const int sz = intrc.size();
	for ( int idx=0; idx<seistrcbufs_.size(); idx++ )
	{
	    SeisTrcBuf* stb = seistrcbufs_[idx];
	    SeisTrc*	outtrc = stb->get( trcidx+start );
	    outtrc->reSize( sz, false );
	    for ( int is=0; is<sz; is++ )
		outtrc->set( is, intrc.get(is, comps_[idx]), 0 );
	}
    }
    addToNrDone( stop-start+1 );
    return true;
}


bool doFinish( bool success )
{
    return true;
}

    const PostStackSyntheticData&	sd_;
    const BufferStringSet&		attribs_;
    ObjectSet<SeisTrcBuf>&		seistrcbufs_;
    TypeSet<int>			comps_;
    PtrMan<Attrib::DescSet>		descset_;
    TrcKeyZSampling			tkzs_;
    TaskRunner*				taskr_;
    uiString				msg_;
};


SyntheticData* StratSynth::createAttribute( const SyntheticData& sd,
					    const SynthGenParams& synthgenpar )
{
    ObjectSet<SeisTrcBuf> seistrcbufs;
    BufferStringSet attribs( Attrib::Instantaneous::toString(
						    synthgenpar.attribtype_) );
    mDynamicCastGet(const PostStackSyntheticData&,pssd,sd);

    AttributeSyntheticCreator asc( pssd, attribs, seistrcbufs );
    if ( !TaskRunner::execute(taskr_,asc) )
	return nullptr;

    SeisTrcBufDataPack* dpname = new SeisTrcBufDataPack( seistrcbufs[0],
				 Seis::Line, SeisTrcInfo::TrcNr,
				 PostStackSyntheticData::sDataPackCategory() );

    return new InstAttributeSyntheticData( synthgenpar, sd.synthGenDP(),
					   *dpname );
}


bool StratSynth::addInstAttribSynthetics( const BufferStringSet& attribs,
					  const SynthGenParams& sgp )
{
    if ( !sgp.isAttribute() )
	mErrRet( tr(" not an attribute."), return false )

    ObjectSet<SeisTrcBuf> seistrcbufs;
    BufferString inputsdnm( sgp.inpsynthnm_ );
    const SyntheticData* insd = getSynthetic( inputsdnm );
    if ( !insd )
	mErrRet( tr(" input synthetic data not found."), return false )

    mDynamicCastGet(const PostStackSyntheticData*,pssd,insd);
    if ( !pssd )
	mErrRet( tr(" input synthetic data not found."), return false )

    AttributeSyntheticCreator asc( *pssd, attribs, seistrcbufs );
    if ( !TaskRunner::execute(taskr_,asc) )
	return false;

    SynthGenParams pars( sgp );
    int propidx = 0;
    while ( propidx<synthetics_.size() )
    {
	if ( synthetics_[propidx]->isStratProp() )
	    break;
	propidx++;
    }

    for ( int idx=0; idx<seistrcbufs.size(); idx++ )
    {
	auto* dpname = new SeisTrcBufDataPack( seistrcbufs[idx],
				 Seis::Line, SeisTrcInfo::TrcNr,
				 PostStackSyntheticData::sDataPackCategory() );
	Attrib::Instantaneous::parseEnum(attribs[idx]->buf(), pars.attribtype_);
	pars.createName( pars.name_ );

	auto* sd = new InstAttributeSyntheticData( pars, insd->synthGenDP(),
						   *dpname );
	if ( sd )
	    synthetics_.insertAt( sd, propidx++ );
	else
	    mErrRet( tr(" synthetic data not created."), return false )
    }

    return true;
}


SyntheticData* StratSynth::createAVOGradient( const SyntheticData& sd,
					      const TrcKeyZSampling& tkzs,
					      const SynthGenParams& sgp )
{
    mCreateDesc()
    mSetEnum(Attrib::PSAttrib::calctypeStr(),PreStack::PropCalc::LLSQ);
    mSetEnum(Attrib::PSAttrib::offsaxisStr(),PreStack::PropCalc::Sinsq);
    mSetEnum(Attrib::PSAttrib::lsqtypeStr(), PreStack::PropCalc::Coeff );

    mSetProc();
    mCreateSeisBuf( angledp );
    return new AVOGradSyntheticData( sgp, sd.synthGenDP(), *angledp );
}


SyntheticData* StratSynth::createAngleStack( const SyntheticData& sd,
					     const TrcKeyZSampling& tkzs,
					     const SynthGenParams& sgp )
{
    mCreateDesc();
    mSetEnum(Attrib::PSAttrib::calctypeStr(),PreStack::PropCalc::Stats);
    mSetEnum(Attrib::PSAttrib::stattypeStr(), Stats::Average );

    mSetProc();
    mCreateSeisBuf( angledp );
    return new AngleStackSyntheticData( sgp, sd.synthGenDP(), *angledp);
}


class ElasticModelCreator : public ParallelTask
{ mODTextTranslationClass(ElasticModelCreator);
public:
ElasticModelCreator( const Strat::LayerModel& lm, TypeSet<ElasticModel>& ems )
    : ParallelTask( "Elastic Model Generator" )
    , lm_(lm)
    , aimodels_(ems)
    , nrmodels_(lm.size())
{
    aimodels_.setSize( lm_.size(), ElasticModel() );
    msg_ = tr( "Generating elastic model" );
}


uiString uiMessage() const override	{ return msg_; }
uiString uiNrDoneText() const override	{ return tr("Models done"); }

private:

static float cMaximumVpWaterVel()
{ return 1510.f; }

od_int64 nrIterations() const override	{ return nrmodels_; }

bool doPrepare( int nrthreads ) override
{
    const ElasticPropSelection& eps = lm_.elasticPropSel();
    uiString errmsg;
    if ( !eps.isValidInput(&errmsg) )
    {
	msg_ = tr( "Cannot create elastic model as %1" ).arg( errmsg );
	return false;
    }

    deepErase( elpgens_ );
    const PropertyRefSelection& props = lm_.propertyRefs();
    for ( int idx=0; idx<nrthreads; idx++ )
    {
	auto* elpgen = new ElasticPropGen( eps, props );
	if ( !elpgen || !elpgen->isOK() )
	{
	    delete elpgen;
	    msg_ = tr( "Cannot determine elastic property definitions" );
	    break;
	}

	elpgens_.add( elpgen );
    }

    return elpgens_.size() == nrthreads;

}

bool doWork( od_int64 start, od_int64 stop, int threadid ) override
{
    if ( !elpgens_.validIdx(threadid) )
	return false;

    ElasticPropGen& elpgen = *elpgens_.get( threadid );
    for ( int idm=int(start); idm<=stop; idm++, addToNrDone(1) )
    {
	const Strat::LayerSequence& seq = lm_.sequence( idm );
	if ( seq.isEmpty() )
	    continue;

	ElasticModel& curem = aimodels_[idm];
	if ( !fillElasticModel(seq,elpgen,curem) )
	    return false;
    }

    return true;
}

bool fillElasticModel( const Strat::LayerSequence& seq, ElasticPropGen& elpgen,
		       ElasticModel& aimodel )
{
    aimodel.setEmpty();

    const float srddepth = -1.f*mCast(float,SI().seismicReferenceDatum() );
    int firstidx = 0;
    if ( seq.startDepth() < srddepth )
	firstidx = seq.nearestLayerIdxAtZ( srddepth );

    if ( seq.isEmpty() )
    {
	mutex_.lock();
	msg_ = tr("Elastic model is not proper to generate synthetics as a "
		  "layer sequence has no layers");
	mutex_.unLock();
	return false;
    }

    const ObjectSet<Strat::Layer>& layers = seq.layers();
    for ( int idx=firstidx; idx<layers.size(); idx++ )
    {
	const Strat::Layer& lay = *layers.get( idx );
	float thickness = lay.thickness();
	if ( idx == firstidx )
	    thickness -= srddepth - lay.zTop();
	if ( thickness < 1e-4f )
	    continue;

	float dval = mUdf(float), pval = mUdf(float), sval = mUdf(float);
	TypeSet<float> layervals; lay.getValues( layervals );
	elpgen.getVals( dval, pval, sval, layervals.arr(), layervals.size() );

	// Detect water - reset Vs
	/* TODO disabled for now
	if ( pval < cMaximumVpWaterVel() )
	    sval = 0;*/

	aimodel += ElasticLayer( thickness, pval, sval, dval );
    }

    if ( aimodel.isEmpty() )
    {
	mutex_.lock();
	msg_ = tr("After discarding layers with no thickness "
		     "no layers remained");
	mutex_.unLock();
	return false;
    }

    return true;
}

bool doFinish( bool /* success */ ) override
{
    deepErase( elpgens_ );
    return true;
}

const Strat::LayerModel&	lm_;
const od_int64			nrmodels_;
TypeSet<ElasticModel>&		aimodels_;
Threads::Mutex			mutex_;
uiString			msg_;

ObjectSet<ElasticPropGen>	elpgens_;

};


bool StratSynth::createElasticModels()
{
    errmsg_.setEmpty();
    clearElasticModels();

    if ( layMod().isEmpty() )
    {
	errmsg_ = tr("Empty layer model");
	return false;
    }

    ElasticModelCreator emcr( layMod(), aimodels_ );
    if ( !TaskRunner::execute(taskr_,emcr) )
    {
	errmsg_ = emcr.uiMessage();
	return false;
    }

    bool modelsvalid = false;
    for ( int idx=0; idx<aimodels_.size(); idx++ )
    {
	if ( !aimodels_[idx].isEmpty() )
	{
	    modelsvalid = true;
	    break;
	}
    }

    if ( !modelsvalid )
    {
	errmsg_ = tr("Some pseudowells have no layers");
	return false;
    }

    return adjustElasticModel( layMod(), aimodels_, useed_ );
}


class PSAngleDataCreator : public Executor
{ mODTextTranslationClass(PSAngleDataCreator)
public:

PSAngleDataCreator( PreStackSyntheticData& pssd )
    : Executor("Creating Angle Gather" )
    , pssd_(pssd)
    , seisgathers_(const_cast<const ObjectSet<PreStack::Gather>&>(
			pssd.preStackPack().getGathers()))
    , refmodels_(const_cast<const ReflectivityModelSet&>(
			pssd.synthGenDP().getModels()))
{
    totalnr_ = refmodels_.nrModels();
    anglecomputer_.setRayTracerPars( pssd.getGenParams().raypars_ );
    anglecomputer_.setFFTSmoother( 10.f, 15.f );
}

od_int64 totalNr() const override
{ return totalnr_; }

od_int64 nrDone() const override
{ return nrdone_; }

uiString uiMessage() const override
{
    return tr("Calculating Angle Gathers");
}

uiString uiNrDoneText() const override
{
    return tr( "Models done" );
}

private:

void convertAngleDataToDegrees( PreStack::Gather& ag ) const
{
    const auto& agdata = const_cast<const Array2D<float>&>( ag.data() );
    ArrayMath::getScaledArray<float>( agdata, nullptr, mRad2DegD, 0.,
				      false, true );
}


bool goImpl( od_ostream* strm, bool first, bool last, int delay ) override
{
    const bool res = Executor::goImpl( strm, first, last, delay );
    if ( res )
	pssd_.setAngleData( anglegathers_ );

    return res;
}


int nextStep() override
{
    if ( !seisgathers_.validIdx(nrdone_) )
	return Finished();

    const ReflectivityModelBase* refmodel = refmodels_.get( nrdone_ );
    if ( !refmodel->isOffsetDomain() )
	return ErrorOccurred();

    mDynamicCastGet(const OffsetReflectivityModel*,offrefmodel,refmodel);

    const PreStack::Gather& seisgather = *seisgathers_[(int)nrdone_];
    anglecomputer_.setOutputSampling( seisgather.posData() );
    anglecomputer_.setGatherIsNMOCorrected( seisgather.isCorrected() );
    anglecomputer_.setRefModel( *offrefmodel, seisgather.getTrcKey() );
    PreStack::Gather* anglegather = anglecomputer_.computeAngles();
    if ( !anglegather )
	return ErrorOccurred();

    convertAngleDataToDegrees( *anglegather );
    TypeSet<float> azimuths;
    seisgather.getAzimuths( azimuths );
    anglegather->setAzimuths( azimuths );
    const BufferString angledpnm( pssd_.name(), "(Angle Gather)" );
    anglegather->setName( angledpnm );
    anglegathers_.add( anglegather );
    nrdone_++;
    return MoreToDo();
}

    PreStackSyntheticData&		pssd_;
    const ObjectSet<PreStack::Gather>&	seisgathers_;
    const ReflectivityModelSet&		refmodels_;
    ObjectSet<PreStack::Gather>		anglegathers_;
    PreStack::ModelBasedAngleComputer	anglecomputer_;
    od_int64				nrdone_ = 0;
    od_int64				totalnr_;

};


void StratSynth::createAngleData( PreStackSyntheticData& pssd )
{
    PSAngleDataCreator angledatacr( pssd );
    TaskRunner::execute( taskr_, angledatacr );
}



bool StratSynth::runSynthGen( Seis::RaySynthGenerator& synthgen,
			      const SynthGenParams& sgp )
{
    BufferString capt( "Generating ", sgp.name_ );
    synthgen.setName( capt.buf() );

    const BufferString wvltnm( sgp.getWaveletNm() );
    bool needsetwvlt = wvltnm.isEmpty();
    if ( !needsetwvlt )
    {
	PtrMan<IOObj> ioobj = Wavelet::getIOObj( wvltnm );
	PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
	if ( !wvlt || (wvlt_ && wvlt_->name()==wvlt->name()) )
	    needsetwvlt = true;
	else
	    synthgen.setWavelet( wvlt, OD::CopyPtr );
    }

    if ( wvlt_ && needsetwvlt )
	synthgen.setWavelet( wvlt_, OD::UsePtr );

    IOPar synthpars = sgp.synthpars_;
    if ( !synthpars.isPresent(Seis::SynthGenBase::sKeyFourier()) )
	synthpars.setYN( Seis::SynthGenBase::sKeyFourier(),
			 !GetEnvVarYN("DTECT_CONVOLVE_USETIME") );
    if ( !synthpars.isEmpty() )
	synthgen.usePar( synthpars );

    return TaskRunner::execute( taskr_, synthgen );
}


SyntheticData* StratSynth::generateSD( const SynthGenParams& sgp )
{
    errmsg_.setEmpty();

    if ( layMod().isEmpty() )
    {
	errmsg_ = tr("Empty layer model.");
	return nullptr;
    }

    const bool ispsbased = sgp.isPSBased();
    if ( sgp.isPreStack() && !swaveinfomsgshown_ )
    {
	if ( !adjustElasticModel(layMod(),aimodels_,true) )
	    return nullptr;
    }

    SyntheticData* sd = nullptr;
    if ( sgp.isRawOutput() )
    {
	PtrMan<Seis::RaySynthGenerator> synthgen;
	ConstRefMan<Seis::SynthGenDataPack> syngendp = getSynthGenRes( sgp );
	if ( syngendp )
	    synthgen = new Seis::RaySynthGenerator( *syngendp.ptr() );
	else
	{
	    uiString msg;
	    ConstRefMan<ReflectivityModelSet> refmodels = getRefModels( sgp );
	    if ( !refmodels )
		refmodels = Seis::RaySynthGenerator::getRefModels( aimodels_,
						sgp.raypars_, msg, taskr_ );
	    if ( !refmodels )
		return nullptr;

	    synthgen = new Seis::RaySynthGenerator( *refmodels.ptr() );
	}

	if ( !runSynthGen(*synthgen.ptr(),sgp) )
	    return nullptr;

	sd = SyntheticData::get( sgp, *synthgen.ptr() );
	if ( sd && sd->isPS() )
	{
	    mDynamicCastGet(PreStackSyntheticData*,presd,sd);
	    const PreStack::GatherSetDataPack* anglegather =
				getRelevantAngleData( sd->synthGenDP() );
	    if ( anglegather )
		presd->setAngleData( anglegather->getGathers() );
	    else
		createAngleData( *presd );
	}
    }
    else if ( ispsbased )
    {
	BufferString inputsdnm( sgp.inpsynthnm_ );
	if ( useed_ )
	    inputsdnm += sKeyFRNameSuffix();

	const SyntheticData* presd = getSynthetic( inputsdnm );
	if ( !presd || !presd->isPS() )
	    mErrRet( tr(" input prestack synthetic data not found."),
		     return nullptr )

	mDynamicCastGet(const PreStack::GatherSetDataPack*,presgdp,
			&presd->getPack())
	if ( !presgdp )
	    mErrRet( tr(" input prestack synthetic data not found."),
		     return nullptr )
	TrcKeyZSampling tkzs( false );
	tkzs.zsamp_ = presgdp->zRange();

	if ( sgp.synthtype_ == SynthGenParams::AngleStack )
	    sd = createAngleStack( *presd, tkzs, sgp );
	else if ( sgp.synthtype_ == SynthGenParams::AVOGradient )
	    sd = createAVOGradient( *presd, tkzs, sgp );
    }
    else if ( sgp.isAttribute() )
    {
	const BufferString inputsdnm( sgp.inpsynthnm_ );
	const SyntheticData* inpsd = getSynthetic( inputsdnm );
	if ( !inpsd )
	    mErrRet( tr(" input synthetic data not found."), return nullptr )

	sd = createAttribute( *inpsd, sgp );
    }

    if ( !sd )
	return nullptr;

    if ( useed_ )
    {
	BufferString sdnm = sd->name();
	sdnm += sKeyFRNameSuffix();
	sd->setName( sdnm );
    }

    sd->id_ = ++lastsyntheticid_;
    return sd;
}


void StratSynth::generateOtherQuantities( double zstep,
					  const BufferStringSet* proplistfilter)
{
    for ( const auto* sd : synthetics_ )
    {
	if ( sd->isPS() || sd->isStratProp() )
	    continue;

	mDynamicCastGet(const PostStackSyntheticData*,pssd,sd);
	if ( !pssd )
	    continue;

	return generateOtherQuantities( *pssd, layMod(), zstep, proplistfilter);
    }
}


mClass(WellAttrib) StratPropSyntheticDataCreator : public ParallelTask
{ mODTextTranslationClass(StratPropSyntheticDataCreator);

public:
StratPropSyntheticDataCreator( ObjectSet<SyntheticData>& synths,
		    const PostStackSyntheticData& sd,
		    const Strat::LayerModel& lm,
		    int& lastsynthid, bool useed, double zstep,
		    const BufferStringSet* proplistfilter )
    : ParallelTask( "Creating Synthetics for Properties" )
    , synthetics_(synths)
    , sd_(sd)
    , lm_(lm)
    , lastsyntheticid_(lastsynthid)
    , useed_(useed)
    , zrg_(sd.postStackPack().posData().range(false))
{
    zrg_.step = zstep;
    prs_.setEmpty();
    for ( const auto* pr : lm_.propertyRefs() )
    {
	if ( pr->isThickness() ||
	     (proplistfilter && !proplistfilter->isPresent(pr->name())) )
	    continue;

	prs_.add( pr );
    }
}


uiString uiMessage() const override
{
    return msg_;
}


uiString uiNrDoneText() const override
{
    return tr("Models done");
}


private:

od_int64 nrIterations() const override
{ return lm_.size(); }


bool doPrepare( int /* nrthreads */ ) override
{
    msg_ = tr("Preparing Models");

    layermodels_.setEmpty();
    const int sz = zrg_.nrSteps() + 1;
    for ( int idz=0; idz<sz; idz++ )
	layermodels_ += new Strat::LayerModel();

    const int nrprops = prs_.size();
    ObjectSet<SeisTrcBuf> trcbufs;
    for ( int iprop=0; iprop<nrprops; iprop++ )
	trcbufs.add( new SeisTrcBuf( true ) );

    const SeisTrcBuf& sdbuf = sd_.postStackPack().trcBuf();
    for ( int itrc=0; itrc<sdbuf.size(); itrc++ )
    {
	const SeisTrc& inptrc = *sdbuf.get( itrc );
	SeisTrc trc( sz );
	trc.info() = inptrc.info();
	trc.info().sampling.step = zrg_.step;
	trc.zero();
	for ( int iprop=0; iprop<nrprops; iprop++ )
	    trcbufs[iprop]->add( new SeisTrc(trc) );
    }

    for ( int iprop=0; iprop<nrprops; iprop++ )
    {
	SeisTrcBuf* trcbuf = trcbufs[iprop];
	auto* seisbuf =
	    new SeisTrcBufDataPack( trcbuf, Seis::Line, SeisTrcInfo::TrcNr,
				  PostStackSyntheticData::sDataPackCategory() );
	seisbufdps_.add( seisbuf );
    }

    for ( int iseq=0; iseq<lm_.size(); iseq++, addToNrDone(1) )
    {
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	const TimeDepthModel* t2d = sd_.getTDModel( iseq );
	if ( !t2d )
	    return false;

	const Interval<float> seqdepthrg = seq.zRange();
	const float seqstarttime = t2d->getTime( seqdepthrg.start );
	const float seqstoptime = t2d->getTime( seqdepthrg.stop );
	Interval<float> seqtimerg( seqstarttime, seqstoptime );
	for ( int idz=0; idz<sz; idz++ )
	{
	    Strat::LayerModel* lmsamp = layermodels_[idz];
	    if ( !lmsamp )
		continue;

	    lmsamp->addSequence();
	    Strat::LayerSequence& curseq = lmsamp->sequence( iseq );
	    const float time = mCast( float, zrg_.atIndex(idz) );
	    if ( !seqtimerg.includes(time,false) )
		continue;

	    const float dptstart = t2d->getDepth( time - (float)zrg_.step );
	    const float dptstop = t2d->getDepth( time + (float)zrg_.step );
	    Interval<float> depthrg( dptstart, dptstop );
	    seq.getSequencePart( depthrg, true, curseq );
	}
    }

    resetNrDone();
    msg_ = tr("Calculating");
    return true;
}


bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int sz = layermodels_.size();
    const PropertyRefSelection& props = lm_.propertyRefs();
    for ( int iseq=mCast(int,start); iseq<=mCast(int,stop); iseq++ )
    {
	addToNrDone( 1 );
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	Interval<float> seqtimerg(  sd_.getTime(seq.zRange().start,iseq),
				    sd_.getTime(seq.zRange().stop,iseq) );
	if ( seqtimerg.isUdf() )
	    return false;

	for ( const auto* pr : prs_ )
	{
	    const int iprop = props.indexOf( pr );
	    const bool propisvel = pr->hasType( Mnemonic::Vel );
	    SeisTrcBufDataPack* dp = seisbufdps_[prs_.indexOf(pr)];
	    SeisTrcBuf& trcbuf = dp->trcBuf();
	    const int bufsz = trcbuf.size();
	    SeisTrc* rawtrc = iseq < bufsz ? trcbuf.get( iseq ) : nullptr;
	    if ( !rawtrc )
		continue;

	    PointBasedMathFunction propvals( PointBasedMathFunction::Linear,
					     PointBasedMathFunction::EndVal );

	    for ( int idz=0; idz<sz; idz++ )
	    {
		const float time = mCast( float, zrg_.atIndex(idz) );
		if ( !seqtimerg.includes(time,false) )
		    continue;

		if ( !layermodels_.validIdx(idz) )
		    continue;

		const Strat::LayerSequence& curseq =
		    layermodels_[idz]->sequence(iseq);
		if ( curseq.isEmpty() )
		    continue;

		Stats::CalcSetup laypropcalc( true );
		laypropcalc.require( Stats::Average );
		Stats::RunCalc<double> propval( laypropcalc );
		for ( int ilay=0; ilay<curseq.size(); ilay++ )
		{
		    const Strat::Layer* lay = curseq.layers()[ilay];
		    if ( !lay )
			continue;

		    const float val = lay->value(iprop);
		    if ( mIsUdf(val) || ( propisvel && val < 1e-5f ) )
			continue;

		    propval.addValue( propisvel ? 1.f / val : val,
				      lay->thickness() );
		}
		const float val = mCast( float, propval.average() );
		if ( mIsUdf(val) || ( propisvel && val < 1e-5f ) )
		    continue;

		propvals.add( time, propisvel ? 1.f / val : val );
	    }

	    Array1DImpl<float> proptr( sz );
	    for ( int idz=0; idz<sz; idz++ )
	    {
		const float time = mCast( float, zrg_.atIndex(idz) );
		proptr.set( idz, propvals.getValue( time ) );
	    }

	    const float step = mCast( float, zrg_.step );
	    ::FFTFilter filter( sz, step );
	    const float f4 = 1.f / (2.f * step );
	    filter.setLowPass( f4 );
	    if ( !filter.apply(proptr) )
		continue;

	    for ( int idz=0; idz<sz; idz++ )
		rawtrc->set( idz, proptr.get( idz ), 0 );
	}

	for ( int idz=0; idz<sz; idz++ )
	{
	    if ( !layermodels_.validIdx(idz) || !layermodels_[idz] )
		continue;

	    layermodels_[idz]->sequence(iseq).setEmpty();
	}
    }

    return true;
}


bool doFinish( bool success )
{
    SynthGenParams sgp( SynthGenParams::StratProp );
    for ( int idx=0; idx<seisbufdps_.size(); idx++ )
    {
	const PropertyRef* pr = prs_[idx];
	SeisTrcBufDataPack* dp = seisbufdps_[idx];
	BufferString propnm = pr->name();
	if ( useed_ )
	    propnm += StratSynth::sKeyFRNameSuffix();
	BufferString nm( "[", propnm, "]" );
	sgp.name_.set( nm );
	dp->setName( nm );
	auto* prsd = new StratPropSyntheticData( sgp, sd_.synthGenDP(),
						 *dp, *pr );
	prsd->id_ = ++lastsyntheticid_;
	synthetics_ += prsd;
    }

    return true;
}

    const PostStackSyntheticData&	sd_;
    const Strat::LayerModel&		lm_;
    PropertyRefSelection		prs_;
    StepInterval<double>		zrg_;
    ManagedObjectSet<Strat::LayerModel> layermodels_;
    ObjectSet<SyntheticData>&		synthetics_;
    ObjectSet<SeisTrcBufDataPack>	seisbufdps_;
    int&				lastsyntheticid_;
    bool				useed_;
    uiString				msg_;

};


void StratSynth::generateOtherQuantities( const PostStackSyntheticData& sd,
				      const Strat::LayerModel& lm,
				      double zstep,
				      const BufferStringSet* proplist )
{
    const BufferString propliststr(
				GetEnvVar("DTECT_SYNTHROCK_TIMEPROPS") );
    if ( !propliststr.isEmpty() && propliststr == sKey::None() )
	return;

    BufferStringSet proplistfilter;
    if ( !propliststr.isEmpty() )
	proplistfilter.unCat( propliststr.buf(), "|" );
    else if ( proplist )
	proplistfilter = *proplist;

    const bool filterprops = !proplistfilter.isEmpty();
    StratPropSyntheticDataCreator propcreator( synthetics_, sd, lm,
			      lastsyntheticid_, useed_, zstep,
			      filterprops ? &proplistfilter : nullptr );
    TaskRunner::execute( taskr_, propcreator );
}



class ElasticModelAdjuster : public ParallelTask
{ mODTextTranslationClass(ElasticModelAdjuster)
public:

ElasticModelAdjuster( const Strat::LayerModel& lm,
		      TypeSet<ElasticModel>& aimodels, bool checksvel )
    : ParallelTask("Checking & adjusting elastic models")
    , lm_(lm)
    , aimodels_(aimodels)
    , checksvel_(checksvel)
{
}

od_int64 nrIterations() const
{
    return aimodels_.size();
}

uiString uiMessage() const
{
    return !errmsg_.isEmpty() ? errmsg_ : tr( "Checking Models" );
}

uiString uiNrDoneText() const
{
    return tr( "Models done" );
}


uiString infoMsg() const			{ return infomsg_; }
uiString errMsg() const				{ return errmsg_; }

protected:

bool doWork( od_int64 start , od_int64 stop , int )
{
    for ( int midx=mCast(int,start); midx<=mCast(int,stop); midx++ )
    {
	addToNrDone( 1 );
	const Strat::LayerSequence& seq = lm_.sequence( midx );
	ElasticModel& aimodel = aimodels_[midx];
	if ( aimodel.isEmpty() ) continue;

	ElasticModel tmpmodel( aimodel );
	int erroridx = -1;
	tmpmodel.checkAndClean( erroridx, !checksvel_, checksvel_, false );
	if ( tmpmodel.isEmpty() )
	{
	    uiString startstr(
		checksvel_ ? tr("Could not generate prestack synthetics as all")
			   : tr("All") );
	    uiString propstr( checksvel_ ? tr("Swave velocity")
					 : tr("Pwave velocity/Density") );
	    errmsg_ = tr( "%1 the values of %2 in elastic model are invalid. "
			  "Probably units are not set correctly." )
				.arg(startstr).arg(propstr);
	    return false;
	}
	else if ( erroridx != -1 )
	{
	    bool needinterpolatedvel = false;
	    bool needinterpoltedden = false;
	    bool needinterpolatedsvel = false;
	    uiString msg;
	    for ( int idx=erroridx; idx<aimodel.size(); idx++ )
	    {
		const ElasticLayer& layer = aimodel[idx];
		const bool needinfo = msg.isEmpty();
		const bool incorrectpvel = !layer.isValidVel();
		const bool incorrectden = !layer.isValidDen();
		const bool incorrectsvel = !layer.isValidVs();
		if ( !incorrectpvel && !incorrectden && !incorrectsvel )
		    continue;

		if ( incorrectpvel )
		{
		    needinterpolatedvel = true;
		    if ( needinfo && !infomsg_.isSet() )
		    {
			const UnitOfMeasure* uom = UoMR().get( "Meter/second" );
			msg.append( tr("'Pwave' ( sample value: %1 %2 )")
				.arg(toString(layer.vel_))
				.arg(uom ? uom->symbol() : "") );
		    }
		}

		if ( incorrectden )
		{
		    needinterpoltedden = true;
		    if ( needinfo && !infomsg_.isSet() )
		    {
			const UnitOfMeasure* uom = UoMR().get( "Kg/m3" );
			msg.append( tr("'Density' ( sample value: %1 %2 )")
				.arg(toString(layer.vel_))
				.arg(uom ? uom->symbol() : "") );
		    }
		}

		if ( incorrectsvel )
		{
		    needinterpolatedsvel = true;
		    if ( needinfo && !infomsg_.isSet() )
		    {
			const UnitOfMeasure* uom = UoMR().get( "Meter/second" );
			msg.append( tr("'Swave' ( sample value: %1 %2 )")
				.arg(toString(layer.vel_))
				.arg(uom ? uom->symbol() : "") );
		    }
		}
	    }

	    if ( infomsg_.isEmpty() )
	    {
		infomsg_ = tr( "Layer model contains invalid values of the "
			       "following properties: %1. First occurence "
			       "found in layer '%2' of pseudo well number '%3'."
			       "Invalid values will be interpolated. "
			       "The resulting synthetics may be incorrect" )
		    .arg( msg ).arg(seq.layers()[erroridx]->name()).arg(midx+1);
	    }

	    aimodel.interpolate( needinterpolatedvel, needinterpoltedden,
				 needinterpolatedsvel );
	}

	aimodel.mergeSameLayers();
	aimodel.upscale( 5.0f );
    }

    return true;
}


    const Strat::LayerModel&	lm_;
    TypeSet<ElasticModel>&	aimodels_;
    uiString			infomsg_;
    uiString			errmsg_;
    bool			checksvel_;
};


bool StratSynth::adjustElasticModel( const Strat::LayerModel& lm,
				     TypeSet<ElasticModel>& aimodels,
				     bool checksvel )
{
    ElasticModelAdjuster emadjuster( lm, aimodels, checksvel );
    const bool res = TaskRunner::execute( taskr_, emadjuster );
    infomsg_ = emadjuster.infoMsg();
    swaveinfomsgshown_ = checksvel;
    return res;
}


void StratSynth::getLevelDepths( const Strat::Level& lvl, int dispeach,
				 TypeSet<float>& zvals ) const
{
    zvals.setEmpty();
    for ( int iseq=0; iseq<layMod().size(); iseq+=dispeach )
	zvals += layMod().sequence(iseq).depthPositionOf( lvl );
}


namespace Strat
{

static void convD2T( const ReflectivityModelSet& refmodels, int dispeach,
		     int offsidx, TypeSet<float>& zvals )
{
    const int nrmodels = refmodels.nrModels();
    for ( int imdl=0, idx=0; imdl<nrmodels; imdl+=dispeach, idx++ )
    {
	const ReflectivityModelBase* refmodel = refmodels.get( imdl );
	if ( !refmodel )
	{
	    zvals[idx] = mUdf(float);
	    continue;
	}

	const TimeDepthModel* tdmodel = offsidx < 0
				      ? &refmodel->getDefaultModel()
				      : refmodel->get( offsidx );
	zvals[idx] = tdmodel ? tdmodel->getTime( zvals[idx] ) : mUdf(float);
    }
}

} // namespace Strat


bool StratSynth::setLevelTimes( const char* sdnm, int dispeach, int offsetidx )
{
    SyntheticData* sd = getSynthetic( sdnm );
    if ( !sd || sd->isPS() )
	return false;

    mDynamicCastGet(PostStackSyntheticData*,postsd,sd);
    if ( !postsd )
	return false;

    getLevelTimes( sd->synthGenDP().getModels(), dispeach,
		   postsd->postStackPack().trcBuf(), offsetidx );
    return true;
}


void StratSynth::getLevelTimes( const Strat::Level& lvl, int dispeach,
				const ReflectivityModelSet& refmodels,
				TypeSet<float>& zvals, int offsetidx ) const
{
    getLevelDepths( lvl, dispeach, zvals );
    Strat::convD2T( refmodels, dispeach, offsetidx, zvals );
}


void StratSynth::getLevelTimes( const ReflectivityModelSet& refmodels,
				int dispeach, SeisTrcBuf& trcs,
				int offsetidx ) const
{
    if ( !level_ )
	return;

    TypeSet<float> times = level_->zvals_;
    Strat::convD2T( refmodels, dispeach, offsetidx, times );
    const int trcstep = trcs.size() == refmodels.nrModels() ? 1 : dispeach;
    for ( int idx=0; idx<trcs.size(); idx+=trcstep )
    {
	const SeisTrc& trc = *trcs.get( idx );
	SeisTrcPropCalc stp( trc );
	float z = times.validIdx( idx ) ? times[idx] : mUdf( float );
	trcs.get( idx )->info().zref = z;
	if ( !mIsUdf( z ) && level_->snapev_ != VSEvent::None )
	{
	    Interval<float> tg( z, trc.startPos() );
	    mFlValSerEv ev1 = stp.find( level_->snapev_, tg, 1 );
	    tg.start = z; tg.stop = trc.endPos();
	    mFlValSerEv ev2 = stp.find( level_->snapev_, tg, 1 );
	    float tmpz = ev2.pos;
	    const bool ev1invalid = mIsUdf(ev1.pos) || ev1.pos < 0;
	    const bool ev2invalid = mIsUdf(ev2.pos) || ev2.pos < 0;
	    if ( ev1invalid && ev2invalid )
		continue;
	    else if ( ev2invalid )
		tmpz = ev1.pos;
	    else if ( fabs(z-ev1.pos) < fabs(z-ev2.pos) )
		tmpz = ev1.pos;

	    z = tmpz;
	}

	trcs.get( idx )->info().pick = z;
    }
}


void StratSynth::setLevel( const StratSynthLevel* lvl )
{ delete level_; level_ = lvl; }



void StratSynth::trimTraces( SeisTrcBuf& tbuf,
			     const ObjectSet<const TimeDepthModel>& d2ts,
			     float zskip ) const
{
    if ( mIsZero(zskip,mDefEps) )
	return;

    float highetszkip = mUdf(float);
    for ( int idx=0; idx<d2ts.size(); idx++ )
    {
	const TimeDepthModel& d2tmodel = *d2ts[idx];
	if ( d2tmodel.getTime(zskip)<highetszkip )
	    highetszkip = d2tmodel.getTime(zskip);
    }

    for ( int idx=0; idx<tbuf.size(); idx++ )
    {
	SeisTrc* trc = tbuf.get( idx );
	SeisTrc* newtrc = new SeisTrc( *trc );
	newtrc->info() = trc->info();
	const int startidx = trc->nearestSample( highetszkip );
	newtrc->reSize( trc->size()-startidx, false );
	newtrc->setStartPos( trc->samplePos(startidx) );
	for ( int sampidx=startidx; sampidx<trc->size(); sampidx++ )
	{
	    const float trcval =
		sampidx<0 || sampidx>=trc->size()-1 ? mUdf(float)
						    : trc->get(sampidx,0);
	    newtrc->set( sampidx-startidx, trcval, 0 );
	}
	delete tbuf.replace( idx, newtrc );
    }
}


void StratSynth::flattenTraces( SeisTrcBuf& tbuf ) const
{
    if ( tbuf.isEmpty() )
	return;

    float tmax = tbuf.get(0)->info().sampling.start;
    float tmin = tbuf.get(0)->info().sampling.atIndex( tbuf.get(0)->size() );
    for ( int idx=tbuf.size()-1; idx>=1; idx-- )
    {
	if ( mIsUdf(tbuf.get(idx)->info().pick) ) continue;
	tmin = mMIN(tmin,tbuf.get(idx)->info().pick);
	tmax = mMAX(tmax,tbuf.get(idx)->info().pick);
    }

    for ( int idx=tbuf.size()-1; idx>=0; idx-- )
    {
	const SeisTrc* trc = tbuf.get( idx );
	const float start = trc->info().sampling.start - tmax;
	const float stop  = trc->info().sampling.atIndex(trc->size()-1) -tmax;
	SeisTrc* newtrc = trc->getRelTrc( ZGate(start,stop) );
	if ( !newtrc )
	{
	    newtrc = new SeisTrc( *trc );
	    newtrc->zero();
	}

	delete tbuf.replace( idx, newtrc );
    }
}


void StratSynth::decimateTraces( SeisTrcBuf& tbuf, int fac ) const
{
    for ( int idx=tbuf.size()-1; idx>=0; idx-- )
    {
	if ( idx%fac )
	    delete tbuf.remove( idx );
    }
}


void StratSynth::fillPar( IOPar& par ) const
{
    const int nrsynthetics = nrSynthetics();
    for ( int idx=0; idx<nrsynthetics; idx++ )
    {
	const SyntheticData* sd = getSyntheticByIdx( idx );
	IOPar sdpar;
	sd->getGenParams().fillPar( sdpar );
	par.mergeComp( sdpar, IOPar::compKey(sKeySyntheticNr(),idx) );
    }

    par.set( sKeyNrSynthetics(), nrsynthetics );
}


bool StratSynth::usePar( const IOPar& par )
{
    ObjectSet<SynthGenParams> genparsset;
    genparsset.setNullAllowed();
    if ( !getAllGenPars(par,genparsset) || genparsset.isEmpty() )
    {
	deepErase( genparsset );
	return false;
    }

    uiRetVal uirv, infos;
    int nrvalidpars = 0, nradded = 0;
    for ( const auto* genpars : genparsset )
    {
	if ( !genpars )
	    continue;

	nrvalidpars++;
	const SyntheticData* sd = addSynthetic( *genpars );
	if ( sd )
	{
	    if ( ++nradded == 0 )
		genparams_ = *genpars;
	}
	if ( !errmsg_.isOK() )
	    uirv.add( errmsg_ );
	if ( !infomsg_.isOK() )
	    infos.add( infomsg_ );
    }

    deepErase( genparsset );
    if ( !uirv.isOK() )
	errmsg_ = uirv;
    if ( !infos.isOK() )
	infomsg_ = infos;

    return nradded == nrvalidpars;
}


IOPar StratSynth::getSelection( const BufferStringSet& synthnms ) const
{
    IOPar ret;
    fillPar( ret );

    ObjectSet<SynthGenParams> genparsset;
    genparsset.setNullAllowed();
    if ( !getAllGenPars(ret,genparsset) || genparsset.isEmpty() )
    {
	deepErase( genparsset );
	return ret;
    }

    BufferStringSet availablepsnms, requiredpsnms;
    TypeSet<int> unusedsynthidxs;
    for ( const auto* genpars : genparsset )
    {
	if ( !genpars )
	    continue;

	if ( genpars->isPreStack() )
	    availablepsnms.add( genpars->name_ );
	else
	{
	    if ( synthnms.isPresent(genpars->name_.buf()) )
	    {
		if ( genpars->isPSBased() )
		    requiredpsnms.addIfNew( genpars->inpsynthnm_ );
	    }
	    else
		unusedsynthidxs.addIfNew( genparsset.indexOf( genpars ) );
	}
    }

    if ( !availablepsnms.isEmpty() )
    {
	BufferStringSet toremovepsnms( availablepsnms );
	for ( const auto* requiredpsnm : requiredpsnms )
	    toremovepsnms.remove( requiredpsnm->buf() );

	for ( const auto* toremovepsnm : toremovepsnms )
	{
	    for ( const auto* genpars : genparsset )
	    {
		if ( !genpars || !genpars->isPreStack() )
		    continue;

		if ( genpars->name_ == toremovepsnm->buf() )
		    unusedsynthidxs.addIfNew( genparsset.indexOf( genpars ) );
	    }
	}
    }

    if ( unusedsynthidxs.isEmpty() )
	return ret;
    else if ( unusedsynthidxs.size() == nrSynthetics() )
	return IOPar();

    PtrMan<IOPar> synthpar = ret.subselect( sKeySyntheticNr() );
    if ( !synthpar )
	return ret;

    for ( const auto& idx : unusedsynthidxs )
	synthpar->removeSubSelection( idx );

    ret.removeSubSelection( sKeySyntheticNr() );
    ret.mergeComp( *synthpar.ptr(), sKeySyntheticNr() );

    return ret;
}


bool StratSynth::getAllGenPars( const IOPar& par,
				ObjectSet<SynthGenParams>& genparsset )
{
    int nrsynthetics = -1;
    if ( !par.get(sKeyNrSynthetics(),nrsynthetics) || nrsynthetics < 0 )
	return false;

    PtrMan<IOPar> synthpar = par.subselect( sKeySyntheticNr() );
    if ( !synthpar )
	return false;

    for ( int idx=0; idx<nrsynthetics; idx++ )
    {
	PtrMan<IOPar> iop = synthpar->subselect( idx );
	if ( !iop )
	{
	    genparsset.add( nullptr );
	    continue;
	}

	auto* genpars = new SynthGenParams;
	genpars->usePar( *iop.ptr() );
	if ( !genpars->isOK() )
	{
	    delete genpars;
	    genparsset.add( nullptr );
	    continue;
	}

	genparsset.add( genpars );
    }

    return true;
}



SyntheticData::SyntheticData( const SynthGenParams& sgp,
			      const Seis::SynthGenDataPack& synthgendp,
			      DataPack& dp )
    : NamedCallBacker(sgp.name_)
    , sgp_(sgp)
    , datapack_(dp)
    , synthgendp_(&synthgendp)
{
}


SyntheticData::~SyntheticData()
{
    sendDelNotif();
    removePack();
}


void SyntheticData::setName( const char* nm )
{
    NamedCallBacker::setName( nm );
    datapack_.setName( nm );
}


void SyntheticData::removePack()
{
    const DataPack::FullID dpid = datapackid_;
    DataPackMgr::ID packmgrid = DataPackMgr::getID( dpid );
    DPM(packmgrid).release( dpid.ID(1) );
}


void SyntheticData::useGenParams( const SynthGenParams& sgp )
{
    if ( sgp.synthtype_ != sgp_.synthtype_ )
    {
	pErrMsg("Should not change type");
	return;
    }

    IOPar par;
    sgp.fillPar( par );
    sgp_.usePar( par );
    setName( sgp_.name_ );
}


void SyntheticData::fillDispPar( IOPar& par ) const
{
    disppars_.fillPar( par );
}


void SyntheticData::useDispPar( const IOPar& par )
{
    disppars_.usePar( par );
}


const char* SyntheticData::waveletName() const
{
    return sgp_.getWaveletNm();
}


float SyntheticData::getTime( float dpt, int seqnr ) const
{
    const TimeDepthModel* tdmodel = getTDModel( seqnr );
    return tdmodel ? tdmodel->getTime( dpt ) : mUdf( float );
}


float SyntheticData::getDepth( float time, int seqnr ) const
{
    const TimeDepthModel* tdmodel = getTDModel( seqnr );
    return tdmodel ? tdmodel->getDepth( time ) : mUdf( float );
}


const Seis::SynthGenDataPack& SyntheticData::synthGenDP() const
{
    return *synthgendp_.ptr();
}


const ReflectivityModelSet& SyntheticData::getRefModels() const
{
    return synthGenDP().getModels();
}


const ReflectivityModelBase* SyntheticData::getRefModel( int imdl ) const
{
    const ReflectivityModelSet& refmodels = getRefModels();
    return refmodels.validIdx(imdl) ? refmodels.get( imdl ) : nullptr;
}


const TimeDepthModel* SyntheticData::getTDModel( int imdl ) const
{
    const ReflectivityModelBase* refmodel = getRefModel( imdl );
    return refmodel ? &refmodel->getDefaultModel() : nullptr;
}


const TimeDepthModel* SyntheticData::getTDModel( int imdl, int ioff ) const
{
    const ReflectivityModelBase* refmodel = getRefModel( imdl );
    return refmodel ? refmodel->get( ioff ) : nullptr;
}


SyntheticData* SyntheticData::get( const SynthGenParams& sgp,
				   Seis::RaySynthGenerator& synthgen )
{
    if ( !sgp.isRawOutput() )
	return nullptr;

    ConstRefMan<Seis::SynthGenDataPack> genres = synthgen.getAllResults();
    if ( !genres )
	return nullptr;

    if ( genres->isStack() )
    {
	auto* dptrcbuf = new SeisTrcBuf( true );
	synthgen.getStackedTraces( *dptrcbuf );
	auto* dp =
	    new SeisTrcBufDataPack( dptrcbuf, Seis::Line, SeisTrcInfo::TrcNr,
				PostStackSyntheticData::sDataPackCategory() );
	return new PostStackSyntheticData( sgp, *genres.ptr(), *dp );
    }

    if ( genres->isPS() )
    {
	ObjectSet<SeisTrcBuf> tbufs;
	if ( !synthgen.getTraces(tbufs) )
	    return nullptr;

	const bool iscorrected = genres->isCorrected();
	ObjectSet<PreStack::Gather> gatherset;
	while ( tbufs.size() )
	{
	    PtrMan<SeisTrcBuf> tbuf = tbufs.removeSingle( 0 );
	    auto* gather = new PreStack::Gather();
	    if ( !gather->setFromTrcBuf(*tbuf,0) )
		{ delete gather; continue; }

	    gather->setCorrected( iscorrected );
	    gatherset += gather;
	}

	auto* dp = new PreStack::GatherSetDataPack( nullptr, gatherset );
	return new PreStackSyntheticData( sgp, *genres.ptr(), *dp );
    }

    return nullptr;
}



PostStackSyntheticData::PostStackSyntheticData( const SynthGenParams& sgp,
				    const Seis::SynthGenDataPack& synthdp,
				    SeisTrcBufDataPack& dp)
    : SyntheticData(sgp,synthdp,dp)
{
    DataPackMgr::ID pmid = DataPackMgr::FlatID();
    DPM( pmid ).addAndObtain( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
}


PostStackSyntheticData::~PostStackSyntheticData()
{
}


const SeisTrc* PostStackSyntheticData::getTrace( int seqnr ) const
{ return postStackPack().trcBuf().get( seqnr ); }


int PostStackSyntheticData::nrPositions() const
{
    return postStackPack().trcBuf().size();
}


SeisTrcBufDataPack& PostStackSyntheticData::postStackPack()
{
    return static_cast<SeisTrcBufDataPack&>( datapack_ );
}


const SeisTrcBufDataPack& PostStackSyntheticData::postStackPack() const
{
    return static_cast<const SeisTrcBufDataPack&>( datapack_ );
}


PreStackSyntheticData::PreStackSyntheticData( const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					PreStack::GatherSetDataPack& dp )
    : SyntheticData(sgp,synthdp,dp)
{
    DataPackMgr::ID pmid = DataPackMgr::SeisID();
    DPM( pmid ).addAndObtain( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
    ObjectSet<PreStack::Gather>& gathers = dp.getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
	gathers[idx]->setName( name() );
}


PreStackSyntheticData::~PreStackSyntheticData()
{
    DPM( DataPackMgr::SeisID() ).release( datapack_.id() );
    if ( angledp_ )
	DPM( DataPackMgr::SeisID() ).release( angledp_->id() );
}


PreStack::GatherSetDataPack& PreStackSyntheticData::preStackPack()
{
    return static_cast<PreStack::GatherSetDataPack&>( datapack_ );
}


const PreStack::GatherSetDataPack& PreStackSyntheticData::preStackPack() const
{
    return static_cast<const PreStack::GatherSetDataPack&>( datapack_ );
}


int PreStackSyntheticData::nrPositions() const
{
    return preStackPack().getGathers().size();
}


void PreStackSyntheticData::convertAngleDataToDegrees(
					PreStack::Gather& ag ) const
{
    const auto& agdata = const_cast<const Array2D<float>&>( ag.data() );
    ArrayMath::getScaledArray<float>( agdata, nullptr, mRad2DegD, 0.,
				      false, true );
}


void PreStackSyntheticData::setAngleData(
					const ObjectSet<PreStack::Gather>& ags )
{
    if ( angledp_ )
	DPM( DataPackMgr::SeisID() ).release( angledp_->id() );

    angledp_ = new PreStack::GatherSetDataPack( nullptr, ags );
    const BufferString angledpnm( name().buf(), " (Angle Gather)" );
    angledp_->setName( angledpnm );
    DPM( DataPackMgr::SeisID() ).addAndObtain( angledp_ );
}


float PreStackSyntheticData::offsetRangeStep() const
{
    float offsetstep = mUdf(float);
    const ObjectSet<PreStack::Gather>& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() )
    {
	const PreStack::Gather& gather = *gathers[0];
	offsetstep = gather.getOffset(1)-gather.getOffset(0);
    }

    return offsetstep;
}


const Interval<float> PreStackSyntheticData::offsetRange() const
{
    Interval<float> offrg( 0, 0 );
    const ObjectSet<PreStack::Gather>& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() )
    {
	const PreStack::Gather& gather = *gathers[0];
	offrg.set(gather.getOffset(0),gather.getOffset( gather.size(true)-1));
    }
    return offrg;
}


bool PreStackSyntheticData::hasOffset() const
{ return offsetRange().width() > 0; }


bool PreStackSyntheticData::isNMOCorrected() const
{
    bool isnmo = true;
    sgp_.raypars_.getYN( Seis::SynthGenBase::sKeyNMO(), isnmo );
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


PostStackSyntheticDataWithInput::PostStackSyntheticDataWithInput(
					const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					SeisTrcBufDataPack& sdp )
    : PostStackSyntheticData(sgp,synthdp,sdp)
{
}


PostStackSyntheticDataWithInput::~PostStackSyntheticDataWithInput()
{}


InstAttributeSyntheticData::InstAttributeSyntheticData(
					const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					SeisTrcBufDataPack& sdp )
    : PostStackSyntheticDataWithInput(sgp,synthdp,sdp)
{
}



PSBasedPostStackSyntheticData::PSBasedPostStackSyntheticData(
					const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					SeisTrcBufDataPack& sdp )
    : PostStackSyntheticDataWithInput(sgp,synthdp,sdp)
{
}


PSBasedPostStackSyntheticData::~PSBasedPostStackSyntheticData()
{}


StratPropSyntheticData::StratPropSyntheticData( const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					SeisTrcBufDataPack& dp,
					const PropertyRef& pr )
    : PostStackSyntheticData(sgp,synthdp,dp)
    , prop_(pr)
{}
