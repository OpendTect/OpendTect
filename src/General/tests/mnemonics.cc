/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "elasticpropsel.h"
#include "unitofmeasure.h"


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
    const UnitOfMeasure* percentuom = UoMR().get( "Percent" );
    mRunStandardTest( percentuom, "% unit of measure" );

    MnemonicSelection emptymnsel;
    MnemonicSelection excludedmnsel( &Mnemonic::defDT() );
    MnemonicSelection stdtypesel( Mnemonic::Temp );
    MnemonicSelection percenttypesel( *percentuom );
    MnemonicSelection volmnsel = MnemonicSelection::getAllVolumetrics();
    MnemonicSelection pormnsel = MnemonicSelection::getAllPorosity();
    MnemonicSelection statsmnsel = MnemonicSelection::getAllSaturations();

    mRunStandardTest( emptymnsel.isEmpty(), "Empty Mnemonic selection" );
    mRunStandardTest( !excludedmnsel.isEmpty() &&
		      !excludedmnsel.getByName(Mnemonic::defDT().name(),false),
		      "Mnemonic selection without DT" );
    mRunStandardTest( stdtypesel.size() == 1, "Mnemonic selection by type" );
    mRunStandardTest( volmnsel.size() == 47 &&
		      !volmnsel.isPresent( &Mnemonic::defPHI() ) &&
		      !volmnsel.isPresent( &Mnemonic::defSW() ),
		      "Volumetric mnemonic selection" );
    mRunStandardTest( pormnsel.size() == 7 &&
		      pormnsel.isPresent( &Mnemonic::defPHI() ),
		      "Porosity mnemonic selection" );
    mRunStandardTest( statsmnsel.size() == 5 &&
		      statsmnsel.isPresent( &Mnemonic::defSW() ),
		      "Saturation mnemonic selection" );
    mRunStandardTest( percenttypesel.size() == 67,"Percent mnemonic selection");

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
    mRunStandardTest( volumeprs.size() == 4,
			"PropertyRefSelection sub-selection" );

    const PropertyRef* fracrhopr =
		       PROPS().getByMnemonic( Mnemonic::defFracDensity() );
    const PropertyRef* fracphipr =
		       PROPS().getByMnemonic( Mnemonic::defFracOrientation() );
    mRunStandardTest( fracrhopr && fracphipr,
		      "Has fracture strike and orientation" );

    return true;
}


static bool testElasticPropSelection()
{
    ElasticPropSelection eprs, prs( RefLayer::Acoustic ), copyeprs,
			 vtiprs( RefLayer::VTI ), copyvtiprs,
			 htiprs( RefLayer::HTI ), copyhtiprs;
    mRunStandardTest( eprs.size() == 3 && prs.size() == 2 &&
		      vtiprs.size()==4 && htiprs.size() == 5,
		      "ElasticPropSelection size" );
    for ( const auto* epr : eprs )
	mRunStandardTest( epr->isElasticForm(), "ElasticPropertyRef type" );
    for ( const auto* epr : prs )
	mRunStandardTest( epr->isElasticForm(), "ElasticPropertyRef type" );
    const Mnemonic& denmn = Mnemonic::defDEN();
    const Mnemonic& pvelmn = Mnemonic::defPVEL();
    const Mnemonic& svelmn = Mnemonic::defSVEL();
    const Mnemonic& fracrhomn = Mnemonic::defFracDensity();
    const Mnemonic& fracazimn = Mnemonic::defFracOrientation();

    mRunStandardTest( eprs.first()->isCompatibleWith(denmn) &&
		      prs.first()->isCompatibleWith(denmn),
		      "ElasticPropSelection has correct Density" );
    mRunStandardTest( eprs.get(1)->isCompatibleWith(pvelmn) &&
		      prs.get(1)->isCompatibleWith(pvelmn),
		      "ElasticPropSelection has correct P-wave velocity" );
    mRunStandardTest( eprs.last()->isCompatibleWith(svelmn),
		      "ElasticPropSelection has correct S-wave velocity" );
    mRunStandardTest( vtiprs.last()->isCompatibleWith(fracrhomn),
		      "ElasticPropSelection has correct fracture density" );
    mRunStandardTest( htiprs.get(3)->isCompatibleWith(fracrhomn),
		      "ElasticPropSelection has correct fracture density" );
    mRunStandardTest( htiprs.last()->isCompatibleWith(fracazimn),
		      "ElasticPropSelection has correct fracture orientation" );

    copyeprs.erase();
    mRunStandardTest( copyeprs.isEmpty(),
			"ElasticPropSelection erase" );

    copyeprs = eprs;
    copyvtiprs = vtiprs;
    copyhtiprs = htiprs;
    mRunStandardTest( copyeprs.size() == eprs.size() &&
		      *copyeprs.first() == *eprs.first() &&
		      *copyeprs.last() == *eprs.last() &&
		      copyvtiprs.size() == vtiprs.size() &&
		      *copyvtiprs.last() == *vtiprs.last() &&
		      copyhtiprs.size() == htiprs.size() &&
		      *copyhtiprs.last() == *htiprs.last(),
			"ElasticPropSelection copy" );

    return true;
}


