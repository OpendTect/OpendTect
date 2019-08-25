/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          July 2011 / Sep 2018
________________________________________________________________________

-*/


#include "stratsynthdatamgr.h"

#include "prestackanglecomputer.h"
#include "prestackgather.h"
#include "prestackprop.h"
#include "prestacksynthdataset.h"
#include "raytracerrunner.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlevel.h"
#include "trckeyzsampling.h"
#include "waveletmanager.h"


typedef StratSynth::DataMgr::size_type size_type;
typedef StratSynth::DataMgr::idx_type idx_type;
typedef StratSynth::DataMgr::lms_idx_type lms_idx_type;
typedef StratSynth::DataMgr::SynthID SynthID;
static const char* sKeySynthetics =     "Synthetics";
static const char* sKeyNrSynthetics =   "Nr of Synthetics";
static const char* sKeySyntheticNr =    "Synthetics Nr";


idx_type StratSynth::DataMgr::gtActualLMIdx( lms_idx_type lmsidx ) const
{
    if ( lmsidx < 0 || lmsidx >= lms_.size() )
	return lms_.curIdx();
    if ( lmsidx >= lms_.size() )
	{ pErrMsg( "Bad idx" ); return lms_.curIdx(); }

    return lmsidx;
}


StratSynth::DataMgr::DataMgr( const LayerModelSuite& lms )
    : lms_(lms)
    , entryChanged(this)
{
    for ( int idx=0; idx<nrLayerModels(); idx++ )
	addLayModelSets();

    addSynthetic( GenParams() ); // default synth with default wavelet
    addStratPropSynths();

    mAttachCB( lms_.editingChanged, DataMgr::lmsEdChgCB );
}


StratSynth::DataMgr::DataMgr( const DataMgr& oth )
    : lms_(oth.lms_)
    , calceach_(oth.calceach_)
    , entryChanged(this)
{
    for ( int idx=0; idx<nrLayerModels(); idx++ )
	addLayModelSets();

    for ( int idx=0; idx<oth.genparams_.size(); idx++ )
    {
	const auto& gp = oth.genparams_[idx];
	if ( !gp.isStratProp() )
	    addSynthetic( oth.genparams_[idx] );
    }
    addStratPropSynths();

    if ( ids_.size() != oth.ids_.size() )
	{ pErrMsg("Huh"); }
    for ( int idx=0; idx<oth.ids_.size(); idx++ )
	ids_[idx] = oth.ids_[idx];

    mAttachCB( lms_.editingChanged, DataMgr::lmsEdChgCB );
}


StratSynth::DataMgr::~DataMgr()
{
    detachAllNotifiers();
    setEmpty();
    deepErase( lmdatasets_ );
}


RefMan<StratSynth::DataMgr> StratSynth::DataMgr::getProdMgr()
{
    if ( calceach_ < 2 )
	return this;

    auto* ret = new DataMgr( *this );
    ret->setCalcEach( 1 );
    return ret;
}

ConstRefMan<StratSynth::DataMgr> StratSynth::DataMgr::getProdMgr() const
{
    return const_cast<DataMgr*>(this)->getProdMgr();
}


void StratSynth::DataMgr::clearData( bool full )
{
    deepErase( elasticmodelsets_ );
    deepErase( levelsets_ );
    for ( auto idx=0; idx<lmdatasets_.size(); idx++ )
    {
	auto* dss = lmdatasets_.get( idx );
	if ( dss )
	    deepUnRef( *dss );
    }
    deepErase( lmdatasets_ );

    const SynthIDSet idstonotif( ids_ );
    if ( full )
    {
	ids_.erase();
	genparams_.erase();
    }

    for ( int idx=0; idx<nrLayerModels(); idx++ )
	addLayModelSets();

    if ( !full )
    {
	for ( int idx=genparams_.size()-1; idx>=0; idx-- )
	{
	    if ( genparams_[idx].type_ == SynthSeis::StratProp )
	    {
		ids_.removeSingle( idx );
		genparams_.removeSingle( idx );
	    }
	}
	addStratPropSynths();
    }

    dirtycount_++;
    for ( auto id : idstonotif )
	entryChanged.trigger( id );
}


void StratSynth::DataMgr::addLayModelSets()
{
    auto* dss = new DataSetSet;
    dss->setNullAllowed( true );
    lmdatasets_ += dss;
    while ( dss->size() < ids_.size() )
	*dss += 0;
    elasticmodelsets_ += new ElasticModelSet;
    levelsets_ += new LevelSet;
}


void StratSynth::DataMgr::addStratPropSynths()
{
    GenParams gp;
    gp.type_ = SynthSeis::StratProp;
    const auto& props = lms_.get( 0 ).propertyRefs();
    for ( int idx=1; idx<props.size(); idx++ )
    {
	gp.name_.set( props[idx]->name() );
	addSynthetic( gp );
    }
}


void StratSynth::DataMgr::lmsEdChgCB( CallBacker* cb )
{
    const int nrneeded = lms_.size();
    const int nrnow = lmdatasets_.size();
    for ( int idx=nrnow-1; idx>=nrneeded; idx-- )
    {
	delete lmdatasets_.removeSingle( idx );
	delete elasticmodelsets_.removeSingle( idx );
	delete levelsets_.removeSingle( idx );
    }

    if ( nrneeded < 2 )
	return;

    for ( int idx=nrnow; idx<nrneeded; idx++ )
	addLayModelSets();

    for ( int idx=1; idx<nrneeded; idx++ )
    {
	delete lmdatasets_.replace( idx, 0 );
	delete elasticmodelsets_.replace( idx, 0 );
	delete levelsets_.replace( idx, 0 );
    }
}


