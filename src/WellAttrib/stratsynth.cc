/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratsynth.h"
#include "stratsynthlevel.h"
#include "syntheticdataimpl.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribfactory.h"
#include "attribprocessor.h"
#include "attribsel.h"
#include "attribsel.h"
#include "binidvalset.h"
#include "elasticpropsel.h"
#include "envvars.h"
#include "fftfilter.h"
#include "hilbertattrib.h"
#include "ioman.h"
#include "keystrs.h"
#include "prestackanglecomputer.h"
#include "prestackgather.h"
#include "prestackprop.h"
#include "propertyref.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "statruncalc.h"
#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "wavelet.h"


int StratSynth::DataMgr::gtActualLMIdx( int lmsidx ) const
{
    if ( lmsidx < 0 || lmsidx >= lms_.size() )
	return lms_.curIdx();
    if ( lmsidx >= lms_.size() )
	{ pErrMsg( "Bad idx" ); return lms_.curIdx(); }

    return lmsidx;
}


StratSynth::DataMgr::DataMgr( const Strat::LayerModelSuite& lms )
    : lms_(lms)
    , entryAdded(this)
    , entryRenamed(this)
    , entryChanged(this)
    , entryDisabled(this)
    , entryRemoved(this)
    , elPropSelChanged(this)
    , newWvltUsed(this)
    , wvltScalingDone(this)
{
    for ( int idx=0; idx<nrLayerModels(); idx++ )
	addLayModelSets();

    mAttachCB( lms_.editingChanged, DataMgr::lmsEdChgCB );
}


StratSynth::DataMgr::DataMgr( const DataMgr& oth, int calceach )
    : lms_(oth.lms_)
    , calceach_(calceach)
    , entryAdded(this)
    , entryRenamed(this)
    , entryChanged(this)
    , entryDisabled(this)
    , entryRemoved(this)
    , elPropSelChanged(this)
    , newWvltUsed(this)
    , wvltScalingDone(this)
{
    ids_ = oth.ids_;
    genparams_ = oth.genparams_;
    wvlt_ = oth.wvlt_;
    for ( int idx=0; idx<nrLayerModels(); idx++ )
	addLayModelSets();

    mAttachCB( lms_.editingChanged, DataMgr::lmsEdChgCB );
}


StratSynth::DataMgr::~DataMgr()
{
    detachAllNotifiers();
    setEmpty();
    deepErase( lmdatasets_ );
}


StratSynth::DataMgr* StratSynth::DataMgr::getProdMgr()
{
    if ( calceach_ < 2 )
	return this;

    return new DataMgr( *this, 1 );
}


const StratSynth::DataMgr* StratSynth::DataMgr::getProdMgr() const
{
    return mSelf().getProdMgr();
}


bool StratSynth::DataMgr::isEmpty() const
{
    return ids_.isEmpty();
}


void StratSynth::DataMgr::clearData( bool lmdata, bool synthdata )
{
    if ( lmdata )
    {
	deepErase( elasticmodelsets_ );
	deepErase( levelsets_ );
    }

    const TypeSet<SynthID> idstonotif = ids_;
    if ( synthdata )
    {
	if ( lmdata )
	{
	    ids_.setEmpty();
	    genparams_.setEmpty();
	}
	else
	{
	    for ( int idx=ids_.size()-1; idx>=0; idx-- )
	    {
		if ( genparams_[idx].isStratProp() )
		    continue;

		ids_.removeSingle( idx );
		genparams_.removeSingle( idx );
	    }
	}
    }
    else
    {
	const PropertyRefSelection allprs( false, nullptr );
	const PropertyRefSelection& prs = layerModel().propertyRefs();
	for ( int idx=ids_.size()-1; idx>=0; idx-- )
	{
	    if ( !genparams_[idx].isStratProp() )
		continue;

	    const PropertyRef* pr = genparams_[idx].getRef( allprs );
	    if ( !pr || !prs.isPresent(pr) )
	    {
		ids_.removeSingle( idx );
		genparams_.removeSingle( idx );
	    }
	}
    }

    deepErase( lmdatasets_ );
    for ( int idx=0; idx<nrLayerModels(); idx++ )
	addLayModelSets( lmdata );

    dirtycount_++;
    if ( !idstonotif.isEmpty() )
	entryRemoved.trigger( idstonotif );
}


void StratSynth::DataMgr::addLayModelSets( bool withmod )
{
    if ( withmod )
    {
	elasticmodelsets_.add( new ElasticModelSet() );
	levelsets_.add( new LevelSet() );
    }

    auto* dss = new RefObjectSet<const SyntheticData>();
    dss->setNullAllowed( true );
    lmdatasets_.add( dss );
    while ( dss->size() < ids_.size() )
	dss->add( nullptr );
}


void StratSynth::DataMgr::lmsEdChgCB( CallBacker* )
{
    const int nrnow = lmdatasets_.size();
    for ( int idx=nrnow-1; idx>=1; idx-- )
    {
	delete elasticmodelsets_.removeSingle( idx );
	delete levelsets_.removeSingle( idx );
	delete lmdatasets_.removeSingle( idx );
    }

    const int nrneeded = lms_.size();
    if ( nrneeded < 2 )
	return;

    for ( int idx=1; idx<nrneeded; idx++ )
	addLayModelSets();
}


const Strat::LayerModel& StratSynth::DataMgr::layerModel( int lmsidx ) const
{
    return lms_.get( gtActualLMIdx(lmsidx) );
}


const ElasticModelSet& StratSynth::DataMgr::elasticModels( int lmsidx ) const
{
    return *elasticmodelsets_.get( gtActualLMIdx(lmsidx) );
}


const StratSynth::LevelSet& StratSynth::DataMgr::levels( int lmsidx ) const
{
    return *levelsets_.get( gtActualLMIdx(lmsidx) );
}


int StratSynth::DataMgr::nrLayerModels() const
{
    return lms_.size();
}


int StratSynth::DataMgr::curLayerModelIdx() const
{
    return lms_.curIdx();
}


ObjectSet<const SyntheticData>& StratSynth::DataMgr::gtDSS( int lmsidx )
{
    return *lmdatasets_.get( gtActualLMIdx(lmsidx) );
}


const ObjectSet<const SyntheticData>& StratSynth::DataMgr::gtDSS(
							     int lmsidx ) const
{
    return *lmdatasets_.get( gtActualLMIdx(lmsidx) );
}


void StratSynth::DataMgr::setCalcEach( int newce )
{
    if ( newce < 1 )
	newce = 1;
    if ( newce == calceach_ )
	return;

    calceach_ = newce;
    modelChange();
}


int StratSynth::DataMgr::iSeq( int itrc ) const
{
    return itrc * calceach_;
}


int StratSynth::DataMgr::iTrc( int iseq ) const
{
    return iseq / calceach_;
}


int StratSynth::DataMgr::nrSequences( int lmsidx ) const
{
    return layerModel( lmsidx ).size();
}


namespace StratSynth
{

static int getNrTraces( int nrseq, int calceach )
{
    if ( nrseq < 1 )
	return 0;
    if ( calceach == 1 )
	return nrseq;

    int ret = 0;
    for ( int idx=0; idx<nrseq; idx+=calceach, ret++ )
    {}

    return ret;
}

} // namespace StratSynth


int StratSynth::DataMgr::nrTraces( int lmsidx ) const
{
    return getNrTraces( nrSequences(lmsidx), calceach_ );
}


BufferString StratSynth::DataMgr::getFinalDataSetName( const char* gpnm,
						bool isprop, int lmsidx ) const
{
    BufferString ret( gpnm );
    if ( isprop )
	ret.embed( '[', ']' );
    if ( lmsidx < 0 )
	lmsidx = lms_.curIdx();

    const BufferString lmdesc( lms_.desc(lms_.curIdx()) );
    if ( !lmdesc.isEmpty() )
	ret.add( " (" ).add( lmdesc ).add( ")" );
    return ret;
}


void StratSynth::DataMgr::fillPar( IOPar& iop,
				   const ObjectSet<IOPar>* disppars ) const
{
    const ElasticPropSelection& elpropsel = layerModel().elasticPropSel();
    if ( !elpropsel.isEmpty() && elpropsel.isOK(&layerModel().propertyRefs()) )
    {
	if ( requiredRefLayerType() > RefLayer::Acoustic ||
	     !elpropsel.getByType(ElasticFormula::SVel) )
	    elpropsel.fillPar( iop );
	else
	{
	    iop.removeSubSelection( ElasticPropSelection::sKeyElasticProp() );
	    ElasticPropSelection aipropsel( elpropsel );
	    aipropsel -= aipropsel.getByType( ElasticFormula::SVel );
	    aipropsel.fillPar( iop );
	}
    }

    PtrMan<IOPar> par = iop.subselect( sKeySynthetics() );
    if ( !par )
	par = new IOPar;

    int nr_nonproprefsynths = 0;
    for ( int idx=0; idx<genparams_.size(); idx++ )
    {
	if ( genparams_[idx].isStratProp() )
	    continue;

	IOPar sdpar;
	genparams_[idx].fillPar( sdpar );
	if ( disppars && disppars->validIdx(nr_nonproprefsynths) )
	{
	    const IOPar* disppar = disppars->get( nr_nonproprefsynths );
	    if ( disppar && !disppar->isEmpty() )
		sdpar.addFrom( *disppar );
	}

	par->mergeComp( sdpar,
		      IOPar::compKey(sKeySyntheticNr(),nr_nonproprefsynths++) );
    }

    par->set( sKeyNrSynthetics(), nr_nonproprefsynths );
    iop.mergeComp( *par.ptr(), sKeySynthetics() );
}


bool StratSynth::DataMgr::usePar( const IOPar& iop )
{
    PtrMan<IOPar> synthpar = iop.subselect( sKeySynthetics() );
    if ( synthpar )
	setElasticProperties( iop );

    const IOPar& par = synthpar ? *synthpar.ptr() : iop;

    ManagedObjectSet<SynthGenParams> genparsset;
    genparsset.setNullAllowed();
    if ( !getAllGenPars(par,genparsset) || genparsset.isEmpty() )
	return false;

    uiRetVal uirv, infos;
    int nrvalidpars = 0, nradded = 0;
    for ( const auto* genpars : genparsset )
    {
	if ( !genpars || genpars->isStratProp() )
	    continue;

	nrvalidpars++;
	if ( addSynthetic(*genpars).isValid() )
	    nradded++;

	if ( !errmsg_.isOK() )
	    uirv.add( errmsg_ );
	if ( !infomsg_.isOK() )
	    infos.add( infomsg_ );
    }

    if ( !uirv.isOK() )
	errmsg_ = uirv;
    if ( !infos.isOK() )
	infomsg_ = infos;

    return nradded == nrvalidpars;
}


