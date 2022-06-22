/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : Apr 2022
-*/


#include "testprog.h"

#include "moddepmgr.h"
#include "paralleltask.h"
#include "prestackgather.h"
#include "raytrace1d.h"
#include "rockphysics.h"
#include "seistrc.h"
#include "statrand.h"
#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratsynth.h"
#include "stratreftree.h"
#include "stratunitref.h"
#include "syntheticdataimpl.h"
#include "wavelet.h"

int nrseq_ = 251;
int calceach_ = 5;
int minnrlayers_ = 90;
int maxnrlayers_ = 110;

const Math::Formula* aiform_ = nullptr;
const Math::Formula* castagnaform_ = nullptr;
ManagedObjectSet<Strat::Level> lvls_;
ObjectSet<const Strat::LeavedUnitRef> units_;

static int nrTraces( bool isdec )
{
    if ( !isdec )
	return nrseq_;

    static int ret = nrseq_ / calceach_;
    return (nrseq_-1)%calceach_ == 0 ? ret+1 : ret;
}

static void cleanup()
{
    lvls_.setEmpty();
    units_.setEmpty();
}

static bool loadStdTree()
{
    BufferStringSet opts;
    Strat::RefTree::getStdNames( opts );
    mRunStandardTest( opts.size() > 2, "Has standard tree names" );
    const char* stdtreenm = "International_Standard_Chronostratigraphic_Chart";
    mRunStandardTest( opts.isPresent(stdtreenm), "Has standard framework name");

    Strat::LevelSet* ls = Strat::LevelSet::createStd( stdtreenm );
    mRunStandardTest( ls && ls->size() == 37, "Loaded the standard level set" );
    Strat::RefTree* rt = Strat::RefTree::createStd( stdtreenm );
    mRunStandardTest( rt && rt->source() == Repos::Rel &&
		      rt->lithologies().size() == 7 &&
		      rt->contents().size() == 2,
		      "Loaded the standard framework" );

    Strat::lvlSetMgr().setLVLS( ls );
    Strat::setRT( rt );

    cleanup();
    lvls_ += new Strat::Level( Strat::LVLS().getByName( "Top_Miocene" ) );
    lvls_ += new Strat::Level( Strat::LVLS().getByName( "Top_Cretaceous" ) );
    lvls_ += new Strat::Level( Strat::LVLS().getByName( "Top_Jurassic" ) );
    mRunStandardTest( lvls_.size() == 3 &&
		      lvls_[0]->id() == 4 &&
		      lvls_[1]->id() == 8 &&
		      lvls_[2]->id() == 10,
		      "Found target levels" )

    for ( const auto* lvl : lvls_ )
	units_.add( Strat::RT().getByLevel( lvl->id() ) );

    mRunStandardTest( units_.size() == 3 &&
		      units_[0]->fullCode() == "Cenozoic.Neogene.Miocene" &&
		      units_[1]->fullCode() == "Mesozoic.Cretaceous.Upper" &&
		      units_[2]->fullCode() == "Mesozoic.Jurassic.Upper",
		      "Found target leaved units" )

    return true;
}


static bool fillPRS( PropertyRefSelection& prs )
{
    prs.add( PROPS().getByName( "Density", false ) );
    prs.add( PROPS().getByName( "Pwave velocity", false ) );
    prs.add( PROPS().getByName( "Swave velocity", false ) );
    prs.add( PROPS().getByName( "Acoustic Impedance", false ) );
    prs.add( PROPS().getByName( "Porosity", false ) );
    prs.add( PROPS().getByName( "Vshale", false ) );
    prs.add( PROPS().getByName( "Water Saturation", false ) );

    mRunStandardTest( prs.size() == 8, "PropertyRef selection" );

    return true;
}


static bool getMathForms( const PropertyRefSelection& prs )
{
    aiform_ = ROCKPHYSFORMS().getByName( Mnemonic::defAI(),
			     "Acoustic Impedance from P-wave velocity" );
    castagnaform_ = ROCKPHYSFORMS().getByName( Mnemonic::defSVEL(),
				"Castagna's equation" );
    mRunStandardTest( aiform_, "Retrieved formula for Acoustic Impedance" );
    mRunStandardTest( castagnaform_, "Retrieved formula for SVel" );

    ObjectSet<Math::Formula> forms;
    forms.add( const_cast<Math::Formula*>( castagnaform_ ) );
    forms.add( const_cast<Math::Formula*>( aiform_ ) );
    for ( auto* form : forms )
    {
	for ( int iinp=0; iinp<form->nrInputs(); iinp++ )
	{
	    if ( form->isConst(iinp) || form->isSpec(iinp) )
		continue;

	    form->setInputDef( iinp,
		    prs.getByMnemonic(*form->inputMnemonic(iinp) )->name() );
	}

	if ( !form->isOK() )
	    return false;
    }

    return true;
}


