/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : January 2014
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibasemapmod.h"

#include "uibasemapwellitem.h"
#include "uibasemapwin.h"

#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uitoolbar.h"

#include "odplugin.h"


mDefODPluginInfo(uiBasemap)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Basemap",
	"OpendTect",
	"dGB (Nanne)",
	"",
	"Basemap") );
    return &retpi;
}


class uiBasemapMgr :  public CallBacker
{
public:
			uiBasemapMgr(uiODMain*);
			~uiBasemapMgr();

    uiODMain*		appl_;
    uiBasemapWin*	dlg_;

    void		updateToolBar(CallBacker*);
    void		updateMenu(CallBacker*);
    void		showCB(CallBacker*);
};


uiBasemapMgr::uiBasemapMgr( uiODMain* a )
	: appl_(a)
	, dlg_(0)
{
    appl_->menuMgr().dTectTBChanged.notify(
		mCB(this,uiBasemapMgr,updateToolBar) );
    appl_->menuMgr().dTectMnuChanged.notify(
		mCB(this,uiBasemapMgr,updateMenu) );
    updateToolBar(0);
    updateMenu(0);
}


uiBasemapMgr::~uiBasemapMgr()
{
    delete dlg_;
}


void uiBasemapMgr::updateToolBar( CallBacker* )
{
    appl_->menuMgr().dtectTB()->addButton( "basemap", "Basemap",
		mCB(this,uiBasemapMgr,showCB) );
}


void uiBasemapMgr::updateMenu( CallBacker* )
{
    delete dlg_; dlg_ = 0;
    uiAction* newitem =
	new uiAction( "&Basemap ...", mCB(this,uiBasemapMgr,showCB), "basemap");
    appl_->menuMgr().viewMnu()->insertItem( newitem );
}


void uiBasemapMgr::showCB( CallBacker* )
{
    if ( !dlg_ )
	dlg_ = new uiBasemapWin( appl_ );

    dlg_->show();
    dlg_->raise();
}


mDefODInitPlugin(uiBasemap)
{
    mDefineStaticLocalObject( uiBasemapMgr*, mgr, = 0 );
    if ( mgr ) return 0;
    mgr = new uiBasemapMgr( ODMainWin() );

    uiBasemapWellItem::initClass();
    uiBasemapWellGroup::initClass();
    uiBasemapWellTreeItem::initClass();

    return 0;
}