RefLayer::Type StratSynth::DataMgr::requiredRefLayerType() const
{
    RefLayer::Type ret = RefLayer::Acoustic;
    for ( const auto& sgp : genparams_ )
    {
	const RefLayer::Type* sgptyp = sgp.requiredRefLayerType();
	if ( sgptyp && *sgptyp > ret )
	    ret = *sgptyp;
	if ( ret == RefLayer::HTI )
	    break;
    }

    return ret;
}


bool StratSynth::DataMgr::setElasticProperties( const IOPar& iop, uiString* msg)
{
    const RefLayer::Type reqtype = requiredRefLayerType();
    ElasticPropSelection elpropsel( RefLayer::Acoustic );
    if ( elpropsel.usePar(iop) &&
	 checkElasticPropSel(elpropsel,&reqtype,msg) )
    {
	setElasticPropSel( elpropsel );
	return true;
    }

    if ( !iop.isPresent(ElasticPropSelection::sKeyElasticPropSel()) )
	return false;

    IOPar par( iop );
    par.removeWithKey( ElasticPropSelection::sKeyElasticPropSel() );
    elpropsel = ElasticPropSelection( RefLayer::Acoustic );
    if ( !elpropsel.usePar(iop) ||
	 !checkElasticPropSel(elpropsel,&reqtype,msg) )
	return false;

    setElasticPropSel( elpropsel );
    return true;
}


void StratSynth::DataMgr::setElasticPropSel(
					const ElasticPropSelection& elpropsel )
{
    const_cast<Strat::LayerModelSuite&>( lms_ ).setElasticPropSel(elpropsel);
    elPropSelChanged.trigger();
    if ( elPropSelChanged.isEnabled() )
	modelChange();
}


bool StratSynth::DataMgr::checkElasticPropSel(
				const ElasticPropSelection& elpropsel,
				const RefLayer::Type* reqtypein,
				uiString* errmsg ) const
{
    const RefLayer::Type reqtype = reqtypein ? *reqtypein
					     : requiredRefLayerType();
    const PropertyRefSelection& prs = layerModel().propertyRefs();
    TypeSet<ElasticFormula::Type> reqtypes;
    reqtypes += ElasticFormula::Den;
    reqtypes += ElasticFormula::PVel;
    if ( reqtype > RefLayer::Acoustic )
	reqtypes += ElasticFormula::SVel;
    if ( reqtype > RefLayer::Elastic )
	reqtypes += ElasticFormula::FracRho;
    if ( reqtype > RefLayer::VTI )
	reqtypes += ElasticFormula::FracAzi;

    return elpropsel.isOK( reqtypes, prs, errmsg );
}


int StratSynth::DataMgr::gtIdx( SynthID id ) const
{
    return ids_.indexOf( id );
}


const SyntheticData* StratSynth::DataMgr::gtDS( SynthID id, int lmsidx ) const
{
    return gtDSByIdx( gtIdx(id), lmsidx );
}


const SyntheticData* StratSynth::DataMgr::gtDSByIdx( int idx, int lmsidx ) const
{
    const ObjectSet<const SyntheticData>& dss = gtDSS( lmsidx );
    return dss.validIdx(idx) ? dss[idx] : nullptr;
}


const SyntheticData* StratSynth::DataMgr::gtDSByName( const char* nm,
						      int lmsidx ) const
{
    const SynthID id = find( nm );
    return id.isValid() ? gtDS( id, lmsidx ) : nullptr;
}


bool StratSynth::DataMgr::checkNeedsInput( const SynthGenParams& sgp ) const
{
    if ( !sgp.needsInput() )
	return true;

    const SynthID inpid = find( sgp.inpsynthnm_ );
    const SynthGenParams* inpsgp = getGenParams( inpid );
    if ( !inpsgp )
	return false;

    if ( (sgp.isPSBased() && !inpsgp->isPreStack()) ||
	 (sgp.isAttribute() && !inpsgp->canBeAttributeInput()) )
	return false;

    return true;
}


SynthID StratSynth::DataMgr::addSynthetic( const SynthGenParams& sgp )
{
    if ( !checkNeedsInput(sgp) )
	return SynthID::udf();

    return addEntry( SyntheticData::getNewID(), sgp );
}


bool StratSynth::DataMgr::addPropertySynthetics( TypeSet<SynthID>* retids,
					       const BufferStringSet* proplist )
{
    if ( !GetEnvVarYN("DTECT_STRAT_MAKE_PROPERTYTRACES",true) )
	return true;

    const BufferString propliststr(
				GetEnvVar("DTECT_SYNTHROCK_TIMEPROPS") );
    if ( !propliststr.isEmpty() && propliststr == sKey::None() )
	return true;

    BufferStringSet proplistfilter;
    if ( !propliststr.isEmpty() )
	proplistfilter.unCat( propliststr.buf(), "|" );
    else if ( proplist )
	proplistfilter = *proplist;

    const bool filterprops = !proplistfilter.isEmpty();

    SynthGenParams sgp( SynthGenParams::StratProp );
    TypeSet<SynthID> synthids;
    const PropertyRefSelection& prs = layerModel().propertyRefs();
    for ( const auto* pr : prs )
    {
	if ( pr->isThickness() ||
	    (filterprops && !proplistfilter.isPresent(pr->name())) )
	    continue;

	sgp.name_ = pr->name();
	sgp.name_.embed( '[', ']' );
	const int idx = genparams_.indexOf( sgp );
	synthids += ids_.validIdx(idx) ? ids_[idx]
		  : addEntry( SyntheticData::getNewID(), sgp );
    }

    if ( retids )
	*retids = synthids;

    return !synthids.isEmpty();
}


bool StratSynth::DataMgr::addInstAttribSynthetics(
			const SynthID& inpid,
			const TypeSet<Attrib::Instantaneous::OutType>& attrdefs,
			TypeSet<SynthID>& retids )
{
    const SynthGenParams* inpsgp = getGenParams( inpid );
    if ( !inpsgp || !checkNeedsInput(*inpsgp) )
	return false;

    SynthGenParams sgp( SynthGenParams::InstAttrib );
    sgp.inpsynthnm_.set( nameOf(inpid) );
    for ( const auto& attrdef : attrdefs )
    {
	sgp.attribtype_ = attrdef;
	sgp.createName( sgp.name_ );
	retids += addEntry( SyntheticData::getNewID(), sgp );
    }

    return !attrdefs.isEmpty();
}


SynthID StratSynth::DataMgr::addEntry( SynthID id, const SynthGenParams& sgp )
{
    if ( genparams_.isPresent(sgp) )
	return id;

    ids_ += id;
    genparams_ += sgp;
    for ( int ilm=0; ilm<lms_.size(); ilm++ )
	*lmdatasets_.get(ilm) += nullptr;

    if ( sgp.isRawOutput() )
	ensureAdequatePropSelection( -1, *sgp.requiredRefLayerType() );

    dirtycount_++;
    entryAdded.trigger( id );
    return id;
}


void StratSynth::DataMgr::setDataSet( const SynthGenParams& sgp,
				      const SyntheticData* sd, int lmsidx )
{
    SynthID oldid = find( sgp.name_ );
    lmsidx = gtActualLMIdx( lmsidx );
    bool ischgd = true;
    if ( oldid.isValid() )
    {
	const_cast<SyntheticData*>( sd )->setID( oldid );
	const int idx = ids_.indexOf( oldid );
	ischgd = genparams_[idx] != sgp;
	genparams_[idx] = sgp;
	lmdatasets_.get(lmsidx)->replace( idx, sd );
    }
    else
    {
	const_cast<SyntheticData*>( sd )->setID( SyntheticData::getNewID() );
	ids_ += sd->id();
	genparams_ += sgp;
	for ( int ilm=0; ilm<lms_.size(); ilm++ )
	    lmdatasets_.get(ilm)->add( ilm == lmsidx ? sd : nullptr );
    }

    if ( ischgd )
    {
	dirtycount_++;
	if ( oldid.isValid() )
	{
	    const TypeSet<SynthID> ids( sd->id() );
	    entryChanged.trigger( ids );
	}
	else
	    entryAdded.trigger( sd->id() );
    }
}


bool StratSynth::DataMgr::updateSynthetic( SynthID id,
					   const SynthGenParams& sgp )
{
    TypeSet<SynthID> handledids;
    if ( id.isValid() )
    {
	const int idx = gtIdx( id );
	if ( genparams_.validIdx(idx) )
	{
	    const BufferString oldnm( genparams_[idx].name_ );
	    genparams_[idx] = sgp;
	    handledids += id;
	    for ( int ilm=0; ilm<nrLayerModels(); ilm++ )
		lmdatasets_[ilm]->replace( idx, nullptr );

	    const char* newnm = sgp.name_.buf();
	    const BufferString inpnmlbl = getFinalDataSetName( oldnm, true, 0 );
	    const BufferString newinpnmlbl =
					  getFinalDataSetName( newnm, true, 0 );
	    TypeSet<SynthID> inpids;
	    getIDs( inpids, OnlyWithInput );
	    NotifyStopper ns( entryChanged );
	    for ( const auto& inpid : inpids )
	    {
		const SynthGenParams& inpsgp = *getGenParams( inpid );
		if ( !inpsgp.name_.contains(inpnmlbl.buf()) &&
						inpsgp.inpsynthnm_ != oldnm )
		    continue;

		SynthGenParams newsgp = inpsgp;
		newsgp.name_.replace( inpnmlbl.buf(), newinpnmlbl.buf() );
		if ( newsgp.inpsynthnm_ == oldnm ||
		     newsgp.inpsynthnm_ == SynthGenParams::sKeyInvalidInputPS())
		    newsgp.inpsynthnm_ = newnm;

		updateSynthetic( inpid, newsgp );
		handledids += inpid;
	    }
	}
	else
	{
	    pErrMsg( "updateSynthetic should be called for new synthetics" );
	    addEntry( id, sgp );
	}
    }
    else
    {
	pErrMsg( "updateSynthetic should be called for new synthetics" );
	id = addSynthetic( sgp );
    }

    dirtycount_++;
    entryChanged.trigger( handledids );
    return true;
}


bool StratSynth::DataMgr::updateWavelet( const MultiID& oldwvltid,
					 const MultiID& newwvltid )
{
    TypeSet<int> idxs;
    gtIdxs( oldwvltid, idxs, false, lms_.curIdx() );
    if ( idxs.isEmpty() )
	return false;

    PtrMan<IOObj> ioobj = IOM().get( newwvltid );
    if ( !ioobj )
	return false;

    PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
    if ( !wvlt )
	return false;

    newWvltUsed.trigger( newwvltid );

    const BufferString wvltnm( wvlt->name() );
    for ( const auto& idx : idxs )
    {
	SynthGenParams sgp( genparams_[idx] );
	BufferStringSet synthnms( sgp.name_.buf() );
	sgp.setWavelet( wvltnm );
	sgp.createName( sgp.name_ );
	synthnms.add( sgp.name_ );
	updateSynthetic( ids_[idx], sgp );
	wvltScalingDone.trigger( synthnms );
    }

    return true;
}


