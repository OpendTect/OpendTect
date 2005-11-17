/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2001
 RCS:           $Id: uisettings.cc,v 1.13 2005-11-17 14:55:46 cvsbert Exp $
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

struct LooknFeelSettings
{
    		LooknFeelSettings()
		    : iconsz( 24 )
		    , isvert( true )
		    , isontop( false )		{}

    int		iconsz;
    bool	isvert;
    bool	isontop;
};


uiLooknFeelSettings::uiLooknFeelSettings( uiParent* p, const char* nm )
	: uiDialog(p,uiDialog::Setup(nm,"Look and Feel Settings",""))
	, setts(Settings::common())
    	, lfsetts(*new LooknFeelSettings)
	, changed(false)
{
    IOPar* iopar = setts.subselect( "Icons" );
    if ( iopar )
	iopar->get( "size", lfsetts.iconsz );
    delete iopar; iopar = setts.subselect( mCBarKey );
    if ( iopar )
    {
	iopar->getYN( mHVKey, lfsetts.isvert );
	iopar->getYN( mTopKey, lfsetts.isontop );
	delete iopar;
    }
   
    iconszfld = new uiGenInput( this, "Icon Size", IntInpSpec(lfsetts.iconsz) );
    colbarhvfld = new uiGenInput( this, "Color bar orientation",
			  BoolInpSpec("Vertical","Horizontal",lfsetts.isvert) );
    colbarhvfld->attach( alignedBelow, iconszfld );

    colbarontopfld = new uiGenInput( this, "Color bar starts on top",
			      BoolInpSpec(0, 0, lfsetts.isontop) );
    colbarontopfld->attach( alignedBelow, colbarhvfld );
}

uiLooknFeelSettings::~uiLooknFeelSettings()
{
    delete &lfsetts;
}


bool uiLooknFeelSettings::acceptOK( CallBacker* )
{
    LooknFeelSettings newsetts;
    newsetts.iconsz = iconszfld->getIntValue();
    newsetts.isvert = colbarhvfld->getBoolValue();
    newsetts.isontop = colbarontopfld->getBoolValue();

    if ( newsetts.iconsz < 10 || newsetts.iconsz > 64 )
    {
	uiMSG().error( "Please spcify a size in the range 10-64" );
	return false;
    }

    if ( newsetts.iconsz != lfsetts.iconsz )
    {
	IOPar* iopar = setts.subselect( "Icons" );
	if ( !iopar ) iopar = new IOPar;
	iopar->set( "size", newsetts.iconsz );
	setts.mergeComp( *iopar, "Icons" );
	changed = true;
	delete iopar;
    }


    if ( newsetts.isvert != lfsetts.isvert
      || newsetts.isontop != lfsetts.isontop )
    {
	IOPar* iopar = setts.subselect( mCBarKey );
	if ( !iopar ) iopar = new IOPar;

	iopar->setYN( mHVKey, newsetts.isvert );
	iopar->setYN( mTopKey, newsetts.isontop );

	setts.mergeComp( *iopar, mCBarKey );
	changed = true;
	delete iopar;
    }

    if ( changed && !setts.write() )
    {
	changed = false;
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    return true;
}
