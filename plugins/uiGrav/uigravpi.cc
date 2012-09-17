/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uigravpi.cc,v 1.2 2011/04/21 13:09:13 cvsbert Exp $";

#include "uigravhorcalc.h"
#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "vishorizondisplay.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "odplugin.h"


mDefODPluginInfo(uiGrav)
{
    static PluginInfo retpi = {
	"Gravity calculation",
	"Bert",
	"0.0.1",
	"Calculates gravity between horizons" };
    return &retpi;
}


class uiGravMgr :  public CallBacker
{
public:

			uiGravMgr(uiODMain&);

    uiODMain&		appl_;
    void		doDlg(CallBacker*);

    uiVisMenuItemHandler mnuitemhndlr_;
};


uiGravMgr::uiGravMgr( uiODMain& a )
	: appl_(a)
	, mnuitemhndlr_(visSurvey::HorizonDisplay::getStaticClassName(),
		*a.applMgr().visServer(),"Calculate &Gravity",
		mCB(this,uiGravMgr,doDlg))
{
}

void uiGravMgr::doDlg( CallBacker* )
{
    const int displayid = mnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_.applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
		    visserv->getObject(displayid));
    if ( !hd ) return;

    uiGravHorCalc dlg( &appl_, hd->getObjectID() );
    dlg.go();
}


mDefODInitPlugin(uiGrav)
{
    (void)new uiGravMgr( *ODMainWin() );
    return 0; // All OK - no error messages
}