bool StratSynth::DataMgr::updateSyntheticName( SynthID id, const char* newnm )
{
    const int idx = gtIdx( id );
    if ( !genparams_.validIdx(idx) )
	return false;

    TypeSet<SynthID> handledids;
    const BufferString oldnm( genparams_[idx].name_ );
    genparams_[idx].name_.set( newnm );
    handledids += id;
    for ( int ilm=0; ilm<nrLayerModels(); ilm++ )
    {
	const SyntheticData* sd = gtDSByIdx( idx, ilm );
	if ( sd )
	{
	    const BufferString dsnm = getFinalDataSetName( newnm,
					genparams_[idx].isStratProp(), ilm );
	    const_cast<SyntheticData*>( sd )->setName( dsnm );
	}
    }

    const BufferString inpnmlbl = getFinalDataSetName( oldnm, true, 0 );
    const BufferString newinpnmlbl = getFinalDataSetName( newnm, true, 0 );
    TypeSet<SynthID> inpids;
    getIDs( inpids, OnlyWithInput );
    for ( const auto& inpid : inpids )
    {
	const SynthGenParams& sgp = *getGenParams( inpid );
	if ( !sgp.name_.contains(inpnmlbl.buf()) && sgp.inpsynthnm_ != oldnm )
	    continue;

	auto& newsgp = const_cast<SynthGenParams&>( sgp );
	newsgp.name_.replace( inpnmlbl.buf(), newinpnmlbl.buf() );
	if ( sgp.inpsynthnm_ == oldnm ||
	     sgp.inpsynthnm_ == SynthGenParams::sKeyInvalidInputPS() )
	    newsgp.inpsynthnm_ = newnm;

	handledids += inpid;
	const int inpidx = gtIdx( inpid );
	for ( int ilm=0; ilm<nrLayerModels(); ilm++ )
	{
	    const SyntheticData* attribsd = gtDSByIdx( inpidx, ilm );
	    if ( attribsd )
		const_cast<SyntheticData*>( attribsd )->useGenParams( sgp );
	}
    }

    dirtycount_++;
    entryRenamed.trigger( handledids );
    return true;
}


bool StratSynth::DataMgr::removeSynthetic( SynthID id )
{
    const int idx = gtIdx( id );
    if ( !ids_.validIdx(idx) )
	return false;

    ids_.removeSingle( idx );
    genparams_.removeSingle( idx );
    for ( int ilm=0; ilm<nrLayerModels(); ilm++ )
	lmdatasets_[ilm]->removeSingle( idx );

    dirtycount_++;
    const TypeSet<SynthID> ids( id );
    entryRemoved.trigger( ids );
    return true;
}


bool StratSynth::DataMgr::disableSynthetic( const TypeSet<SynthID>& ids )
{
    TypeSet<SynthID> handledids;
    for ( const auto& id : ids )
    {
	const int idx = gtIdx( id );
	if ( !genparams_.validIdx(idx) )
	    continue;

	SynthGenParams& sgp = genparams_[idx];
	if ( !sgp.needsInput() )
	    continue;

	sgp.inpsynthnm_.set( SynthGenParams::sKeyInvalidInputPS() );
	for ( int ilm=0; ilm<nrLayerModels(); ilm++ )
	{
	    const SyntheticData* sd = gtDS( id, ilm );
	    if ( sd )
		const_cast<SyntheticData*>( sd )->useGenParams( sgp );
	}

	handledids += id;
    }

    const bool res = !handledids.isEmpty();
    if ( res )
	entryDisabled.trigger( handledids );

    return res;
}


SynthID StratSynth::DataMgr::getIDByIdx( int idx ) const
{
    return ids_.validIdx(idx) ? ids_[idx] : SynthID::udf();
}


bool StratSynth::DataMgr::haveDS( int idx, int lmsidx ) const
{
    const ObjectSet<const SyntheticData>& dss = gtDSS( lmsidx );
    return dss.validIdx(idx) ? (bool)dss.get( idx ) : false;
}


bool StratSynth::DataMgr::hasValidDataSet( SynthID id, int lmsidx ) const
{
    return haveDS( gtIdx(id), lmsidx );
}


SynthID StratSynth::DataMgr::find( const char* dsnm, int* lmsidx ) const
{
    for ( int idx=0; idx<genparams_.size(); idx++ )
    {
	if ( genparams_[idx].name_ == dsnm )
	{
	    if ( lmsidx )
		*lmsidx = 0;
	    return ids_[idx];
	}
    }

    if ( !lmsidx || lms_.size() < 2 )
	return SynthID::udf();

    for ( int ilm=1; ilm<lms_.size(); ilm++ )
    {
	for ( int idx=0; idx<genparams_.size(); idx++ )
	{
	    const SynthGenParams& sgp = genparams_[idx];
	    const BufferString finaldsnm =
		    getFinalDataSetName( sgp.name_, sgp.isStratProp(), ilm );
	    if ( finaldsnm == dsnm )
	    {
		*lmsidx = ilm;
		return ids_[idx];
	    }
	}
    }

    return SynthID::udf();
}


SynthID StratSynth::DataMgr::find( const PropertyRef& pr,
				    bool require_generated, int lmsidx ) const
{
    const ObjectSet<const SyntheticData>& dss = gtDSS( lmsidx );
    const PropertyRefSelection& prs = layerModel( lmsidx ).propertyRefs();
    for ( int idx=0; idx<genparams_.size(); idx++ )
    {
	if ( !genparams_[idx].isStratProp() )
	    continue;

	if ( require_generated )
	{
	    const SyntheticData* sd = dss.get( idx );
	    if ( !sd )
		{ pErrMsg("prop dataset not generated yet"); continue; }
	    else if ( !sd->isStratProp() )
		{ pErrMsg("wrong type for prop dataset"); continue; }

	    mDynamicCastGet(const StratPropSyntheticData*,pssd,sd);
	    if ( !pssd )
		continue;

	    if ( &pr == &pssd->propRef() )
		return ids_[idx];
	}
	else
	{
	    if ( &pr == genparams_[idx].getRef(prs) )
		return ids_[idx];
	}
    }

    return SynthID::udf();
}


SynthID StratSynth::DataMgr::first( bool isps, bool genreq,
							 int lmsidx ) const
{
    const ObjectSet<const SyntheticData>& dss = gtDSS( lmsidx );
    for ( int idx=0; idx<genparams_.size(); idx++ )
    {
	if ( genparams_[idx].isPreStack() == isps && (!genreq || dss.get(idx)) )
	    return ids_[idx];
    }

    return SynthID::udf();
}


bool StratSynth::DataMgr::isElasticStack( SynthID id ) const
{
    const SynthGenParams* sgp = getGenParams( id );
    return sgp ? sgp->isElasticStack() : false;
}


bool StratSynth::DataMgr::isElasticPS( SynthID id ) const
{
    const SynthGenParams* sgp = getGenParams( id );
    return sgp ? sgp->isElasticGather() : false;
}


bool StratSynth::DataMgr::isPS( SynthID id ) const
{
    const SynthGenParams* sgp = getGenParams( id );
    return sgp ? sgp->isPreStack() : false;
}


bool StratSynth::DataMgr::isAttribute( SynthID id ) const
{
    const SynthGenParams* sgp = getGenParams( id );
    return sgp ? sgp->isAttribute() : false;
}


bool StratSynth::DataMgr::isStratProp( SynthID id ) const
{
    const SynthGenParams* sgp = getGenParams( id );
    return sgp ? sgp->isStratProp() : false;
}


void StratSynth::DataMgr::getAllNames( const SynthGenParams& sgp, int lmsidx,
				       BufferStringSet& nms ) const
{
    nms.add( sgp.name_ );
    if ( lmsidx > -2 || lms_.size() < 2 )
	return;

    for ( int ilm=1; ilm<lms_.size(); ilm++ )
	nms.add( getFinalDataSetName( sgp.name_, sgp.isStratProp(), ilm ) );
}


void StratSynth::DataMgr::getNames( BufferStringSet& nms, SubSelType ss,
				    bool noempty, int lmsidx ) const
{
    TypeSet<int> idxs;
    gtIdxs( idxs, ss, noempty, lmsidx );
    for ( const auto& idx : idxs )
	getAllNames( genparams_[idx], lmsidx, nms );
}


void StratSynth::DataMgr::getNames( const MultiID& wvltid, BufferStringSet& nms,
				    bool noempty, int lmsidx ) const
{
    TypeSet<int> idxs;
    gtIdxs( wvltid, idxs, noempty, lmsidx );
    for ( const auto& idx : idxs )
	getAllNames( genparams_[idx], lmsidx, nms );
}


void StratSynth::DataMgr::getIDs( TypeSet<SynthID>& ids, SubSelType ss,
				  bool noempty, int lmsidx ) const
{
    TypeSet<int> idxs;
    gtIdxs( idxs, ss, noempty, lmsidx );
    for ( const auto& idx : idxs )
	ids.addIfNew( ids_[idx] );
}


void StratSynth::DataMgr::getIDs( const MultiID& wvltid, TypeSet<SynthID>& ids,
				  bool noempty, int lmsidx ) const
{
    TypeSet<int> idxs;
    gtIdxs( idxs, OnlyRaw, noempty, lmsidx );
    for ( const auto& idx : idxs )
	ids.addIfNew( ids_[idx] );
}


void StratSynth::DataMgr::gtIdxs( TypeSet<int>& idxs, SubSelType ss,
				  bool noempty, int lmsidx ) const
{
    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	if ( noempty && !haveDS(idx,lmsidx) )
	    continue;

	const SynthGenParams& sgp = genparams_[idx];
	bool doadd = true;
#	define mHandleCase(typ,val) case typ: doadd = val; break
	switch ( ss )
	{
	    mHandleCase( OnlyZO,	sgp.isZeroOffset() );
	    mHandleCase( NoZO,		!sgp.isZeroOffset() );
	    mHandleCase( OnlyEIStack,	sgp.isElasticStack() );
	    mHandleCase( NoEIStack,	!sgp.isElasticStack() );
	    mHandleCase( OnlyEIGather,	sgp.isElasticGather() );
	    mHandleCase( NoEIGather,	!sgp.isElasticGather() );
	    mHandleCase( OnlyPS,	sgp.isPreStack() );
	    mHandleCase( NoPS,		!sgp.isPreStack() );
	    mHandleCase( OnlyPSBased,	sgp.isPSBased() );
	    mHandleCase( NoPSBased,	!sgp.isPSBased() );
	    mHandleCase( OnlyAttrib,	sgp.isAttribute() );
	    mHandleCase( NoAttrib,	!sgp.isAttribute() );
	    mHandleCase( OnlyRaw,	sgp.isRawOutput() );
	    mHandleCase( NoRaw,		!sgp.isRawOutput() );
	    mHandleCase( OnlyInput,	sgp.canBeAttributeInput() );
	    mHandleCase( OnlyWithInput, sgp.needsInput() );
	    mHandleCase( NoWithInput,	!sgp.needsInput() );
	    mHandleCase( OnlyProps,	sgp.isStratProp() );
	    mHandleCase( NoProps,	!sgp.isStratProp() );
	    default: break;
	}
	if ( doadd )
	    idxs.addIfNew( idx );
    }
}