const Strat::LayerModel& StratSynth::DataMgr::layerModel(
					lms_idx_type lmsidx ) const
{
    return lms_.get( gtActualLMIdx(lmsidx) );
}


const ElasticModelSet& StratSynth::DataMgr::elasticModels(
					lms_idx_type lmsidx ) const
{
    return *elasticmodelsets_.get( gtActualLMIdx(lmsidx) );
}


const StratSynth::LevelSet& StratSynth::DataMgr::levels(
					lms_idx_type lmsidx ) const
{
    return *levelsets_.get( gtActualLMIdx(lmsidx) );
}


size_type StratSynth::DataMgr::nrLayerModels() const
{
    return lms_.size();
}


size_type StratSynth::DataMgr::curLayerModelIdx() const
{
    return lms_.curIdx();
}


StratSynth::DataMgr::DataSetSet& StratSynth::DataMgr::gtDSS(
			lms_idx_type lmsidx )
{
    return *lmdatasets_.get( gtActualLMIdx(lmsidx) );
}


const StratSynth::DataMgr::DataSetSet& StratSynth::DataMgr::gtDSS(
			lms_idx_type lmsidx ) const
{
    return *lmdatasets_.get( gtActualLMIdx(lmsidx) );
}


void StratSynth::DataMgr::setCalcEach( size_type newce )
{
    if ( newce < 1 )
	newce = 1;
    if ( newce == calceach_ )
	return;

    calceach_ = newce;
    clearData( false );
}


idx_type StratSynth::DataMgr::iSeq( int itrc ) const
{
    return itrc * calceach_;
}


idx_type StratSynth::DataMgr::iTrc( int iseq ) const
{
    return iseq / calceach_;
}


size_type StratSynth::DataMgr::nrSequences( lms_idx_type lmsidx ) const
{
    return layerModel(lmsidx).size();
}


static size_type getNrTraces( size_type nrseq, size_type calceach )
{
    if ( nrseq < 1 )
	return 0;

    const size_type ret = nrseq / calceach;
    return ret < 1 ? 1 : ret;
}


size_type StratSynth::DataMgr::nrTraces( lms_idx_type lmsidx ) const
{
    return getNrTraces( nrSequences(lmsidx), calceach_ );
}


BufferString StratSynth::DataMgr::getFinalDataSetName( const char* gpnm,
						       bool isprop ) const
{
    BufferString ret( gpnm );
    if ( isprop )
	ret.embed( '[', ']' );
    const BufferString lmdesc( lms_.desc(lms_.curIdx()) );
    if ( !lmdesc.isEmpty() )
	ret.add( " (" ).add( lmdesc ).add( ")" );
    return ret;
}


void StratSynth::DataMgr::fillPar( IOPar& iop ) const
{
    IOPar synthpar;

    int nrgp = 0;
    for ( int igp=0; igp<genparams_.size(); igp++ )
    {
	const auto& gp = genparams_.get( igp );
	if ( gp.isStratProp() )
	    continue;

	IOPar subpar;
	genparams_.get(igp).fillPar( subpar );
	synthpar.mergeComp( subpar, IOPar::compKey(sKeySyntheticNr,nrgp) );
	nrgp++;
    }
    synthpar.set( sKeyNrSynthetics, nrgp );
    iop.mergeComp( synthpar, sKeySynthetics );
}


void StratSynth::DataMgr::usePar( const IOPar& iop )
{
    PtrMan<IOPar> synthpar = iop.subselect( sKeySynthetics );
    if ( !synthpar || synthpar->isEmpty() )
	return;
    int nrsynth = 0;
    synthpar->get( sKeyNrSynthetics, nrsynth );
    if ( nrsynth < 0 )
	return;

    setEmpty();

    for ( int igp=0; igp<nrsynth; igp++ )
    {
	PtrMan<IOPar> subpar = synthpar->subselect(
				    IOPar::compKey(sKeySyntheticNr,igp) );
	if ( !subpar || subpar->isEmpty() )
	    continue;

	GenParams gp;
	gp.usePar( *subpar );
	addSynthetic( gp );
    }
}


idx_type StratSynth::DataMgr::gtIdx( SynthID id ) const
{
    return ids_.indexOf( id );
}


StratSynth::DataMgr::DataSet* StratSynth::DataMgr::gtDS( SynthID id,
						lms_idx_type lmsidx ) const
{
    return gtDSByIdx( gtIdx(id), lmsidx );
}


StratSynth::DataMgr::DataSet* StratSynth::DataMgr::gtDSByIdx(
				idx_type idx, lms_idx_type lmsidx ) const
{
    const auto& dss = gtDSS( lmsidx );
    return dss.validIdx(idx) ? const_cast<DataSet*>( dss[idx] ) : 0;
}


SynthID StratSynth::DataMgr::addSynthetic( const GenParams& gp )
{
    return addEntry( DataSet::getNewID(), gp );
}


SynthID StratSynth::DataMgr::addEntry( SynthID id, const GenParams& gp )
{
    ids_ += id;
    genparams_ += gp;
    for ( int ilm=0; ilm<lms_.size(); ilm++ )
	*lmdatasets_.get(ilm) += 0;

    dirtycount_++;
    entryChanged.trigger( id );
    return id;
}


