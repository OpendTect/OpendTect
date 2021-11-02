/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "elasticpropsel.h"
#include "mathspecvars.h"
#include "rockphysics.h"
#include "testprog.h"
#include "unitofmeasure.h"


#define mTestValErr(var,val) \
	{ od_cout() << "Fail:\n" << #var <<'='<< var << \
	    ", not " << val << od_endl; return false; }
#define mTestValSucces(var,val) \
    logStream() << "Success: " << #var <<'='<< var << od_endl

#define mTestVal(var,val) \
    if ( var != val ) mTestValErr(var,val) mTestValSucces(var,val)
#define mTestValD(var,val) \
    if ( !mIsEqual(var,val,0.001) ) mTestValErr(var,val) mTestValSucces(var,val)


static bool testSimpleFormula()
{
     const char* expr = "c0 * x + y - this[-2]";

    if ( !quiet_ )
	od_cout() << "Expression: '" << expr << "'\n";

    Math::Formula tryform( false, expr );
    if ( tryform.isOK() )
	{ od_cout() << "Fail:\n" << expr
	    << " should not parse in single mode" << od_endl; return false; }
    if ( !quiet_ )
	od_cout() << "OK, single mode err msg='" << tryform.errMsg() << "'\n";

    Math::Formula form( true, expr );

    if ( !form.isOK() )
	{ od_cout() << "Fail:\ndata series mode errmsg="
			<< form.errMsg() << od_endl; return false; }

    const int nrinp = form.nrInputs();
    mTestVal(nrinp,3);

    const int nrshft = form.maxRecShift();
    mTestVal(nrshft,2);

    form.recStartVals()[0] = 3;
    form.recStartVals()[1] = 4;

    const UnitOfMeasure* muom = UoMR().get("Meter");
    const UnitOfMeasure* ftuom = UoMR().get("Feet");

    form.setInputValUnit( 1, muom );
    form.setInputFormUnit( 1, ftuom );

    double inpvals[3];
    inpvals[0] = 1; inpvals[1] = 2; inpvals[2] = 3;
    double val = form.getValue( inpvals );
    mTestValD(val,6.56168);

    val = form.getValue( inpvals );
    mTestValD(val,5.56168);

    form.startNewSeries();
    form.setOutputFormUnit( ftuom );
    val = form.getValue( inpvals );
    mTestValD(val,6.56168);
    form.startNewSeries();
    form.setOutputValUnit( muom );
    val = form.getValue( inpvals );
    mTestValD(val,2.);

    if ( !quiet_ )
    {
	IOPar iop;
	form.fillPar( iop );
	BufferString str;
	iop.dumpPretty( str );
	od_cout() << str << od_endl;
	Math::Formula form2( true, "" );
	form2.usePar( iop );
	IOPar iop2;
	form2.fillPar( iop2 );
	str.setEmpty();
	iop2.dumpPretty( str );
	od_cout() << str << od_endl;
    }

    return true;
}


static bool testRepeatingVar()
{
     const char* expr = "x[-1] + 2*y + out[-1] + x[1] + aap";

    if ( !quiet_ )
	od_cout() << "Expression: '" << expr << "'\n";

    Math::SpecVarSet svs;
    svs += Math::SpecVar( "Aap", "Dit is aapje", true, &Mnemonic::distance() );
    svs += Math::SpecVar( "Noot", "Dit is nootje" );
    Math::Formula form( true, svs, expr );

    if ( !form.isOK() )
	{ od_cout() << "Fail:\n" << form.errMsg() << od_endl; return false; }

    const int nrinp = form.nrInputs();
    mTestVal(nrinp,3);

    const int nrshft = form.maxRecShift();
    mTestVal(nrshft,1);

    const int nrinpvals = form.nrValues2Provide();
    mTestVal(nrinpvals,4);
    mTestVal(form.isSpec(0),false);
    mTestVal(form.isSpec(2),true);

    double inpvals[4];
    inpvals[0] = -3; inpvals[1] = 7; // values for x[-1] and x[1]
    inpvals[2] = 11; // value for y
    inpvals[3] = -10; // value for aap
    const double val = form.getValue( inpvals );
    mTestValD(val,16);

    return true;
}


static bool testRockPhys( const Math::Formula& form )
{
    const RockPhysics::Formula* fm =
		ROCKPHYSFORMS().getByName( Mnemonic::defSVEL(), form.name() );
    mRunStandardTest( fm, "Found Castagna's formula in repository" );
    mRunStandardTest( fm->hasSameForm(form), "Forms are identical" );

    TypeSet<double> inpvals( fm->nrInputs(), mUdf(double) );
    inpvals[0] = fm->getConstVal(0);
    inpvals[2] = fm->getConstVal(2);

    inpvals[1] = 2200.;
    const double val = fm->getValue( inpvals.arr() );
    mTestValD(val,724.18);

    /* Use a more complete rpform, with several variables and constants,
       that makes use of all class members: */
    fm = ROCKPHYSFORMS().getByName( Mnemonic::defPHI(),
				    "Porosity from Density and Resistivity" );
    mRunStandardTest( fm, "Found Porosity from Den-RT formula in repository" );

    IOPar iop;
    fm->fillPar( iop );
    BufferString str;
    iop.dumpPretty( str );
    if ( !quiet_ )
	od_cout() << od_newline << str << od_endl;

    RockPhysics::Formula newfm( *fm->outputMnemonic() );
    RockPhysics::Formula formcp( *fm );
    newfm.usePar( iop );

    mRunStandardTest( formcp == *fm, "Copy constructor" );
    mRunStandardTest( newfm == *fm, "Copy using fillPar/usePar" );

    return true;
}


