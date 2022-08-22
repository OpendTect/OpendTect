/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutodmad.h"
#include "odplugin.h"

#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"


mDefODPluginInfo(uiTutMadagascar)
{
    static PluginInfo retpi(
	"Madagascar Tutorial plugin (GUI)",
	"User Interface for Madagascar tutorial plugin." );
    return &retpi;
}


// We need an object to receive the CallBacks, we will thus create a manager
// inheriting from CallBacker.

class uiMadTutMgr :  public uiPluginInitMgr
{ mODTextTranslationClass(uiMadTutMgr);
public:

			uiMadTutMgr();

private:

    void		dTectMenuChanged() override;

    void                dispDlg(CallBacker*);
};


uiMadTutMgr::uiMadTutMgr()
    : uiPluginInitMgr()
{
    init();
}


void uiMadTutMgr::dTectMenuChanged()
{
    auto* newitem = new uiAction( m3Dots( tr( "Display Madagascar data" ) ),
				  mCB(this,uiMadTutMgr,dispDlg) );
    appl().menuMgr().utilMnu()->insertAction( newitem );
}


void uiMadTutMgr::dispDlg( CallBacker* )
{
    uiTutODMad maddispdlg( &appl() );
    maddispdlg.go();
}


mDefODInitPlugin(uiTutMadagascar)
{
    mDefineStaticLocalObject( PtrMan<uiMadTutMgr>, theinst_,
		    = new uiMadTutMgr() );

    if ( !theinst_ )
	return "Cannot instantiate the Madagascar tutorial plugin";

    return nullptr; // All OK - no error messages
}