static bool find( const char* mnnm, const char* prnm, const char* findnm )
{
    const MnemonicSelection mnsel( nullptr );

    const Mnemonic* defmn = mnsel.getByName( mnnm, false );
    mRunStandardTest( defmn,
		      BufferString("Retrieved mnemonic '", mnnm, "'") );

    BufferStringSet allnms( findnm );
    allnms.add( BufferString("With prefix ",findnm) )
	  .add( BufferString(findnm," with suffix") )
	  .add( BufferString("With prefix (",findnm,") with suffix") );

    BufferString msg;
    if ( prnm )
    {
	const PropertyRefSelection prs( false, nullptr );
	const PropertyRef* defpr = prs.getByName( prnm, false );
	mRunStandardTest( defpr,
		     BufferString("Retrieved PropertyRef '", prnm, "'") );

	for ( const auto* nm : allnms )
	{
	    const PropertyRef* pr = prs.getByName( nm->str() );
	    if ( pr )
	    {
		if ( pr != defpr )
		{
		    msg.set( "Wrong property match: returned '" )
		       .add( pr->name() ).add ("' while using '" )
		       .add( nm->str() ).add( "'" );
		    break;
		}
		else if ( &pr->mn() != defmn )
		{
		    msg.set( "Wrong mnemonic match: returned '" )
		       .add( pr->name() ).add ("' while using '" )
		       .add( nm->str() ).add( "'" );
		    break;
		}
	    }
	    else
	    {
		msg.set( "No match found using '" ).add( nm->str() ).add( "'" );
		break;
	    }
	}
	mRunStandardTestWithError( msg.isEmpty(),
	    BufferString("Partial match for PropertyRef '", prnm, "'"), msg);
    }

    msg.setEmpty();
    for ( const auto* nm : allnms )
    {
	const Mnemonic* mn = mnsel.getByName( nm->str() );
	if ( !mn )
	{
	    msg.set( "No match found using '" ).add( nm->str() ).add( "'" );
	    break;
	}
	else if ( mn && mn != defmn )
	{
	    msg.set( "Wrong match: returned '" ).add( mn->name() )
	       .add ("' while using '" ).add( nm->str() ).add( "'" );
	    break;
	}
    }
    mRunStandardTestWithError( msg.isEmpty(),
	    BufferString("Partial match for mnemonic '", mnnm, "'"), msg );

    return true;
}


