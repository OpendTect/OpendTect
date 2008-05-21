/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: uihorattribpi.cc,v 1.7 2008-05-21 09:24:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uihorizonattrib.h"
#include "uistratamp.h"
#include "uiflattenedcube.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
#include "vishorizondisplay.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "plugins.h"

extern "C" int GetuiHorizonAttribPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiHorizonAttribPluginInfo()
{
    static PluginInfo retpi = {
	"Horizon-Attribute",
	"dGB - Nanne Hemstra",
	"=od",
	"The 'Horizon' Attribute allows getting values from horizons.\n"
    	"Not to be confused with calculating attributes on horizons.\n"
        "It can even be useful to apply the 'Horizon' attribute on horizons.\n"
        "Also, the Stratal Amplitude is provided by this plugin,\n"
	"and the writing of flattened cubes" };
    return &retpi;
}


class uiHorAttribPIMgr :  public CallBacker
{
public:
			uiHorAttribPIMgr(uiODMain*);

    void		updateMenu(CallBacker*);
    void		makeStratAmp(CallBacker*);
    void		doFlattened(CallBacker*);

    uiODMain*		appl_;
    uiVisMenuItemHandler flattenmnuitemhndlr_;
};


uiHorAttribPIMgr::uiHorAttribPIMgr( uiODMain* a )
	: appl_(a)
    	, flattenmnuitemhndlr_(visSurvey::HorizonDisplay::getStaticClassName(),
				*a->applMgr().visServer(),
				"Write &Flattened cube ...",
				mCB(this,uiHorAttribPIMgr,doFlattened))
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    mnumgr.dTectMnuChanged.notify(mCB(this,uiHorAttribPIMgr,updateMenu));
    updateMenu(0);
}


void uiHorAttribPIMgr::updateMenu( CallBacker* )
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    MenuItemSeparString horprocstr( "Create output using &Horizon" );
    uiMenuItem* itm = mnumgr.procMnu()->find( horprocstr );
    if ( !itm ) return;

    mDynamicCastGet(uiPopupItem*,horpocitm,itm)
    if ( !horpocitm ) return;

    horpocitm->menu().insertItem( new uiMenuItem("&Stratal Amplitude ...",
	       			  mCB(this,uiHorAttribPIMgr,makeStratAmp)) );
}


void uiHorAttribPIMgr::makeStratAmp( CallBacker* )
{
    uiCalcStratAmp dlg( appl_ );
    dlg.go();
}


void uiHorAttribPIMgr::doFlattened( CallBacker* )
{
    const int displayid = flattenmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;

    uiWriteFlattenedCube dlg( appl_, hd->getObjectID() );
    dlg.go();
}


extern "C" const char* InituiHorizonAttribPlugin( int, char** )
{
    uiHorizonAttrib::initClass();
    static uiHorAttribPIMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiHorAttribPIMgr( ODMainWin() );

    return 0;
}
