/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "testprog.h"

#include "elasticpropsel.h"
#include "mathproperty.h"
#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlayseqgendesc.h"
#include "stratsinglaygen.h"
#include "stratreftree.h"

static float xpos = 0.25f;
static float z0 = 300.f;
static int nrlayers = 5;

using namespace Strat;

static LayerGenerator* layerGenerator( const LayerGenerator* gen =nullptr )
{
    static PtrMan<LayerGenerator> thegen;
    if ( !thegen && gen )
        thegen = gen->clone();

    return thegen.ptr();
}


static LayerSequenceGenDesc& layerGenDesc(
				const LayerSequenceGenDesc* oth =nullptr )
{
    static LayerSequenceGenDesc thedesc( RT() );
    if ( oth )
        thedesc = *oth;
    return thedesc;
}


static bool fillPRS( PropertyRefSelection& prs )
{
    prs.add( PROPS().getByName( Mnemonic::defAI().description(), false ) );
    prs.add( PROPS().getByName( PropertyRef::standardDenStr(), false ) );
    prs.add( PROPS().getByName( PropertyRef::standardPVelStr(), false ) );

    mRunStandardTest( prs.size() == 4, "PropertyRef selection" );
    mRunStandardTest( prs.get(1)->hasFixedDef(),
		      "AI property has a fixed math definition" );

    return true;
}


static bool setProperties( const PropertyRefSelection& prs, PropertySet& props )
{
    mRunStandardTest( props.size() == prs.size(), "Number of properties" );
    mDynamicCastGet(ValueProperty*,thickprop,props.get(0))
    mDynamicCastGet(MathProperty*,aiprop,props.get(1))
    mDynamicCastGet(ValueProperty*,denprop,props.get(2))
    mDynamicCastGet(ValueProperty*,pvelprop,props.get(3))
    mRunStandardTest( thickprop && aiprop && denprop && pvelprop,
		      "Properties type" );

    thickprop->setValue( 75.f );
    denprop->setValue( 2.6f );
    pvelprop->setValue( 3200.f );

    return true;
}


static bool testLayer( const Layer& lay )
{
    return mIsEqual(lay.thickness(),75.f,1e-2f) &&
	   mIsEqual(lay.value(1),8320.f,1e-2f) &&
	   mIsEqual(lay.value(2),2.6f,1e-5f) &&
	   mIsEqual(lay.value(3),3200.f,1e-2f);
}


static bool testGenerator( const PropertyRefSelection& prs )
{
    SingleLayerGenerator slg( &RT().undefLeaf() );
    slg.syncProps( prs );
    if ( !setProperties(prs,slg.properties()) )
	return false;

    const LayerGenerator* slgcp = layerGenerator( &slg );
    mRunStandardTest( slgcp, "Cloned single generator" );

    LayerSequence seq( &prs );
    const Property::EvalOpts eo( Property::EvalOpts::New, xpos );
    mRunStandardTestWithError( slg.generateMaterial(seq,eo) &&
			       seq.size() == 1 && seq.propertyRefs() == prs &&
			       seq.layers().last()->nrValues() == prs.size(),
			       "Generating a single layer",
			       toString(slg.errMsg()) );
    const Layer& lay0 = *seq.layers().last();
    mRunStandardTest( lay0.isMath(1), "Generated AI value is math-based" );
    mRunStandardTest( testLayer(lay0), "Layer values" );

    mRunStandardTestWithError( slgcp->generateMaterial(seq,eo) &&
			       seq.size() == 2 && seq.propertyRefs() == prs &&
			       seq.layers().last()->nrValues() == prs.size(),
			       "Generating a single layer",
			       toString(slgcp->errMsg()) );
    mRunStandardTest( seq.layers().last()->isMath(1),
		      "Cloned generator AI value is math-based" );
    mRunStandardTest( testLayer(*seq.layers().last()), "Layer values" );

    // Regression: syncProps used to destroy MathProperty formulas while
    // generated layers still evaluated them (use-after-free in synthetics).
    slg.syncProps( prs );
    if ( !setProperties(prs,slg.properties()) )
	return false;

    mRunStandardTest( testLayer(*seq.layers().get(0)),
		      "Math layer survives generator syncProps (layer 0)" );
    mRunStandardTest( testLayer(*seq.layers().get(1)),
		      "Math layer survives generator syncProps (layer 1)" );

    return true;
}


