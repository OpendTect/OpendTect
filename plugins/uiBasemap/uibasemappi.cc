/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : January 2014
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibasemapmod.h"

#include "uibasemapcontouritem.h"
#include "uibasemapgeom2ditem.h"
#include "uibasemapgriditem.h"
#include "uibasemaphorizon3ditem.h"
#include "uibasemappicksetitem.h"
#include "uibasemappolygonitem.h"
#include "uibasemaprandomlineitem.h"
#include "uibasemapseisoutlineitem.h"
#include "uibasemapwellitem.h"
#include "uibasemapwin.h"

#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uitoolbar.h"

#include "ioman.h"
#include "odplugin.h"
#include "survinfo.h"
#include "vissurvscene.h"


mDefODPluginInfo(uiBasemap)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Basemap",
	"OpendTect",
	"dGB",
	"",
	"Basemap") );
    return &retpi;
}


class uiBasemapMgr :  public CallBacker
{ mODTextTranslationClass(uiBasemapMgr)
public:
			uiBasemapMgr(uiODMain*);
			~uiBasemapMgr();

    uiODMain*		appl_;
    uiBasemapWin*	dlg_;

    void		updateToolBar(CallBacker*);
    void		updateMenu(CallBacker*);
    void		showCB(CallBacker*);
    void		surveyChangeCB(CallBacker*);
};


uiBasemapMgr::uiBasemapMgr( uiODMain* a )
	: appl_(a)
	, dlg_(0)
{
    appl_->menuMgr().dTectTBChanged.notify(
		mCB(this,uiBasemapMgr,updateToolBar) );
    appl_->menuMgr().dTectMnuChanged.notify(
		mCB(this,uiBasemapMgr,updateMenu) );

    IOM().surveyToBeChanged.notify(
		mCB(this,uiBasemapMgr,surveyChangeCB) );
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
    const uiString itmtxt = tr("Basemap ...");
    uiMenu* vwmnu = appl_->menuMgr().viewMnu();
    if ( vwmnu->findAction(itmtxt) )
	return;

    uiAction* newitem =
	new uiAction( itmtxt, mCB(this,uiBasemapMgr,showCB), "basemap" );
    appl_->menuMgr().viewMnu()->insertItem( newitem );
}


void uiBasemapMgr::surveyChangeCB( CallBacker* )
{
    TypeSet<int> sceneids;
    appl_->applMgr().visServer()->getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	mDynamicCastGet(visSurvey::Scene*,scene,
			appl_->applMgr().visServer()->getObject(sceneids[idx]))
	if ( scene ) scene->setBaseMap( 0 );
    }
}


void uiBasemapMgr::showCB( CallBacker* )
{
    if ( !dlg_ )
    {
	const int sceneid = appl_->sceneMgr().askSelectScene();
	mDynamicCastGet(visSurvey::Scene*,scene,
			appl_->applMgr().visServer()->getObject(sceneid))

	dlg_ = new uiBasemapWin( appl_ );
	dlg_->setMouseCursorExchange( &appl_->applMgr().mouseCursorExchange() );

	if ( scene )
	{
	    scene->setBaseMap( dlg_->getBasemap() );
	    dlg_->setCaption( BufferString("Basemap - ",scene->name()) );
	}
    }

    dlg_->show();
    dlg_->raise();
}


mDefODInitPlugin(uiBasemap)
{
    mDefineStaticLocalObject( uiBasemapMgr*, mgr, = 0 );
    if ( mgr ) return 0;
    mgr = new uiBasemapMgr( ODMainWin() );

    uiBasemapContourItem::initClass();
    uiBasemapGeom2DItem::initClass();
    uiBasemapGridItem::initClass();
    uiBasemapHorizon3DItem::initClass();
    uiBasemapPickSetItem::initClass();
    uiBasemapPolygonItem::initClass();
    uiBasemapRandomLineItem::initClass();
    uiBasemapWellItem::initClass();
    uiBasemapSeisOutlineItem::initClass();

    return 0;
}
