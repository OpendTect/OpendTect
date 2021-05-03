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
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "unitofmeasure.h"
#include "timeser.h"
#include "wavelet.h"

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
  0
};


SynthGenParams::SynthGenParams()
{
    synthtype_ = ZeroOffset;	//init to avoid nasty crash in generateSD!
    setDefaultValues();
}

void SynthGenParams::setDefaultValues()
{
    anglerg_ = sDefaultAngleRange;
    raypars_.setEmpty();
    FixedString defrayparstr = sKeyAdvancedRayTracer();
    const BufferStringSet& facnms = RayTracer1D::factory().getNames();
    if ( !facnms.isEmpty() )
    {
	const int typeidx = facnms.indexOf( defrayparstr );
	FixedString facnm( typeidx>=0 ? facnms.get(typeidx) : facnms.get(0) );
	raypars_.set( sKey::Type(), facnm );
    }

    if ( synthtype_==ZeroOffset )
	RayTracer1D::setIOParsToZeroOffset( raypars_ );
    else
    {
	const StepInterval<float> offsetrg = sDefaultOffsetRange;
	TypeSet<float> offsets;
	for ( int idx=0; idx<offsetrg.nrSteps()+1; idx++ )
	    offsets += offsetrg.atIndex( idx );
	raypars_.set( RayTracer1D::sKeyOffset(), offsets );
    }
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
    par.set( sKeySynthType(), SynthGenParams::toString(synthtype_) );
    if ( synthtype_ == SynthGenParams::AngleStack ||
	 synthtype_ == SynthGenParams::AVOGradient )
    {
	par.set( sKeyInput(), inpsynthnm_ );
	par.set( sKeyAngleRange(), anglerg_ );
    }
    par.set( sKeyWaveLetName(), wvltnm_ );
    IOPar raypar;
    raypar.mergeComp( raypars_, sKeyRayPar() );
    par.merge( raypar );
}


void SynthGenParams::usePar( const IOPar& par )
{
    par.get( sKey::Name(), name_ );
    par.get( sKeyWaveLetName(), wvltnm_ );
    PtrMan<IOPar> raypar = par.subselect( sKeyRayPar() );
    if ( raypar )
	raypars_ = *raypar;
    else
	raypars_.setEmpty();

    if ( par.hasKey( sKeyIsPreStack()) )
    {
	bool isps = false;
	par.getYN( sKeyIsPreStack(), isps );
	if ( !isps && hasOffsets() )
	    synthtype_ = SynthGenParams::AngleStack;
	else if ( !isps )
	    synthtype_ = SynthGenParams::ZeroOffset;
	else
	    synthtype_ = SynthGenParams::PreStack;
    }
    else
    {
	BufferString typestr;
	parseEnum( par, sKeySynthType(), synthtype_ );
	if ( synthtype_ == SynthGenParams::AngleStack ||
	     synthtype_ == SynthGenParams::AVOGradient )
	{
	    par.get( sKeyInput(), inpsynthnm_ );
	    par.get( sKeyAngleRange(), anglerg_ );
	}
    }
}


void SynthGenParams::createName( BufferString& nm ) const
{
    if ( synthtype_==SynthGenParams::AngleStack ||
	 synthtype_==SynthGenParams::AVOGradient )
    {
	nm = SynthGenParams::toString( synthtype_ );
	nm += " ["; nm += anglerg_.start; nm += ",";
	nm += anglerg_.stop; nm += "] degrees";
	return;
    }
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


void StratSynth::clearSynthetics()
{
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
    return 0;
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
	    return true;
	}
    }

    return false;
}


SyntheticData* StratSynth::addSynthetic()
{
    SyntheticData* sd = generateSD();
    if ( sd )
    {
	int propidx = 0;
	while ( propidx<synthetics_.size() )
	{
	    if ( synthetics_[propidx]->synthType() ==
		 SynthGenParams::StratProp )
		break;
	    propidx++;
	}
	synthetics_.insertAt( sd, propidx );
    }

    return sd;
}


SyntheticData* StratSynth::addSynthetic( const SynthGenParams& synthgen )
{
    SyntheticData* sd = generateSD( synthgen );
    if ( sd )
	synthetics_ += sd;
    return sd;
}



