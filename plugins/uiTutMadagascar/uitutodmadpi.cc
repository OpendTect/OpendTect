/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : H. Huck
 * DATE     : Jun 2009
-*/

static const char* rcsID = "$Id";

#include "uitutodmad.h"
#include "plugins.h"

#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"

mExternC int GetuiTutMadagascarPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiTutMadagascarPluginInfo()
{
    static PluginInfo retpi = {
	"Madagascar Tutorial plugin",
	"dGB (Helene)",
	"=od",
	"User Interface for Madagascar tutorial plugin." };
    return &retpi;
}


// We need an object to receive the CallBacks, we will thus create a manager
// inheriting from CallBacker.

mClass uiMadTutMgr :  public CallBacker
{
    public:

			uiMadTutMgr(uiODMain&);

    uiODMain&           mainappl;
    void                dispDlg(CallBacker*);
};


uiMadTutMgr::uiMadTutMgr( uiODMain& a )
    : mainappl( a )
{
    uiMenuItem* newitem = new uiMenuItem( "Display &Madagascar data ...",
					  mCB(this,uiMadTutMgr,dispDlg) );
    mainappl.menuMgr().utilMnu()->insertItem( newitem );
}


void uiMadTutMgr::dispDlg( CallBacker* )
{
    uiTutODMad maddispdlg( &mainappl );
    maddispdlg.go();
}


mExternC const char* InituiTutMadagascarPlugin( int, char** )
{
    (void)new uiMadTutMgr( *ODMainWin() );
    return 0; // All OK - no error messages
}
