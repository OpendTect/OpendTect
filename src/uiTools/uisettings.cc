/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2001
 RCS:           $Id: uisettings.cc,v 1.12 2005-11-02 16:46:58 cvsarend Exp $
________________________________________________________________________

-*/

#include "uisettings.h"
#include "settings.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uiselsimple.h"


uiSettings::uiSettings( uiParent* p, const char* nm, const char* settskey )
	: uiDialog(p,uiDialog::Setup(nm,"Default User Settings","0.2.1"))
	, setts(Settings::fetch(settskey))
{
    keyfld = new uiGenInput( this, "Settings keyword", StringInpSpec() );
    uiButton* pb = new uiPushButton( this, "Select ...",
	    			     mCB(this,uiSettings,selPush) );
    pb->attach( rightOf, keyfld );

    valfld = new uiGenInput( this, "Value", StringInpSpec() );
    valfld->attach( alignedBelow, keyfld );
}

uiSettings::~uiSettings()
{
}


void uiSettings::selPush( CallBacker* )
{
    BufferStringSet keys;
    for ( int idx=0; idx<setts.size(); idx++ )
	keys.add( setts.getKey(idx) );
    uiSelectFromList dlg( this, keys, keyfld->text(),
	    		  "Select an existing setting" );
    if ( !dlg.go() ) return;
    const int selidx = dlg.selection();
    if ( selidx < 0 ) return;

    keyfld->setText( setts.getKey(selidx) );
    valfld->setText( setts.getValue(selidx) );
}


bool uiSettings::acceptOK( CallBacker* )
{
    const BufferString ky = keyfld->text();
    if ( ky == "" )
    {
	uiMSG().error( "Please enter a keyword to set" );
	return false;
    }

    setts.set( ky, valfld->text() );

    if ( !setts.write() )
    {
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    return true;
}


#define mCBarKey        "dTect.ColorBar"
#define mHVKey          "show vertical"
#define mTopKey         "show on top"


uiLooknFeelSettings::uiLooknFeelSettings( uiParent* p, const char* nm )
	: uiDialog(p,uiDialog::Setup(nm,"Look and Feel Settings",""))
	, setts(Settings::common())
	, iconsz( 24 )
	, isvert( true )
	, isontop( false )
{

    IOPar* iopar = setts.subselect( "Icons" );
    if ( iopar ) iopar->get( "size", iconsz );

    iconszfld = new uiGenInput( this, "Icon Size", IntInpSpec(iconsz) );


    iopar = setts.subselect( mCBarKey );
    if ( iopar )
    {
	iopar->getYN( mHVKey, isvert );
	iopar->getYN( mTopKey, isontop );
    }
   
    colbarhvfld = new uiGenInput( this, "Color bar orientation",
			      BoolInpSpec("Vertical", "Horizontal", isvert) );

    colbarontopfld = new uiGenInput( this, "Color bar starts on top",
			      BoolInpSpec(0, 0, isontop) );
    
    colbarhvfld->attach( alignedBelow, iconszfld );
    colbarontopfld->attach( alignedBelow, colbarhvfld );
}

uiLooknFeelSettings::~uiLooknFeelSettings()
{
}

bool uiLooknFeelSettings::acceptOK( CallBacker* )
{
    bool changed = false;
    int nwiconsz = iconszfld->getIntValue();
    if ( nwiconsz > 1 && nwiconsz < 100 && nwiconsz != iconsz )
    {
	iconsz = nwiconsz;
	IOPar* iopar = setts.subselect( "Icons" );
	if ( !iopar ) iopar = new IOPar;

	iopar->set( "size", iconsz );

	setts.mergeComp( *iopar, "Icons" );
	changed = true;
    }

    bool nwisvert = colbarhvfld->getBoolValue();
    bool nwisontop = colbarontopfld->getBoolValue();

    if ( isvert != nwisvert || isontop != nwisontop )
    {
	isvert = nwisvert;
	isontop = nwisontop;

	IOPar* iopar = setts.subselect( mCBarKey );
	if ( !iopar ) iopar = new IOPar;

	iopar->setYN( mHVKey, isvert );
	iopar->setYN( mTopKey, isontop );

	setts.mergeComp( *iopar, mCBarKey );
	changed = true;
    }

    if ( changed && !setts.write() )
    {
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    return true;
}
