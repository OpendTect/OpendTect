/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: uihorattribpi.cc,v 1.11 2008-10-13 05:22:31 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uihorizonattrib.h"
#include "uiempartserv.h"
#include "uistratamp.h"
#include "uiflattenedcube.h"
#include "uiisopachmaker.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodemsurftreeitem.h"
#include "vishorizondisplay.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "attribsel.h"
#include "emobject.h"
#include "emmanager.h"
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
    void		doIsopach(CallBacker*);
    void		doIsopachThruMenu(CallBacker*);

    uiODMain*		appl_;
    uiVisMenuItemHandler flattenmnuitemhndlr_;
    uiVisMenuItemHandler isopachmnuitemhndlr_;
};


uiHorAttribPIMgr::uiHorAttribPIMgr( uiODMain* a )
	: appl_(a)
    	, flattenmnuitemhndlr_(visSurvey::HorizonDisplay::getStaticClassName(),
				*a->applMgr().visServer(),
				"Write &Flattened cube ...",
				mCB(this,uiHorAttribPIMgr,doFlattened))
    	, isopachmnuitemhndlr_(visSurvey::HorizonDisplay::getStaticClassName(),
				*a->applMgr().visServer(),
				"Calculate &Isopach ...",
				mCB(this,uiHorAttribPIMgr,doIsopach))
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    mnumgr.dTectMnuChanged.notify(mCB(this,uiHorAttribPIMgr,updateMenu));
    updateMenu(0);
}


void uiHorAttribPIMgr::updateMenu( CallBacker* )
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    MenuItemSeparString gridprocstr( "Create Grid output" );
    uiMenuItem* itm = mnumgr.procMnu()->find( gridprocstr );
    if ( !itm ) return;

    mDynamicCastGet(uiPopupItem*,gridpocitm,itm)
    if ( !gridpocitm ) return;

    gridpocitm->menu().insertItem( new uiMenuItem("&Stratal Amplitude ...",
	       			  mCB(this,uiHorAttribPIMgr,makeStratAmp)) );

    gridpocitm->menu().insertItem( new uiMenuItem("&Isopach ...", mCB(this,
		    			 uiHorAttribPIMgr,doIsopachThruMenu)) );
}


void uiHorAttribPIMgr::makeStratAmp( CallBacker* )
{
    uiStratAmpCalc dlg( appl_ );
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


void uiHorAttribPIMgr::doIsopach( CallBacker* )
{
    const int displayid = isopachmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;
    uiTreeItem* parent = appl_->sceneMgr().findItem( displayid );
    if ( !parent ) return;

    uiIsopachMaker dlg( appl_, hd->getObjectID() );
    if ( !dlg.go() )
	return;

    const int attrid = visserv->addAttrib( displayid );
    Attrib::SelSpec selspec( dlg.attrName(), Attrib::SelSpec::cOtherAttrib(),
	    		     false, 0 );
    visserv->setSelSpec( displayid, attrid, selspec );
    visserv->setRandomPosData( displayid, attrid, &dlg.getDPS() );
    uiODAttribTreeItem* itm = new uiODEarthModelSurfaceDataTreeItem(
	    	hd->getObjectID(), 0, typeid(*parent).name() );
    parent->addChild( itm, false );
    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
    parent->updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiHorAttribPIMgr::doIsopachThruMenu( CallBacker* )
{
    uiIsopachMaker dlg( appl_, -1 );
    if ( !dlg.go() )
	return;
}


extern "C" const char* InituiHorizonAttribPlugin( int, char** )
{
    uiHorizonAttrib::initClass();
    static uiHorAttribPIMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiHorAttribPIMgr( ODMainWin() );

    return 0;
}
