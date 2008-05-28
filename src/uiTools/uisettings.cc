/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2001
 RCS:           $Id: uisettings.cc,v 1.29 2008-05-28 12:31:54 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uisettings.h"

#include "ptrman.h"
#include "settings.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiselsimple.h"


uiSettings::uiSettings( uiParent* p, const char* nm, const char* settskey )
	: uiDialog(p,uiDialog::Setup(nm,"Default User Settings","0.2.1"))
	, setts(Settings::fetch(settskey))
{
    keyfld = new uiGenInput( this, "Settings keyword", StringInpSpec() );
    uiButton* pb = new uiPushButton( this, "&Select",
	    			     mCB(this,uiSettings,selPush), false );
    pb->setName( "Select Keyword" );
    pb->attach( rightOf, keyfld );

    valfld = new uiGenInput( this, "Value", StringInpSpec() );
    valfld->attach( alignedBelow, keyfld );
}


uiSettings::~uiSettings()
{
}


void uiSettings::selPush( CallBacker* )
{
    PtrMan<IOPar> dtectsetts = setts.subselect( "dTect" );
    BufferStringSet keys;
    for ( int idx=0; idx<dtectsetts->size(); idx++ )
	keys.add( dtectsetts->getKey(idx) );
    keys.sort();
    uiSelectFromList::Setup listsetup( "Setting selection", keys );
    listsetup.dlgtitle( keyfld->text() );
    uiSelectFromList dlg( this, listsetup );
    dlg.selFld()->setHSzPol( uiObject::Wide );
    if ( !dlg.go() ) return;
    const int selidx = dlg.selection();
    if ( selidx < 0 ) return;

    const char* key = keys.get( selidx ).buf();
    keyfld->setText( key );
    valfld->setText( dtectsetts->find(key) );
}


bool uiSettings::acceptOK( CallBacker* )
{
    const BufferString ky = keyfld->text();
    if ( ky.isEmpty() )
    {
	uiMSG().error( "Please enter a keyword to set" );
	return false;
    }

    setts.set( IOPar::compKey("dTect",ky), valfld->text() );
    if ( !setts.write() )
    {
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    return true;
}


static int iconsize = -1;
#define mIconsKey	"dTect.Icons"
#define mCBarKey        "dTect.ColorBar"
#define mHVKey          "show vertical"
#define mTopKey         "show on top"

struct LooknFeelSettings
{
    		LooknFeelSettings()
		    : iconsz(iconsize < 0 ? uiObject::iconSize() : iconsize)
		    , isvert(true)
		    , isontop(false)
		    , noshading(false)	
		    , volrenshading(false)		{}

    int		iconsz;
    bool	isvert;
    bool	isontop;
    bool	noshading;
    bool	volrenshading;
};



uiLooknFeelSettings::uiLooknFeelSettings( uiParent* p, const char* nm )
	: uiDialog(p,uiDialog::Setup(nm,"Look and Feel Settings","0.2.3"))
	, setts(Settings::common())
    	, lfsetts(*new LooknFeelSettings)
	, changed(false)
{
    IOPar* iopar = setts.subselect( mCBarKey );
    if ( iopar )
    {
	iopar->getYN( mHVKey, lfsetts.isvert );
	iopar->getYN( mTopKey, lfsetts.isontop );
	delete iopar;
    }
   
    iconszfld = new uiGenInput( this, "Icon Size", IntInpSpec(lfsetts.iconsz) );
    colbarhvfld = new uiGenInput( this, "Color bar orientation",
			BoolInpSpec(lfsetts.isvert,"Vertical","Horizontal") );
    colbarhvfld->valuechanged.notify(
			mCB(this,uiLooknFeelSettings,ctOrientChange) );
    colbarhvfld->attach( alignedBelow, iconszfld );

    colbarontopfld = new uiGenInput( this, "Color bar starts on top",
				     BoolInpSpec(lfsetts.isontop) );
    colbarontopfld->attach( alignedBelow, colbarhvfld );

    setts.getYN( "dTect.No shading", lfsetts.noshading );
    useshadingfld = new uiGenInput( this, "Use OpenGL shading when available",
				    BoolInpSpec(!lfsetts.noshading) );
    useshadingfld->attach( alignedBelow, colbarontopfld );
    useshadingfld->valuechanged.notify(
	    		mCB(this,uiLooknFeelSettings,shadingChange) );
    setts.getYN( "dTect.Use VolRen shading", lfsetts.volrenshading );
    volrenshadingfld = new uiGenInput( this, "Also for volume rendering?",
				    BoolInpSpec(lfsetts.volrenshading) );
    volrenshadingfld->attach( alignedBelow, useshadingfld );

    ctOrientChange(0);
    shadingChange(0);
}

uiLooknFeelSettings::~uiLooknFeelSettings()
{
    delete &lfsetts;
}


void uiLooknFeelSettings::ctOrientChange( CallBacker* )
{
    colbarontopfld->display( colbarhvfld->getBoolValue() );
}


void uiLooknFeelSettings::shadingChange( CallBacker* )
{
    volrenshadingfld->display( useshadingfld->getBoolValue() );
}


bool uiLooknFeelSettings::acceptOK( CallBacker* )
{
    LooknFeelSettings newsetts;
    newsetts.iconsz = iconszfld->getIntValue();
    newsetts.isvert = colbarhvfld->getBoolValue();
    newsetts.isontop = newsetts.isvert ? colbarontopfld->getBoolValue() : false;

    if ( newsetts.iconsz < 10 || newsetts.iconsz > 64 )
    {
	uiMSG().setNextCaption( "Yeah right" );
	uiMSG().error( "Please specify an icon size in the range 10-64" );
	return false;
    }

    if ( newsetts.iconsz != lfsetts.iconsz )
    {
	IOPar* iopar = setts.subselect( mIconsKey );
	if ( !iopar ) iopar = new IOPar;
	iopar->set( "size", newsetts.iconsz );
	setts.mergeComp( *iopar, mIconsKey );
	changed = true;
	delete iopar;
	iconsize = newsetts.iconsz;
    }

    if ( newsetts.isvert != lfsetts.isvert ||
	 newsetts.isontop != lfsetts.isontop )
    {
	IOPar* iopar = setts.subselect( mCBarKey );
	if ( !iopar ) iopar = new IOPar;

	iopar->setYN( mHVKey, newsetts.isvert );
	iopar->setYN( mTopKey, newsetts.isontop );

	setts.mergeComp( *iopar, mCBarKey );
	changed = true;
	delete iopar;
    }

    const bool newnoshading = !useshadingfld->getBoolValue();
    if ( lfsetts.noshading != newnoshading )
    {
	changed = true;
	setts.setYN( "dTect.No shading", newnoshading );
    }

    bool newvolrenshading = !newnoshading;
    if ( newvolrenshading )
	newvolrenshading = volrenshadingfld->getBoolValue();

    if ( lfsetts.volrenshading != newvolrenshading )
    {
	changed = true;
	setts.setYN( "dTect.Use VolRen shading", newvolrenshading );
    }

    if ( changed && !setts.write() )
    {
	changed = false;
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    return true;
}
