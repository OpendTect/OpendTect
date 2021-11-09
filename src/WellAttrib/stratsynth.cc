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
#include "arrayndimpl.h"
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
#include "timeser.h"
#include "transl.h"
#include "wavelet.h"
#include "waveletio.h"

static const char* sKeyIsPreStack()		{ return "Is Pre Stack"; }
static const char* sKeySynthType()		{ return "Synthetic Type"; }
static const char* sKeyWaveLetName()		{ return "Wavelet Name"; }
static const char* sKeyRayPar()			{ return "Ray Parameter"; }
static const char* sKeyDispPar()		{ return "Display Parameter"; }
static const char* sKeyInput()			{ return "Input Synthetic"; }
static const char* sKeyAngleRange()		{ return "Angle Range"; }
static const char* sKeyAdvancedRayTracer()	{ return "FullRayTracer"; }
const char* PostStackSyntheticData::sDataPackCategory()
{ return "Post-stack synthetics"; }

#define sDefaultAngleRange Interval<float>( 0.0f, 30.0f )
#define sDefaultOffsetRange StepInterval<float>( 0.f, 6000.f, 100.f )


mDefineEnumUtils(SynthGenParams,SynthType,"Synthetic Type")
{ "Pre Stack",
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
	   raypars_ == oth.raypars_ && hassameanglerg && hassameinput &&
	   hassameattrib;
}


bool SynthGenParams::operator!= ( const SynthGenParams& oth ) const
{
    return !(*this == oth);
}


bool SynthGenParams::isOK() const
{
    if ( synthtype_ == ZeroOffset || synthtype_ == PreStack )
	return !wvltnm_.isEmpty();
    else if ( synthtype_ == AngleStack || synthtype_ == AVOGradient )
	return !inpsynthnm_.isEmpty() && !anglerg_.isUdf();
    else if ( synthtype_ == InstAttrib )
	return !inpsynthnm_.isEmpty();

    return true;
}


