/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
________________________________________________________________________

-*/


#include "stratsynth.h"
#include "stratsynthlevel.h"
#include "syntheticdataimpl.h"

#include "angles.h"
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
#include "elasticpropsel.h"
#include "envvars.h"
#include "fftfilter.h"
#include "hilbertattrib.h"
#include "ioman.h"
#include "prestackattrib.h"
#include "prestackgather.h"
#include "prestackanglecomputer.h"
#include "propertyref.h"
#include "raytracerrunner.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "unitofmeasure.h"
#include "wavelet.h"


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


void StratSynth::clearSynthetics( bool excludeprops )
{
    if ( excludeprops )
    {
	for ( int idx=synthetics_.size()-1; idx>=0; idx-- )
	{
	    if ( !synthetics_.get(idx)->isStratProp() )
		synthetics_.removeSingle( idx );
	}
    }
    else
	synthetics_.setEmpty();
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


const SyntheticData* StratSynth::addDefaultSynthetic()
{
    return addSynthetic( SynthGenParams() );
}


const SyntheticData* StratSynth::addSynthetic( const SynthGenParams& synthgen )
{
    ConstRefMan<SyntheticData> sd = generateSD( synthgen );
    if ( !sd )
	return nullptr;

    int propidx = 0;
    while ( propidx<synthetics_.size() )
    {
	if ( synthetics_[propidx]->isStratProp() )
	    break;
	propidx++;
    }

    synthetics_.insertAt( sd.ptr(), propidx );

    return sd.ptr();
}


const SyntheticData* StratSynth::replaceSynthetic( SynthID id )
{
    const int idx = syntheticIdx( id );
    if ( !synthetics_.validIdx(idx) )
	return nullptr;

    const SyntheticData* sd = synthetics_[idx];
    const SynthGenParams synthgen( sd->getGenParams() );
    ConstRefMan<SyntheticData> newsd = generateSD( synthgen );
    if ( newsd )
    {
	synthetics_.replace( idx, newsd );
	return newsd;
    }

    return nullptr;
}


bool StratSynth::removeSynthetic( const char* nm )
{
    const int idx = syntheticIdx( nm );
    if ( synthetics_.validIdx(idx) )
    {
	synthetics_.removeSingle( idx );
	return true;
    }

    return false;
}


bool StratSynth::disableSynthetic( const char* nm )
{
    for ( const auto* sd : synthetics_ )
    {
	if ( sd->name() != nm )
	    continue;

	SynthGenParams sgp( sd->getGenParams() );
	if ( sgp.needsInput() )
	{
	    sgp.inpsynthnm_.set( SynthGenParams::sKeyInvalidInputPS() );
	    const_cast<SyntheticData*>( sd )->useGenParams( sgp );
	    return true;
	}
    }

    return false;
}


const SynthFVSpecificDispPars* StratSynth::dispPars( const char* synthnm ) const
{
    return mSelf().dispPars( synthnm );
}


SynthFVSpecificDispPars* StratSynth::dispPars( const char* synthnm )
{
    const int idx = syntheticIdx( synthnm );
    if ( !synthetics_.validIdx(idx) )
	return nullptr;

    auto* sd = const_cast<SyntheticData*>( synthetics_[idx] );
    return &sd->dispPars();
}


const SyntheticData* StratSynth::getSyntheticByIdx( int idx ) const
{
    return synthetics_.validIdx( idx ) ?  synthetics_[idx] : nullptr;
}


const SyntheticData* StratSynth::getSynthetic( const char* nm ) const
{
    const int idx = syntheticIdx( nm );
    return synthetics_.validIdx(idx) ? synthetics_[idx] : nullptr;
}


const SyntheticData* StratSynth::getSynthetic( SynthID id ) const
{
    const int idx = syntheticIdx( id );
    return synthetics_.validIdx(idx) ? synthetics_[idx] : nullptr;
}


const SyntheticData* StratSynth::getSynthetic( const PropertyRef& pr ) const
{
    const int idx = syntheticIdx( pr );
    return synthetics_.validIdx(idx) ? synthetics_[idx] :  nullptr;
}


int StratSynth::syntheticIdx( const char* nm ) const
{
    for ( int idx=0; idx<synthetics().size(); idx++ )
    {
	if ( synthetics_[idx]->name() == nm )
	    return idx;
    }

    return -1;
}


int StratSynth::syntheticIdx( SynthID id ) const
{
    for ( int idx=0; idx<synthetics().size(); idx++ )
    {
	if ( synthetics_[idx]->id_ == id )
	    return idx;
    }

    return -1;
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


const char* StratSynth::getSyntheticName( int idx ) const
{
    return synthetics_.validIdx(idx) ? synthetics_.get(idx)->name().buf()
				     : nullptr;
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
{
}


~AttributeSyntheticCreator()
{
    delete descset_;
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
    delete descset_;
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
    Attrib::DescSet*			descset_ = nullptr;
    TrcKeyZSampling			tkzs_;
    TaskRunner*				taskr_ = nullptr;
    uiString				msg_;
};


ConstRefMan<SyntheticData> StratSynth::createAttribute( const SyntheticData& sd,
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

    ConstRefMan<SyntheticData> ret =
	new InstAttributeSyntheticData( synthgenpar, sd.synthGenDP(), *dpname );
    return ret;
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

	ConstRefMan<SyntheticData> sd =
	    new InstAttributeSyntheticData( pars, insd->synthGenDP(), *dpname );
	if ( sd )
	{
	    const_cast<SyntheticData*>( sd.ptr() )->id_ = ++lastsyntheticid_;
	    synthetics_.insertAt( sd, propidx++ );
	}
	else
	    mErrRet( tr(" synthetic data not created."), return false )
    }

    return true;
}


ConstRefMan<SyntheticData> StratSynth::createAVOGradient(
					      const SyntheticData& sd,
					      const TrcKeyZSampling& tkzs,
					      const SynthGenParams& sgp )
{
    mCreateDesc()
    mSetEnum(Attrib::PSAttrib::calctypeStr(),PreStack::PropCalc::LLSQ);
    mSetEnum(Attrib::PSAttrib::offsaxisStr(),PreStack::PropCalc::Sinsq);
    mSetEnum(Attrib::PSAttrib::lsqtypeStr(), PreStack::PropCalc::Coeff );

    mSetProc();
    mCreateSeisBuf( angledp );
    ConstRefMan<SyntheticData> ret =
		new AVOGradSyntheticData( sgp, sd.synthGenDP(), *angledp );
    return ret;
}


ConstRefMan<SyntheticData> StratSynth::createAngleStack(
					     const SyntheticData& sd,
					     const TrcKeyZSampling& tkzs,
					     const SynthGenParams& sgp )
{
    mCreateDesc();
    mSetEnum(Attrib::PSAttrib::calctypeStr(),PreStack::PropCalc::Stats);
    mSetEnum(Attrib::PSAttrib::stattypeStr(), Stats::Average );

    mSetProc();
    mCreateSeisBuf( angledp );
    ConstRefMan<SyntheticData> ret =
		new AngleStackSyntheticData( sgp, sd.synthGenDP(), *angledp );
    return ret;
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


class PSAngleDataCreator : public ParallelTask
{ mODTextTranslationClass(PSAngleDataCreator)
public:

PSAngleDataCreator( PreStackSyntheticData& pssd )
    : ParallelTask("Creating Angle Gather" )
    , pssd_(pssd)
    , seisgathers_(const_cast<const ObjectSet<PreStack::Gather>&>(
			pssd.preStackPack().getGathers()))
    , refmodels_(const_cast<const ReflectivityModelSet&>(
			pssd.synthGenDP().getModels()))
{
    totalnr_ = refmodels_.nrModels();
}


~PSAngleDataCreator()
{
    deepErase( anglecomputers_ );
    deleteGatherSets();
}

uiString uiMessage() const override
{
    return msg_;
}

uiString uiNrDoneText() const override
{
    return tr( "Models done" );
}

private:

od_int64 nrIterations() const override
{ return totalnr_; }


void deleteGatherSets()
{
    for ( auto* gathersset : anglegathers_ )
	deepErase( *gathersset );
    deepErase( anglegathers_ );
}


bool doPrepare( int nrthreads ) override
{
    msg_ =  tr("Calculating Angle Gathers");

    if ( !pssd_.isOK() )
	return false;

    deepErase( anglecomputers_ );
    deleteGatherSets();
    for ( int ithread=0; ithread<nrthreads; ithread++ )
    {
	auto* anglecomputer = new PreStack::ModelBasedAngleComputer;
	anglecomputer->setRayTracerPars( pssd_.getGenParams().raypars_ );
	anglecomputer->setFFTSmoother( 10.f, 15.f );
	anglecomputers_.add( anglecomputer );
	anglegathers_.add( new ObjectSet<PreStack::Gather> );
    }

    return true;
}


bool doWork( od_int64 start, od_int64 stop, int threadid ) override
{
    if ( !anglecomputers_.validIdx(threadid) ||
	 !anglegathers_.validIdx(threadid) )
	return false;

    PreStack::ModelBasedAngleComputer& anglecomputer =
				       *anglecomputers_.get( threadid );
    ObjectSet<PreStack::Gather>& anglegathers =
				       *anglegathers_.get( threadid );

    for ( int idx=int(start); idx<=stop; idx++, addToNrDone(1) )
    {
	const ReflectivityModelBase* refmodel = refmodels_.get( idx );
	if ( !refmodel->isOffsetDomain() )
	    return false;

	mDynamicCastGet(const OffsetReflectivityModel*,offrefmodel,refmodel);
	const PreStack::Gather& seisgather = *seisgathers_[idx];
	anglecomputer.setOutputSampling( seisgather.posData() );
	anglecomputer.setGatherIsNMOCorrected( seisgather.isCorrected() );
	anglecomputer.setRefModel( *offrefmodel, seisgather.getTrcKey() );
	PreStack::Gather* anglegather = anglecomputer.computeAngles();
	if ( !anglegather )
	    return false;

	convertAngleDataToDegrees( *anglegather );
	TypeSet<float> azimuths;
	seisgather.getAzimuths( azimuths );
	anglegather->setAzimuths( azimuths );
	const BufferString angledpnm( pssd_.name(), "(Angle Gather)" );
	anglegather->setName( angledpnm );
	anglegathers.add( anglegather );
    }

    return true;
}


bool doFinish( bool success ) override
{
    deepErase( anglecomputers_ );
    if ( success )
    {
	ObjectSet<PreStack::Gather> anglegathers;
	for ( auto* gathersset : anglegathers_ )
	{
	    anglegathers.append( *gathersset );
	    gathersset->setEmpty();
	}

	deepErase( anglegathers_ );
	pssd_.setAngleData( anglegathers );
    }
    else
	deleteGatherSets();

    return success;
}


static void convertAngleDataToDegrees( PreStack::Gather& ag )
{
    const auto& agdata = const_cast<const Array2D<float>&>( ag.data() );
    ArrayMath::getScaledArray<float>( agdata, nullptr, mRad2DegD, 0.,
				      false, true );
}

    PreStackSyntheticData&		pssd_;
    const ObjectSet<PreStack::Gather>&	seisgathers_;
    const ReflectivityModelSet&		refmodels_;
    ObjectSet<ObjectSet<PreStack::Gather> >	anglegathers_;
    ObjectSet<PreStack::ModelBasedAngleComputer> anglecomputers_;
    uiString				msg_;
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
    if ( !wvltnm.isEmpty() )
    {
	PtrMan<IOObj> ioobj = Wavelet::getIOObj( wvltnm );
	PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
	if ( wvlt  )
	    synthgen.setWavelet( wvlt, OD::CopyPtr );
    }

    if ( !sgp.synthpars_.isEmpty() )
	synthgen.usePar( sgp.synthpars_ );

    return TaskRunner::execute( taskr_, synthgen );
}


ConstRefMan<SyntheticData> StratSynth::generateSD( const SynthGenParams& sgp )
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

    ConstRefMan<SyntheticData> sd;
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
	    const PreStack::GatherSetDataPack* anglegather =
				getRelevantAngleData( sd->synthGenDP() );
	    mDynamicCastGet(const PreStackSyntheticData*,presdc,sd.ptr());
	    auto* presd = const_cast<PreStackSyntheticData*>( presdc );
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
	const_cast<SyntheticData*>( sd.ptr() )->setName( sdnm );
    }

    const_cast<SyntheticData*>( sd.ptr() )->id_ = ++lastsyntheticid_;
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


mClass(WellAttrib) StratSeqSplitter : public ParallelTask
{ mODTextTranslationClass(StratSeqSplitter);

public:
StratSeqSplitter( const Strat::LayerModel& lm, const PostStackSyntheticData& sd,
		  double zstep, ObjectSet<Strat::LayerModel>& layermodels )
    : ParallelTask("Splitting layermodel")
    , lm_(lm)
    , sd_(sd)
    , layermodels_(layermodels)
    , zrg_(sd.postStackPack().posData().range(false))
    , totalnr_(lm.size())
{
    zrg_.step = zstep;
    msg_ = tr("Preparing Models");
}


uiString uiMessage() const override
{
    return msg_;
}


uiString uiNrDoneText() const
{
    return tr("Models done");
}

private:

od_int64 nrIterations() const override
{ return totalnr_; }

bool doPrepare( int /* nrthreads */ ) override
{
    const int nrlm = zrg_.nrSteps() + 1;
    const int nrmodels = lm_.size();

    layermodels_.setEmpty();
    for ( int idz=0; idz<nrlm; idz++ )
    {
	auto* layermodel = new Strat::LayerModel;
	for ( int iseq=0; iseq<nrmodels; iseq++ )
	    layermodel->addSequence();

	layermodels_.add( layermodel );
    }

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int /* threadidx */ ) override
{
    const int nrlm = zrg_.nrSteps()+1;

    for ( int iseq=mCast(int,start); iseq<=mCast(int,stop); iseq++,
							    addToNrDone(1) )
    {
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	const TimeDepthModel* t2d = sd_.getTDModel( iseq );
	if ( !t2d )
	    return false;

	const Interval<float> seqdepthrg = seq.zRange();
	const float seqstarttime = t2d->getTime( seqdepthrg.start );
	const float seqstoptime = t2d->getTime( seqdepthrg.stop );
	const Interval<float> seqtimerg( seqstarttime, seqstoptime );
	for ( int idz=0; idz<nrlm; idz++ )
	{
	    Strat::LayerSequence& curseq =
				  layermodels_.get( idz )->sequence( iseq );
	    const float time = mCast( float, zrg_.atIndex(idz) );
	    if ( !seqtimerg.includes(time,false) )
		continue;

	    const float dptstart = t2d->getDepth( time - (float)zrg_.step );
	    const float dptstop = t2d->getDepth( time + (float)zrg_.step );
	    const Interval<float> depthrg( dptstart, dptstop );
	    seq.getSequencePart( depthrg, true, curseq );
	}
    }

    return true;
}

    const Strat::LayerModel&	lm_;
    const PostStackSyntheticData& sd_;
    ObjectSet<Strat::LayerModel>& layermodels_;
    StepInterval<double>	zrg_;
    const od_int64		totalnr_;
    uiString			msg_;

};


mClass(WellAttrib) StratPropSyntheticDataCreator : public ParallelTask
{ mODTextTranslationClass(StratPropSyntheticDataCreator);

public:
StratPropSyntheticDataCreator( RefObjectSet<const SyntheticData>& synths,
		    const PostStackSyntheticData& sd,
		    const Strat::LayerModel& lm,
		    const ObjectSet<Strat::LayerModel>& layermodels,
		    int& lastsynthid, bool useed, double zstep,
		    const BufferStringSet* proplistfilter )
    : ParallelTask( "Creating Synthetics for Properties" )
    , synthetics_(synths)
    , lm_(lm)
    , sd_(sd)
    , layermodels_(layermodels)
    , lastsyntheticid_(lastsynthid)
    , useed_(useed)
    , zrg_(sd.postStackPack().posData().range(false))
    , totalnr_(lm.size())
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

    msg_ = tr("Converting depth layer model to time traces");
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
{ return totalnr_; }


bool doPrepare( int /* nrthreads */ ) override
{
    const int sz = zrg_.nrSteps()+1;
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

    return true;
}


bool doWork( od_int64 start, od_int64 stop, int /* threadid */ ) override
{
    const int sz = layermodels_.size();
    const PropertyRefSelection& props = lm_.propertyRefs();
    for ( int iseq=mCast(int,start); iseq<=mCast(int,stop); iseq++,
							    addToNrDone(1) )
    {
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	const Interval<float> seqtimerg(  sd_.getTime(seq.zRange().start,iseq),
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
    }

    return true;
}


bool doFinish( bool success ) override
{
    if ( !success )
	return false;

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
	ConstRefMan<SyntheticData> prsd =
		new StratPropSyntheticData( sgp, sd_.synthGenDP(), *dp, *pr );
	const_cast<SyntheticData*>( prsd.ptr() )->id_ = ++lastsyntheticid_;
	synthetics_ += prsd.ptr();
    }

    return true;
}

    const Strat::LayerModel&		lm_;
    const PostStackSyntheticData&	sd_;
    PropertyRefSelection		prs_;
    StepInterval<double>		zrg_;
    const ObjectSet<Strat::LayerModel>& layermodels_;
    RefObjectSet<const SyntheticData>&	synthetics_;
    ObjectSet<SeisTrcBufDataPack>	seisbufdps_;
    int&				lastsyntheticid_;
    bool				useed_;
    const od_int64			totalnr_;
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

    ManagedObjectSet<Strat::LayerModel> layermodels;
    StratSeqSplitter splitter( lm, sd, zstep, layermodels );
    if ( !TaskRunner::execute(taskr_,splitter) )
	return;

    StratPropSyntheticDataCreator propcreator( synthetics_, sd, lm,
				layermodels, lastsyntheticid_, useed_, zstep,
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
    const SyntheticData* sd = getSynthetic( sdnm );
    if ( !sd || sd->isPS() )
	return false;

    mDynamicCastGet(const PostStackSyntheticData*,postsd,sd);
    if ( !postsd )
	return false;

    auto& trcs = const_cast<SeisTrcBuf&>( postsd->postStackPack().trcBuf() );
    getLevelTimes( sd->synthGenDP().getModels(), dispeach, trcs, offsetidx );
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
	if ( !genpars || genpars->isStratProp() )
	    continue;

	nrvalidpars++;
	addSynthetic( *genpars );
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