void StratSynth::DataMgr::setDataSet( const GenParams& gp, DataSet* ds,
					lms_idx_type lmsidx )
{
    SynthID oldid = find( gp.name_ );
    lmsidx = gtActualLMIdx( lmsidx );
    bool ischgd = true;
    if ( oldid.isValid() )
    {
	ds->setID( oldid );
	const auto idx = ids_.indexOf( oldid );
	ischgd = genparams_[idx] != gp;
	genparams_[idx] = gp;
	auto* oldds = lmdatasets_.get(lmsidx)->replace( idx, ds );
	if ( oldds )
	    oldds->unRef();
    }
    else
    {
	ds->setID( DataSet::getNewID() );
	ids_ += ds->id();
	genparams_ += gp;
	for ( int ilm=0; ilm<lms_.size(); ilm++ )
	    *lmdatasets_.get(ilm) += ilm == lmsidx ? ds : 0;
    }

    if ( ischgd )
    {
	dirtycount_++;
	entryChanged.trigger( ds->id() );
    }
}


void StratSynth::DataMgr::removeSynthetic( SynthID id )
{
    const auto idx = gtIdx( id );
    if ( idx >= 0 )
    {
	ids_.removeSingle( idx );
	genparams_.removeSingle( idx );
	for ( int ilm=0; ilm<nrLayerModels(); ilm++ )
	{
	    auto* ds = lmdatasets_[ilm]->removeSingle( idx );
	    if ( ds )
		ds->unRef();
	}
	dirtycount_++;
	entryChanged.trigger( id );
    }
}


SynthID StratSynth::DataMgr::setSynthetic( SynthID id, const GenParams& gp )
{
    bool ischgd = true;
    if ( !id.isValid() )
	id = addSynthetic( gp );
    else
    {
	const auto idx = gtIdx( id );
	if ( idx < 0 )
	    addEntry( id, gp );
	else
	{
	    const bool samegenpars = gp == genparams_[idx];
	    genparams_[idx] = gp;
	    if ( samegenpars )
		ischgd = false;
	    else
	    {
		for ( int ilm=0; ilm<nrLayerModels(); ilm++ )
		{
		    auto* ds = lmdatasets_[ilm]->replace( idx, 0 );
		    if ( ds )
			ds->unRef();
		}
	    }
	}
    }

    if ( ischgd )
    {
	dirtycount_++;
	entryChanged.trigger( id );
    }

    return id;
}


void StratSynth::DataMgr::setWavelet( SynthID id, const DBKey& wvltid )
{
    if ( !id.isValid() || !ids_.isPresent(id) )
	{ pErrMsg("Invalid ID passed"); }

    GenParams newgp( genparams_[ gtIdx(id) ] );
    newgp.wvltid_ = wvltid;
    newgp.name_ = newgp.createName();
    setSynthetic( id, newgp );
}


DBKey StratSynth::DataMgr::waveletID( SynthID id ) const
{
    return ids_.isPresent(id) ? genparams_[ gtIdx(id) ].wvltid_ : DBKey();
}


SynthID StratSynth::DataMgr::getIDByIdx( idx_type idx ) const
{
    return ids_.validIdx(idx) ? ids_[idx] : SynthID();
}


bool StratSynth::DataMgr::haveDS( idx_type idx, lms_idx_type lmsidx ) const
{
    const auto& dss = gtDSS( lmsidx );
    if ( !dss.validIdx(idx) )
	return false;

    const auto* ds = dss.get( idx );
    return ds && !ds->isEmpty();
}


bool StratSynth::DataMgr::hasValidDataSet( SynthID id,
					   lms_idx_type lmsidx ) const
{
    return haveDS( gtIdx(id), lmsidx );
}


SynthID StratSynth::DataMgr::find( const char* dsnm ) const
{
    for ( int idx=0; idx<genparams_.size(); idx++ )
	if ( genparams_[idx].name_ == dsnm )
	    return ids_[idx];
    return SynthID();
}


SynthID StratSynth::DataMgr::find( const PropertyRef& pr,
				   lms_idx_type lmsidx ) const
{
    const auto& dss = gtDSS( lmsidx );
    for ( int idx=0; idx<genparams_.size(); idx++ )
    {
	if ( genparams_[idx].type_ != SynthSeis::StratProp )
	    continue;

	const DataSet* ds = dss.get( idx );
	if ( !ds )
	    { pErrMsg("prop dataset not generated yet"); continue; }

	mDynamicCastGet( const SynthSeis::StratPropDataSet*, propds, ds )
	if ( !propds )
	    { pErrMsg("Huh"); continue; }

	if ( &pr == &propds->propRef() )
	    return ids_[idx];
    }

    return SynthID();
}


SynthID StratSynth::DataMgr::first( bool isps, bool genreq,
				    lms_idx_type lmsidx ) const
{
    const auto& dss = gtDSS( lmsidx );
    for ( int idx=0; idx<genparams_.size(); idx++ )
    {
	if ( genparams_[idx].isPS() == isps && (!genreq || dss.get(idx)) )
	    return ids_[idx];
    }
    return SynthID();
}


void StratSynth::DataMgr::getNames( BufferStringSet& nms, SubSelType ss,
				    bool noempty, lms_idx_type lmsidx ) const
{
    TypeSet<idx_type> idxs;
    gtIdxs( idxs, ss, noempty, lmsidx );
    nms.setEmpty();
    for ( auto idx : idxs )
	nms.add( genparams_.get(idx).name_ );
}


void StratSynth::DataMgr::getIDs( SynthIDSet& ids, SubSelType ss,
				  bool noempty, lms_idx_type lmsidx ) const
{
    TypeSet<idx_type> idxs;
    gtIdxs( idxs, ss, noempty, lmsidx );
    ids.setEmpty();
    for ( auto idx : idxs )
	ids.add( ids_.get(idx) );
}


