/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "testprog.h"
#include "moddepmgr.h"

#include "nrbytes2string.h"
#include "odsysmem.h"
#include "paralleltask.h"
#include "rockphysics.h"
#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "statrand.h"
#include "stratreftree.h"
#include "stratunitref.h"

using namespace Strat;

bool simpledraw_ = false;
bool usemath_ = false;
int nrseq_ = 25;	// Too low to detect mem/speed issues: try --nrseq 5000
int nrlayers_ = 50;	/* Too low to detect mem/speed issues: try --nrlay 400
			     or the combination: --nrseq 1 --nrlay 5000000 */

od_int64 freemem_;
od_int64 totmem_;
NrBytesToStringCreator fmtcreator_;
Stats::RandGen uniformrg_ = Stats::randGen();
Stats::NormalRandGen gaussianrg_;
const Math::Formula* aiform_ = nullptr;
const Math::Formula* castagnaform_ = nullptr;


class TestLayerValue
{
public:
		TestLayerValue();
private:
    double	val_;
};


class TestLayer
{
public:

TestLayer( const LeafUnitRef& ref )
    : ref_(&ref)
{
}

~TestLayer()
{
}

private:

    const LeafUnitRef*	ref_;
    float		ztop_;
    ObjectSet<LayerValue> vals_;
    const Content*	content_ = nullptr;
    char		buf_[57];
};


static void printTestUsage( const char* prognm )
{
    od_ostream& strm = tstStream();
    strm << "Usage: " << prognm << od_newline;
    strm << "Creates a layer model with a fixed number of sequences\n";
    strm << "and a fixed number of layers per sequence." << od_newline;
    strm << "Optional arguments:" << od_newline;
    strm << "\t --nrseq" << "\tNumber of sequences to draw" << od_newline;
    strm << "\t --nrlay" << "\tNumber of layers per sequence to draw";
    strm << od_newline;
    strm << "\t --simpledraw" << "\tSet a value of 1 for each property ";
    strm << "in each layer (ultrafast, ignores '--math')" << od_newline;
    strm << "\t --math" << "\t\tSet values for AI, SVel from a math-based ";
    strm << "formula (slowest)" << od_newline;
    strm << "\t --help" << "\t\tPrints the usage and exits" << od_endl;
}