static bool testCastagna()
{
    const BufferString formnm( "Castagna's equation" );
    const BufferString desc( "S-wave from P-wave velocity." );
    const BufferString formexp( "c0*Vp + c1" );

    Math::Formula form;
    form.setName( formnm );
    form.setDescription( desc );
    form.setText( formexp );

    const int nrinp = form.nrInputs();
    mTestVal(nrinp,3);

    form.setInputDef( 0, "0.8619" );
    form.setInputDescription( 0, "a" );
    form.setInputDef( 1, "Vp" );
    form.setInputDescription( 1, "P-wave velocity" );
    form.setInputMnemonic( 1, &Mnemonic::defPVEL() );
    form.setInputFormUnit( 1, UoMR().get("Meter/second") );
    form.setInputDef( 2, "-1172" );
    form.setInputDescription( 2, "b" );

    const UnitOfMeasure* msuom = UoMR().getInternalFor( Mnemonic::Vel );
    const UnitOfMeasure* ftsuom = UoMR().get( "ft/s" );

    form.setOutputMnemonic( &Mnemonic::defSVEL() );
    form.setOutputFormUnit( msuom );

    mTestVal(form.nrValues2Provide(),3);

    IOPar iop;
    form.fillPar( iop );
    BufferString str;
    iop.dumpPretty( str );
    if ( !quiet_ )
	od_cout() << od_newline << str << od_endl;

    TypeSet<double> inpvals( nrinp, mUdf(double) );
    inpvals[0] = form.getConstVal(0);
    inpvals[2] = form.getConstVal(2);

    inpvals[1] = 2200.;
    const double val = form.getValue( inpvals.arr() );
    mTestValD(val,724.18);

    Math::Formula formcp( form );
    Math::Formula formiop;
    formiop.usePar( iop );

    mRunStandardTest( formcp==form, "Copy constructor" );
    mRunStandardTest( formiop==form, "Copy using fillPar/usePar" );

    formcp.setInputValUnit( 1, ftsuom );
    formiop.setInputValUnit( 1, ftsuom );
    inpvals[1] = ftsuom->getUserValueFromSI( inpvals[1] );
    mTestValD(formcp.getValue(inpvals.arr()),724.18);

    formiop.setOutputValUnit( ftsuom );
    mTestValD( formiop.getValue( inpvals.arr() ),
	       ftsuom->getUserValueFromSI(724.18) );

    mRunStandardTest( formcp!=form, "Unequal operator 1" );
    mRunStandardTest( formiop!=form, "Unequal operator 2" );

    return testRockPhys( form );
}