static void setValues( const PropertyRefSelection& prs, float xpos,
		       Stats::RandGen& ugen, Stats::NormalRandGen& normgen,
		       Strat::Layer& lay )
{
    lay.setThickness( 1.f );
    lay.setValue( 1, normgen.get( 2.700f, 0.2f ) );	// Rho (g/cc)
    lay.setValue( 2, normgen.get( 2300.f, 150.f ) );	// Vp
    lay.setValue( 3, *castagnaform_, prs, xpos );	// Vs
    lay.setValue( 4, *aiform_, prs, xpos );		// AI
    lay.setValue( 5, normgen.get( 10.f, 2.f ) );	// Phi (%)
    lay.setValue( 6, 100.f * ugen.get() );		// Vsh (%)
    lay.setValue( 7, 100.f * ugen.get() );		// Sw (%)
}


class LayerModelFiller : public ParallelTask
{ mODTextTranslationClass(LayerModelFiller)
public:
LayerModelFiller( Strat::LayerModel& lm )
    : ParallelTask("Layer Model Filler")
    , lm_(lm)
{}

private:

od_int64 nrIterations() const override
{
    return lm_.size();
}

bool doPrepare( int /* nrthreads */ ) override
{
    return lvls_.size() == 3 && units_.size() == 3 &&
	   getMathForms( lm_.propertyRefs() );
}

bool doWork( od_int64 start, od_int64 stop, int ) override
{
    const PropertyRefSelection& prs = lm_.propertyRefs();

    Stats::RandGen uniformrg;
    Stats::NormalRandGen gaussianrg;
    const int nrseqs = lm_.size();
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	const int iseq = int(idx);
	const float xpos = float(iseq) / float(nrseqs-1);
	Strat::LayerSequence& seq = lm_.sequence( iseq );
	seq.setStartDepth( 0.f );
	const int nrlayers = uniformrg.getInt( minnrlayers_, maxnrlayers_ );
	const int startidx = nrlayers * 2./5.;
	const int stopidx = nrlayers * 3./5.;
	ObjectSet<Strat::Layer>& layers = seq.layers();
	for ( int ilay=0; ilay<nrlayers; ilay++ )
	{
	    const Strat::LeafUnitRef& leafunit = ilay < startidx
			    ? *units_.get(0)->firstLeaf()
			    : (ilay < stopidx ? *units_.get(1)->firstLeaf()
					      : *units_.get(2)->firstLeaf() );
	    auto* lay = new Strat::Layer( leafunit );
	    setValues( prs, xpos, uniformrg, gaussianrg, *lay );
	    layers.add( lay );
	}

	seq.prepareUse();
    }

    return true;
}

    Strat::LayerModel&	lm_;
};


static bool createModel( const PropertyRefSelection& prs,
			 Strat::LayerModel& lm )
{
    lm.propertyRefs() = prs;
    for ( int iseq=0; iseq<nrseq_; iseq++ )
	lm.addSequence();

    LayerModelFiller filler( lm );
    filler.execute();

    mRunStandardTest( lm.size() == nrseq_, "Layer model is set" );

    const ElasticPropSelection esel( true, prs );
    mRunStandardTest( esel.isOK(&prs), "Has elastic properties selection" );
    lm.setElasticPropSel( esel );

    return true;
}


