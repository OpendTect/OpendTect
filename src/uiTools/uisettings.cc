/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2001
 RCS:           $Id: uisettings.cc,v 1.10 2004-12-23 16:47:57 bert Exp $
________________________________________________________________________

-*/

#include "uisettings.h"
#include "settings.h"
#include "uigeninput.h"
#include "uimsg.h"


uiSettings::uiSettings( uiParent* p, const char* nm, const char* settskey )
	: uiDialog(p,uiDialog::Setup(nm,"Default User Settings","0.2.1"))
	, setts(Settings::fetch(settskey))
{
    keyfld = new uiGenInput( this, "Settings keyword", StringInpSpec() );

    valfld = new uiGenInput( this, "Value", StringInpSpec() );
    valfld->attach( alignedBelow, keyfld );
}

uiSettings::~uiSettings()
{
}


bool uiSettings::acceptOK( CallBacker* )
{
    if ( false && !setts.write() )
    {
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    return true;
}
