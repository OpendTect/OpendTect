/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: uihorattribpi.cc,v 1.5 2008-04-08 11:15:20 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uihorizonattrib.h"
#include "uistratamp.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
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
        "It can even be useful to apply the 'Horizon' attribute on horizons." };
    return &retpi;
}


class uiSAMgr :  public CallBacker
{
public:
			uiSAMgr(uiODMain*);

    uiODMain*		appl_;
    void		makeStratAmp(CallBacker*);
};


uiSAMgr::uiSAMgr( uiODMain* a )
	: appl_(a)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    MenuItemSeparString horprocstr( "Create output using &Horizon" );
    uiMenuItem* itm = mnumgr.procMnu()->find( horprocstr );
    if ( !itm ) return;

    mDynamicCastGet(uiPopupItem*,horpocitm,itm)
    if ( !horpocitm ) return;

    horpocitm->menu().insertItem( new uiMenuItem("&Stratal Amplitude...",
	       			  mCB(this,uiSAMgr,makeStratAmp)) );
}


void uiSAMgr::makeStratAmp( CallBacker* )
{
    uiCalcStratAmp dlg( appl_ );
    dlg.go();
}


extern "C" const char* InituiHorizonAttribPlugin( int, char** )
{
    uiHorizonAttrib::initClass();
    static uiSAMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiSAMgr( ODMainWin() );

    return 0;
}