static bool addGenerators( StratSynth::DataMgr& datamgr,
			const char* wvltnm,
			StratSynth::DataMgr::SynthID& defid,
			StratSynth::DataMgr::SynthID& presdid,
			StratSynth::DataMgr::SynthID& anglestackid,
			StratSynth::DataMgr::SynthID& avogradid,
			TypeSet<StratSynth::DataMgr::SynthID>& instantattribids,
			TypeSet<StratSynth::DataMgr::SynthID>& propids,
			const TypeSet<float>& offsets )
{
    // Post-stack
    SynthGenParams defsgp;
    defsgp.setWavelet( wvltnm );
    defsgp.createName( defsgp.name_ );
    defid = datamgr.addSynthetic( defsgp );

    // Pre-stack
    SynthGenParams presgp( SynthGenParams::PreStack );
    presgp.setWavelet( wvltnm );
    presgp.raypars_.set( RayTracer1D::sKeyOffset(), offsets );
    presgp.createName( presgp.name_ );
    presdid = datamgr.addSynthetic( presgp );

    // Angle stack
    SynthGenParams assgp( SynthGenParams::AngleStack );
    assgp.inpsynthnm_.set( datamgr.nameOf(presdid) );
    assgp.anglerg_ = Interval<float>( 3.f, 13.f );
    assgp.createName( assgp.name_ );
    anglestackid = datamgr.addSynthetic( assgp );

    // AVO Gradient
    SynthGenParams agsgp( SynthGenParams::AVOGradient );
    agsgp.inpsynthnm_.set( datamgr.nameOf(presdid) );
    agsgp.anglerg_ = Interval<float>( 13.f, 23.f );
    agsgp.createName( agsgp.name_ );
    avogradid = datamgr.addSynthetic( agsgp );

    // Instant attribs
    SynthGenParams iattrsgp( SynthGenParams::InstAttrib );
    TypeSet<Attrib::Instantaneous::OutType> attribdefs;
    attribdefs += Attrib::Instantaneous::Amplitude;
    attribdefs += Attrib::Instantaneous::Frequency;
    datamgr.addInstAttribSynthetics( defid, attribdefs, instantattribids );

    SynthGenParams attribsgp( SynthGenParams::InstAttrib );
    attribsgp.inpsynthnm_.set( datamgr.nameOf(defid) );
    attribsgp.attribtype_ = Attrib::Instantaneous::QFactor;
    attribsgp.createName( attribsgp.name_ );
    instantattribids += datamgr.addSynthetic( attribsgp );

    // Properties
    const PropertyRefSelection& prs = datamgr.layerModel().propertyRefs();
    PropertyRefSelection propsel( false );
    propsel.add( prs.getByMnemonic( Mnemonic::defDEN() ) );
    propsel.add( prs.getByMnemonic( Mnemonic::defAI() ) );
    propsel.add( prs.getByMnemonic( Mnemonic::defPHI() ) );
    BufferStringSet propnms;
    for ( const auto* pr : propsel )
	propnms.add( pr->name() );

    datamgr.addPropertySynthetics( &propids, &propnms );

    return true;
}


static bool ensureAllGenerated( StratSynth::DataMgr& datamgr,
		StratSynth::DataMgr::SynthID defid,
		StratSynth::DataMgr::SynthID presdid,
		StratSynth::DataMgr::SynthID anglestackid,
		StratSynth::DataMgr::SynthID avogradid,
		const TypeSet<StratSynth::DataMgr::SynthID>& instantattribids,
		const TypeSet<StratSynth::DataMgr::SynthID>& propids )
{
    mRunStandardTestWithError( datamgr.ensureGenerated( defid ) &&
	       datamgr.hasValidDataSet(defid),
	       "Generate default synthetic", datamgr.errMsg().getText() );

    mRunStandardTestWithError( datamgr.ensureGenerated( presdid ) &&
	       datamgr.hasValidDataSet(presdid),
	       "Generate pre-stack synthetics", datamgr.errMsg().getText() );

    mRunStandardTestWithError( datamgr.ensureGenerated( anglestackid ) &&
	       datamgr.hasValidDataSet(anglestackid),
	       "Generate angle stacks", datamgr.errMsg().getText() );

    mRunStandardTestWithError( datamgr.ensureGenerated( avogradid ) &&
	       datamgr.hasValidDataSet(avogradid),
	       "Generate AVO gradient", datamgr.errMsg().getText() );

    TypeSet<StratSynth::DataMgr::SynthID> instantattrs = instantattribids;
    const StratSynth::DataMgr::SynthID lastattrid = instantattrs.pop();
    mRunStandardTestWithError(
		datamgr.ensureInstantAttribsDataSet(instantattrs) &&
		datamgr.hasValidDataSet( instantattrs.first() ) &&
		datamgr.hasValidDataSet( instantattrs.last() ),
		"Generate two instant attributes", datamgr.errMsg().getText() );

    mRunStandardTestWithError( datamgr.ensureGenerated( lastattrid ) &&
		datamgr.hasValidDataSet( lastattrid ),
		"Generate a single instant attribute",
		datamgr.errMsg().getText() );

    mRunStandardTestWithError( propids.size() == 3 &&
	    datamgr.ensurePropertyDataSets( nullptr, -1, 0.001 ),
	    "Generate time-converted properties", datamgr.errMsg().getText() );

    return true;
}