void StratSynth::DataMgr::gtIdxs( TypeSet<idx_type>& idxs, SubSelType ss,
				  bool noempty, lms_idx_type lmsidx ) const
{
    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	if ( noempty && !haveDS(idx,lmsidx) )
	    continue;

	const GenParams& gp = genparams_.get( idx );
	bool doadd = true;
#	define mHandleCase(typ,val) case typ: doadd = val; break
	switch ( ss )
	{
	    mHandleCase( NoZO,		!gp.isZeroOffset() );
	    mHandleCase( OnlyZO,	gp.isZeroOffset() );
	    mHandleCase( NoPS,		!gp.isPS() );
	    mHandleCase( OnlyPS,	gp.isPS() );
	    mHandleCase( NoProps,	!gp.isStratProp() );
	    mHandleCase( OnlyProps,	gp.isStratProp() );
	    default: break;
	}
	if ( doadd )
	    idxs.add( idx );
    }
}


bool StratSynth::DataMgr::haveOfType( const SynthType typ ) const
{
    for ( auto gp : genparams_ )
	if ( gp.type_ == typ )
	    return true;
    return false;
}


const TimeDepthModelSet& StratSynth::DataMgr::d2TModels(
					lms_idx_type lmsidx ) const
{
    auto id = first( false, true );
    if ( !id.isValid() )
	id = first( true, true );

    if ( !id.isValid() )
    {
	// duh, nothing generated. need to generate on-the-fly:
	id = first( false, false );
	if ( !id.isValid() )
	    id = first( true, false );
	if ( id.isValid() )
	{
	    auto& self = *const_cast<DataMgr*>( this );
	    if ( !self.generate(id,SilentTaskRunnerProvider(),lmsidx) )
		id = SynthID();
	}
    }

    const auto* ds = getDataSet( id, lmsidx );
    if ( !ds )
    {
	static TimeDepthModelSet emptyd2tmdlset;
	return emptyd2tmdlset;
    }
    return ds->d2TModels();
}


BufferString StratSynth::DataMgr::nameOf( SynthID id ) const
{
    return getGenParams( id ).name_;
}


lms_idx_type StratSynth::DataMgr::lmsIndexOf( const DataSet* reqds ) const
{
    if ( reqds )
    {
	for ( auto ilm=0; ilm<lms_.size(); ilm++ )
	    if ( indexOf(reqds,ilm) >= 0 )
		return ilm;
    }
    return -1;
}


idx_type StratSynth::DataMgr::indexOf( const DataSet* reqds,
					lms_idx_type lmsidx ) const
{
    if ( reqds )
    {
	const auto& dss = gtDSS( lmsidx );
	for ( int idss=0; idss<dss.size(); idss++ )
	    if ( dss.get(idss) == reqds )
		return idss;
    }
    return -1;
}


const SynthSeis::GenParams& StratSynth::DataMgr::getGenParams(
						SynthID id ) const
{
    static GenParams defgenparams;
    const auto idx = gtIdx( id );
    return idx<0 ? defgenparams : genparams_[idx];
}


bool StratSynth::DataMgr::isPS( SynthID id ) const
{
    return getGenParams( id ).isPS();
}


bool StratSynth::DataMgr::isStratProp( SynthID id ) const
{
    return getGenParams( id ).isStratProp();
}


#define mDefDSGetFn(pubfn,protfn,partyp,cnst) \
cnst StratSynth::DataMgr::DataSet* StratSynth::DataMgr::pubfn( \
    partyp inp, lms_idx_type lmsidx ) cnst { return protfn( inp, lmsidx ); }

mDefDSGetFn( getDataSet, gtDS, SynthID, )
mDefDSGetFn( getDataSet, gtDS, SynthID, const )
mDefDSGetFn( getDataSetByIdx, gtDSByIdx, idx_type, )
mDefDSGetFn( getDataSetByIdx, gtDSByIdx, idx_type, const )


idx_type StratSynth::DataMgr::gtGenIdx( SynthID id, const TRProv& trprov ) const
{
    const auto idx = gtIdx( id );
    if ( idx < 0 )
    {
	static const char* msg = "Generate for non-existing synthetics ID";
	pErrMsg( msg );
	trprov.emitErrorMessage( mINTERNAL(msg) );
    }
    return idx;
}


#define mGetGenIdx(idx) \
    const int idx = gtGenIdx( id, trprov ); \
    if ( idx < 0 ) \
	return false

bool StratSynth::DataMgr::ensureGenerated( SynthID id, const TRProv& trprov,
					   lms_idx_type lmsidx ) const
{
    if ( nrTraces(lmsidx) < 1 )
	return true;

    mGetGenIdx(idx);
    return haveDS(idx,lmsidx) ? true : generate( id, trprov, lmsidx );
}


bool StratSynth::DataMgr::generate( SynthID id, const TRProv& trprov,
				    lms_idx_type lmsidx ) const
{
    mGetGenIdx(idx);
    auto& dss = const_cast<DataMgr*>(this)->gtDSS( lmsidx );

    DataSet* oldds = dss.get( idx );
    if ( oldds )
	unRefAndZeroPtr( oldds );
    dss.replace( idx, 0 );

    if ( !ensureElasticModels(trprov,lmsidx) )
	return false;

    DataSet* newds = generateDataSet( genparams_[idx], trprov, lmsidx );
    dss.replace( idx, newds );
    if ( newds )
    {
	newds->setID( id );
	ensurePropertyDataSets( trprov, lmsidx );
	ensureLevels( lmsidx );
    }

    return true;
}


