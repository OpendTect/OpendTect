/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : August 2010
-*/

static const char* rcsID = "$Id Exp $";

#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uitranslatedlg.h"

#include "plugins.h"
#include "googletranslator.h"
#include "settings.h"
#include "texttranslator.h"


mExternC int GetGoogleTranslatePluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetGoogleTranslatePluginInfo()
{
    static PluginInfo retpi = {
	"Google Translate",
	"dGB",
	"=od",
	"Translate Gui text with Google Translate" };
    return &retpi;
}


class GoogleTranslateMgr : public CallBacker
{
public:

GoogleTranslateMgr( uiODMain& a )
    : appl_(a)
{
    appl_.menuMgr().utilMnu()->insertItem(
	    new uiMenuItem("T&ranslate ...",
			   mCB(this,GoogleTranslateMgr,handleMenu)) );
}

void handleMenu( CallBacker* )
{
    uiTranslateDlg dlg( &appl_ );
    if ( dlg.go() )
	appl_.translate();
}

    uiODMain&		appl_;
};


mExternC const char* InitGoogleTranslatePlugin( int, char** )
{
    TrMgr().setTranslator( new GoogleTranslator );

    static GoogleTranslateMgr* mgr = 0;
    if ( !mgr )
	mgr = new GoogleTranslateMgr( *ODMainWin() );

    return 0;
}