static bool testDataMgr( StratSynth::DataMgr& datamgr,
		StratSynth::DataMgr::SynthID defid,
		StratSynth::DataMgr::SynthID presdid,
		StratSynth::DataMgr::SynthID anglestackid,
		StratSynth::DataMgr::SynthID avogradid,
		const TypeSet<StratSynth::DataMgr::SynthID>& instantattribids,
		const TypeSet<StratSynth::DataMgr::SynthID>& propids,
		const TypeSet<float>& offsets )
{
    if ( !ensureAllGenerated(datamgr,defid,presdid,anglestackid,avogradid,
			     instantattribids,propids) )
	return false;

    const int nrpos = nrTraces( datamgr.calcEach() > 1 );

    const StratSynth::LevelSet& sslvls = datamgr.levels();
    mRunStandardTest( sslvls.size() == Strat::LVLS().size() &&
		      sslvls.isPresent( lvls_.get(0)->id() ) &&
		      sslvls.isPresent( lvls_.get(1)->id() ) &&
		      sslvls.isPresent( lvls_.get(2)->id() ),
		      "Generated levels are found" );
    const StratSynth::Level& ktlvl = sslvls.get( lvls_.get(1)->id() );
    mRunStandardTest( ktlvl.size() == nrpos &&
	    !mIsUdf(ktlvl.zvals_.first()) && !mIsUdf(ktlvl.zvals_.last()),
	    "Level size" );

    const Strat::LayerModel& laymodel = datamgr.layerModel();

    ConstRefMan<SyntheticData> defsd = datamgr.getDataSet( defid );
    mRunStandardTest( defsd && !defsd->isPS() &&
		      defsd->nrPositions() == nrpos,
			"Default dataset size" );
    const SeisTrc* lasttrc = defsd->getTrace( nrpos-1 );
    const TrcKey lasttk = lasttrc->info().trcKey();
    mRunStandardTest( lasttrc && lasttrc->info().isSynthetic() &&
		      lasttrc->info().trcNr() == laymodel.size(),
			"Last trace info is correct" );
    const BufferString newnm( "stat 120 offset 0 renamed" );
    mRunStandardTestWithError( datamgr.updateSyntheticName( defid, newnm ),
			      "Renamed synthetic",
			      datamgr.errMsg().getText() );

    ConstRefMan<SyntheticData> presd = datamgr.getDataSet( presdid );
    mRunStandardTest( presd && presd->isPS() &&
		      presd->nrPositions() == nrpos,
			"Default dataset size for prestack" );
    mDynamicCastGet(const PreStackSyntheticData*,presddp,presd.ptr())
    const PreStack::GatherSetDataPack& presdamps = presddp->preStackPack();
    const PreStack::GatherSetDataPack& presdangles = presddp->angleData();
    const PreStack::Gather* lastgather = presdamps.getGathers().last();
    int lastoffsidx = offsets.size()-1; //TODO: should be const
    const float lastoffs = offsets[lastoffsidx];
    SeisTrcInfo lastgathertrc1info, lastgathertrc2info, lastangletrcinfo;
    bool haslasttrace1 = false, haslasttrace2 = false, haslasttrace3 = false;
    const SeisTrc* lastgathertrc1 =
			    presdamps.getTrace( nrpos-1, lastoffsidx );
    if ( lastgathertrc1 )
    {
	lastgathertrc1info = lastgathertrc1->info();
	haslasttrace1 = true;
    }
    const SeisTrc* lastgathertrc2 =
			    presddp->getTrace( nrpos-1, &lastoffsidx );
    if ( lastgathertrc2 )
    {
	lastgathertrc2info = lastgathertrc2->info();
	haslasttrace2 = true;
    }
    const SeisTrc* lastangletrc =
			    presdangles.getTrace( nrpos-1, lastoffsidx );
    if ( lastangletrc )
    {
	lastangletrcinfo = lastangletrc->info();
	haslasttrace3 = true;
    }
    mRunStandardTest( lastgather && haslasttrace1 && haslasttrace2 &&
		      haslasttrace3, "Has pre-stack traces" );
    mRunStandardTest( lastgather->getTrcKey().isSynthetic() &&
		      lastgathertrc1info.isSynthetic() &&
		      lastgathertrc2info.isSynthetic() &&
		      lastangletrcinfo.isSynthetic() &&
		      lastgather->getTrcKey() == lasttk &&
		      lastgathertrc1info.trcKey() == lasttk &&
		      lastgathertrc2info.trcKey() == lasttk &&
		      lastangletrcinfo.trcKey() == lasttk &&
		      mIsEqual(lastoffs,lastgathertrc1info.offset,1e-3f) &&
		      mIsEqual(lastoffs,lastgathertrc2info.offset,1e-3f) &&
		      mIsEqual(lastoffs,lastangletrcinfo.offset,1e-3f),
		      "Pre-stack gather/trace info is OK" );

    TypeSet<StratSynth::DataMgr::SynthID> checkids;
    TypeSet<SynthGenParams::SynthType> expectedtypes;
    checkids += anglestackid;
    checkids += avogradid;
    checkids.append( instantattribids );
    expectedtypes += SynthGenParams::AngleStack;
    expectedtypes += SynthGenParams::AVOGradient;
    for ( int idx=0; idx<instantattribids.size(); idx++ )
	expectedtypes += SynthGenParams::InstAttrib;

    for ( int idx=0; idx<checkids.size(); idx++ )
    {
	ConstRefMan<SyntheticData> retsd = datamgr.getDataSet( checkids[idx] );
	mRunStandardTest( retsd && retsd->synthType() == expectedtypes[idx] &&
		retsd->nrPositions() == nrpos &&
		retsd->getTrace( nrpos-1 )->info().isSynthetic() &&
		retsd->getTrace( nrpos-1 )->info().trcKey() == lasttk,
		"Checked derived synthetic data" );
    }

    TypeSet<StratSynth::DataMgr::SynthID> retpropids;
    datamgr.getIDs( retpropids, StratSynth::DataMgr::OnlyProps, true );
    mRunStandardTest( retpropids == propids, "Generated properties" );
    ConstRefMan<SyntheticData> propsd = datamgr.getDataSet( propids.last() );
    const SeisTrc* lastproptrc =
			propsd ? propsd->getTrace(nrpos-1) : nullptr;
    mRunStandardTest( lastproptrc->info().trcKey() == lasttrc->info().trcKey(),
			"Last property trace info is correct" );

    return true;
}