void StratSynth::DataMgr::ensureLevels( lms_idx_type lmsidx ) const
{
    lmsidx = gtActualLMIdx( lmsidx );
    LevelSet& lvlset = *const_cast<LevelSet*>( levelsets_.get( lmsidx ) );
    if ( !lvlset.isEmpty() )
	return;

    const auto& laymod = layerModel( lmsidx );
    const auto nrtrcs = nrTraces( lmsidx );
    for ( int ilvl=0; ilvl<Strat::LVLS().size(); ilvl++ )
    {
	const auto id = Strat::LVLS().getIDByIdx( ilvl );
	auto& lvl = lvlset.add( id );
	const Strat::Level stratlvl = Strat::LVLS().get( id );
	for ( int itrc=0; itrc<nrtrcs; itrc++ )
	{
	    const auto& seq = laymod.sequence( iSeq(itrc) );
	    lvl.zvals_ += seq.depthPositionOf( stratlvl, mUdf(float) );
	}
    }
}


void StratSynth::DataMgr::getLevelDepths( LevelID id, ZValueSet& depths,
					  lms_idx_type lmsidx ) const
{
    const auto& lvlset = *levelsets_.get( gtActualLMIdx(lmsidx) );
    for ( auto lvl : lvlset.levels() )
	if ( lvl->id_ == id )
	    { depths = lvl->zvals_; return; }
    depths = ZValueSet( nrTraces(lmsidx), mUdf(float) );
}


void StratSynth::DataMgr::setPackLevelTimes( SynthID id, LevelID lvlid ) const
{
    for ( int ilm=0; ilm<lms_.size(); ilm++ )
    {
	DataSet* ds = gtDS( id, ilm );
	if ( !ds )
	    continue;

	const auto& lvlset = *levelsets_.get( ilm );
	const ZValueSet* zvals = 0;
	for ( auto lvl : lvlset.levels() )
	    if ( lvl->id_ == lvlid )
		{ zvals = &lvl->zvals_; break; }
	if ( !zvals )
	    continue;

	const auto& t2dmdls = d2TModels( ilm );
	if ( ds->isPS() )
	    { pErrMsg("TODO: set picks in PS gathers"); }
	else
	{
	    mDynamicCastGet(SynthSeis::PostStackDataSet*,postds,ds);
	    SeisTrcBuf& tbuf = postds->postStackPack().trcBuf();
	    auto nr2set = tbuf.size();
	    if ( zvals->size() != nr2set )
	    {
		pErrMsg("Sizes should be equal");
		nr2set = std::max(nr2set,zvals->size());
	    }
	    for ( auto itrc=0; itrc<nr2set; itrc++ )
	    {
		auto& trc = *tbuf.get( itrc );
		const float zval = zvals->get( itrc );
		const float tval = t2dmdls.get( itrc )->getTime( zval );
		trc.info().zref_ = trc.info().pick_ = tval;
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
		     size_type calceach )
    : ::ParallelTask( "Elastic Model Generator" )
    , lm_(lm)
    , elasticmodels_(ems)
    , calceach_(calceach)
{
    elasticmodels_.setSize( getNrTraces(lm_.size(),calceach_) );
}

od_int64 nrIterations() const
{
    return elasticmodels_.size();
}

uiString message() const
{
    if ( errmsg_.isEmpty() )
	return tr("Generating elastic model");
    else
	return errmsg_;
}

uiString nrDoneText() const
{
    return tr("Models done");
}

bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( idx_type imdl=(idx_type)start; imdl<=(idx_type)stop; imdl++ )
    {
	addToNrDone(1);
	const idx_type iseq = imdl * calceach_;
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	if ( seq.isEmpty() )
	    continue;

	ElasticModel& curem = *elasticmodels_[imdl];
	if ( !fillElasticModel(seq,curem) )
	    return false;
    }

    return true;
}

bool fillElasticModel( const Strat::LayerSequence& seq, ElasticModel& elmod )
{
    const ElasticPropSelection& eps = lm_.elasticPropSel();
    const PropertyRefSelection& props = lm_.propertyRefs();

    elmod.erase();
    uiString errmsg;
    if ( !eps.isValidInput(&errmsg) )
    {
	Threads::Locker locker( errmsglock_ );
	errmsg_ = tr("Cannot create elastic model as %1").arg( errmsg );
	return false;
    }

    ElasticPropGen elpgen( eps, props );
    const float srddepth = -1.f*mCast(float,SI().seismicReferenceDatum() );
    int firstidx = 0;
    if ( seq.startDepth() < srddepth )
	firstidx = seq.nearestLayerIdxAtZ( srddepth );

    if ( seq.isEmpty() )
    {
	Threads::Locker locker( errmsglock_ );
	errmsg_ = tr("Elastic model is not proper to generate synthetics as a "
		  "layer sequence has no layers");
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

	elmod += ElasticLayer( thickness, pval, sval, dval );
    }

    if ( elmod.isEmpty() )
    {
	Threads::Locker locker( errmsglock_ );
	errmsg_ = tr("After discarding layers with no thickness "
		     "no layers remained");
	return false;
    }

    return true;
}

static float cMaximumVpWaterVel()
{
    return 1510.f;
}

    const Strat::LayerModel&	lm_;
    const size_type		calceach_;
    ElasticModelSet&		elasticmodels_;
    Threads::Lock		errmsglock_;
    uiString			errmsg_;

};

} // namespace StratSynth


bool StratSynth::DataMgr::ensureElasticModels( const TRProv& trprov,
					       lms_idx_type lmsidx ) const
{
    ElasticModelSet& elmdls = *const_cast<ElasticModelSet*>(
			    elasticmodelsets_.get( gtActualLMIdx(lmsidx) ) );
    if ( !elmdls.isEmpty() )
	return true;

    ElasticModelCreator emc( layerModel(lmsidx), elmdls, calceach_ );
    return trprov.execute( emc );
}