SyntheticData* StratSynth::replaceSynthetic( int id )
{
    SyntheticData* sd = getSynthetic( id );
    if ( !sd ) return 0;

    const int sdidx = synthetics_.indexOf( sd );
    sd = generateSD();
    if ( sd )
    {
	sd->setName( synthetics_[sdidx]->name() );
	delete synthetics_.replace( sdidx, sd );
    }

    return sd;
}


SyntheticData* StratSynth::addDefaultSynthetic()
{
    genparams_.synthtype_ = SynthGenParams::ZeroOffset;
    genparams_.createName( genparams_.name_ );
    SyntheticData* sd = addSynthetic();
    return sd;
}


int StratSynth::syntheticIdx( const char* nm ) const
{
    for ( int idx=0; idx<synthetics().size(); idx ++ )
    {
	if( synthetics_[idx]->name() == nm )
	    return idx;
    }
    return 0;
}


SyntheticData* StratSynth::getSynthetic( const char* nm )
{
    for ( int idx=0; idx<synthetics().size(); idx ++ )
    {
	if ( synthetics_[idx]->name() == nm )
	    return synthetics_[idx];
    }
    return 0;
}


SyntheticData* StratSynth::getSynthetic( int id )
{
    for ( int idx=0; idx<synthetics().size(); idx ++ )
    {
	if ( synthetics_[idx]->id_ == id )
	    return synthetics_[ idx ];
    }
    return 0;
}


SyntheticData* StratSynth::getSyntheticByIdx( int idx )
{
    return synthetics_.validIdx( idx ) ?  synthetics_[idx] : 0;
}


const SyntheticData* StratSynth::getSyntheticByIdx( int idx ) const
{
    return synthetics_.validIdx( idx ) ?  synthetics_[idx] : 0;
}


int StratSynth::syntheticIdx( const PropertyRef& pr ) const
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	mDynamicCastGet(const StratPropSyntheticData*,pssd,synthetics_[idx]);
	if ( !pssd ) continue;
	if ( pr == pssd->propRef() )
	    return idx;
    }
    return 0;
}


void StratSynth::getSyntheticNames( BufferStringSet& nms, bool wantprest ) const
{
    nms.erase();
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	if ( synthetics_[idx]->isPS()==wantprest )
	    nms.add( synthetics_[idx]->name() );
    }
}


void StratSynth::getSyntheticNames( BufferStringSet& nms,
				    SynthGenParams::SynthType synthtype ) const
{
    nms.erase();
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	const SyntheticData* sd = synthetics_[idx];
	if ( sd->synthType()==synthtype )
	    nms.add( sd->name() );
    }
}


SyntheticData* StratSynth::getSynthetic( const	PropertyRef& pr )
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	mDynamicCastGet(StratPropSyntheticData*,pssd,synthetics_[idx]);
	if ( !pssd ) continue;
	if ( pr == pssd->propRef() )
	    return pssd;
    }
    return 0;
}


int StratSynth::nrSynthetics() const
{
    return synthetics_.size();
}


SyntheticData* StratSynth::generateSD()
{ return generateSD( genparams_ ); }

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


#define mCreateSeisBuf() \
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
SeisTrcBufDataPack* angledp = \
    new SeisTrcBufDataPack( dptrcbufs, Seis::Line, \
			    SeisTrcInfo::TrcNr, \
			    PostStackSyntheticData::sDataPackCategory() ); \

SyntheticData* StratSynth::createAVOGradient( const SyntheticData& sd,
					     const TrcKeyZSampling& cs,
					     const SynthGenParams& synthgenpar )
{
    mCreateDesc()
    mSetEnum(Attrib::PSAttrib::calctypeStr(),PreStack::PropCalc::LLSQ);
    mSetEnum(Attrib::PSAttrib::offsaxisStr(),PreStack::PropCalc::Sinsq);
    mSetEnum(Attrib::PSAttrib::lsqtypeStr(), PreStack::PropCalc::Coeff );

    mSetProc();
    mCreateSeisBuf();
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
    mCreateSeisBuf();
    return new AngleStackSyntheticData( synthgenpar, *angledp );
}