static bool testSharedFormulaCache( const PropertyRefSelection& prs )
{
    LayerModel lm;
    lm.propertyRefs() = prs;
    LayerSequence& seq = lm.addSequence();

    const int aidx = 1;
    const Math::Formula& aiform = prs.get(aidx)->fixedDef().getForm();
    ConstRefMan<SharedFormula> sf0 = lm.getSharedFormula( aidx, aiform );
    ConstRefMan<SharedFormula> sf1 = lm.getSharedFormula( aidx, aiform );
    mRunStandardTest( sf0.ptr() && sf0.ptr() == sf1.ptr(),
		      "LayerModel reuses one SharedFormula per property" );

    auto* lay = new Layer( RT().undefLeaf() );
    lay->setThickness( 75.f );
    lay->setValue( 2, 2.6f );
    lay->setValue( 3, 3200.f );
    lay->setValue( aidx, *sf0, prs, xpos );
    seq.layers() += lay;

    mRunStandardTest( lay->isMath(aidx), "SharedFormula attaches as math" );
    mRunStandardTest( testLayer(*lay), "SharedFormula evaluates AI = Den*Vp" );

    Layer laycopy( *lay );
    mRunStandardTest( laycopy.isMath(aidx) && testLayer(laycopy),
		      "Cloned layer keeps shared math evaluation" );

    lm.setEmpty();
    mRunStandardTest( true, "LayerModel clear after shared math use" );

    return true;
}


static bool testDesc( const PropertyRefSelection& prs )
{
    const LayerGenerator* slg = layerGenerator();

    LayerSequenceGenDesc gendesc( RT() );
    gendesc.setStartDepth( z0 );
    gendesc.setPropSelection( prs );
    const ElasticPropSelection elprop( RefLayer::Acoustic, prs );
    mRunStandardTestWithError( elprop.isOK(&prs) && elprop.size() == 2,
			       "Elastic Property selection",
			       toString(elprop.errMsg()) );
    elprop.fillPar( gendesc.getWorkBenchParams() );
    for ( int idx=0; idx<nrlayers; idx++ )
	gendesc.add( slg->clone() );

    LayerSequenceGenDesc gendesccp( gendesc );
    LayerSequence seq( &prs );
    mRunStandardTestWithError( gendesccp.prepareGenerate() &&
			       gendesccp.generate( seq, xpos ) &&
			       seq.size() == nrlayers &&
			       seq.layers().last()->nrValues() == prs.size() &&
			       seq.propertyRefs() == prs,
			       "Generating a sequence from a set of generators",
			       toString(gendesccp.errMsg()) );
    mRunStandardTest( seq.layers().last()->isMath(1),
		      "Sequence AI value is math-based" );
    mRunStandardTest( testLayer(*seq.layers().last()), "Layer values" );
    mRunStandardTest( mIsEqual(seq.startDepth(),z0,z0*1e-6f), "Top depth" );

    // Prop selection refresh reclones generator properties; existing sequences
    // must keep evaluating math.
    gendesccp.setPropSelection( prs );
    mRunStandardTest( testLayer(*seq.layers().first()) &&
		      testLayer(*seq.layers().last()),
		      "Math layers survive setPropSelection" );

    const LayerSequenceGenDesc& gendesccp2 = layerGenDesc( &gendesccp );
    LayerSequence seq2( &prs );
    mRunStandardTestWithError( gendesccp2.prepareGenerate() &&
			       gendesccp2.generate( seq2, xpos ) &&
			       seq2.size() == nrlayers &&
			       seq2.layers().last()->nrValues() == prs.size() &&
			       seq2.propertyRefs() == prs,
			       "Generating a sequence from a set of generators",
			       toString(gendesccp2.errMsg()) );
    mRunStandardTest( testLayer(*seq2.layers().last()), "Layer values" );
    mRunStandardTest( mIsEqual(seq2.startDepth(),z0,z0*1e-6f), "Top depth" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    OD::ModDeps().ensureLoaded("Strat");

    PropertyRefSelection prs;
    if ( !fillPRS(prs) || !testGenerator(prs) || !testSharedFormulaCache(prs)
      || !testDesc(prs) )
	return 1;

    return 0;
}