void SynthGenParams::setDefaultValues()
{
    if ( synthtype_ == ZeroOffset )
	RayTracer1D::setIOParsToZeroOffset( raypars_ );
    else if ( synthtype_ == PreStack )
    {
	const FixedString defrayparstr = sKeyAdvancedRayTracer();
	const BufferStringSet& facnms = RayTracer1D::factory().getNames();
	if ( !facnms.isEmpty() )
	{
	    const int typeidx = facnms.indexOf( defrayparstr );
	    const BufferString& facnm = typeidx>=0 ? facnms.get(typeidx)
						   : facnms.get(0);
	    raypars_.set( sKey::Type(), facnm );
	}

	const StepInterval<float> offsetrg = sDefaultOffsetRange;
	TypeSet<float> offsets;
	for ( int idx=0; idx<offsetrg.nrSteps()+1; idx++ )
	    offsets += offsetrg.atIndex( idx );
	raypars_.set( RayTracer1D::sKeyOffset(), offsets );
    }

    if ( synthtype_ == ZeroOffset || synthtype_ == PreStack )
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

    anglerg_ = synthtype_ == AngleStack || synthtype_ == AVOGradient
	     ? sDefaultAngleRange : Interval<float>::udf();
    createName( name_ );
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
    if ( synthtype_ == ZeroOffset || synthtype_ == PreStack )
    {
	par.set( sKeyWaveLetName(), wvltnm_ );
	IOPar raypar;
	raypar.mergeComp( raypars_, sKeyRayPar() );
	par.merge( raypar );
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
    if ( synthtype_ == ZeroOffset || synthtype_ == PreStack )
    {
	par.get( sKeyWaveLetName(), wvltnm_ );
	PtrMan<IOPar> raypar = par.subselect( sKeyRayPar() );
	if ( raypar )
	    raypars_ = *raypar;
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
    if ( synthtype_ == AngleStack || synthtype_ == AVOGradient )
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
    else if ( synthtype_ != ZeroOffset && synthtype_ != PreStack )
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
    , level_(0)
    , taskr_(0)
    , wvlt_(0)
    , lastsyntheticid_(0)
    , swaveinfomsgshown_(0)
{
}


StratSynth::~StratSynth()
{
    clearSynthetics();
    setLevel( 0 );
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
    genparams_.wvltnm_ = wvlt->name();
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
    clearRayModels();
}


#define mErrRet( msg, act )\
{\
    errmsg_ = toUiString("Can not generate synthetics %1 : %2\n") \
		    .arg( synthgenpar.name_ ) \
		    .arg( msg ); \
    act;\
}


bool StratSynth::canRayModelsBeRemoved( const IOPar& sdraypar ) const
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	SynthGenParams sdsgp;
	synthetics_[idx]->fillGenParams( sdsgp );
	const bool ispsbased = sdsgp.synthtype_==SynthGenParams::AngleStack ||
			       sdsgp.synthtype_==SynthGenParams::AVOGradient;
	if ( !ispsbased && synthrmmgr_.haveSameRM(sdraypar,sdsgp.raypars_) )
	    return false;
    }

    return true;
}


const PreStack::GatherSetDataPack* StratSynth::getRelevantAngleData(
					const IOPar& sdraypar ) const
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	const SyntheticData* sd = synthetics_[idx];
	mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
	if ( !presd ) continue;
	SynthGenParams sgp;
	sd->fillGenParams( sgp );
	if ( synthrmmgr_.haveSameRM(sgp.raypars_,sdraypar) )
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

	SynthGenParams sgp;
	sd->fillGenParams( sgp );
	if ( sgp.isPSBased() )
	{
	    sgp.inpsynthnm_ = SynthGenParams::sKeyInvalidInputPS();
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
	    SyntheticData* sd = synthetics_.removeSingle( idx );
	    SynthGenParams sgp;
	    sd->fillGenParams( sgp );
	    if ( canRayModelsBeRemoved(sgp.raypars_) )
		synthrmmgr_.removeRayModelSet( sgp.raypars_ );

	    delete sd;
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

    SynthGenParams synthgen;
    sd->fillGenParams( synthgen );
    SyntheticData* newsd = generateSD( synthgen );
    if ( newsd )
    {
	newsd->setName( sd->name() );
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
mSetFloat(Attrib::PSAttrib::angleStartStr(), synthgenpar.anglerg_.start ); \
mSetFloat(Attrib::PSAttrib::angleStopStr(), synthgenpar.anglerg_.stop ); \
mSetFloat(Attrib::PSAttrib::angleDPIDStr(),\
	&presd.angleData() ? presd.angleData().id() : -1 ); \
psdesc->setUserRef( synthgenpar.name_ ); \
psdesc->updateParams(); \
PtrMan<Attrib::DescSet> descset = new Attrib::DescSet( false ); \
if ( !descset ) return 0; \
Attrib::DescID attribid = descset->addDesc( psdesc ); \
PtrMan<Attrib::EngineMan> aem = new Attrib::EngineMan; \
TypeSet<Attrib::SelSpec> attribspecs; \
Attrib::SelSpec sp( 0, attribid ); \
sp.set( *psdesc ); \
attribspecs += sp; \
aem->setAttribSet( descset ); \
aem->setAttribSpecs( attribspecs ); \
aem->setTrcKeyZSampling( cs ); \
BinIDValueSet bidvals( 0, false ); \
const ObjectSet<PreStack::Gather>& gathers = gdp.getGathers(); \
for ( int idx=0; idx<gathers.size(); idx++ ) \
    bidvals.add( gathers[idx]->getBinID() ); \
SeisTrcBuf* dptrcbufs = new SeisTrcBuf( true ); \
Interval<float> zrg( cs.zsamp_ ); \
uiString errmsg; \
PtrMan<Attrib::Processor> proc = \
    aem->createTrcSelOutput( errmsg, bidvals, *dptrcbufs, 0, &zrg); \
if ( !proc || !proc->getProvider() ) \
    mErrRet( errmsg, return 0 ) ; \
proc->getProvider()->setDesiredVolume( cs ); \
proc->getProvider()->setPossibleVolume( cs ); \
mDynamicCastGet(Attrib::PSAttrib*,psattr,proc->getProvider()); \
if ( !psattr ) \
    mErrRet( proc->uiMessage(), return 0 ) ;


#define mCreateSeisBuf( dpname ) \
if ( !TaskRunner::execute(taskr_,*proc) ) \
    mErrRet( proc->uiMessage(), return 0 ) ; \
const int crlstep = SI().crlStep(); \
const BinID bid0( SI().inlRange(false).stop + SI().inlStep(), \
		  SI().crlRange(false).stop + crlstep ); \
for ( int trcidx=0; trcidx<dptrcbufs->size(); trcidx++ ) \
{ \
    const BinID bid = dptrcbufs->get( trcidx )->info().binid; \
    SeisTrcInfo& trcinfo = dptrcbufs->get( trcidx )->info(); \
    trcinfo.coord = SI().transform( bid ); \
    trcinfo.nr = trcidx+1; \
} \
SeisTrcBufDataPack* dpname = \
    new SeisTrcBufDataPack( dptrcbufs, Seis::Line, \
			    SeisTrcInfo::TrcNr, \
			    PostStackSyntheticData::sDataPackCategory() ); \

SyntheticData* StratSynth::createAttribute( const SyntheticData& sd,
					     const SynthGenParams& synthgenpar )
{
    if ( sd.isPS() )
	return nullptr;

    mDynamicCastGet(const PostStackSyntheticData&,pssd,sd);
    const SeisTrcBufDataPack& indp = pssd.postStackPack();
    TrcKeyZSampling tkzs;
    indp.getTrcKeyZSampling( tkzs );

    PtrMan<Attrib::DescSet> descset = new Attrib::DescSet( false );
    BufferString dpidstr( "#", sd.datapackid_.buf() );
    Attrib::DescID did = descset->getStoredID( dpidstr.buf(), 0, true );

    Attrib::Desc* imagdesc = Attrib::PF().createDescCopy(
						Attrib::Hilbert::attribName() );
    imagdesc->selectOutput( 0 );
    imagdesc->setInput(0, descset->getDesc(did) );
    imagdesc->setHidden( true );
    BufferString usrref( dpidstr.buf(), "_imag" );
    imagdesc->setUserRef( usrref );
    descset->addDesc( imagdesc );

    Attrib::Desc* psdesc = Attrib::PF().createDescCopy(
					 Attrib::Instantaneous::attribName());
    psdesc->selectOutput( synthgenpar.attribtype_ );
    psdesc->setInput( 0, descset->getDesc(did) );
    psdesc->setInput( 1, imagdesc );
    psdesc->setUserRef( synthgenpar.name_ );
    Attrib::DescID attribid = descset->addDesc( psdesc );
    descset->updateInputs();

    PtrMan<Attrib::EngineMan> aem = new Attrib::EngineMan;
    TypeSet<Attrib::SelSpec> attribspecs;
    Attrib::SelSpec sp( 0, attribid );
    sp.set( *psdesc );
    attribspecs += sp;
    aem->setAttribSet( descset );
    aem->setAttribSpecs( attribspecs );
    aem->setTrcKeyZSampling( tkzs );
    BinIDValueSet bidvals( 0, false );
    const SeisTrcBuf& trcs = indp.trcBuf();
    for ( int idx=0; idx<trcs.size(); idx++ )
	bidvals.add( trcs.get(idx)->info().binID() );

    SeisTrcBuf* dptrcbufs = new SeisTrcBuf( true );
    Interval<float> zrg( tkzs.zsamp_ );
    uiString errmsg;
    PtrMan<Attrib::Processor> proc = aem->createTrcSelOutput( errmsg, bidvals,
							  *dptrcbufs, 0, &zrg);
    if ( !proc || !proc->getProvider() )
	mErrRet( errmsg, return 0 ) ;
    proc->getProvider()->setDesiredVolume( tkzs );
    proc->getProvider()->setPossibleVolume( tkzs );

    mCreateSeisBuf( instattribdp );
    return new InstAttributeSyntheticData( synthgenpar, *instattribdp );
}


SyntheticData* StratSynth::createAVOGradient( const SyntheticData& sd,
					     const TrcKeyZSampling& cs,
					     const SynthGenParams& synthgenpar )
{
    mCreateDesc()
    mSetEnum(Attrib::PSAttrib::calctypeStr(),PreStack::PropCalc::LLSQ);
    mSetEnum(Attrib::PSAttrib::offsaxisStr(),PreStack::PropCalc::Sinsq);
    mSetEnum(Attrib::PSAttrib::lsqtypeStr(), PreStack::PropCalc::Coeff );

    mSetProc();
    mCreateSeisBuf( angledp );
    return new AVOGradSyntheticData( synthgenpar, *angledp );
}


SyntheticData* StratSynth::createAngleStack( const SyntheticData& sd,
					     const TrcKeyZSampling& cs,
					     const SynthGenParams& synthgenpar )
{
    mCreateDesc();
    mSetEnum(Attrib::PSAttrib::calctypeStr(),PreStack::PropCalc::Stats);
    mSetEnum(Attrib::PSAttrib::stattypeStr(), Stats::Average );

    mSetProc();
    mCreateSeisBuf( angledp );
    return new AngleStackSyntheticData( synthgenpar, *angledp );
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
    clearElasticModels();

    if ( layMod().isEmpty() )
	return false;

    ElasticModelCreator emcr( layMod(), aimodels_ );
    if ( !TaskRunner::execute(taskr_,emcr) )
	return false;

    bool modelsvalid = false;
    for ( int idx=0; idx<aimodels_.size(); idx++ )
    {
	if ( !aimodels_[idx].isEmpty() )
	{
	    modelsvalid = true;
	    break;
	}
    }

    errmsg_.setEmpty();
    if ( !modelsvalid )
	return false;

    return adjustElasticModel( layMod(), aimodels_, useed_ );
}


class PSAngleDataCreator : public Executor
{ mODTextTranslationClass(PSAngleDataCreator)
public:

PSAngleDataCreator( const PreStackSyntheticData& pssd,
		    const ObjectSet<RayTracer1D>& rts )
    : Executor("Creating Angle Gather" )
    , gathers_(pssd.preStackPack().getGathers())
    , rts_(rts)
    , nrdone_(0)
    , pssd_(pssd)
    , anglecomputer_(new PreStack::ModelBasedAngleComputer)
{
    anglecomputer_->setFFTSmoother( 10.f, 15.f );
}

~PSAngleDataCreator()
{
    delete anglecomputer_;
}

od_int64 totalNr() const
{ return rts_.size(); }

od_int64 nrDone() const
{ return nrdone_; }

uiString uiMessage() const
{
    return tr("Calculating Angle Gathers");
}

uiString uiNrDoneText() const
{
    return tr( "Models done" );
}

ObjectSet<PreStack::Gather>& angleGathers()	{ return anglegathers_; }

protected:

void convertAngleDataToDegrees( PreStack::Gather* ag ) const
{
    Array2D<float>& agdata = ag->data();
    const int dim0sz = agdata.info().getSize(0);
    const int dim1sz = agdata.info().getSize(1);
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


int nextStep()
{
    if ( !gathers_.validIdx(nrdone_) )
	return Finished();
    const PreStack::Gather* gather = gathers_[(int)nrdone_];
    anglecomputer_->setOutputSampling( gather->posData() );
    anglecomputer_->setGatherIsNMOCorrected( gather->isCorrected() );
    const TrcKey trckey( gather->getBinID() );
    anglecomputer_->setRayTracer( rts_[(int)nrdone_], trckey );
    PreStack::Gather* anglegather = anglecomputer_->computeAngles();
    convertAngleDataToDegrees( anglegather );
    TypeSet<float> azimuths;
    gather->getAzimuths( azimuths );
    anglegather->setAzimuths( azimuths );
    const BufferString angledpnm( pssd_.name(), "(Angle Gather)" );
    anglegather->setName( angledpnm );
    anglegather->setBinID( gather->getBinID() );
    anglegathers_ += anglegather;
    nrdone_++;
    return MoreToDo();
}

    const ObjectSet<RayTracer1D>&	rts_;
    const ObjectSet<PreStack::Gather>&	gathers_;
    const PreStackSyntheticData&	pssd_;
    ObjectSet<PreStack::Gather>		anglegathers_;
    PreStack::ModelBasedAngleComputer*	anglecomputer_;
    od_int64				nrdone_;

};


void StratSynth::createAngleData( PreStackSyntheticData& pssd,
				  const ObjectSet<RayTracer1D>& rts )
{
    PSAngleDataCreator angledatacr( pssd, rts );
    TaskRunner::execute( taskr_, angledatacr );
    pssd.setAngleData( angledatacr.angleGathers() );
}



bool StratSynth::runSynthGen( Seis::RaySynthGenerator& synthgen,
			      const SynthGenParams& synthgenpar )
{
    BufferString capt( "Generating ", synthgenpar.name_ );
    synthgen.setName( capt.buf() );
    const IOPar& raypars = synthgenpar.raypars_;
    synthgen.usePar( raypars );
    bool needsetwvlt = synthgenpar.wvltnm_.isEmpty();
    if ( !needsetwvlt )
    {
	PtrMan<IOObj> ioobj = Wavelet::getIOObj( synthgenpar.wvltnm_ );
	PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
	if ( !wvlt || (wvlt_ && wvlt_->name()==wvlt->name()) )
	    needsetwvlt = true;
	else
	    synthgen.setWavelet( wvlt, OD::CopyPtr );
    }
    if ( wvlt_ && needsetwvlt )
	synthgen.setWavelet( wvlt_, OD::UsePtr );
    synthgen.enableFourierDomain( !GetEnvVarYN("DTECT_CONVOLVE_USETIME") );

    return TaskRunner::execute( taskr_, synthgen );
}


SyntheticData* StratSynth::generateSD( const SynthGenParams& synthgenpar )
{
    errmsg_.setEmpty();

    if ( layMod().isEmpty() )
    {
	errmsg_ = tr("Empty layer model.");
	return nullptr;
    }

    const bool ispsbased =
	synthgenpar.synthtype_ == SynthGenParams::AngleStack ||
	synthgenpar.synthtype_ == SynthGenParams::AVOGradient;

    if ( synthgenpar.synthtype_ == SynthGenParams::PreStack &&
	 !swaveinfomsgshown_ )
    {
	if ( !adjustElasticModel(layMod(),aimodels_,true) )
	    return nullptr;
    }

    ObjectSet<SynthRayModel>* rms = nullptr;
    if ( ispsbased )
    {
	const SyntheticData* sd = getSynthetic( synthgenpar.inpsynthnm_ );
	if ( sd )
	{
	    SynthGenParams pssynthgenpar;
	    sd->fillGenParams( pssynthgenpar );
	    rms = synthrmmgr_.getRayModelSet( pssynthgenpar.raypars_ );
	}
    }
    else
	rms = synthrmmgr_.getRayModelSet( synthgenpar.raypars_ );

    PtrMan<Seis::RaySynthGenerator> synthgen;
    if ( rms )
	synthgen = new Seis::RaySynthGenerator( rms );
    else
	synthgen = new Seis::RaySynthGenerator( &aimodels_, false );
    if ( !ispsbased )
    {
	if ( !runSynthGen(*synthgen,synthgenpar) )
	    return nullptr;
    }

    SyntheticData* sd = nullptr;
    if ( synthgenpar.synthtype_ == SynthGenParams::PreStack || ispsbased )
    {
	if ( !ispsbased )
	{
	    ObjectSet<SeisTrcBuf> tbufs;
	    synthgen->getTraces( tbufs );
	    ObjectSet<PreStack::Gather> gatherset;
	    while ( tbufs.size() )
	    {
		PtrMan<SeisTrcBuf> tbuf = tbufs.removeSingle( 0 );
		PreStack::Gather* gather = new PreStack::Gather();
		if ( !gather->setFromTrcBuf( *tbuf, 0 ) )
		    { delete gather; continue; }

		bool iscorrected = true;
		synthgenpar.raypars_.getYN( Seis::SynthGenBase::sKeyNMO(),
					    iscorrected );
		gather->setCorrected( iscorrected );
		gatherset += gather;
	    }

	    auto* dp = new PreStack::GatherSetDataPack( nullptr, gatherset );
	    sd = new PreStackSyntheticData( synthgenpar, *dp );
	    mDynamicCastGet(PreStackSyntheticData*,presd,sd);
	    if ( rms )
	    {
		const PreStack::GatherSetDataPack* anglegather =
		    getRelevantAngleData( synthgenpar.raypars_ );
		if ( anglegather )
		    presd->setAngleData( anglegather->getGathers() );
	    }
	    else
		createAngleData( *presd, synthgen->rayTracers() );
	}
	else
	{
	    BufferString inputsdnm( synthgenpar.inpsynthnm_ );
	    if ( useed_ )
		inputsdnm += sKeyFRNameSuffix();
	    const SyntheticData* presd = getSynthetic( inputsdnm );
	    if ( !presd )
		mErrRet( tr(" input prestack synthetic data not found."),
			 return nullptr )

	    mDynamicCastGet(const PreStack::GatherSetDataPack*,presgdp,
			    &presd->getPack())
	    if ( !presgdp )
		mErrRet( tr(" input prestack synthetic data not found."),
			 return nullptr )
	    CubeSampling cs( false );
	    cs.zsamp_ = presgdp->zRange();

	    if ( synthgenpar.synthtype_ == SynthGenParams::AngleStack )
		sd = createAngleStack( *presd, cs, synthgenpar );
	    else if ( synthgenpar.synthtype_ == SynthGenParams::AVOGradient )
		sd = createAVOGradient( *presd, cs, synthgenpar );
	}
    }
    else if ( synthgenpar.synthtype_ == SynthGenParams::ZeroOffset )
    {
	auto* dptrcbuf = new SeisTrcBuf( true );
	synthgen->getStackedTraces( *dptrcbuf );
	auto* dp =
	    new SeisTrcBufDataPack( dptrcbuf, Seis::Line, SeisTrcInfo::TrcNr,
				PostStackSyntheticData::sDataPackCategory() );
	sd = new PostStackSyntheticData( synthgenpar, *dp );
    }
    else if ( synthgenpar.synthtype_ == SynthGenParams::InstAttrib )
    {
	BufferString inputsdnm( synthgenpar.inpsynthnm_ );
	const SyntheticData* inpsd = getSynthetic( inputsdnm );
	if ( !inpsd )
	    mErrRet( tr(" input synthetic data not found."), return nullptr )

	sd = createAttribute( *inpsd, synthgenpar );
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
    if ( !rms )
    {
	rms = synthgen->rayModels();
	if ( !rms )
	    { pErrMsg("No ray models"); return sd; }
	synthrmmgr_.addRayModelSet( rms, sd );
    }

    putD2TModelsInSD( *sd, *rms );
    return sd;
}

void StratSynth::putD2TModelsInSD( SyntheticData& sd,
				   ObjectSet<SynthRayModel>& rms )
{
    deepErase( sd.d2tmodels_ );
    deepErase( sd.zerooffsd2tmodels_ );
    ObjectSet<TimeDepthModel> zeroofsetd2tms;
    for ( int imdl=0; imdl<aimodels_.size(); imdl++ )
    {
	Seis::RaySynthGenerator::RayModel* rm = rms[imdl];
	if ( !rm ) continue;
	TimeDepthModel* zeroofsetd2tm = new TimeDepthModel();
	rm->getZeroOffsetD2T( *zeroofsetd2tm );
	zeroofsetd2tms += zeroofsetd2tm;
	mDynamicCastGet(const PreStackSyntheticData*,presd,&sd);
	if ( presd && !presd->isNMOCorrected() )
	{
	    ObjectSet<TimeDepthModel> tmpd2ts, sdd2ts;
	    sdd2ts.allowNull( true );
	    rm->getD2T( tmpd2ts, false );
	    deepCopy( sdd2ts, tmpd2ts );
	    adjustD2TModels( sdd2ts );
	    while( sdd2ts.size() )
		sd.d2tmodels_ += sdd2ts.removeSingle( 0 );
	    deepErase( sdd2ts );
	}
    }

    adjustD2TModels( zeroofsetd2tms );
    while( !zeroofsetd2tms.isEmpty() )
	sd.zerooffsd2tmodels_ += zeroofsetd2tms.removeSingle( 0 );
}


void StratSynth::adjustD2TModels( ObjectSet<TimeDepthModel>& d2tmodels ) const
{
    for ( int imdl=0; imdl<d2tmodels.size(); imdl++ )
    {
	TimeDepthModel* d2tmodel = d2tmodels[imdl];
	if ( !d2tmodel ) continue;
	const int d2tmsz = d2tmodel->size();
	TypeSet<float> depths;
	depths.setSize( d2tmsz );
	TypeSet<float> times;
	times.setSize( d2tmsz );
	for ( int isamp=0; isamp<d2tmsz; isamp++ )
	{
	    depths[isamp] = d2tmodel->getDepth( isamp ) -
			    mCast(float,SI().seismicReferenceDatum());
	    times[isamp] = d2tmodel->getTime( isamp );
	}

	d2tmodel->setModel( depths.arr(), times.arr(), d2tmsz );
    }
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
	const TimeDepthModel& t2d = *sd_.zerooffsd2tmodels_[iseq];
	const Interval<float> seqdepthrg = seq.zRange();
	const float seqstarttime = t2d.getTime( seqdepthrg.start );
	const float seqstoptime = t2d.getTime( seqdepthrg.stop );
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

	    const float dptstart = t2d.getDepth( time - (float)zrg_.step );
	    const float dptstop = t2d.getDepth( time + (float)zrg_.step );
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
	const TimeDepthModel& t2d = *sd_.zerooffsd2tmodels_[iseq];
	Interval<float> seqtimerg(  t2d.getTime(seq.zRange().start),
				    t2d.getTime(seq.zRange().stop) );

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
    SynthGenParams sgp;
    sd_.fillGenParams( sgp );
    for ( int idx=0; idx<seisbufdps_.size(); idx++ )
    {
	const PropertyRef* pr = prs_[idx];
	SeisTrcBufDataPack* dp = seisbufdps_[idx];
	BufferString propnm = pr->name();
	if ( useed_ )
	    propnm += StratSynth::sKeyFRNameSuffix();
	BufferString nm( "[", propnm, "]" );
	dp->setName( nm );
	auto* prsd = new StratPropSyntheticData( sgp, *dp, *pr );
	prsd->id_ = ++lastsyntheticid_;
	prsd->setName( nm );

	deepCopy( prsd->zerooffsd2tmodels_, sd_.zerooffsd2tmodels_ );
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


void StratSynth::getLevelDepths( const Strat::Level& lvl,
				 TypeSet<float>& zvals ) const
{
    zvals.setEmpty();
    for ( int iseq=0; iseq<layMod().size(); iseq++ )
	zvals += layMod().sequence(iseq).depthPositionOf( lvl );
}


static void convD2T( TypeSet<float>& zvals,
		     const ObjectSet<const TimeDepthModel>& d2ts )
{
    for ( int imdl=0; imdl<zvals.size(); imdl++ )
	zvals[imdl] = d2ts.validIdx(imdl) && !mIsUdf(zvals[imdl]) ?
		d2ts[imdl]->getTime( zvals[imdl] ) : mUdf(float);
}


bool StratSynth::setLevelTimes( const char* sdnm )
{
    SyntheticData* sd = getSynthetic( sdnm );
    if ( !sd ) return false;

    mDynamicCastGet(PostStackSyntheticData*,postsd,sd);
    if ( !postsd ) return false;
    SeisTrcBuf& tb = postsd->postStackPack().trcBuf();
    getLevelTimes( tb, sd->zerooffsd2tmodels_ );
    return true;
}


void StratSynth::getLevelTimes( const Strat::Level& lvl,
				const ObjectSet<const TimeDepthModel>& d2ts,
				TypeSet<float>& zvals ) const
{
    getLevelDepths( lvl, zvals );
    convD2T( zvals, d2ts );
}


void StratSynth::getLevelTimes( SeisTrcBuf& trcs,
			const ObjectSet<const TimeDepthModel>& d2ts ) const
{
    getLevelTimes( trcs, d2ts, -1 );
}


void StratSynth::getLevelTimes( SeisTrcBuf& trcs,
			const ObjectSet<const TimeDepthModel>& d2ts,
			int dispeach ) const
{
    if ( !level_ ) return;

    TypeSet<float> times = level_->zvals_;
    convD2T( times, d2ts );

    for ( int idx=0; idx<trcs.size(); idx++ )
    {
	const SeisTrc& trc = *trcs.get( idx );
	SeisTrcPropCalc stp( trc );
	const int d2tmidx = dispeach==-1 ? idx : idx*dispeach;
	float z = times.validIdx( d2tmidx ) ? times[d2tmidx] : mUdf( float );
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
	SynthGenParams sgpars;
	sd->fillGenParams( sgpars );
	IOPar sdpar;
	sgpars.fillPar( sdpar );
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



SyntheticData::SyntheticData( const SynthGenParams& sgp, DataPack& dp )
    : NamedCallBacker(sgp.name_)
    , datapack_(dp)
    , id_(-1)
{
}


SyntheticData::~SyntheticData()
{
    sendDelNotif();
    deepErase( d2tmodels_ );
    deepErase( zerooffsd2tmodels_ );
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


PostStackSyntheticData::PostStackSyntheticData( const SynthGenParams& sgp,
						SeisTrcBufDataPack& dp)
    : SyntheticData(sgp,dp)
{
    useGenParams( sgp );
    DataPackMgr::ID pmid = DataPackMgr::FlatID();
    DPM( pmid ).addAndObtain( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
}


PostStackSyntheticData::~PostStackSyntheticData()
{
}


const SeisTrc* PostStackSyntheticData::getTrace( int seqnr ) const
{ return postStackPack().trcBuf().get( seqnr ); }


SeisTrcBufDataPack& PostStackSyntheticData::postStackPack()
{
    return static_cast<SeisTrcBufDataPack&>( datapack_ );
}


const SeisTrcBufDataPack& PostStackSyntheticData::postStackPack() const
{
    return static_cast<const SeisTrcBufDataPack&>( datapack_ );
}


PreStackSyntheticData::PreStackSyntheticData( const SynthGenParams& sgp,
					     PreStack::GatherSetDataPack& dp)
    : SyntheticData(sgp,dp)
    , angledp_(0)
{
    useGenParams( sgp );
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


void PreStackSyntheticData::convertAngleDataToDegrees(
					PreStack::Gather* ag ) const
{
    Array2D<float>& agdata = ag->data();
    const int dim0sz = agdata.info().getSize(0);
    const int dim1sz = agdata.info().getSize(1);
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
					const ObjectSet<PreStack::Gather>& ags )
{
    if ( angledp_ )
	DPM( DataPackMgr::SeisID() ).release( angledp_->id() );

    angledp_ = new PreStack::GatherSetDataPack( 0, ags );
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


PostStackSyntheticDataWithInput::PostStackSyntheticDataWithInput(
	const SynthGenParams& sgp, SeisTrcBufDataPack& sdp )
    : PostStackSyntheticData(sgp, sdp)
{
    useGenParams( sgp );
}


PostStackSyntheticDataWithInput::~PostStackSyntheticDataWithInput()
{}


void PostStackSyntheticDataWithInput::fillGenParams( SynthGenParams& sgp ) const
{
    SyntheticData::fillGenParams( sgp );
    sgp.inpsynthnm_ = inpsynthnm_;
}


void PostStackSyntheticDataWithInput::useGenParams( const SynthGenParams& sgp )
{
    SyntheticData::useGenParams( sgp );
    inpsynthnm_ = sgp.inpsynthnm_;
}


InstAttributeSyntheticData::InstAttributeSyntheticData(
	const SynthGenParams& sgp, SeisTrcBufDataPack& sdp )
    : PostStackSyntheticDataWithInput(sgp,sdp)
{
    useGenParams( sgp );
}


void InstAttributeSyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
    PostStackSyntheticDataWithInput::fillGenParams( sgp );
    sgp.attribtype_ = attribtype_;
}


void InstAttributeSyntheticData::useGenParams( const SynthGenParams& sgp )
{
    PostStackSyntheticDataWithInput::useGenParams( sgp );
    attribtype_ = sgp.attribtype_;
}


PSBasedPostStackSyntheticData::PSBasedPostStackSyntheticData(
	const SynthGenParams& sgp, SeisTrcBufDataPack& sdp )
    : PostStackSyntheticDataWithInput(sgp,sdp)
{
    useGenParams( sgp );
}


PSBasedPostStackSyntheticData::~PSBasedPostStackSyntheticData()
{}


void PSBasedPostStackSyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
    PostStackSyntheticDataWithInput::fillGenParams( sgp );
    sgp.anglerg_ = anglerg_;
}


void PSBasedPostStackSyntheticData::useGenParams( const SynthGenParams& sgp )
{
    PostStackSyntheticDataWithInput::useGenParams( sgp );
    anglerg_ = sgp.anglerg_;
}


StratPropSyntheticData::StratPropSyntheticData( const SynthGenParams& sgp,
						    SeisTrcBufDataPack& dp,
						    const PropertyRef& pr )
    : PostStackSyntheticData( sgp, dp )
    , prop_(pr)
{}


void SyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
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


bool SynthRayModelManager::haveSameRM( const IOPar& par1,
				       const IOPar& par2 ) const
{
    uiString msg;
    PtrMan<RayTracer1D> rt1d1 = RayTracer1D::createInstance( par1, msg );
    PtrMan<RayTracer1D> rt1d2 = RayTracer1D::createInstance( par2, msg );
    if ( !rt1d1 || !rt1d2 )
	return false;
    return rt1d1->hasSameParams( *rt1d2 );
}


void SynthRayModelManager::removeRayModelSet( const IOPar& raypar )
{
    for ( int sidx=0; sidx<synthraypars_.size(); sidx++ )
    {
	IOPar synthrapar = synthraypars_[sidx];
	if ( haveSameRM(raypar,synthrapar) )
	{
	    synthraypars_.removeSingle( sidx );
	    RayModelSet* rms = raymodels_.removeSingle( sidx );
	    deepErase( *rms );
	    delete rms;
	}
    }
}


ObjectSet<SynthRayModel>* SynthRayModelManager::getRayModelSet(
				const IOPar& raypar )
{
    for ( int sidx=0; sidx<synthraypars_.size(); sidx++ )
    {
	const IOPar& synthrapar = synthraypars_[sidx];
	if ( haveSameRM(raypar,synthrapar) )
	    return raymodels_[sidx];
    }

    return nullptr;
}


void SynthRayModelManager::clearRayModels()
{
    while ( raymodels_.size() )
    {
	RayModelSet* rms = raymodels_.removeSingle( 0 );
	deepErase( *rms );
	delete rms;
    }
    synthraypars_.erase();
}


void SynthRayModelManager::addRayModelSet(
	ObjectSet<SynthRayModel>* rms, const SyntheticData* sd )
{
    if ( raymodels_.isPresent(rms) )
	return;
    SynthGenParams sgp;
    sd->fillGenParams( sgp );
    synthraypars_ += sgp.raypars_;
    raymodels_ += rms;
}