namespace StratSynth
{

class PropertyDataSetsCreator : public ::ParallelTask
{ mODTextTranslationClass(StratSynth::PropertyDataSetsCreator)
public:

    typedef TaskRunnerProvider		TRProv;
    typedef Strat::LayerModel		LayerModel;
    typedef SynthSeis::DataSet		DataSet;

PropertyDataSetsCreator( DataMgr& dm, const DataSet& ds, const TRProv& trprov,
			 lms_idx_type lmsidx )
    : ::ParallelTask("Property Trace Creator")
    , datamgr_(dm)
    , lmsidx_(lmsidx)
    , t2dds_(ds)
    , trprov_(trprov)
    , lm_(dm.layerModel(lmsidx))
    , nrtrcs_(dm.nrTraces(lmsidx))
    , calceach_(dm.calcEach())
{
}

od_int64 nrIterations() const	{ return nrtrcs_; }
uiString message() const	{ return uiStrings::sCalculating(); }
uiString nrDoneText() const	{ return tr("Models done"); }

bool doPrepare( int )
{
    const ZSampling zrg = t2dds_.zRange();
    const int nrsamps = zrg.nrSteps();
    const auto& proprefs = lm_.propertyRefs();

    for ( int iprop=1; iprop<proprefs.size(); iprop++ )
    {
	SeisTrcBuf* tbuf = new SeisTrcBuf( true );
	for ( int itrc=0; itrc<nrtrcs_; itrc++ )
	{
	    SeisTrc* trc = new SeisTrc( nrsamps );
	    auto& ti = trc->info();
	    ti.setTrcKey( TrcKey::getSynth(itrc*calceach_+1) );
	    ti.coord_.setUdf();
	    ti.sampling_.start = zrg.start;
	    ti.sampling_.step = zrg.step;
	    tbuf->add( trc );
	}
	tbufs_ += tbuf;
    }

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( int iprop=1; iprop<lm_.propertyRefs().size(); iprop++ )
    {
	SeisTrcBuf& tbuf = *tbufs_[iprop-1];

	for ( idx_type itrc=(idx_type)start; itrc<=(idx_type)stop; itrc++ )
	{
	    SeisTrc& trc = *tbuf.get( itrc );
	    trc.setAll( mUdf(float) );
	    const auto& seq = lm_.sequence( datamgr_.iSeq(itrc) );
	    const auto nrlayers = seq.size();
	    if ( nrlayers < 1 )
		continue;

	    const auto& t2d = *t2dds_.d2TModels().get( itrc );

	    int layidx = 0;
	    const auto* lay = seq.layers().get( layidx );
	    float tlaytop = t2d.getTime( lay->zTop() );
	    float tlaybot = t2d.getTime( lay->zBot() );

	    const auto& tsampling = trc.info().sampling_;
	    const float halftstep = 0.5f * tsampling.step;
	    const float teps = halftstep * 0.001f;

	    for ( int isamp=0; isamp<trc.size(); isamp++ )
	    {
		const float tstart = tsampling.atIndex( isamp ) - halftstep;
		const float tstop = tstart + tsampling.step;
		if ( tlaytop > tstop || tlaybot < tstart )
		    continue; // sample outside sequence, leave undef

		float layval = lay->value( iprop );
		bool layvalisudf = mIsUdf( layval );
		float sumval = 0.f, sumwt = 0.f;
		while ( tlaybot < tstop )
		{
		    if ( !layvalisudf )
		    {
			const float tinside = tlaybot
					    - std::max( tstart, tlaytop );
			sumval += layval * tinside;
			sumwt += tinside;
		    }

		    layidx++;
		    if ( layidx >= nrlayers )
			break;

		    lay = seq.layers().get( layidx );
		    tlaytop = tlaybot;
		    tlaybot = t2d.getTime( lay->zBot() );
		    layval = lay->value( iprop );
		    layvalisudf = mIsUdf( layval );
		}

		if ( tlaytop < tstop && !layvalisudf )
		{
		    const float tinside = tstop - tlaytop;
		    sumval += layval * tinside;
		    sumwt += tinside;
		}

		if ( sumwt > teps )
		    trc.set( isamp, sumval / sumwt, 0 );
	    }
	}
    }

    return true;
}

bool doFinish( bool success )
{
    if ( !success )
	{ deepErase( tbufs_ ); return false; }

    SynthSeis::GenParams gp = t2dds_.genParams();
    gp.type_ = SynthSeis::StratProp;
    const auto& proprefs = lm_.propertyRefs();
    for ( int iprop=1; iprop<proprefs.size(); iprop++ )
    {
	const auto& propref = *proprefs.get( iprop );
	gp.name_.set( propref.name() );
	const BufferString dsnm( datamgr_.getFinalDataSetName(gp.name_,true) );
	auto* dp = new SeisTrcBufDataPack( tbufs_[iprop-1], Seis::Line,
			   SeisTrcInfo::TrcNr,
			   SynthSeis::PostStackDataSet::sDataPackCategory() );
	auto* ds = new SynthSeis::StratPropDataSet( gp, *dp, propref );
	ds->setName( dsnm );
	ds->getD2TFrom( t2dds_ );
	datamgr_.setDataSet( gp, ds, lmsidx_ );
    }

    return true;
}

    DataMgr&		datamgr_;
    const lms_idx_type	lmsidx_;
    const DataSet&	t2dds_;
    const LayerModel&	lm_;
    const TRProv&	trprov_;
    const size_type	nrtrcs_;
    const size_type	calceach_;
    ObjectSet<SeisTrcBuf> tbufs_;

};

} // namespace StratSynth