static bool testAllMgrs( const Strat::LayerModelSuite& lms )
{
    const Wavelet syntheticricker( true, 50.f, 0.004f, 1.f );

    StratSynth::DataMgr datamgr( lms );
    datamgr.setCalcEach( calceach_ );
    datamgr.setWavelet( syntheticricker );

    TypeSet<float> offsets;
    for ( int idx=0; idx<21; idx++ )
	offsets += 5.f * idx;

    StratSynth::DataMgr::SynthID defid, presdid, anglestackid, avogradid;
    TypeSet<StratSynth::DataMgr::SynthID> instantattribids, propids;
    if ( !addGenerators(datamgr,syntheticricker.name(),
			defid,presdid,anglestackid,avogradid,
			instantattribids,propids,offsets) )
	return false;

    if ( !testDataMgr(datamgr,defid,presdid,anglestackid,avogradid,
		      instantattribids,propids,offsets) )
	return false;

    StratSynth::DataMgr* prodmgr = datamgr.getProdMgr();
    if ( !prodmgr || prodmgr == &datamgr || prodmgr->calcEach() != 1 ||
	 !testDataMgr(*prodmgr,defid,presdid,anglestackid,avogradid,
		      instantattribids,propids,offsets) )
	return false;

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    OD::ModDeps().ensureLoaded( "WellAttrib" );
    if ( !loadStdTree() )
	{ cleanup(); return 1; }

    PropertyRefSelection prs;
    Strat::LayerModelSuite lms;
    if ( !fillPRS(prs) ||
	 !createModel(prs,lms.baseModel()) ||
	 !testAllMgrs(lms) )
	{ cleanup(); return 1; }

    cleanup();
    return 0;
}