class ElasticModelCreator : public ParallelTask
{ mODTextTranslationClass(ElasticModelCreator);
public:
ElasticModelCreator( const Strat::LayerModel& lm, TypeSet<ElasticModel>& ems )
    : ParallelTask( "Elastic Model Generator" )
    , lm_(lm)
    , aimodels_(ems)
{
    aimodels_.setSize( lm_.size(), ElasticModel() );
}


uiString uiMessage() const
{
    if ( errmsg_.isEmpty() )
	return tr("Generating elastic model");
    else
	return errmsg_;
}

uiString uiNrDoneText() const	{ return tr("Models done"); }

protected :

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    for ( int idm=(int) start; idm<=stop; idm++ )
    {
	addToNrDone(1);
	ElasticModel& curem = aimodels_[idm];
	const Strat::LayerSequence& seq = lm_.sequence( idm );
	if ( seq.isEmpty() )
	    continue;

	if ( !fillElasticModel(seq,curem) )
	    return false;
    }

    return true;
}

bool fillElasticModel( const Strat::LayerSequence& seq, ElasticModel& aimodel )
{
    const ElasticPropSelection& eps = lm_.elasticPropSel();
    const PropertyRefSelection& props = lm_.propertyRefs();

    aimodel.erase();
    uiString errmsg;
    if ( !eps.isValidInput(&errmsg) )
    {
	mutex_.lock();
	errmsg_ = tr("Cannot create elastic model as %1").arg(errmsg);
	mutex_.unLock();
	return false;
    }

    ElasticPropGen elpgen( eps, props );
    const float srddepth = -1.f*mCast(float,SI().seismicReferenceDatum() );
    int firstidx = 0;
    if ( seq.startDepth() < srddepth )
	firstidx = seq.nearestLayerIdxAtZ( srddepth );

    if ( seq.isEmpty() )
    {
	mutex_.lock();
	errmsg_ = tr("Elastic model is not proper to generate synthetics as a "
		  "layer sequence has no layers");
	mutex_.unLock();
	return false;
    }

    for ( int idx=firstidx; idx<seq.size(); idx++ )
    {
	const Strat::Layer* lay = seq.layers()[idx];
	float thickness = lay->thickness();
	if ( idx == firstidx )
	    thickness -= srddepth - lay->zTop();
	if ( thickness < 1e-4 )
	    continue;

	float dval =mUdf(float), pval = mUdf(float), sval = mUdf(float);
	TypeSet<float> layervals; lay->getValues( layervals );
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
	errmsg_ = tr("After discarding layers with no thickness "
		     "no layers remained");
	mutex_.unLock();
	return false;
    }

    return true;
}

static float cMaximumVpWaterVel()
{ return 1510.f; }

od_int64 nrIterations() const
{ return lm_.size(); }

const Strat::LayerModel&	lm_;
TypeSet<ElasticModel>&		aimodels_;
Threads::Mutex			mutex_;
uiString			errmsg_;

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
	return 0;
    }

    const bool ispsbased =
	synthgenpar.synthtype_ == SynthGenParams::AngleStack ||
	synthgenpar.synthtype_ == SynthGenParams::AVOGradient;

    if ( synthgenpar.synthtype_ == SynthGenParams::PreStack &&
	 !swaveinfomsgshown_ )
    {
	if ( !adjustElasticModel(layMod(),aimodels_,true) )
	    return 0;
    }

    ObjectSet<SynthRayModel>* rms =
			synthrmmgr_.getRayModelSet( synthgenpar.raypars_ );
    PtrMan<Seis::RaySynthGenerator> synthgen;
    if ( rms )
	synthgen = new Seis::RaySynthGenerator( rms );
    else
	synthgen = new Seis::RaySynthGenerator( &aimodels_, false );
    if ( !ispsbased )
    {
	if ( !runSynthGen(*synthgen,synthgenpar) )
	    return 0;
    }

    SyntheticData* sd = 0;
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

	    PreStack::GatherSetDataPack* dp =
				new PreStack::GatherSetDataPack( 0, gatherset );
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
			 return 0 )

	    mDynamicCastGet(const PreStack::GatherSetDataPack*,presgdp,
			    &presd->getPack())
	    if ( !presgdp )
		mErrRet( tr(" input prestack synthetic data not found."),
			 return 0 )
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
	SeisTrcBuf* dptrcbuf = new SeisTrcBuf( true );
	synthgen->getStackedTraces( *dptrcbuf );
	SeisTrcBufDataPack* dp =
	    new SeisTrcBufDataPack( dptrcbuf, Seis::Line, SeisTrcInfo::TrcNr,
				  PostStackSyntheticData::sDataPackCategory() );
	sd = new PostStackSyntheticData( synthgenpar, *dp );
    }

    if ( !sd )
	return 0;

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