void StratSynth::DataMgr::ensurePropertyDataSets( const TRProv& trprov,
					lms_idx_type lmsidx ) const
{
    const DataSet* tdds = 0;
    const auto& dss = gtDSS( lmsidx );
    for ( const auto* ds : dss )
    {
	if ( !ds || ds->isEmpty() )
	    continue;

	const auto synthtype = ds->synthType();
	if ( synthtype == SynthSeis::StratProp )
	    return;
	else if ( synthtype == SynthSeis::PreStack )
	    tdds = ds;
	else if ( synthtype == SynthSeis::ZeroOffset )
	    { tdds = ds; break; }
    }
    if ( !tdds )
	return;

    PropertyDataSetsCreator creator( *const_cast<DataMgr*>(this), *tdds,
				     trprov, lmsidx );
    trprov.execute( creator );
}


namespace StratSynth
{

class PSAngleDataCreator : public ::Executor
{ mODTextTranslationClass(PSAngleDataCreator)
public:

PSAngleDataCreator( const SynthSeis::PreStackDataSet& pssd )
    : ::Executor("Angle Gather Creator" )
    , angledpnm_(pssd.name(), " (Angle Gather)" )
    , gathers_(pssd.preStackPack().getGathers())
    , raymodels_(&pssd.rayModels())
    , nrdone_(0)
    , anglecomputer_(new PreStack::ModelBasedAngleComputer)
{
    anglecomputer_->setFFTSmoother();
}

~PSAngleDataCreator()
{
}

od_int64 totalNr() const	{ return gathers_.size(); }
od_int64 nrDone() const		{ return nrdone_; }
uiString message() const	{ return tr("Calculating Angle Gathers"); }
uiString nrDoneText() const	{ return tr("Models done"); }

ObjectSet<Gather>& angleGathers()
{
    return anglegathers_;
}


int nextStep()
{
    if ( !gathers_.validIdx(nrdone_) )
	return Finished();
    const Gather* gather = gathers_[(int)nrdone_];
    anglecomputer_->outputDegrees( true );
    anglecomputer_->setOutputSampling( gather->posData() );
    anglecomputer_->gatherIsNMOCorrected( gather->isCorrected() );
    ConstRefMan<SynthSeis::RayModel> raymodel =
				     raymodels_->get(mCast(int,nrdone_));
    anglecomputer_->setRayTraceData( raymodel->rayTracerOutput() );
    RefMan<Gather> anglegather = anglecomputer_->computeAngles();
    if ( !anglegather )
	return ErrorOccurred();

    TypeSet<float> azimuths;
    gather->getAzimuths( azimuths );
    anglegather->setAzimuths( azimuths );
    anglegather->setName( angledpnm_ );
    anglegather->setBinID( gather->getBinID() );
    anglegathers_ += anglegather;
    nrdone_++;
    return MoreToDo();
}

    BufferString			angledpnm_;
    const RefObjectSet<Gather>		gathers_;
    ConstRefMan<SynthSeis::RayModelSet> raymodels_;
    RefObjectSet<Gather>		anglegathers_;
    RefMan<PreStack::ModelBasedAngleComputer>	anglecomputer_;
    od_int64				nrdone_;

};

} // namespace StratSynth


static bool sameRayPars( const IOPar& iop1, const IOPar& iop2 )
{
    uiString dum;
    PtrMan<RayTracer1D> rt1 = RayTracer1D::createInstance( iop1, dum );
    PtrMan<RayTracer1D> rt2 = RayTracer1D::createInstance( iop2, dum );
    return rt1 && rt2 && rt1->hasSameParams( *rt2 );
}


#define mErrRet( s ) { trprov.emitErrorMessage( s ); return 0; }
#define mErrRetInternal( s ) mErrRet( mINTERNAL(s) )


StratSynth::DataMgr::DataSet*
StratSynth::DataMgr::genRaySynthDataSet( const GenParams& gp,
					 const TRProv& trprov,
					 lms_idx_type lmsidx ) const
{
    if ( gp.isStratProp() || gp.isPSPostProc() )
	return 0;

    const DataSet* raymdlds = 0;
    const auto& dss = gtDSS( lmsidx );
    for ( int dsidx=0; dsidx<dss.size(); dsidx++ )
    {
	const auto* ds = dss[dsidx];
	if ( ds && !ds->isEmpty() && !ds->genParams().isStratProp() &&
	     !ds->genParams().isPSPostProc() &&
	     sameRayPars(gp.raypars_,ds->rayPars()) )
	    { raymdlds = ds; break; }
    }

    PtrMan<RaySynthGenerator> rsg;
    if ( raymdlds && !raymdlds->rayModels().isEmpty() )
    {
	rsg = new RaySynthGenerator( *const_cast<DataSet*>( raymdlds ) );
	// what purpose? New wavelet: OK. Otherwise?
    }
    else
    {
	const auto& elmdls = *elasticmodelsets_[ curLayerModelIdx() ];
	RayTracerRunner raytracerunner( elmdls, gp.raypars_ );
	if ( !trprov.execute(raytracerunner) )
	    return 0;

	const RefObjectSet<RayTracerData>& runnerres = raytracerunner.results();
	RefMan<SynthSeis::RayModelSet> raymodels = new SynthSeis::RayModelSet;
	for ( int imod=0; imod<runnerres.size(); imod++ )
	{
	    RefMan<SynthSeis::RayModel> raymodel =
				new SynthSeis::RayModel(*runnerres.get(imod) );
	    raymodels->add( raymodel.ptr() );
	}

	rsg = new RaySynthGenerator( gp, *raymodels.ptr() );
    }

    rsg->setNrStep( calceach_ );
    rsg->setName( BufferString("Generate ",gp.name_) );
    rsg->setWavelet( WaveletMGR().fetch(gp.wvltid_) );
    if ( !trprov.execute(*rsg) || !rsg->isResultOK() )
	return 0;

    DataSet& ds = rsg->dataSet();
    ds.ref();
    if ( !gp.isPS() )
	return &ds;

    mDynamicCastGet( SynthSeis::PreStackDataSet*, psds, &ds )
    mDynamicCastGet( const SynthSeis::PreStackDataSet*, raymdlpsds, raymdlds )
    if ( raymdlpsds )
	psds->setAngleData( raymdlpsds->angleData().getGathers() );
    else
    {
	PSAngleDataCreator angledatacr( *psds );
	if ( trprov.execute(angledatacr) )
	    psds->setAngleData( angledatacr.angleGathers() );
    }

    return &ds;
}