void StratSynth::DataMgr::gtIdxs( const MultiID& wvltid, TypeSet<int>& idxs,
				  bool noempty, int lmsidx ) const
{
    TypeSet<int> rawidxs;
    gtIdxs( rawidxs, OnlyRaw, noempty, lmsidx );
    for ( const auto& idx : rawidxs )
    {
	const SynthGenParams& sgp = genparams_[idx];
	PtrMan<IOObj> ioobj = Wavelet::getIOObj( sgp.getWaveletNm() );
	if ( !ioobj )
	    continue;

	const MultiID wvltky = ioobj->key();
	if ( wvltky == wvltid )
	    idxs.addIfNew( idx );
    }
}


bool StratSynth::DataMgr::haveOfType(
				const SynthGenParams::SynthType typ ) const
{
    for ( const auto& sgp : genparams_ )
	if ( sgp.synthtype_ == typ )
	    return true;

    return false;
}


BufferString StratSynth::DataMgr::nameOf( SynthID id ) const
{
    BufferString ret;
    const SynthGenParams* sgp = getGenParams( id );
    if ( sgp )
	ret.set( sgp->name_.buf() );

    return ret;
}


int StratSynth::DataMgr::lmsIndexOf( const SyntheticData* reqsd ) const
{
    if ( reqsd )
    {
	for ( int ilm=0; ilm<lms_.size(); ilm++ )
	    if ( indexOf(reqsd,ilm) >= 0 )
		return ilm;
    }

    return -1;
}


int StratSynth::DataMgr::indexOf( const SyntheticData* reqsd, int lmsidx ) const
{
    if ( reqsd )
    {
	const ObjectSet<const SyntheticData>& dss = gtDSS( lmsidx );
	for ( int idss=0; idss<dss.size(); idss++ )
	    if ( dss.get(idss) == reqsd )
		return idss;
    }

    return -1;
}


const SynthGenParams* StratSynth::DataMgr::getGenParams( SynthID id ) const
{
    const int idx = gtIdx( id );
    return genparams_.validIdx( idx ) ? &genparams_[idx] : nullptr;
}


ConstRefMan<SyntheticData> StratSynth::DataMgr::getDataSet( SynthID id,
							    int lmsidx ) const
{
    return gtDS( id, lmsidx );
}


ConstRefMan<SyntheticData> StratSynth::DataMgr::getDataSetByIdx( int idx,
							    int lmsidx ) const
{
    return gtDSByIdx( idx, lmsidx );
}


int StratSynth::DataMgr::gtGenIdx( SynthID id, TaskRunner* /* taskrun */ ) const
{
    const int idx = gtIdx( id );
    if ( idx < 0 )
    {
	static const char* msg = "Generate for non-existing synthetics ID";
	pErrMsg( msg );
    }

    return idx;
}


#define mGetGenIdx(idx) \
    const int idx = gtGenIdx( id, taskrun ); \
    if ( idx < 0 ) \
	return false

bool StratSynth::DataMgr::ensureGenerated( SynthID id, TaskRunner* taskrun,
					   int lmsidx ) const
{
    if ( nrTraces(lmsidx) < 1 )
	return true;

    mGetGenIdx(idx);
    return haveDS(idx,lmsidx) ? true : generate( id, lmsidx, taskrun );
}


bool StratSynth::DataMgr::generate( SynthID id, int lmsidx,
				    TaskRunner* taskrun ) const
{
    mGetGenIdx(idx);

    const SynthGenParams& sgp = genparams_[idx];
    RefLayer::Type checktyp = RefLayer::Acoustic;
    if ( !swaveinfomsgshown_ && sgp.requiredRefLayerType() )
	checktyp = *sgp.requiredRefLayerType();
    mSelf().gtDSS( lmsidx ).replace( idx, nullptr );
    if ( !ensureElasticModels(lmsidx,checktyp,taskrun) )
	return false;

    ConstRefMan<SyntheticData> newds = generateDataSet( sgp, lmsidx, taskrun );
    mSelf().gtDSS( lmsidx ).replace( idx, newds.ptr() );
    if ( newds )
    {
	const_cast<SyntheticData&>( *newds.ptr() ).setID( id );
	ensureLevels( lmsidx );
    }

    return newds.ptr();
}


void StratSynth::DataMgr::ensureLevels( int lmsidx ) const
{
    lmsidx = gtActualLMIdx( lmsidx );
    LevelSet& lvlset = *const_cast<LevelSet*>( levelsets_.get(lmsidx) );
    if ( !lvlset.isEmpty() )
	return;

    const Strat::LevelSet& lvls = Strat::LVLS();
    const Strat::LayerModel& laymod = layerModel( lmsidx );
    const int nrtrcs = nrTraces( lmsidx );
    //TODO: parallel
    for ( int ilvl=0; ilvl<lvls.size(); ilvl++ )
    {
	const Strat::LevelID id = lvls.getIDByIdx( ilvl );
	StratSynth::Level& lvl = lvlset.add( id );
	const Strat::Level stratlvl = lvls.get( id );
	for ( int itrc=0; itrc<nrtrcs; itrc++ )
	{
	    const Strat::LayerSequence& seq = laymod.sequence( iSeq(itrc) );
	    lvl.zvals_ += seq.depthPositionOf( stratlvl, mUdf(float) );
	}
    }
}


void StratSynth::DataMgr::getLevelDepths( Strat::LevelID id,
					  TypeSet<float>& depths,
					  int lmsidx ) const
{
    const LevelSet& lvlset = *levelsets_.get( gtActualLMIdx(lmsidx) );
    for ( const auto* lvl : lvlset.levels() )
    {
	if ( lvl->id_ == id )
	{
	    depths = lvl->zvals_;
	    return;
	}
    }

    depths.setSize( nrTraces(lmsidx), mUdf(float) );
}


void StratSynth::DataMgr::setPackLevelTimes( SynthID id,
					     Strat::LevelID lvlid ) const
{
    for ( int ilm=0; ilm<lms_.size(); ilm++ )
    {
	ConstRefMan<SyntheticData> sd = gtDS( id, ilm );
	if ( !sd )
	    continue;

	const LevelSet& lvlset = *levelsets_.get( ilm );
	const TypeSet<float>* zvals = nullptr;
	for ( const auto* lvl : lvlset.levels() )
	    if ( lvl->id_ == lvlid )
		{ zvals = &lvl->zvals_; break; }

	if ( !zvals )
	    continue;

	if ( sd->isPS() )
	{
	    { pErrMsg("TODO: set picks in PS gathers"); }
	    //Probably add cache in StratSynth::Level
	}
	else
	{
	    /*TODO Probably add and use cache in StratSynth::Level
	      instead of writing to the traces */
	    const SeisTrcBuf& ctbuf =
		   static_cast<const PostStackSyntheticData&>( *sd.ptr() )
						   .postStackPack().trcBuf();
	    auto& tbuf = const_cast<SeisTrcBuf&>( ctbuf );
	    int nr2set = tbuf.size();
	    if ( zvals->size() != nr2set )
	    {
		pErrMsg("Sizes should be equal");
		nr2set = std::max( nr2set, zvals->size() );
	    }

	    for ( int itrc=0; itrc<nr2set; itrc++ )
	    {
		const TimeDepthModel* d2tmodel = sd->getTDModel( itrc );
		if ( !d2tmodel )
		    { pErrMsgOnce("Should not be empty"); continue; }

		SeisTrc& trc = *tbuf.get( itrc );
		const float zval = zvals->get( itrc );
		const float tval = d2tmodel->getTime( zval );
		trc.info().zref = trc.info().pick = tval;
	    }
	}
    }
}