void StratSynth::generateOtherQuantities()
{
    if ( synthetics_.isEmpty() ) return;

    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	const SyntheticData* sd = synthetics_[idx];
	mDynamicCastGet(const PostStackSyntheticData*,pssd,sd);
	mDynamicCastGet(const StratPropSyntheticData*,prsd,sd);
	if ( !pssd || prsd ) continue;
	return generateOtherQuantities( *pssd, layMod() );
    }
}


mClass(WellAttrib) StratPropSyntheticDataCreator : public ParallelTask
{ mODTextTranslationClass(StratPropSyntheticDataCreator);

public:
StratPropSyntheticDataCreator( ObjectSet<SyntheticData>& synths,
		    const PostStackSyntheticData& sd,
		    const Strat::LayerModel& lm,
		    int& lastsynthid, bool useed )
    : ParallelTask( "Creating Synthetics for Properties" )
    , synthetics_(synths)
    , sd_(sd)
    , lm_(lm)
    , lastsyntheticid_(lastsynthid)
    , isprepared_(false)
    , useed_(useed)
{
}


od_int64 nrIterations() const
{ return lm_.size(); }


uiString uiMessage() const
{
    return !isprepared_ ? tr("Preparing Models") : tr("Calculating");
}

uiString uiNrDoneText() const
{
    return tr("Models done");
}


protected:

bool doPrepare( int nrthreads )
{
    const StepInterval<double>& zrg =
	sd_.postStackPack().posData().range( false );

    layermodels_.setEmpty();
    const int sz = zrg.nrSteps() + 1;
    for ( int idz=0; idz<sz; idz++ )
	layermodels_ += new Strat::LayerModel();
    const PropertyRefSelection& props = lm_.propertyRefs();
    for ( int iprop=1; iprop<props.size(); iprop++ )
    {
	SeisTrcBuf* trcbuf = new SeisTrcBuf( sd_.postStackPack().trcBuf() );
	SeisTrcBufDataPack* seisbuf =
	    new SeisTrcBufDataPack( trcbuf, Seis::Line, SeisTrcInfo::TrcNr,
				  PostStackSyntheticData::sDataPackCategory() );
	seisbufdps_ += seisbuf;
    }

    for ( int iseq=0; iseq<lm_.size(); iseq++ )
    {
	addToNrDone( 1 );
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
	    const float time = mCast( float, zrg.atIndex(idz) );
	    if ( !seqtimerg.includes(time,false) )
		continue;

	    const float dptstart = t2d.getDepth( time - (float)zrg.step );
	    const float dptstop = t2d.getDepth( time + (float)zrg.step );
	    Interval<float> depthrg( dptstart, dptstop );
	    seq.getSequencePart( depthrg, true, curseq );
	}
    }

    proplistfilter_.setEmpty();
    filterprops_ = false;
    BufferString proplistfilt( GetEnvVar("DTECT_SYNTHROCK_TIMEPROPS") );
    if ( !proplistfilt.isEmpty() )
    {
	if ( proplistfilt != sKey::None() )
	    proplistfilter_.unCat( proplistfilt.buf(), "|" );
	filterprops_ = true;
    }

    isprepared_ = true;
    resetNrDone();
    return true;
}


bool doFinish( bool success )
{
    const PropertyRefSelection& props = lm_.propertyRefs();
    SynthGenParams sgp;
    sd_.fillGenParams( sgp );
    for ( int idx=0; idx<seisbufdps_.size(); idx++ )
    {
	SeisTrcBufDataPack* dp = seisbufdps_[idx];
	BufferString propnm = props[idx+1]->name();
	if ( useed_ )
	    propnm += StratSynth::sKeyFRNameSuffix();
	BufferString nm( "[", propnm, "]" );
	dp->setName( nm );
	StratPropSyntheticData* prsd =
		 new StratPropSyntheticData( sgp, *dp, *props[idx+1] );
	prsd->id_ = ++lastsyntheticid_;
	prsd->setName( nm );

	deepCopy( prsd->zerooffsd2tmodels_, sd_.zerooffsd2tmodels_ );
	synthetics_ += prsd;
    }

    return true;
}


bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const StepInterval<double>& zrg =
	sd_.postStackPack().posData().range( false );
    const int sz = layermodels_.size();
    const PropertyRefSelection& props = lm_.propertyRefs();
    for ( int iseq=mCast(int,start); iseq<=mCast(int,stop); iseq++ )
    {
	addToNrDone( 1 );
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	const TimeDepthModel& t2d = *sd_.zerooffsd2tmodels_[iseq];
	Interval<float> seqtimerg(  t2d.getTime(seq.zRange().start),
				    t2d.getTime(seq.zRange().stop) );

	for ( int iprop=1; iprop<props.size(); iprop++ )
	{
	    const PropertyRef& pr = *props[iprop];
	    const OD::String& propnm = pr.name();
	    const PropertyRef::StdType prtype = pr.stdType();
	    const bool propisvel = prtype == PropertyRef::Vel;
	    const UnitOfMeasure* uom = UoMR().getDefault( propnm, prtype );
	    SeisTrcBufDataPack* dp = seisbufdps_[iprop-1];
	    SeisTrcBuf& trcbuf = dp->trcBuf();
	    const int bufsz = trcbuf.size();
	    SeisTrc* rawtrc = iseq < bufsz ? trcbuf.get( iseq ) : 0;
	    if ( !rawtrc )
		continue;

	    if ( filterprops_ && !proplistfilter_.isPresent(propnm) )
	    {
		rawtrc->zero();
		continue;
	    }

	    PointBasedMathFunction propvals( PointBasedMathFunction::Linear,
					     PointBasedMathFunction::EndVal );

	    for ( int idz=0; idz<sz; idz++ )
	    {
		const float time = mCast( float, zrg.atIndex(idz) );
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

		    const float userval =
			!uom ? mUdf(float) : uom->getUserValueFromSI( val );
		    propval.addValue( propisvel ? 1.f / userval : userval,
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
		const float time = mCast( float, zrg.atIndex(idz) );
		proptr.set( idz, propvals.getValue( time ) );
	    }

	    const float step = mCast( float, zrg.step );
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


    const PostStackSyntheticData&	sd_;
    const Strat::LayerModel&		lm_;
    ManagedObjectSet<Strat::LayerModel> layermodels_;
    ObjectSet<SyntheticData>&		synthetics_;
    ObjectSet<SeisTrcBufDataPack>	seisbufdps_;
    BufferStringSet			proplistfilter_;
    bool				filterprops_;
    int&				lastsyntheticid_;
    bool				isprepared_;
    bool				useed_;

};


void StratSynth::generateOtherQuantities( const PostStackSyntheticData& sd,
					  const Strat::LayerModel& lm )
{
    StratPropSyntheticDataCreator propcreator( synthetics_, sd, lm,
					       lastsyntheticid_, useed_ );
    TaskRunner::execute( taskr_, propcreator );
}


uiString StratSynth::errMsg() const
{ return errmsg_; }


uiString StratSynth::infoMsg() const
{ return infomsg_; }



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


SyntheticData::SyntheticData( const SynthGenParams& sgp, DataPack& dp )
    : NamedObject(sgp.name_)
    , datapack_(dp)
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


StratPropSyntheticData::StratPropSyntheticData( const SynthGenParams& sgp,
						    SeisTrcBufDataPack& dp,
						    const PropertyRef& pr )
    : PostStackSyntheticData( sgp, dp )
    , prop_(pr)
{}


bool SyntheticData::isAngleStack() const
{
    TypeSet<float> offsets;
    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
    return !isPS() && offsets.size()>1;
}


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

    return 0;
}


void SynthRayModelManager::clearRayModels()
{
    while ( raymodels_.size() )
    {
	RayModelSet* rms = raymodels_.removeSingle( 0 );
	deepErase( *rms );
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
