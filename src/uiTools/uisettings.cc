/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2001
 RCS:           $Id: uisettings.cc,v 1.11 2004-12-23 17:16:38 bert Exp $
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