namespace StratSynth
{

class ElasticModelCreator : public ::ParallelTask
{ mODTextTranslationClass(StratSynth::ElasticModelCreator);
public:
ElasticModelCreator( const Strat::LayerModel& lm, ElasticModelSet& ems,
		     int calceach )
    : ::ParallelTask( "Elastic Model Generator" )
    , lm_(lm)
    , laydepthuom_(PropertyRef::thickness().unit())
    , elasticmodels_(ems)
    , calceach_(calceach)
{
    nrmodels_ = getNrTraces( lm_.size(), calceach_ );
    msg_ = tr( "Generating elastic model" );
}


uiString uiMessage() const override	{ return msg_; }
uiString uiNrDoneText() const override	{ return tr("Models done"); }

private:

od_int64 nrIterations() const override	{ return nrmodels_; }

bool doPrepare( int nrthreads ) override
{
    if ( lm_.size() < calceach_ )
	return false;

    if ( !elasticmodels_.setSize(nrmodels_) )
	return false;

    const ElasticPropSelection& eps = lm_.elasticPropSel();
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
    const ElasticPropSelection& eps = lm_.elasticPropSel();
    const int nrelvals = eps.size();
    TypeSet<float> elvals( nrelvals, mUdf(float) );
    float* elvalsptr = elvals.arr();

    const PropertyRefSelection& prs = lm_.propertyRefs();
    const int nrpropvals = prs.size();
    TypeSet<float> layvals( nrpropvals, mUdf(float) );
    float* valsptr = layvals.arr();
    for ( int imdl=int(start); imdl<=stop; imdl++, addToNrDone(1) )
    {
	const int iseq = imdl * calceach_;
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	if ( seq.isEmpty() )
	    continue;

	ElasticModel& curem = *elasticmodels_.get( imdl );
	if ( !fillElasticModel(seq,elpgen,elvalsptr,nrelvals,
			       valsptr,nrpropvals,curem) )
	    return false;
    }

    return true;
}

bool fillElasticModel( const Strat::LayerSequence& seq, ElasticPropGen& elpgen,
		       float* elvalsptr, int elvalssz,
		       float* propvalsptr, int prssz, ElasticModel& elmod )
{
    elmod.setEmpty();

    const float srd =
		getConvertedValue( SI().seismicReferenceDatum(),
				   UnitOfMeasure::surveyDefSRDStorageUnit(),
				   laydepthuom_ );
    int firstidx = 0;
    if ( seq.startDepth() < -srd )
	firstidx = seq.nearestLayerIdxAtZ( -srd );

    if ( seq.isEmpty() )
    {
	mutex_.lock();
	msg_ = tr("Elastic model is not proper to generate synthetics as a "
		  "layer sequence has no layers");
	mutex_.unLock();
	return false;
    }

    //TODO: compute and store the array indices in doPrepare
    const float* dval = &elvalsptr[0];
    const float* pval = &elvalsptr[1];
    const float* sval = elvalssz < 3 ? nullptr : &elvalsptr[2];
    const float* fracrho = elvalssz < 4 ? nullptr : &elvalsptr[3];
    const float* fracazi = elvalssz < 5 ? nullptr : &elvalsptr[4];
    const ObjectSet<Strat::Layer>& layers = seq.layers();
    for ( int idx=firstidx; idx<layers.size(); idx++ )
    {
	const Strat::Layer& lay = *layers.get( idx );
	float thickness = lay.thickness();
	if ( idx == firstidx )
	    thickness += lay.zTop() + srd;
	if ( thickness < 1e-4f )
	    continue;

	thickness = laydepthuom_->getSIValue( thickness );
	const int propvalssz = lay.nrValues(); // May be smaller than prssz
	lay.getValues( propvalsptr, propvalssz );
	elpgen.getVals( propvalsptr, propvalssz, elvalsptr, elvalssz );

	// Detect water - reset Vs
	/* TODO disabled for now
	if ( pval < cMaximumVpWaterVel() )
	    sval = 0;*/

	if ( fracazi )
	    { pErrMsg("TODO: impl"); }
	else if ( fracrho )
	    { pErrMsg("TODO: impl"); }
	else if ( sval )
	    elmod.add( new ElasticLayer( thickness, *pval, *sval, *dval ) );
	else
	    elmod.add( new AILayer( thickness, *pval, *dval ) );
    }

    if ( elmod.isEmpty() )
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

static float cMaximumVpWaterVel()
{ return 1510.f; }

    const Strat::LayerModel&	lm_;
    od_int64			nrmodels_;
    const int			calceach_;
    const UnitOfMeasure*	laydepthuom_;
    ElasticModelSet&		elasticmodels_;
    Threads::Mutex		mutex_;
    uiString			msg_;

    ObjectSet<ElasticPropGen>	elpgens_;

}; // class ElasticModelCreator


class ElasticModelAdjuster : public ::ParallelTask
{ mODTextTranslationClass(StratSynth::ElasticModelAdjuster)
public:

ElasticModelAdjuster( const Strat::LayerModel& lm,
		      ElasticModelSet& ems, RefLayer::Type checktyp,
		      int calceach )
    : ::ParallelTask("Checking & adjusting elastic models")
    , lm_(lm)
    , elasticmodels_(ems)
    , checktyp_(checktyp)
    , nrmodels_(ems.size())
    , calceach_(calceach)
{
}


uiString uiMessage() const override
{
    return !errmsg_.isEmpty() ? errmsg_ : tr( "Checking Models" );
}

uiString uiNrDoneText() const override
{
    return tr( "Models done" );
}

uiString infoMsg() const	{ return infomsg_; }
uiString errMsg() const		{ return errmsg_; }

private:

od_int64 nrIterations() const override
{
    return nrmodels_;
}


bool doWork( od_int64 start , od_int64 stop , int /* threadidx */ ) override
{
    const bool dosvelcheck = checktyp_ > RefLayer::Acoustic;
    const bool dodencheck = !dosvelcheck; // ??
    for ( int imdl=mCast(int,start); imdl<=stop; imdl++, addToNrDone(1) )
    {
	const int iseq = imdl * calceach_;
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	ElasticModel& curem = *elasticmodels_.get(imdl);
	if ( curem.isEmpty() )
	    continue;

	ElasticModel tmpmodel( curem );
	int erroridx = -1;
	tmpmodel.checkAndClean( erroridx, dodencheck, dosvelcheck, false );
	if ( tmpmodel.isEmpty() )
	{
	    uiString startstr(
	       dosvelcheck ? tr("Could not generate prestack synthetics as all")
			   : tr("All") );
	    uiString propstr( dosvelcheck ? tr("Swave velocity")
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
	    for ( int idx=erroridx; idx<curem.size(); idx++ )
	    {
		const RefLayer& layer = *curem.get(idx);
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
			const Mnemonic& pvelmn = Mnemonic::defPVEL();
			const float pvel =
			    pvelmn.unit()->getUserValueFromSI(layer.getPVel());
			msg.append( tr("'Pwave' ( sample value: %1 %2 )")
				.arg(toString(pvel))
				.arg(pvelmn.disp_.getUnitLbl()) );
		    }
		}

		if ( incorrectden )
		{
		    needinterpoltedden = true;
		    if ( needinfo && !infomsg_.isSet() )
		    {
			const Mnemonic& denmn = Mnemonic::defDEN();
			const float den =
			    denmn.unit()->getUserValueFromSI(layer.getDen());
			msg.append( tr("'Density' ( sample value: %1 %2 )")
				.arg(toString(den))
				.arg(denmn.disp_.getUnitLbl()) );
		    }
		}

		if ( incorrectsvel )
		{
		    needinterpolatedsvel = true;
		    if ( needinfo && !infomsg_.isSet() )
		    {
			const Mnemonic& svelmn = Mnemonic::defSVEL();
			const float svel =
			    svelmn.unit()->getUserValueFromSI(layer.getSVel());
			msg.append( tr("'Swave' ( sample value: %1 %2 )")
				.arg(toString(svel))
				.arg(svelmn.disp_.getUnitLbl()) );
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
		    .arg( msg ).arg(seq.layers()[erroridx]->name()).arg(iseq+1);
	    }

	    curem.interpolate( needinterpolatedvel, needinterpoltedden,
			       needinterpolatedsvel );
	}

	curem.mergeSameLayers();
	curem.upscale( 5.0f );
    }

    return true;
}

    const Strat::LayerModel&	lm_;
    ElasticModelSet&		elasticmodels_;
    const od_int64		nrmodels_;
    const int			calceach_;
    uiString			infomsg_;
    uiString			errmsg_;
    const RefLayer::Type	checktyp_;
}; // class ElasticModelAdjuster

} // namespace StratSynth


bool StratSynth::DataMgr::ensureAdequatePropSelection( int lmsidx,
					       RefLayer::Type reqtype ) const
{
    const Strat::LayerModel& lm = layerModel( lmsidx );
    const PropertyRefSelection& prs = lm.propertyRefs();
    if ( prs.size() < 2 )
    {
	errmsg_ = tr("No properties set in layer model");
	return false;
    }

    const ElasticPropSelection& lmelpropsel = lm.elasticPropSel();
    if ( checkElasticPropSel(lmelpropsel,&reqtype) )
	return true;

    const bool checkswave = reqtype > RefLayer::Acoustic;
    const bool checkfracrho = reqtype > RefLayer::Elastic;
    const bool checkfracazi = reqtype > RefLayer::VTI;
    ElasticPropSelection elpropsel = lmelpropsel;
    if ( !elpropsel.ensureHasType(ElasticFormula::Den) ||
	 !elpropsel.ensureHasType(ElasticFormula::PVel) ||
	 (checkswave && !elpropsel.ensureHasType(ElasticFormula::SVel)) ||
	 (checkfracrho && !elpropsel.ensureHasType(ElasticFormula::FracRho)) ||
	 (checkfracazi && !elpropsel.ensureHasType(ElasticFormula::FracAzi)) )
	return false;

    PropertyRefSelection inpprs( false );
    for ( const auto* pr : prs )
    {
	if ( !pr->isThickness() && !pr->hasFixedDef() )
	    inpprs.add( pr );
    }

    if ( !elpropsel.setFor(inpprs) ||
	 !checkElasticPropSel(elpropsel,&reqtype) )
	return false;

    mSelf().setElasticPropSel( elpropsel );

    return true;
}


bool StratSynth::DataMgr::ensureElasticModels( int lmsidx,
					       RefLayer::Type reqtype,
					       TaskRunner* taskrun ) const
{
    if ( !ensureAdequatePropSelection(lmsidx,reqtype) )
    {
	errmsg_ =
	    tr("No suitable elastic properties found with the layer model");
	return false;
    }

    ElasticModelSet& elmdls = *const_cast<ElasticModelSet*>(
			    elasticmodelsets_.get( gtActualLMIdx(lmsidx) ) );
    if ( !elmdls.isEmpty() )
	return true;

    const Strat::LayerModel& lm = layerModel( lmsidx );
    if ( lm.isEmpty() )
    {
	errmsg_ = tr("Empty layer model");
	return false;
    }

    ElasticModelCreator emc( lm, elmdls, calceach_ );
    if ( !TaskRunner::execute(taskrun,emc) )
    {
	errmsg_ = emc.uiMessage();
	return false;
    }

    bool modelsvalid = false;
    for ( const auto* emodel : elmdls )
    {
	if ( !emodel->isEmpty() )
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

    return adjustElasticModel( lm, elmdls, reqtype, taskrun );
}


bool StratSynth::DataMgr::adjustElasticModel( const Strat::LayerModel& lm,
				  ElasticModelSet& elmdls,
				  RefLayer::Type checktyp,
				  TaskRunner* taskrun ) const
{
    ElasticModelAdjuster emadjuster( lm, elmdls, checktyp, calceach_ );
    const bool res = TaskRunner::execute( taskrun, emadjuster );
    infomsg_ = emadjuster.infoMsg();
    swaveinfomsgshown_ = swaveinfomsgshown_ || checktyp > RefLayer::Acoustic;
    return res;
}


#define mErrRet( msg, act )\
{ \
    errmsg_ = toUiString("Can not generate synthetics %1 : %2\n") \
			.arg( sgp.name_ ).arg( msg ); \
    act;\
}


ConstRefMan<SyntheticData> StratSynth::DataMgr::generateDataSet(
					const SynthGenParams& sgp, int lmsidx,
					TaskRunner* taskrun ) const
{
    if ( layerModel(lmsidx).isEmpty() )
    {
	errmsg_ = tr("Empty layer model.");
	return nullptr;
    }

    ConstRefMan<SyntheticData> sd;
    if ( sgp.isRawOutput() )
    {
	PtrMan<Seis::RaySynthGenerator> synthgen;
	ConstRefMan<Seis::SynthGenDataPack> syngendp =
						getSynthGenRes( sgp, lmsidx );
	if ( syngendp )
	    synthgen = new Seis::RaySynthGenerator( *syngendp.ptr() );
	else
	{
	    uiString msg;
	    ConstRefMan<ReflectivityModelSet> refmodels =
						getRefModels( sgp, lmsidx );
	    if ( !refmodels )
	    {
		const ElasticModelSet& elmdls =
				    *elasticmodelsets_[ curLayerModelIdx() ];
		const float srd = getConvertedValue(
				    SI().seismicReferenceDatum(),
				    UnitOfMeasure::surveyDefSRDStorageUnit(),
				    UnitOfMeasure::surveyDefDepthUnit() );
		const bool depthsinfeet = SI().depthsInFeet();
		const Seis::OffsetType offstyp = SI().xyInFeet()
				    ? Seis::OffsetFeet : Seis::OffsetMeter;
		refmodels = Seis::RaySynthGenerator::getRefModels( elmdls,
					    *sgp.reflPars(), msg, taskrun,
					    srd, depthsinfeet, offstyp );
	    }
	    if ( !refmodels )
		return nullptr;

	    synthgen = new Seis::RaySynthGenerator( *refmodels.ptr() );
	}

	if ( !runSynthGen(*synthgen.ptr(),sgp,taskrun) )
	    return nullptr;

	sd = SyntheticData::get( sgp, *synthgen.ptr() );
	if ( sd && sd->isPS() )
	{
	    ConstRefMan<PreStack::GatherSetDataPack> anglegather =
			    getRelevantAngleData( sd->synthGenDP(), lmsidx );
	    mDynamicCastGet(const PreStackSyntheticData*,presdc,sd.ptr());
	    auto* presd = const_cast<PreStackSyntheticData*>( presdc );
	    if ( anglegather )
		presd->setAngleData( anglegather.ptr() );
	    else
		createAngleData( *presd, taskrun );
	}
    }
    else if ( sgp.needsInput() )
    {
	const BufferString inpdsnm( getFinalDataSetName(sgp.inpsynthnm_,
						    sgp.isStratProp(),lmsidx) );
	const SynthID inpid = find( inpdsnm );
	if ( !inpid.isValid() )
	{
	    errmsg_ = tr("input synthetic data '%1' not present").arg(inpdsnm);
	    return nullptr;
	}

	if ( !ensureGenerated(inpid,taskrun,lmsidx) )
	{
	    errmsg_ = tr("Cannot generate '%1'").arg(inpdsnm);
	    return nullptr;
	}

	const SyntheticData* inpsd = gtDS( inpid, lmsidx );
	if ( !inpsd )
	    { pErrMsg("Huh"); }

	if ( sgp.isPSBased() )
	{
	    if ( !inpsd->isPS() )
		mErrRet( tr(" input synthetic data is not prestack"),
			 return nullptr )

	    const auto& presgdp =
			static_cast<const PreStackSyntheticData&>( *inpsd );
	    sd = genPSPostProcDataSet( presgdp, sgp, taskrun );
	}
	else if ( sgp.isAttribute() )
	{
	    if ( !inpsd->getGenParams().canBeAttributeInput() )
		mErrRet( tr(" input synthetic data is not poststack"),
			 return nullptr )

	    const auto& postdp =
			static_cast<const PostStackSyntheticData&>( *inpsd );
	    sd = createAttribute( postdp, sgp, taskrun );
	}
    }
    else if ( sgp.isStratProp() )
    {
	if ( ensurePropertyDataSets(taskrun,lmsidx) )
	{
	    const SynthID sid = find( sgp.name_ );
	    if ( sid.isValid() )
		sd = gtDS( sid, lmsidx );
	}
    }

    return sd;
}


bool StratSynth::DataMgr::runSynthGen( Seis::RaySynthGenerator& synthgen,
				       const SynthGenParams& sgp,
				       TaskRunner* taskrunner ) const
{
    BufferString capt( "Generating ", sgp.name_ );
    synthgen.setName( capt.buf() );

    const BufferString wvltnm( sgp.getWaveletNm() );
    if ( !wvltnm.isEmpty() )
    {
	if ( wvlt_ && wvlt_->name() == wvltnm )
	    synthgen.setWavelet( wvlt_, OD::CopyPtr );
	else
	{
	    PtrMan<IOObj> ioobj = Wavelet::getIOObj( wvltnm );
	    PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
	    if ( wvlt  )
		synthgen.setWavelet( wvlt, OD::CopyPtr );
	}
    }

    if ( !sgp.synthpars_.isEmpty() )
	synthgen.usePar( sgp.synthpars_ );

    synthgen.setTrcStep( calceach_ );

    return TaskRunner::execute( taskrunner, synthgen );
}


ConstRefMan<SyntheticData> StratSynth::DataMgr::genPSPostProcDataSet(
				const PreStackSyntheticData& presd,
				const SynthGenParams& sgp,
				TaskRunner* taskrunner ) const
{
    const bool isanglestack = sgp.synthtype_ == SynthGenParams::AngleStack;
    const int nrgathers = presd.nrPositions();
    if ( nrgathers < 1 || !presd.hasAngles() )
	return nullptr;

    const PreStack::GatherSetDataPack& angledp = presd.angleData();
    if ( angledp.nrGathers() != nrgathers )
	return nullptr;

    PreStack::PropCalc::Setup calcsetup;
    calcsetup.anglerg_.set( mCast(float,sgp.anglerg_.start),
			    mCast(float,sgp.anglerg_.stop) );
    if ( isanglestack )
    {
	calcsetup.calctype_ = PreStack::PropCalc::Stats;
	calcsetup.stattype_ = Stats::Average;
	calcsetup.useangle_ = true;
    }
    else
    {
	calcsetup.calctype_ = PreStack::PropCalc::LLSQ;
	calcsetup.lsqtype_ = PreStack::PropCalc::Coeff;
	// calcsetup.offsaxis_ = PreStack::PropCalc::Sinsq; TODO: enable ?
    }

    //TODO: parallel
    PreStack::PropCalc pspropcalc( calcsetup );
    PtrMan<SeisTrcBuf> tbuf = new SeisTrcBuf( true );
    const ZSampling zrg = presd.zRange();
    const int nrsamples = zrg.nrSteps() + 1;
    const SamplingData<float> tsampling( zrg );
    ConstRefMan<PreStack::Gather> amplgather, anglegather;
    for ( int igath=0; igath<nrgathers; igath++ )
    {
	amplgather = presd.getGather( igath );
	anglegather = presd.getGather( igath, true );
	if ( !amplgather || !anglegather )
	    return nullptr;

	pspropcalc.setGather( *amplgather.ptr() );
	pspropcalc.setAngleData( *anglegather.ptr() );

	auto* trc = new SeisTrc( nrsamples );
	SeisTrcInfo& ti = trc->info();
	ti.sampling = tsampling;
	ti.setTrcKey( amplgather->getTrcKey() );
	for ( int isamp=0; isamp<nrsamples; isamp++ )
	    trc->set( isamp, pspropcalc.getVal(isamp), 0 );
	tbuf->add( trc );
    }

    /*TODO: add wavelet reshaping: on stack for isanglestack,
		on a copy of prestack input for AVO Gradient */

    auto* retdp = new SeisTrcBufDataPack( tbuf.release(), Seis::Line,
				SeisTrcInfo::TrcNr,
				PostStackSyntheticData::sDataPackCategory() );

    ConstRefMan<SyntheticData> ret;
    if ( isanglestack )
	ret = new AngleStackSyntheticData( sgp, presd.synthGenDP(), *retdp );
    else
	ret = new AVOGradSyntheticData( sgp, presd.synthGenDP(), *retdp );

    return ret;
}


namespace StratSynth
{

mClass(WellAttrib) StratSeqSplitter : public ::ParallelTask
{ mODTextTranslationClass(StratSeqSplitter);

public:
StratSeqSplitter( const DataMgr& dm, int lmsidx, const SyntheticData& sd,
		  double zstep, ObjectSet<Strat::LayerModel>& layermodels )
    : ::ParallelTask("Splitting layermodel")
    , datamgr_(dm)
    , lm_(dm.layerModel(lmsidx))
    , sd_(sd)
    , layermodels_(layermodels)
    , totalnr_(dm.nrTraces(lmsidx))
{
    const ZSampling zrg = sd.zRange();
    zrg_.start = zrg.start;
    zrg_.stop = zrg.stop;
    zrg_.step = zstep;
    msg_ = tr("Preparing Models");
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
    const int nrlm = zrg_.nrSteps() + 1;
    const int nrmodels = int(nrIterations());

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

    for ( int itrc=mCast(int,start); itrc<=mCast(int,stop); itrc++,
							    addToNrDone(1) )
    {
	const int iseq = datamgr_.iSeq( itrc );
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	const TimeDepthModel* t2d = sd_.getTDModel( itrc );
	if ( !t2d )
	    return false;

	const Interval<float> seqdepthrg = seq.zRange();
	const float seqstarttime = t2d->getTime( seqdepthrg.start );
	const float seqstoptime = t2d->getTime( seqdepthrg.stop );
	const Interval<float> seqtimerg( seqstarttime, seqstoptime );
	for ( int idz=0; idz<nrlm; idz++ )
	{
	    Strat::LayerSequence& curseq =
				  layermodels_.get( idz )->sequence( itrc );
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

    const DataMgr&		datamgr_;
    const Strat::LayerModel&	lm_;
    const SyntheticData& sd_;
    ObjectSet<Strat::LayerModel>& layermodels_;
    StepInterval<double>	zrg_;
    const od_int64		totalnr_;
    uiString			msg_;

}; // class StratSeqSplitter


mClass(WellAttrib) PropertyDataSetsCreator : public ParallelTask
{ mODTextTranslationClass(PropertyDataSetsCreator);

public:
PropertyDataSetsCreator( DataMgr& dm, int lmsidx,
		    const SyntheticData& sd,
		    const ObjectSet<Strat::LayerModel>& layermodels,
		    double zstep, const TypeSet<SynthGenParams>& propsgps )
    : ParallelTask( "Creating Synthetics for Properties" )
    , datamgr_(dm)
    , propsgps_(propsgps)
    , lmsidx_(lmsidx)
    , sd_(sd)
    , prs_(false)
    , layermodels_(layermodels)
    , totalnr_(sd.nrPositions())
{
    const ZSampling zrg = sd.zRange();
    zrg_.start = zrg.start;
    zrg_.stop = zrg.stop;
    zrg_.step = zstep;
    const PropertyRefSelection& prs =
			datamgr_.layerModel( lmsidx_ ).propertyRefs();
    for ( const auto& sgp : propsgps )
	prs_.addIfNew( sgp.getRef( prs ) );

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

    for ( int itrc=0; itrc<sd_.nrPositions(); itrc++ )
    {
	const SeisTrc& inptrc = *sd_.getTrace( itrc );
	SeisTrc trc( sz );
	trc.info() = inptrc.info();
	trc.info().sampling.step = zrg_.step;
	trc.info().offset = 0.f;
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
    const Strat::LayerModel& lm = datamgr_.layerModel( lmsidx_ );
    const int sz = layermodels_.size();
    const float step = mCast( float, zrg_.step );
    ::FFTFilter filter( sz, step );
    const float f4 = 1.f / (2.f * step );
    filter.setLowPass( f4 );
    const PropertyRefSelection& props = lm.propertyRefs();
    for ( int itrc=mCast(int,start); itrc<=mCast(int,stop); itrc++,
							    addToNrDone(1) )
    {
	const int iseq = datamgr_.iSeq( itrc );
	const Strat::LayerSequence& seq = lm.sequence( iseq );
	const Interval<float> seqtimerg(  sd_.getTime(seq.zRange().start,itrc),
					  sd_.getTime(seq.zRange().stop,itrc) );
	if ( seqtimerg.isUdf() )
	    return false;

	for ( const auto* pr : prs_ )
	{
	    const int iprop = props.indexOf( pr );
	    const bool propisclass =  pr->hasType( Mnemonic::Class );
	    const bool propisvel = pr->hasType( Mnemonic::Vel );
	    SeisTrcBufDataPack* dp = seisbufdps_[prs_.indexOf(pr)];
	    SeisTrcBuf& trcbuf = dp->trcBuf();
	    const int bufsz = trcbuf.size();
	    SeisTrc* rawtrc = itrc < bufsz ? trcbuf.get( itrc ) : nullptr;
	    if ( !rawtrc )
		continue;

	    const PointBasedMathFunction::InterpolType intertyp =
				propisclass ? PointBasedMathFunction::Snap
					    : PointBasedMathFunction::Linear;
	    PointBasedMathFunction propvals( intertyp,
					     PointBasedMathFunction::EndVal );

	    for ( int idz=0; idz<sz; idz++ )
	    {
		const float time = mCast( float, zrg_.atIndex(idz) );
		if ( !seqtimerg.includes(time,false) )
		    continue;

		if ( !layermodels_.validIdx(idz) )
		    continue;

		const Strat::LayerSequence& curseq =
					layermodels_[idz]->sequence( itrc );
		if ( curseq.isEmpty() )
		    continue;

		Stats::CalcSetup laypropcalc( true );
		laypropcalc.require( propisclass ? Stats::MostFreq
						 : Stats::Average );
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
		const float val = mCast( float, propisclass
						? propval.mostFreq()
						: propval.average() );
		if ( mIsUdf(val) || ( propisvel && val < 1e-5f ) )
		    continue;

		propvals.add( time, propisvel ? 1.f / val : val );
	    }

	    if ( propisclass )
	    {
		for ( int idz=0; idz<sz; idz++ )
		{
		    const float time = mCast( float, zrg_.atIndex(idz) );
		    rawtrc->set( idz, propvals.getValue( time ), 0 );
		}

		continue;
	    }

	    Array1DImpl<float> proptr( sz );
	    for ( int idz=0; idz<sz; idz++ )
	    {
		const float time = mCast( float, zrg_.atIndex(idz) );
		proptr.set( idz, propvals.getValue( time ) );
	    }

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

    for ( int idx=0; idx<propsgps_.size(); idx++ )
    {
	const SynthGenParams& sgp = propsgps_[idx];
	const PropertyRef* pr = sgp.getRef( prs_ );
	SeisTrcBufDataPack* dp = seisbufdps_[idx];
	ConstRefMan<SyntheticData> prsd =
		new StratPropSyntheticData( sgp, sd_.synthGenDP(), *dp, *pr );
	datamgr_.setDataSet( sgp, prsd.ptr(), lmsidx_ );
    }

    return true;
}

    DataMgr&				datamgr_;
    const TypeSet<SynthGenParams>&	propsgps_;
    const int				lmsidx_;
    const SyntheticData&		sd_;
    PropertyRefSelection		prs_;
    StepInterval<double>		zrg_;
    const ObjectSet<Strat::LayerModel>& layermodels_;
    ObjectSet<SeisTrcBufDataPack>	seisbufdps_;
    const od_int64			totalnr_;
    uiString				msg_;

}; // class PropertyDataSetsCreator

} // namespace StratSynth


bool StratSynth::DataMgr::ensurePropertyDataSets( TaskRunner* taskrun,
					int lmsidx, double zstep ) const
{
    TypeSet<SynthID> propids;
    getIDs( propids, OnlyProps );
    if ( propids.isEmpty() )
	return false;

    TypeSet<SynthID> rawids;
    getIDs( rawids, OnlyZO ); // Fastest
    getIDs( rawids, OnlyPS ); // to slowest
    SynthID inpid;
    for ( const auto& rawid : rawids )
    {
	if ( hasValidDataSet(rawid,lmsidx) )
	{
	    inpid = rawid;
	    break;
	}
    }

    if ( !inpid.isValid() )
    {
	if ( rawids.isEmpty() )
	    return false;

	inpid = rawids.first();
	if ( !ensureGenerated(inpid,taskrun,lmsidx) )
	    return false;
    }

    ConstRefMan<SyntheticData> inpsd = getDataSet( inpid, lmsidx );
    if ( !inpsd )
    {
	errmsg_ = tr("No synthetic data found to generate the properties");
	return false;
    }

    ManagedObjectSet<Strat::LayerModel> layermodels;
    StratSeqSplitter splitter( *this, lmsidx, *inpsd, zstep, layermodels );
    if ( !TaskRunner::execute(taskrun,splitter) )
    {
	errmsg_ = splitter.uiMessage();
	return false;
    }

    TypeSet<SynthGenParams> propsgps;
    for ( const auto& id : propids )
	propsgps += *getGenParams(id);

    PropertyDataSetsCreator propcreator( *const_cast<DataMgr*>(this), lmsidx,
					 *inpsd, layermodels, zstep, propsgps );
    if ( !TaskRunner::execute(taskrun,propcreator) )
    {
	errmsg_ = propcreator.uiMessage();
	return false;
    }

    return true;
}


void StratSynth::DataMgr::kick()
{
    dirtycount_++;
    static SynthID emptysid = SynthID();
    const TypeSet<SynthID> ids( emptysid );
    entryChanged.trigger( ids );
}


bool StratSynth::DataMgr::getUnscaledSynthetics(
					RefObjectSet<const SyntheticData>* res,
					TypeSet<MultiID>* unscaledwvlts,
					int lmsidx ) const
{
    bool needscale = false;
    TypeSet<SynthID> ids;
    getIDs( ids, OnlyRaw );
    for ( const auto& id : ids )
    {
	const SynthGenParams& sgp = *getGenParams( id );

	PtrMan<IOObj> ioobj = Wavelet::getIOObj( sgp.getWaveletNm() );
	if ( !ioobj )
	    continue;

	const MultiID wvltky = ioobj->key();
	if ( Wavelet::isScaled(wvltky) )
	    continue;

	needscale = true;
	if ( unscaledwvlts )
	    unscaledwvlts->addIfNew( wvltky );

	if ( !res )
	    continue;

	TypeSet<int> idxs;
	gtIdxs( idxs, OnlyRaw, true, lmsidx );
	for ( const int& idx : idxs )
	{
	    ConstRefMan<SyntheticData> sd = gtDSByIdx( idx, lmsidx );
	    if ( sd )
		res->add( sd );
	}
    }

    return needscale;
}


bool StratSynth::DataMgr::getAllGenPars( const IOPar& par,
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


const ReflectivityModelSet* StratSynth::DataMgr::getRefModels(
				const SynthGenParams& sgp, int lmsidx ) const
{
    TypeSet<SynthID> ids;
    if ( sgp.isZeroOffset() )
	getIDs( ids, OnlyZO, true, lmsidx );
    else if ( sgp.isElasticStack() )
	getIDs( ids, OnlyEIStack, true, lmsidx );
    else if ( sgp.isElasticGather() )
	getIDs( ids, OnlyEIGather, true, lmsidx );
    else if ( sgp.isPreStack() )
	getIDs( ids, OnlyPS, true, lmsidx );
    else
	return nullptr;

    const IOPar& reflpars = *sgp.reflPars();
    for ( const auto& id : ids )
    {
	const SyntheticData* sd = gtDS( id, lmsidx );
	if ( sd && sd->getRefModels().hasSameParams(reflpars) )
	    return &sd->getRefModels();
    }

    return nullptr;
}


const Seis::SynthGenDataPack* StratSynth::DataMgr::getSynthGenRes(
				const SynthGenParams& sgp, int lmsidx ) const
{
    if ( sgp.needsInput() )
    {
	const SyntheticData* sd = gtDSByName( sgp.inpsynthnm_, lmsidx );
	return sd ? &sd->synthGenDP() : nullptr;
    }

    TypeSet<SynthID> ids;
    getIDs( ids, OnlyZO, true, lmsidx );
    getIDs( ids, OnlyEIStack, true, lmsidx );
    getIDs( ids, OnlyEIGather, true, lmsidx );
    getIDs( ids, OnlyPS, true, lmsidx );
    getIDs( ids, NoRaw, true, lmsidx );
    const IOPar* reflpar = sgp.reflPars();
    for ( const auto& id : ids )
    {
	const SyntheticData* sd = gtDS( id, lmsidx );
	const Seis::SynthGenDataPack& syngendp = sd->synthGenDP();
	if ( reflpar && syngendp.hasSameParams(*reflpar,sgp.synthpars_) )
	    return &syngendp;
    }

    return nullptr;
}


ConstRefMan<PreStack::GatherSetDataPack>
StratSynth::DataMgr::getRelevantAngleData(
		    const Seis::SynthGenDataPack& synthgendp, int lmsidx ) const
{
    const ObjectSet<const SyntheticData>& dss = gtDSS( lmsidx );
    for ( const auto* sd : dss )
    {
	if ( !sd || !sd->isPS() ||
	     !sd->synthGenDP().hasSameParams(synthgendp) )
	    continue;

	mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
	if ( presd )
	    return &presd->angleData();
    }

    return nullptr;
}


namespace StratSynth
{

mClass(WellAttrib) AttributeSyntheticCreator : public ::ParallelTask
{ mODTextTranslationClass(AttributeSyntheticCreator);

public:
AttributeSyntheticCreator( const PostStackSyntheticData& sd,
		   const TypeSet<Attrib::Instantaneous::OutType>& attribdefs,
		   ObjectSet<SeisTrcBuf>& seisbufs )
    : ::ParallelTask( "Creating Attribute Synthetics" )
    , seistrcbufs_(seisbufs)
    , sd_(sd)
    , attribdefs_(attribdefs)
{
    const DataPack::FullID fid = sd.fullID();
    if ( !DPM( fid.mgrID() ).isPresent(fid.packID()) )
	DPM( fid.mgrID() ).add<DataPack>( sd.getPack() );
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

bool createInstAttributeSet()
{
    delete descset_;
    descset_ = new Attrib::DescSet( false );
    const MultiID dbky = sd_.fullID().asMultiID();
    const Attrib::DescID did = descset_->getStoredID( dbky, 0, true );
    const Attrib::Desc* inpdesc = descset_->getDesc( did );
    if ( !inpdesc )
	return false;

    Attrib::Desc* imagdesc = Attrib::PF().createDescCopy(
						Attrib::Hilbert::attribName() );
    imagdesc->selectOutput( 0 );
    imagdesc->setInput(0, inpdesc );
    imagdesc->setHidden( true );
    const BufferString usrref( dbky.toString(), "_imag" );
    imagdesc->setUserRef( usrref );
    const Attrib::DescID hilbid = descset_->addDesc( imagdesc );
    if ( !descset_->getDesc(hilbid) )
	return false;

    Attrib::Desc* psdesc = Attrib::PF().createDescCopy(
					 Attrib::Instantaneous::attribName());
    psdesc->setInput( 0, inpdesc );
    psdesc->setInput( 1, imagdesc );
    psdesc->setUserRef( "synthetic attributes" );
    const Attrib::DescID psid = descset_->addDesc( psdesc );
    if ( !descset_->getDesc(psid) )
	return false;

    descset_->updateInputs();
    return true;
}


od_int64 nrIterations() const override
{ return sd_.postStackPack().trcBuf().size(); }


bool doPrepare( int /* nrthreads */ ) override
{
    msg_ = tr("Preparing Attributes");

    if ( !createInstAttributeSet() )
    {
	if ( descset_ )
	    msg_ = descset_->errMsg();

	return false;
    }

    sd_.postStackPack().getTrcKeyZSampling( tkzs_ );

    seistrcbufs_.setEmpty();
    comps_.setEmpty();
    const SeisTrcBuf& trcs = sd_.postStackPack().trcBuf();
    int order = 1;
    for ( const auto& attribdef : attribdefs_ )
    {
	auto* stb = new SeisTrcBuf( true );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    auto* trc = new SeisTrc;
	    trc->info() = trcs.get( idx )->info();
	    stb->add( trc );
	}

	seistrcbufs_ += stb;
	if ( attribdef == Attrib::Instantaneous::Amplitude )
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


bool doWork( od_int64 start, od_int64 stop, int /* threadid */ ) override
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
    for ( const auto& attribdef : attribdefs_ )
	proc->addOutputInterest( int(attribdef) );

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

    const PostStackSyntheticData&	sd_;
    const TypeSet<Attrib::Instantaneous::OutType>& attribdefs_;
    ObjectSet<SeisTrcBuf>&		seistrcbufs_;
    TypeSet<int>			comps_;
    Attrib::DescSet*			descset_ = nullptr;
    TrcKeyZSampling			tkzs_;
    uiString				msg_;
}; // class AttributeSyntheticCreator

} // namespace StratSynth


ConstRefMan<SyntheticData> StratSynth::DataMgr::createAttribute(
					    const PostStackSyntheticData& pssd,
					    const SynthGenParams& synthgenpar,
					    TaskRunner* taskrunner ) const
{
    TypeSet<Attrib::Instantaneous::OutType> attribdefs;
    attribdefs += synthgenpar.attribtype_;
    ObjectSet<SeisTrcBuf> seistrcbufs;
    AttributeSyntheticCreator asc( pssd, attribdefs, seistrcbufs );
    if ( !TaskRunner::execute(taskrunner,asc) )
    {
	errmsg_ = asc.uiMessage();
	return nullptr;
    }

    auto* dpname = new SeisTrcBufDataPack( seistrcbufs[0],
				 Seis::Line, SeisTrcInfo::TrcNr,
				 PostStackSyntheticData::sDataPackCategory() );

    ConstRefMan<SyntheticData> ret =
	new InstAttributeSyntheticData( synthgenpar, pssd.synthGenDP(),
					*dpname );
    return ret;
}


bool StratSynth::DataMgr::ensureInstantAttribsDataSet(
			const TypeSet<SynthID>& synthids, TaskRunner* taskrun,
			int lmsidx ) const
{
    if ( synthids.isEmpty() || nrTraces(lmsidx) < 1 )
	return true;

    const SynthID& firstid = synthids.first();
    const SynthGenParams* firstsgp = getGenParams( firstid );
    if ( !firstsgp )
	return false;

    const SynthID inpid = find( firstsgp->inpsynthnm_ );
    if ( !ensureGenerated(inpid,taskrun,lmsidx) )
	return false;

    TypeSet<SynthID> gensynthids;
    TypeSet<Attrib::Instantaneous::OutType> attribtypes;
    int alreadygenerated = 0;
    for ( const auto& attrid : synthids )
    {
	const int idx = gtGenIdx( attrid, taskrun );
	if ( idx < 0 )
	    continue;

	if ( haveDS(idx,lmsidx) )
	    { alreadygenerated++; continue; }

	gensynthids += attrid;
	attribtypes += getGenParams(attrid)->attribtype_;
    }

    if ( gensynthids.isEmpty() )
	return true;

    if ( gensynthids.size()+alreadygenerated != synthids.size() )
	return false;

    ObjectSet<SeisTrcBuf> seistrcbufs;
    ConstRefMan<SyntheticData> insd = getDataSet( inpid, lmsidx );
    if ( !insd || !insd->getGenParams().canBeAttributeInput() )
	return false;

    const auto& pssd = static_cast<const PostStackSyntheticData&>( *insd.ptr());
    AttributeSyntheticCreator asc( pssd, attribtypes, seistrcbufs );
    if ( !TaskRunner::execute(taskrun,asc) )
    {
	errmsg_ = asc.uiMessage();
	return false;
    }

    for ( int idx=0; idx<seistrcbufs.size(); idx++ )
    {
	const SynthGenParams* retsgp = getGenParams( gensynthids[idx] );
	if ( !retsgp )
	    return false;

	const SynthGenParams& sgp = *retsgp;
	auto* dpname = new SeisTrcBufDataPack( seistrcbufs[idx],
				 Seis::Line, SeisTrcInfo::TrcNr,
				 PostStackSyntheticData::sDataPackCategory() );
	ConstRefMan<SyntheticData> sd =
	    new InstAttributeSyntheticData( sgp, insd->synthGenDP(), *dpname );
	if ( sd )
	    mSelf().setDataSet( sgp, sd.ptr(), lmsidx );
	else
	    mErrRet( tr(" synthetic data not created."), return false )
    }

    return true;
}


namespace StratSynth
{

class PSAngleDataCreator : public ::ParallelTask
{ mODTextTranslationClass(PSAngleDataCreator)
public:

PSAngleDataCreator( PreStackSyntheticData& pssd )
    : ::ParallelTask("Creating Angle Gather" )
    , pssd_(pssd)
    , refmodels_(const_cast<const ReflectivityModelSet&>(
			pssd.synthGenDP().getModels()))
{
    totalnr_ = refmodels_.nrModels();
    const int nrgathers = pssd.nrPositions();
    for ( int idx=0; idx<nrgathers; idx++ )
	seisgathers_.add( pssd.getGather(idx) );
}


~PSAngleDataCreator()
{
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


bool doPrepare( int nrthreads ) override
{
    msg_ =  tr("Calculating Angle Gathers");

    if ( !pssd_.isOK() )
	return false;

    anglecomputers_.setEmpty();
    anglegathers_.setEmpty();
    for ( int ithread=0; ithread<nrthreads; ithread++ )
    {
	RefMan<PreStack::ModelBasedAngleComputer> anglecomputer =
				new PreStack::ModelBasedAngleComputer();
	anglecomputer->setRayTracerPars( *pssd_.getGenParams().reflPars() );
	anglecomputer->setFFTSmoother( 10.f, 15.f );
	anglecomputers_.add( anglecomputer );
	anglegathers_.add( new RefObjectSet<PreStack::Gather> );
    }

    return true;
}


bool doWork( od_int64 start, od_int64 stop, int threadid ) override
{
    if ( !anglecomputers_.validIdx(threadid) ||
	 !anglegathers_.validIdx(threadid) )
	return false;

    const int setidx = anglegathers_.size() - threadid - 1;

    PreStack::ModelBasedAngleComputer& anglecomputer =
				       *anglecomputers_.get( setidx );
    ObjectSet<PreStack::Gather>& anglegathers =
				       *anglegathers_.get( setidx );

    for ( int idx=int(start); idx<=stop; idx++, addToNrDone(1) )
    {
	const ReflectivityModelBase* refmodel = refmodels_.get( idx );
	if ( !refmodel->isOffsetDomain() )
	    return false;

	mDynamicCastGet(const OffsetReflectivityModel*,offrefmodel,refmodel);
	const PreStack::Gather& seisgather = *seisgathers_[idx];
	anglecomputer.setOutputSampling( seisgather.posData(),
					 seisgather.offsetType(),
					 seisgather.zDomain() );
	anglecomputer.setGatherIsNMOCorrected( seisgather.isCorrected() );
	anglecomputer.setRefModel( *offrefmodel, seisgather.getTrcKey() );
	RefMan<PreStack::Gather> anglegather = anglecomputer.computeAngles();
	if ( !anglegather )
	    return false;

	convertAngleDataToDegrees( *anglegather.ptr() );
	TypeSet<float> azimuths;
	seisgather.getAzimuths( azimuths );
	anglegather->setAzimuths( azimuths );
	anglegathers.add( anglegather );
    }

    return true;
}


bool doFinish( bool success ) override
{
    anglecomputers_.setEmpty();
    if ( success )
    {
	RefObjectSet<PreStack::Gather> anglegathers;
	for ( auto* gathersset : anglegathers_ )
	{
	    anglegathers.append( *gathersset );
	    gathersset->setEmpty();
	}

	RefMan<PreStack::GatherSetDataPack> angledp =
		    new PreStack::GatherSetDataPack( nullptr, anglegathers );
	const BufferString angledpnm( pssd_.name().buf(), " (Angle Gather)" );
	angledp->setName( angledpnm );

	pssd_.setAngleData( angledp.ptr() );
    }

    anglegathers_.setEmpty();

    return success;
}


static void convertAngleDataToDegrees( PreStack::Gather& ag )
{
    const auto& agdata = const_cast<const Array2D<float>&>( ag.data() );
    ArrayMath::getScaledArray<float>( agdata, nullptr, mRad2DegD, 0.,
				      false, true );
}

    PreStackSyntheticData&		pssd_;
    RefObjectSet<const PreStack::Gather> seisgathers_;
    const ReflectivityModelSet&		refmodels_;
    ManagedObjectSet<RefObjectSet<PreStack::Gather> > anglegathers_;
    RefObjectSet<PreStack::ModelBasedAngleComputer> anglecomputers_;
    uiString				msg_;
    od_int64				totalnr_;

}; // class PSAngleDataCreator

} // namespace StratSynth


void StratSynth::DataMgr::createAngleData( PreStackSyntheticData& pssd,
					   TaskRunner* taskrun ) const
{
    PSAngleDataCreator angledatacr( pssd );
    TaskRunner::execute( taskrun, angledatacr );
}
