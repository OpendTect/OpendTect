/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Wayne Mogg
 * DATE     : Aug 2021
 * FUNCTION :
-*/


#include "testprog.h"

#include "elasticpropsel.h"


static bool testRanges()
{
    const Mnemonic* mnem = MNC().getByName( "DT", false );
    const PropertyRef* pr = PROPS().getByName( "Sonic", false );

    mRunStandardTest( mnem &&
		      mnem->disp_.typicalrange_ == Interval<float>(200.f,50.f),
		      "Typical range for DT mnemonic" );
    mRunStandardTest( pr &&
		      pr->disp_.typicalrange_ == Interval<float>(40.f,140.f),
		      "Typical range for Sonic PropertyRef" );
    return true;
}


static bool testMnemonicsSel()
{
    mRunStandardTest( MNC().size() > 20, "MnemonicSet size" )

    MnemonicSelection emptymnsel;
    MnemonicSelection excludedmnsel( &Mnemonic::defDT() );
    MnemonicSelection stdtypesel( Mnemonic::Temp );
    MnemonicSelection volmnsel = MnemonicSelection::getAllVolumetrics();
    MnemonicSelection pormnsel = MnemonicSelection::getAllPorosity();

    mRunStandardTest( emptymnsel.isEmpty(), "Empty Mnemonic selection" );
    mRunStandardTest( !excludedmnsel.isEmpty() &&
		      !excludedmnsel.getByName(Mnemonic::defDT().name(),false),
		      "Mnemonic selection without DT" );
    mRunStandardTest( stdtypesel.size() == 1, "Mnemonic selection by type" );
    mRunStandardTest( volmnsel.size() == 1 &&
		      volmnsel.first()->name() == "VCL",
		      "Volumetric mnemonic selection" );
    mRunStandardTest( pormnsel.size() == 6 &&
		      pormnsel.first()->name() == Mnemonic::defPHI().name(),
		      "Porosity mnemonic selection" );

    return true;
}


static bool testPropertyRefSet()
{
    const PropertyRef* denpr =
			PROPS().getByName(PropertyRef::standardDenStr(),false);
    const PropertyRef* pvelpr =
			PROPS().getByName(PropertyRef::standardPVelStr(),false);
    const PropertyRef* svelpr =
			PROPS().getByName(PropertyRef::standardSVelStr(),false);
    const PropertyRef* thicknpr =
			PROPS().getByName( "thickness" );
    mRunStandardTest( !thicknpr, "PropertyRefSet does not have thickness" );
    mRunStandardTest( denpr, "PropertyRefSet has Density" );
    mRunStandardTest( pvelpr, "PropertyRefSet has Pwave velocity" );
    mRunStandardTest( svelpr, "PropertyRefSet has Swave velocity" );
    mRunStandardTest( denpr->isCompatibleWith(Mnemonic::defDEN()),
		      "Density has the correct mnemonic" );
    mRunStandardTest( pvelpr->isCompatibleWith(Mnemonic::defPVEL()),
		      "Pwave velocity has the correct mnemonic" );
    mRunStandardTest( svelpr->isCompatibleWith(Mnemonic::defSVEL()),
		      "Swave velocity has the correct mnemonic" );
    thicknpr = &PropertyRef::thickness();
    mRunStandardTest( thicknpr && thicknpr->isThickness() &&
		      thicknpr->isCompatibleWith(Mnemonic::distance()),
		      "Correct thickness PropertyRef" );

    const int nrprops = PROPS().size();
    ePROPS().ensureHasElasticProps( false );
    mRunStandardTest( PROPS().size() == nrprops,
		      "ensureHasElasticProps not required (no S-wave)" );
    ePROPS().ensureHasElasticProps( true );
    mRunStandardTest( PROPS().size() == nrprops,
		      "ensureHasElasticProps not required (with S-wave)" );

    return true;
}


static bool testPropertyRefSelection()
{
    mRunStandardTest( PROPS().size() > 25, "PropertyRefSet size" )

    const PropertyRef* pvelpr =
			PROPS().getByName(PropertyRef::standardPVelStr(),false);
    PropertyRefSelection thickonlyprs1( true ),
			 thickonlyprs2( false ),
			 thickonlyprs3( true, nullptr ),
			 thickonlyprs4( false, nullptr ),
			 thicknessnopvelprs( true, pvelpr );

    mRunStandardTest( thickonlyprs1.size() == 1,
		      "Thickness only PropertyRefSelection" );
    mRunStandardTest( thickonlyprs2.isEmpty(), "Empty PropertyRefSelection" );
    mRunStandardTest( thickonlyprs3.size() > 25 &&
	  thickonlyprs3.getByName(PropertyRef::thickness().name(),false),
		    "PropertyRefSelection with thickness and PROPS" );
    mRunStandardTest( thickonlyprs4.size() > 25 &&
	  !thickonlyprs4.getByName(PropertyRef::thickness().name(),false),
		    "PropertyRefSelection without thickness and with PROPS" );
    mRunStandardTest( thicknessnopvelprs.size() == thickonlyprs3.size()-1 &&
	    !thicknessnopvelprs.getByName(pvelpr->name(),false),
	    "PropertyRefSelection with excluded PVel" )

    const PropertyRefSelection volumeprs =
			thicknessnopvelprs.subselect( Mnemonic::Volum );
    mRunStandardTest( volumeprs.size() == 3,
			"PropertyRefSelection sub-selection" );

    return true;
}


static bool testElasticPropSelection()
{
    ElasticPropSelection eprs, prs( false ), copyeprs;
    mRunStandardTest( eprs.size() == 3 && prs.size() == 2,
		      "ElasticPropSelection size" );
    for ( const auto* epr : eprs )
	mRunStandardTest( epr->isElasticForm(), "ElasticPropertyRef type" );
    for ( const auto* epr : prs )
	mRunStandardTest( epr->isElasticForm(), "ElasticPropertyRef type" );
    const Mnemonic& denmn = Mnemonic::defDEN();
    const Mnemonic& pvelmn = Mnemonic::defPVEL();
    const Mnemonic& svelmn = Mnemonic::defSVEL();

    mRunStandardTest( eprs.first()->isCompatibleWith(denmn) &&
		      prs.first()->isCompatibleWith(denmn),
		      "ElasticPropSelection has correct Density" );
    mRunStandardTest( eprs.get(1)->isCompatibleWith(pvelmn) &&
		      prs.get(1)->isCompatibleWith(pvelmn),
		      "ElasticPropSelection has correct P-wave velocity" );
    mRunStandardTest( eprs.last()->isCompatibleWith(svelmn),
		      "ElasticPropSelection has correct S-wave velocity" );

    copyeprs.erase();
    mRunStandardTest( copyeprs.isEmpty(),
			"ElasticPropSelection erase" );

    copyeprs = eprs;
    mRunStandardTest( copyeprs.size() == eprs.size() &&
		      *copyeprs.first() == *eprs.first() &&
		      *copyeprs.last() == *eprs.last(),
			"ElasticPropSelection copy" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testRanges() ||
	 !testMnemonicsSel() ||
	 !testPropertyRefSet() ||
	 !testPropertyRefSelection() ||
	 !testElasticPropSelection() )
	return 1;

    return 0;
}
