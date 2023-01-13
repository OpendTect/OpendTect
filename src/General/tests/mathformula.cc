/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "elasticpropsel.h"
#include "mathexpression.h"
#include "mathproperty.h"
#include "mathspecvars.h"
#include "rockphysics.h"
#include "testprog.h"
#include "unitofmeasure.h"

static const float propvals[] = { 50.f, 2.5f, 3000.f, 1250.f, 7500.f, 3125.f,
    2.4f, 0.41666667f, 0.394958f, 36.71875f, 9.765625f, 101.6f, 243.84f,
    2.2942567f, 2.2942567f, 1524.7951f, 9.615385f
};


#define mTestValErr(var,val) \
	{ od_cout() << "Fail:\n" << #var <<'='<< var << \
	    ", not " << val << od_endl; return false; }
#define mTestValSucces(var,val) \
    logStream() << "Success: " << #var <<'='<< var << od_endl

#define mTestVal(var,val) \
    if ( var != val ) mTestValErr(var,val) mTestValSucces(var,val)
#define mTestValD(var,val) \
    if ( !mIsEqual(var,val,0.001) ) \
	mTestValErr(var,val) mTestValSucces(var,val)


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

    ElasticPropSelection eps, ps( RefLayer::Acoustic ), neweps;
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

    const RefLayer::Type aityp = RefLayer::Acoustic;
    const RefLayer::Type sityp = RefLayer::Elastic;

    const ElasticPropSelection aiselfromDenVp( aityp, DenVpSel );
    const ElasticPropSelection elselfromDenVp( sityp, DenVpSel );
    const ElasticPropSelection aiselfromDenSon( aityp, DenSonSel );
    const ElasticPropSelection elselfromDenSon( sityp, DenSonSel );
    const ElasticPropSelection aiselfromDenVpVs( aityp, DenVpVsSel );
    const ElasticPropSelection elselfromDenVpVs( sityp, DenVpVsSel );
    const ElasticPropSelection aiselfromDenSonShear( aityp, DenSonShearSel );
    const ElasticPropSelection elselfromDenSonShear( sityp, DenSonShearSel );
    const ElasticPropSelection elselfromAIVp( sityp, AIVpSel );
    mRunStandardTest( aiselfromDenVp.isOK(&DenVpSel) &&
		      aiselfromDenVp.size() == 2,
		      "Acoustic Sel from Den-Vp" );
    mRunStandardTest( elselfromDenVp.isOK(&DenVpSel) &&
		      elselfromDenVp.size() == 3,
		      "Elastic Sel from Den-Vp" );
    mRunStandardTest( aiselfromDenSon.isOK(&DenSonSel) &&
		      aiselfromDenSon.size() == 2,
		      "Acoustic Sel from Den-Son" );
    mRunStandardTest( elselfromDenSon.isOK(&DenSonSel) &&
		      elselfromDenSon.size() == 3,
		      "Elastic Sel fron Den-Son" );
    mRunStandardTest( aiselfromDenVpVs.isOK(&DenVpVsSel) &&
		      aiselfromDenVpVs.size() == 2,
		      "Acoustic Sel from Den-Vp-Vs" );
    mRunStandardTest( elselfromDenVpVs.isOK(&DenVpVsSel) &&
		      elselfromDenVpVs.size() == 3,
		      "Elastic Sel from Den-Vp-Vs" );
    mRunStandardTest( aiselfromDenSonShear.isOK(&DenSonShearSel) &&
		      aiselfromDenSonShear.size() == 2,
		      "Acoustic Sel from Den-Son-Shear" );
    mRunStandardTest( elselfromDenSonShear.isOK(&DenSonShearSel) &&
		      elselfromDenSonShear.size() == 3,
		      "Elastic Sel from Den-Son-Shear" );
    mRunStandardTest( elselfromAIVp.isOK(&AIVpSel) &&
		      elselfromAIVp.size() == 3,
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


static bool testMathProperty()
{
    const PropertyRef* denpr = PROPS().getByMnemonic( Mnemonic::defDEN() );
    const PropertyRef* pvelpr = PROPS().getByMnemonic( Mnemonic::defPVEL() );
    const PropertyRef* svelpr = PROPS().getByMnemonic( Mnemonic::defSVEL() );
    const PropertyRef* aipr = PROPS().getByMnemonic( Mnemonic::defAI() );
    const PropertyRef* sipr = PROPS().getByMnemonic( Mnemonic::defSI() );
    const PropertyRef* vpvspr =
		PROPS().getByMnemonic( *MNC().getByName("VPVS"),false );
    const PropertyRef* vsvppr =
		PROPS().getByMnemonic( *MNC().getByName("VSVP"),false );
    const PropertyRef* prpr =
		PROPS().getByMnemonic( *MNC().getByName("PR"), false );
    const PropertyRef* lrpr =
		PROPS().getByMnemonic( *MNC().getByName("LR"), false );
    const PropertyRef* mrpr =
		PROPS().getByMnemonic( *MNC().getByName("MR"), false );
    const PropertyRef* dtpr = PROPS().getByMnemonic( Mnemonic::defDT() );
    const PropertyRef* dtspr = PROPS().getByMnemonic( Mnemonic::defDTS() );
    const PropertyRef* phipr = PROPS().getByMnemonic( Mnemonic::defPHI() );

    mRunStandardTest( denpr && pvelpr && svelpr && sipr && aipr &&
		      vpvspr && vsvppr && prpr && dtpr && dtspr && phipr,
		      "Has required props" );

    PropertyRefSelection fixeddefprs( false );
    fixeddefprs.add( aipr ).add( sipr ).add( vpvspr ).add( vsvppr )
	       .add( prpr ).add( lrpr ).add( mrpr );
    for ( const auto* pr : fixeddefprs )
    {
	mRunStandardTest( pr->hasFixedDef() &&
			  pr->fixedDef().getForm().hasFixedUnits(),
	    BufferString( pr->name(), " has a fixed definition") );
    }

    auto* denfromvppr = new PropertyRef( *denpr );
    denfromvppr->setName( "Density from Vp (Gardner)" );
    auto* denfromsonpr = new PropertyRef( *denpr );
    denfromsonpr->setName( "Density from Sonic (Gardner)" );
    auto* svelfrompvelpr = new PropertyRef( *svelpr );
    svelfrompvelpr->setName( "Swave velocity (Krief)" );
    auto* phifromdenpr = new PropertyRef( *phipr );
    phifromdenpr->setName( "Porosity from density" );
    ePROPS().add( denfromvppr ).add( denfromsonpr )
	    .add( svelfrompvelpr ).add( phifromdenpr );

    PropertyRefSelection prs;
    prs.add( denpr ).add( pvelpr ).add( svelpr );

    const BufferString formstr( "Formula: " );
    BufferString defstr;
    PtrMan<MathProperty> mathaiprop = new MathProperty( *aipr );
    PtrMan<MathProperty> mathsiprop = new MathProperty( *sipr );
    PtrMan<MathProperty> mathvpvsprop = new MathProperty( *vpvspr );
    PtrMan<MathProperty> mathvsvpprop = new MathProperty( *vsvppr );
    PtrMan<MathProperty> mathprprop = new MathProperty( *prpr );
    PtrMan<MathProperty> mathlrprop = new MathProperty( *lrpr );
    PtrMan<MathProperty> mathmrprop = new MathProperty( *mrpr );
    PtrMan<MathProperty> mathdtprop = new MathProperty( *dtpr );
    PtrMan<MathProperty> mathdtsprop = new MathProperty( *dtspr );
    PtrMan<MathProperty> mathden1prop = new MathProperty( *denfromvppr );
    PtrMan<MathProperty> mathden2prop = new MathProperty( *denfromsonpr );
    PtrMan<MathProperty> mathvsfromvpprop = new MathProperty( *svelfrompvelpr);
    PtrMan<MathProperty> mathphiprop = new MathProperty( *phifromdenpr );
    ObjectSet<PtrMan<MathProperty> > mathpropsman;

    mathpropsman.add( &mathaiprop ).add( &mathsiprop ).add( &mathvpvsprop )
		.add( &mathvsvpprop ).add( &mathprprop ).add( &mathlrprop )
		.add( &mathmrprop ).add( &mathdtprop ).add( &mathdtsprop )
		.add( &mathden1prop ).add( &mathden2prop )
		.add( &mathvsfromvpprop ).add( &mathphiprop );

    IOPar defiop;
    defiop.set( "Expression", "den * vel" );
    defiop.set( "Input.0.Def", PropertyRef::standardDenStr() );
    defiop.set( "Input.1.Def", PropertyRef::standardPVelStr() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathaiprop->setDef( defstr.buf() );

    defiop.set( "Input.1.Def", PropertyRef::standardSVelStr() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathsiprop->setDef( defstr.buf() );

    defiop.set( "Expression", "Vp / Vs" );
    defiop.set( "Input.0.Def", PropertyRef::standardPVelStr() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathvpvsprop->setDef( defstr.buf() );

    defiop.set( "Expression", "Vs / Vp" );
    defiop.set( "Input.0.Def", PropertyRef::standardSVelStr() );
    defiop.set( "Input.1.Def", PropertyRef::standardPVelStr() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathvsvpprop->setDef( defstr.buf() );

    defiop.set( "Expression", "( (Vp/Vs)^2 - 2 ) / ( 2*( (Vp/Vs)^2 - 1) )" );
    defiop.set( "Input.0.Def", PropertyRef::standardPVelStr() );
    defiop.set( "Input.1.Def", PropertyRef::standardSVelStr() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathprprop->setDef( defstr.buf() );

    defiop.set( "Expression", "AI^2 - 2 * SI^2" );
    defiop.set( "Input.0.Def", aipr->name() );
    defiop.set( "Input.1.Def", sipr->name() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathlrprop->setDef( defstr.buf() );

    defiop.set( "Expression", "SI^2" );
    defiop.set( "Input.0.Def", sipr->name() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathmrprop->setDef( defstr.buf() );

    defiop.removeWithKey( "Input.1.Def" );

    defiop.set( "Expression", "1 / Vp" );
    defiop.set( "Input.0.Def", PropertyRef::standardPVelStr() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathdtprop->setDef( defstr.buf() );

    defiop.set( "Expression", "1 / Vs" );
    defiop.set( "Input.0.Def", PropertyRef::standardSVelStr() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathdtsprop->setDef( defstr.buf() );

    defiop.set( "Expression", "0.31 * Vp^0.25" );
    defiop.set( "Input.0.Def", PropertyRef::standardPVelStr() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathden1prop->setDef( defstr.buf() );

    defiop.set( "Expression", "0.31 * (1/son)^0.25" );
    defiop.set( "Input.0.Def", "sonic" );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathden2prop->setDef( defstr.buf() );

    defiop.set( "Expression", "sqrt( 0.452*Vp^2 - 1.743 )" );
    defiop.set( "Input.0.Def", PropertyRef::standardPVelStr() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathvsfromvpprop->setDef( defstr.buf() );

    defiop.set( "Expression", "(2.65 - Rhob) / (2.65 - 1.09)" );
    defiop.set( "Input.0.Def", PropertyRef::standardDenStr() );
    defiop.putTo( defstr ); defstr.insertAt( 0, formstr.buf() );
    mathphiprop->setDef( defstr.buf() );

    Property& aiprop = *mathaiprop;
    mRunStandardTest( aiprop.isFormula(), "Math property is a formula" );
    mRunStandardTest( mathaiprop->getForm().expression() &&
	BufferString( mathaiprop->getForm().expression()->type() )
		== "ExpressionMultiply", "AI expression type" );

    PropertySet props( prs ); ObjectSet<const MathProperty> mathprops;
    for ( auto* mathprop : mathpropsman )
    {
	MathProperty* mathpropptr = mathprop->ptr();
	props.set( mathpropptr );
	if ( props.isPresent(mathpropptr) )
	{
	    mathprops.add( mathpropptr );
	    mathprop->release();
	}
    }

    mRunStandardTestWithError( mathprops.size() == mathpropsman.size() &&
		      props.prepareUsage(), "PropertySet is ready for use",
		      toString(props.errMsg()) );
    mRunStandardTest( props.errMsg().isEmpty(), "PropertySet has no error" );

    BufferStringSet rpformnames;
    rpformnames.add("Acoustic Impedance from P-wave velocity" )
	       .add( "Shear Impedance from S-wave velocity" )
	       .add( "Vp/Vs ratio from Velocity curves" )
	       .add( "Vs/Vp ratio from Velocity curves" )
	       .add( "Poisson's ratio from Velocity logs" )
	       .add( "Lambda-Rho" )
	       .add( "Mhu-Rho" )
	       .add( "Sonic from velocity" )
	       .add( "Shear Sonic from shear velocity" )
	       .add( "Gardner equation - P-wave" )
	       .add( "Gardner equation - Sonic" )
	       .add( "Krief's equation" )
	       .add( "Porosity from Density" );

    for ( int idx=0; idx<rpformnames.size(); idx++ )
    {
	const MathProperty* mathprop = mathprops.get( idx );
	const BufferString propnm( mathprop ? mathprop->name() : nullptr );
	mRunStandardTest( mathprop,
	    BufferString("Found math property '", propnm,"' in PropertySet") );

	const Mnemonic& mn = mathprop->ref().mn();
	const BufferString& formnm = rpformnames.get( idx );
	const RockPhysics::Formula* fm =
			   ROCKPHYSFORMS().getByName( mn, formnm.buf() );
	mRunStandardTest( fm, "Found formula in repository" );
	mRunStandardTest( mathprop->getForm().hasFixedUnits(),
			  "Formula has fixed units" );
	mRunStandardTest( fm->isCompatibleWith(mathprop->getForm()),
			  "Forms are identical" );
    }

    TypeSet<float> vals;
    for ( const auto* prop : props )
	vals += prop->value();

    for ( int idx=0; idx<props.size(); idx++ )
    {
	const float val = vals[idx];
	const float expval = propvals[idx];
	const bool test = mIsEqual( val, expval, expval*1e-6f );
	BufferString msg( "Expected: ", expval, " for property '" );
	msg.add( props.get(idx)->name() ).add( "', calculated: " )
	   .add( val );
	mRunStandardTest( test, msg );
    }

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testSimpleFormula() ||
	 !testRepeatingVar() ||
	 !testCastagna() ||
	 !testElasticForms() ||
	 !testMathProperty() )
	return 1;

    return 0;
}