static bool testFindProp()
{
    BufferStringSet searchnms;

    const char* denmnnm = Mnemonic::defDEN().name().str();
    const char* denprnm = PropertyRef::standardDenStr();
    if ( !find(denmnnm,nullptr,denmnnm) || !find(denmnnm,denprnm,denprnm) )
	return false;

    const char* pvelmnnm = Mnemonic::defPVEL().name().str();
    const char* pvelprnm = PropertyRef::standardPVelStr();
    searchnms.setEmpty();
    searchnms.add( pvelprnm ).add( "Pwave" ).add( "P-wave" ).add( "P_wave" )
	     .add( "Pwave vel" ).add( "P-wave vel" );
    for ( const auto* nm : searchnms )
    {
	if ( !find(pvelmnnm,nullptr,nm->str()) ||
	     !find(pvelmnnm,pvelprnm,nm->str()) )
	return false;
    }

    const char* svelmnnm = Mnemonic::defSVEL().name().str();
    const char* svelprnm = PropertyRef::standardSVelStr();
    searchnms.setEmpty();
    searchnms.add( svelprnm ).add( "Swave" ).add( "S-wave" ).add( "S_wave" )
	     .add( "Swave vel" ).add( "S-wave vel" );
    for ( const auto* nm : searchnms )
    {
	if ( !find(svelmnnm,nullptr,nm->str()) ||
	     !find(svelmnnm,svelprnm,nm->str()) )
	return false;
    }

    const char* sonmnnm = Mnemonic::defDT().name().str();
    static const char* sonicprnm = "Sonic";
    if ( !find(sonmnnm,nullptr,sonmnnm) )
	return false;

    searchnms.setEmpty();
    searchnms.add( sonicprnm );
    for ( const auto* nm : searchnms )
    {
	if ( !find(sonmnnm,nullptr,nm->str()) ||
	     !find(sonmnnm,sonicprnm,nm->str()) )
	return false;
    }

    const char* shearsonicmnnm = Mnemonic::defDTS().name().str();
    static const char* shearsonicprnm = "Shear Sonic";
    if ( !find(shearsonicmnnm,nullptr,shearsonicmnnm) )
	return false;

    searchnms.setEmpty();
    searchnms.add( shearsonicprnm );
    for ( const auto* nm : searchnms )
    {
	if ( !find(shearsonicmnnm,nullptr,nm->str()) ||
	     !find(shearsonicmnnm,shearsonicprnm,nm->str()) )
	return false;
    }

    const char* swmnnm = Mnemonic::defSW().name().str();
    static const char* swprnm = "Water Saturation";
    if ( !find(swmnnm,nullptr,swmnnm) )
	return false;

    searchnms.setEmpty();
    searchnms.add( swprnm );
    for ( const auto* nm : searchnms )
    {
	if ( !find(swmnnm,nullptr,nm->str()) ||
	     !find(swmnnm,swprnm,nm->str()) )
	return false;
    }

    static const char* vpvsmnnm = "VPVS";
    static const char* vpvsprnm = "Vp/Vs";
    if ( !find(vpvsmnnm,nullptr,vpvsmnnm) )
	return false;

    searchnms.setEmpty();
    searchnms.add( vpvsprnm ).add( "VpVs" )
	     .add( "VpVsRatio" ).add( "VpVs Ratio" );
    for ( const auto* nm : searchnms )
    {
	if ( !find(vpvsmnnm,nullptr,nm->str()) ||
	     !find(vpvsmnnm,vpvsprnm,nm->str()) )
	return false;
    }

    static const char* vsvpmnnm = "VSVP";
    static const char* vsvpprnm = "Vs/Vp";
    if ( !find(vsvpmnnm,nullptr,vsvpmnnm) )
	return false;

    searchnms.setEmpty();
    searchnms.add( vsvpprnm ).add( "VsVp" )
	     .add( "VsVpRatio" ).add( "VsVp Ratio" );
    for ( const auto* nm : searchnms )
    {
	if ( !find(vsvpmnnm,nullptr,nm->str()) ||
	     !find(vsvpmnnm,vsvpprnm,nm->str()) )
	return false;
    }

    return true;
}


static bool testCustomMn()
{
    const Mnemonic* eimn = MNC().getByName( "EI", false );
    mRunStandardTest( eimn && eimn->isTemplate(),
		      "EI Mnemonic is default template" );

    PtrMan<Mnemonic> eei15mn = Mnemonic::getFromTemplate( *eimn,
				"EEI 15", Repos::Survey );
    mRunStandardTest( eei15mn && !eei15mn->isTemplate(),
			"EEI 15 Mnemonic is not a default mnemonic" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testRanges() ||
	 !testMnemonicsSel() ||
	 !testPropertyRefSet() ||
	 !testPropertyRefSelection() ||
	 !testElasticPropSelection() ||
	 !testFindProp() ||
	 !testCustomMn() )
	return 1;

    return 0;
}