static bool testElasticForms()
{
    const ElasticFormulaRepository& elfr = ElFR();
    ObjectSet<const Math::Formula> denforms, pvelforms, svelforms;
    elfr.getByType( ElasticFormula::Den, denforms );
    elfr.getByType( ElasticFormula::PVel, pvelforms );
    elfr.getByType( ElasticFormula::SVel, svelforms );
    mRunStandardTest( !denforms.isEmpty(),
		      "Has density-based elastic formulas" );
    mRunStandardTest( !pvelforms.isEmpty(),
		      "Has PVel-based elastic formulas" );
    mRunStandardTest( !svelforms.isEmpty(),
		      "Has SVel-based elastic formulas" );

    ElasticPropSelection eps, ps( false ), neweps;
    neweps.setEmpty();
    const bool validsel = eps.size() == 3 && ps.size() == 2 &&
			  neweps.isEmpty() &&
			  eps.getByType( ElasticFormula::Den ) &&
			  ps.getByType( ElasticFormula::Den ) &&
			  eps.getByType( ElasticFormula::PVel ) &&
			  ps.getByType( ElasticFormula::PVel ) &&
			  eps.getByType( ElasticFormula::SVel );
    mRunStandardTest( validsel, "Valid ElasticProp Selections" );
    mRunStandardTest( neweps.ensureHasType( ElasticFormula::Den ),
		      "Successfully added Den" );
    mRunStandardTest( neweps.ensureHasType( ElasticFormula::PVel ),
		      "Successfully added PVel" );
    mRunStandardTest( neweps.ensureHasType( ElasticFormula::SVel ),
		      "Successfully added SVel" );
    mRunStandardTest( neweps.size() == 3, "Successfully added 3" );
    uiString errmsg;
    mRunStandardTestWithError( !eps.isValidInput(&errmsg),
		      "Empty input for Den/Vp/Vs", toString(errmsg) );
    mRunStandardTestWithError( !ps.isValidInput(&errmsg),
		      "Empty input for Den/Vp", toString(errmsg) );
    mRunStandardTestWithError( !neweps.isValidInput(&errmsg),
		      "Empty input for filled Den/Vp/Vs", toString(errmsg) );

    const PropertyRef* denpr = PROPS().getByMnemonic( Mnemonic::defDEN() );
    const PropertyRef* pvelpr = PROPS().getByMnemonic( Mnemonic::defPVEL() );
    const PropertyRef* svelpr = PROPS().getByMnemonic( Mnemonic::defSVEL() );
    const PropertyRef* sonicpr = PROPS().getByMnemonic( Mnemonic::defDT() );
    const PropertyRef* shearpr = PROPS().getByMnemonic( Mnemonic::defDTS() );
    const PropertyRef* aipr = PROPS().getByMnemonic( Mnemonic::defAI() );
    mRunStandardTest( denpr && pvelpr && svelpr && sonicpr && shearpr && aipr,
		      "Has required props" );

    PropertyRefSelection DenVpSel( false ), DenVpVsSel( false ),
			 DenSonSel( false ), DenSonShearSel( false ),
			 AIVpSel( false );
    DenVpSel.add( denpr ).add( pvelpr );
    DenSonSel.add( denpr ).add( sonicpr );
    DenVpVsSel.add( denpr ).add( pvelpr ).add( svelpr );
    DenSonShearSel.add( denpr ).add( sonicpr ).add( shearpr );
    AIVpSel.add( aipr ).add( pvelpr );
    ps.setFor( DenVpSel );
    eps.setFor( DenVpVsSel );
    mRunStandardTestWithError( ps.isValidInput(&errmsg),
		      "Valid input for Den/Vp", toString(errmsg) );
    mRunStandardTestWithError( eps.isValidInput(&errmsg),
		      "Valid input for Den/Vp/Vs", toString(errmsg) );

    const ElasticPropSelection aiselfromDenVp( false, DenVpSel );
    const ElasticPropSelection elselfromDenVp( true, DenVpSel );
    const ElasticPropSelection aiselfromDenSon( false, DenSonSel );
    const ElasticPropSelection elselfromDenSon( true, DenSonSel );
    const ElasticPropSelection aiselfromDenVpVs( false, DenVpVsSel );
    const ElasticPropSelection elselfromDenVpVs( true, DenVpVsSel );
    const ElasticPropSelection aiselfromDenSonShear( false, DenSonShearSel );
    const ElasticPropSelection elselfromDenSonShear( true, DenSonShearSel );
    const ElasticPropSelection elselfromAIVp( true, AIVpSel );
    mRunStandardTest( aiselfromDenVp.isOK() && aiselfromDenVp.size() == 2,
		      "Acoustic Sel from Den-Vp" );
    mRunStandardTest( elselfromDenVp.isOK() && elselfromDenVp.size() == 3,
		      "Elastic Sel from Den-Vp" );
    mRunStandardTest( aiselfromDenSon.isOK() && aiselfromDenSon.size() == 2,
		      "Acoustic Sel from Den-Son" );
    mRunStandardTest( elselfromDenSon.isOK() && elselfromDenSon.size() == 3,
		      "Elastic Sel fron Den-Son" );
    mRunStandardTest( aiselfromDenVpVs.isOK() && aiselfromDenVpVs.size() == 2,
		      "Acoustic Sel from Den-Vp-Vs" );
    mRunStandardTest( elselfromDenVpVs.isOK() && elselfromDenVpVs.size() == 3,
		      "Elastic Sel from Den-Vp-Vs" );
    mRunStandardTest( aiselfromDenSonShear.isOK() &&
		      aiselfromDenSonShear.size() == 2,
		      "Acoustic Sel from Den-Son-Shear" );
    mRunStandardTest( elselfromDenSonShear.isOK() &&
		      elselfromDenSonShear.size() == 3,
		      "Elastic Sel from Den-Son-Shear" );
    mRunStandardTest( elselfromAIVp.isOK() && elselfromAIVp.size() == 3,
		      "Elastic Sel from AI-Vp" );

    IOPar par, repar;
    elselfromDenVp.fillPar( par );
    if ( !quiet_ )
    {
	BufferString str;
	par.dumpPretty( str );
	od_cout() << od_newline << str << od_endl;
    }

    ElasticPropSelection newesel;
    newesel.usePar( par );
    newesel.fillPar( repar );
    if ( !quiet_ )
    {
	BufferString str;
	repar.dumpPretty( str );
	od_cout() << od_newline << str << od_endl;
    }

    mRunStandardTest( par == repar, "Identical propsel using IOPar" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testSimpleFormula() ||
	 !testRepeatingVar() ||
	 !testCastagna() ||
	 !testElasticForms() )
	return 1;

    return 0;
}