StratSynth::DataMgr::DataSet*
StratSynth::DataMgr::genPSPostProcDataSet( const GenParams& gp,
			const DataSet& inpds, const TrcKeyZSampling& cs,
			const TRProv& trprov ) const
{
    const bool isanglestack = gp.type_ == SynthSeis::AngleStack;
    mDynamicCastGet( const SynthSeis::PreStackDataSet&, psds, inpds );
    const GatherSetDataPack& gatherdp = psds.preStackPack();
    if ( gatherdp.isEmpty() )
	return 0;

    const GatherSetDataPack& angledp = psds.angleData();
    const auto& gatherset = gatherdp.getGathers();
    const auto& anglesset = angledp.getGathers();
    if ( anglesset.size() != gatherset.size() )
	return 0;

    PreStack::PropCalc::Setup calcsetup;
    calcsetup.anglerg_.set( mCast(float,gp.anglerg_.start),
			    mCast(float,gp.anglerg_.stop) );
    if ( isanglestack )
    {
	calcsetup.calctype_ = PreStack::PropCalc::Stats;
	calcsetup.stattype_ = Stats::Average;
    }
    else
    {
	calcsetup.calctype_ = PreStack::PropCalc::LLSQ;
	calcsetup.lsqtype_ = PreStack::PropCalc::Coeff;
    }

    PreStack::PropCalc pspropcalc( calcsetup );
    auto* tbuf = new SeisTrcBuf( true );
    const ZSampling zrg = gatherdp.zRange();
    const auto nrsamples = zrg.nrSteps() + 1;
    const SamplingData<float> tsampling( zrg );
    for ( int igath=0; igath<gatherset.size(); igath++ )
    {
	const auto& amplgather = *gatherset.get(igath);
	pspropcalc.setGather( amplgather );
	pspropcalc.setAngleData( *anglesset.get(igath) );

	auto* trc = new SeisTrc( nrsamples );
	auto& ti = trc->info();
	ti.sampling_ = tsampling;
	ti.setTrcKey( TrcKey::getSynth(igath+1) );
	for ( int isamp=0; isamp<nrsamples; isamp++ )
	    trc->set( isamp, pspropcalc.getVal(isamp), 0 );
	tbuf->add( trc );
    }

    /*TODO: add wavelet reshaping: on stack for isanglestack,
		on a copy of prestack input for AVO Gradient */

    SeisTrcBufDataPack* retdp =
	new SeisTrcBufDataPack( tbuf, Seis::Line, SeisTrcInfo::TrcNr,
			SynthSeis::PostStackDataSet::sDataPackCategory() );
    DataSet* retds = 0;
    if ( isanglestack )
	retds = new SynthSeis::AngleStackDataSet( gp, *retdp );
    else
	retds = new SynthSeis::AVOGradDataSet( gp, *retdp );
    retds->ref();

    return retds;
}


StratSynth::DataMgr::DataSet*
StratSynth::DataMgr::generateDataSet( const GenParams& gp, const TRProv& trprov,
				      lms_idx_type lmsidx ) const
{
    if ( layerModel(lmsidx).isEmpty() )
	mErrRet( tr("No geology generated") )

    DataSet* retsd = 0;

    if ( !gp.isPSPostProc() )
	retsd = genRaySynthDataSet( gp, trprov, lmsidx );
    else
    {
	const BufferString inpdsnm( getFinalDataSetName(gp.inpsynthnm_) );
	const SynthID inpid = find( inpdsnm );
	if ( !inpid.isValid() )
	    mErrRetInternal( BufferString("input '",inpdsnm,"' not present") )

	if ( !ensureGenerated(inpid,trprov,lmsidx) )
	    mErrRet( tr("Cannot generate '%1'").arg(inpdsnm) )

	auto* inpds = gtDS( inpid, lmsidx );
	if ( !inpds )
	    { pErrMsg("Huh"); }
	if ( !inpds || !inpds->isPS() )
	    mErrRetInternal( BufferString("input '",inpdsnm,"' is not PS") )

	TrcKeyZSampling cs( false );
	cs.hsamp_.setIsSynthetic();
	for ( int idx=0; idx<inpds->size(); idx++ )
	{
	    const SeisTrc& trc = *inpds->getTrace( idx );
	    if ( idx == 0 )
		cs.zsamp_ = trc.zRange();
	    else
	    {
		cs.zsamp_.include( trc.startPos(), false );
		cs.zsamp_.include( trc.endPos(), false );
	    }
	}

	retsd = genPSPostProcDataSet( gp, *inpds, cs, trprov );
    }

    if ( retsd )
	retsd->setName( getFinalDataSetName(retsd->name()) );

    return retsd;
}