static void printMem( const char* msg )
{
    if ( quiet_ )
	return;

    OD::getSystemMemory( totmem_, freemem_ );
    tstStream() << msg << ": " << fmtcreator_.getString(freemem_) << od_newline;
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


static bool getMathForms()
{
    if ( !usemath_ )
	return true;

    aiform_ = ROCKPHYSFORMS().getByName( Mnemonic::defAI(),
			     "Acoustic Impedance from P-wave velocity" );
    castagnaform_ = ROCKPHYSFORMS().getByName( Mnemonic::defSVEL(),
				"Castagna's equation" );
    mRunStandardTest( aiform_, "Retrieved formula for Acoustic Impedance" );
    mRunStandardTest( castagnaform_, "Retrieved formula for SVel" );

    return true;
}


static void setValues( const PropertyRefSelection& prs, float xpos, Layer& lay )
{
    if ( simpledraw_ )
    {
	// The easy way (values won't make sense):
	for ( int idx=0; idx<prs.size(); idx++ )
	    lay.setValue( idx, 1.f );
	return;
    }

    lay.setThickness( 1.f );
    lay.setValue( 1, gaussianrg_.get( 2700.f, 200.f ) ); // Rho
    lay.setValue( 2, gaussianrg_.get( 2300.f, 150.f ) ); // Vp
    if ( usemath_ )
    {
	lay.setValue( 3, *castagnaform_, prs, xpos );	 // Vs
	lay.setValue( 4, *aiform_, prs, xpos );		 // AI
    }
    else
    {
	lay.setValue( 3, gaussianrg_.get( 1600.f, 130.f ) ); // Vs
	lay.setValue( 4, gaussianrg_.get( 11000.f, 1500.f ) ); // AI
    }

    lay.setValue( 5, gaussianrg_.get( 10.f, 2.f ) );	 // Phi (%)
    lay.setValue( 6, 100.f * uniformrg_.get() );	 // Vsh (%)
    lay.setValue( 7, 100.f * uniformrg_.get() ); // Sw (%)
}


static bool mUnusedVar testArrayLayers( const PropertyRefSelection& prs )
{
    printMem( "Free memory before creating isolated layers in C Array" );

    const LeafUnitRef& udfleaf = RT().undefLeaf();

    const od_uint64 totnrlayers = od_uint64(nrseq_) * nrlayers_;

    auto* layers = new TestLayer*[totnrlayers];
//    auto* layers = new Layer*[totnrlayers];
    for ( od_uint64 ilay=0; ilay<totnrlayers; ilay++ )
    {
	layers[ilay] = new TestLayer( udfleaf );
//	layers[ilay] = new Layer( udfleaf );
//	setValues( prs, 0.f, *layers[ilay] );
    }

    printMem( "Free memory after creating isolated layers in C Array" );
    for ( od_uint64 ilay=0; ilay<totnrlayers; ilay++ )
    {
	delete layers[ilay];
	layers[ilay] = nullptr;
    }

    delete [] layers;
    Threads::sleep( 2. );
    printMem( "Free memory after deleting isolated layers in C Array" );

    return true;
}


static bool mUnusedVar testObjectSetLayers( const PropertyRefSelection& prs )
{
    printMem( "Free memory before creating isolated layers in ObjectSet" );

    const LeafUnitRef& udfleaf = RT().undefLeaf();

    const od_uint64 totnrlayers = od_uint64(nrseq_) * nrlayers_;
    ManagedObjectSet<Layer> layers;
    for ( od_uint64 ilay=0; ilay<totnrlayers; ilay++ )
    {
	auto* lay = new Layer( udfleaf );
	setValues( prs, 0.f, *lay );
	layers += lay;
    }

    printMem( "Free memory after creating isolated layers in ObjectSet" );
    if ( layers.isManaged() )
	layers.setEmpty();
    else
	deepErase( layers );

    printMem( "Free memory after deleting isolated layers in ObjectSet" );

    return true;
}


mDefParallelCalc1Par( LayerModelFiller,
	od_static_tr("test_layermodel","Filling layer model values"),
	LayerModel&,lm)
mDefParallelCalcBody(
    const PropertyRefSelection& prs = lm_.propertyRefs();
    const LeafUnitRef& udfleaf = RT().undefLeaf();
    const int nrseqs = lm_.size();
    ,
    const int iseq = int(idx);
    const float xpos = float(iseq) / float(nrseqs-1);
    LayerSequence& seq = lm_.sequence( iseq );
    seq.setStartDepth( 0.f );
    ObjectSet<Layer>& layers = seq.layers();
    for ( int ilay=0; ilay<nrlayers_; ilay++ )
    {
	auto* lay = new Layer( udfleaf );
	setValues( prs, xpos, *lay );
	layers.add( lay );
    }
    seq.prepareUse();
    ,
    // lm_.prepareUse(); Not efficient, but same (not parallel)
)

static bool mUnusedVar createModel( const PropertyRefSelection& prs )
{
    printMem( "Free memory before creating layer model" );

    auto* lm = new LayerModel;
    lm->propertyRefs() = prs;
    for ( int iseq=0; iseq<nrseq_; iseq++ )
	lm->addSequence();

    auto* filler = new LayerModelFiller( lm->size(), *lm );
    filler->execute();
    delete filler;

    printMem( "Free memory after creating layer model" );
    delete lm;
    printMem( "Free memory after deleting the layer model" );

    return true;
}

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    OD::ModDeps().ensureLoaded("General");

    CommandLineParser& clp = clParser();
    if ( clp.hasKey("help") )
    {
	printTestUsage( clp.getExecutableName() );
	return 1;
    }

    clp.setKeyHasValue( "nrseq" );
    clp.setKeyHasValue( "nrlay" );
    simpledraw_ = clp.hasKey( "simpledraw" );
    usemath_ = clp.hasKey( "math" );
    clp.getVal( "nrseq", nrseq_ );
    clp.getVal( "nrlay", nrlayers_ );

    if ( !simpledraw_ )
    {
	uniformrg_.init( 12345 );
	gaussianrg_.init( 54321 );
    }

    OD::getSystemMemory( totmem_, freemem_ );
    fmtcreator_ = NrBytesToStringCreator( totmem_ );
    printMem( "Free memory when starting the Application" );

    OD::ModDeps().ensureLoaded("Strat");

    PropertyRefSelection prs;
    if ( !fillPRS(prs)
	 || !getMathForms()
//	 || !testArrayLayers(prs)
//	 || !testObjectSetLayers(prs)
	 || !createModel(prs)
	 )
	return 1;

    printMem( "Free memory before closing the Application" );

    return 0;
}
