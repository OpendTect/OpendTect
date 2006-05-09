/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodplanedatatreeitem.cc,v 1.2 2006-05-09 11:00:53 cvsbert Exp $
___________________________________________________________________

-*/

#include "uiodplanedatatreeitem.h"

#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"

#include "survinfo.h"
#include "zaxistransform.h"

#include "uilistview.h"
#include "uimenuhandler.h"
#include "uiodscenemgr.h"
#include "uiseispartserv.h"
#include "uislicesel.h"
#include "uigridlinesdlg.h"

#include "vissurvscene.h"
#include "visplanedatadisplay.h"


static const int sPositionIdx = 990;
static const int sGridLinesIdx = 980;


#define mParentShowSubMenu( creation ) \
    uiPopupMenu mnu( getUiParent(), "Action" ); \
    mnu.insertItem( new uiMenuItem("Add"), 0 ); \
    addStandardItems( mnu ); \
    const int mnuid = mnu.exec(); \
    if ( mnuid == 0 ) creation; \
    handleStandardItems( mnuid ); \
    return true


uiODPlaneDataTreeItem::uiODPlaneDataTreeItem( int did, int dim_ )
    : dim(dim_)
    , positiondlg(0)
    , positionmnuitem_("Position ...",sPositionIdx)
    , gridlinesmnuitem_("Gridlines ...",sGridLinesIdx)
{
    displayid_ = did;
}


uiODPlaneDataTreeItem::~uiODPlaneDataTreeItem()
{
    delete positiondlg;
}


bool uiODPlaneDataTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::PlaneDataDisplay* pdd=visSurvey::PlaneDataDisplay::create();
	displayid_ = pdd->id();
	pdd->setOrientation( (visSurvey::PlaneDataDisplay::Orientation) dim );
	visserv->addObject( pdd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
			visserv->getObject(displayid_));
	if ( !pdd ) return false;
    }

    getItem()->moveForwdReq.notify(
			mCB(this,uiODPlaneDataTreeItem,moveForwdCB) );
    getItem()->moveBackwdReq.notify(
			mCB(this,uiODPlaneDataTreeItem,moveBackwdCB) );

    return uiODDisplayTreeItem::init();
}


BufferString uiODPlaneDataTreeItem::createDisplayName() const
{
    BufferString res;
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv->getObject(displayid_))
    const CubeSampling cs = pdd->getCubeSampling(true,true);
    const visSurvey::PlaneDataDisplay::Orientation orientation =
						    pdd->getOrientation();

    if ( orientation==visSurvey::PlaneDataDisplay::Inline )
	res = cs.hrg.start.inl;
    else if ( orientation==visSurvey::PlaneDataDisplay::Crossline )
	res = cs.hrg.start.crl;
    else
    {
	float zfactor = 1;
	mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()))
	if ( scene && !scene->getDataTransform() )
	    zfactor = SI().zFactor();
	res = cs.zrg.start * zfactor;
    }

    return res;
}


void uiODPlaneDataTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID() != displayID() )
	return;

    mAddMenuItem( menu, &positionmnuitem_, !visserv->isLocked(displayid_),
	          false );
    mAddMenuItem( menu, &gridlinesmnuitem_, true, false );

    uiSeisPartServer* seisserv = applMgr()->seisServer();
    int type = menu->getMenuType();
    if ( type==uiMenuHandler::fromScene )
    {
	MenuItem* displaygathermnu = seisserv->storedGathersSubMenu( true );
	if ( displaygathermnu )
	{
	    mAddMenuItem( menu, displaygathermnu, displaygathermnu->nrItems(),
		         false );
	    displaygathermnu->placement = -500;
	}
    }
}


void uiODPlaneDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;
    
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv->getObject(displayid_))

    if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( !pdd ) return;
	delete positiondlg;
	CubeSampling maxcs = SI().sampling(true);
	mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()))
	if ( scene && scene->getDataTransform() )
	{
	    const Interval<float> zintv =
		scene->getDataTransform()->getZInterval( false );
	    maxcs.zrg.start = zintv.start;
	    maxcs.zrg.stop = zintv.stop;
	}

	positiondlg = new uiSliceSel( getUiParent(),
				pdd->getCubeSampling(true,true), maxcs,
				mCB(this,uiODPlaneDataTreeItem,updatePlanePos), 
				(uiSliceSel::Type)dim );
	positiondlg->windowClosed.notify( 
		mCB(this,uiODPlaneDataTreeItem,posDlgClosed) );
	positiondlg->go();
	pdd->getMovementNotification()->notify(
		mCB(this,uiODPlaneDataTreeItem,updatePositionDlg) );
	applMgr()->enableMenusAndToolbars( false );
    }
    else if ( mnuid == gridlinesmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( !pdd ) return;

	uiGridLinesDlg gldlg( getUiParent(), pdd );
	gldlg.go();
    }
    else
    {
	menu->setIsHandled(true);
	const Coord3 inlcrlpos = visserv->getMousePos(false);
	const BinID bid( (int)inlcrlpos.x, (int)inlcrlpos.y );
	applMgr()->seisServer()->handleGatherSubMenu( mnuid, bid );
    }
}


void uiODPlaneDataTreeItem::updatePositionDlg( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid_))
    const CubeSampling newcs = pdd->getCubeSampling();
    positiondlg->setCubeSampling( newcs );
}


void uiODPlaneDataTreeItem::posDlgClosed( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid_))
    CubeSampling newcs = positiondlg->getCubeSampling();
    bool samepos = newcs == pdd->getCubeSampling();
    if ( positiondlg->uiResult() && !samepos )
    {
	pdd->setCubeSampling( newcs );
	pdd->resetManipulation();
	for ( int attrib=visserv->getNrAttribs(displayid_); attrib>=0; attrib--)
	    visserv->calculateAttrib( displayid_, attrib, false );

	updateColumnText( uiODSceneMgr::cNameColumn() );
	updateColumnText(1);
    }

    applMgr()->enableMenusAndToolbars( true );
    applMgr()->enableSceneManipulation( true );
    pdd->getMovementNotification()->remove(
		mCB(this,uiODPlaneDataTreeItem,updatePositionDlg) );
}


void uiODPlaneDataTreeItem::updatePlanePos( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid_))
    mDynamicCastGet(uiSliceSel*,dlg,cb)
    if ( !dlg ) return;

    CubeSampling cs = dlg->getCubeSampling();
    pdd->setCubeSampling( cs );
    pdd->resetManipulation();
    for ( int attrib=visserv->getNrAttribs(displayid_); attrib>=0; attrib--)
	visserv->calculateAttrib( displayid_, attrib, false );

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText(1);
}


void uiODPlaneDataTreeItem::movePlane( const CubeSampling& cs )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid_))

    pdd->setCubeSampling( cs );
    pdd->resetManipulation();
    for ( int attrib=visserv->getNrAttribs(displayid_); attrib>=0; attrib--)
	visserv->calculateAttrib( displayid_, attrib, false );
    updateColumnText(0);
    updateColumnText(1);
}


void uiODPlaneDataTreeItem::moveForwdCB( CallBacker* cb )
{
    movePlane( true );
}


void uiODPlaneDataTreeItem::moveBackwdCB( CallBacker* cb )
{
    movePlane( false );
}


void uiODPlaneDataTreeItem::movePlane( bool forward )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv->getObject(displayid_))

    CubeSampling cs = pdd->getCubeSampling();
    const int dir = forward ? 1 : -1;

    if ( pdd->getOrientation() == visSurvey::PlaneDataDisplay::Inline )
    {
	cs.hrg.start.inl += cs.hrg.step.inl * dir;
	cs.hrg.stop.inl = cs.hrg.start.inl;
    }
    else if ( pdd->getOrientation() == visSurvey::PlaneDataDisplay::Crossline )
    {
	cs.hrg.start.crl += cs.hrg.step.crl * dir;
	cs.hrg.stop.crl = cs.hrg.start.crl;
    }
    else if ( pdd->getOrientation() == visSurvey::PlaneDataDisplay::Timeslice )
    {
	cs.zrg.start += cs.zrg.step * dir;
	cs.zrg.stop = cs.zrg.start;
    }
    else
	return;

    movePlane( cs );
}


uiTreeItem* uiODInlineTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd, 
	    	    ODMainWin()->applMgr().visServer()->getObject(visid))
    return pdd && pdd->getOrientation()==visSurvey::PlaneDataDisplay::Inline
    	   ? new uiODInlineTreeItem(visid) : 0;
}


uiODInlineParentTreeItem::uiODInlineParentTreeItem()
    : uiODTreeItem( "Inline" )
{ }


bool uiODInlineParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild(new uiODInlineTreeItem(-1),false); );
}


uiODInlineTreeItem::uiODInlineTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 0 )
{}


uiTreeItem* uiODCrosslineTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return pdd && pdd->getOrientation()==visSurvey::PlaneDataDisplay::Crossline
	? new uiODCrosslineTreeItem(visid) : 0;
}


uiODCrosslineParentTreeItem::uiODCrosslineParentTreeItem()
    : uiODTreeItem( "Crossline" )
{ }


bool uiODCrosslineParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild(new uiODCrosslineTreeItem(-1),false); );
}


uiODCrosslineTreeItem::uiODCrosslineTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 1 )
{}


uiTreeItem* uiODTimesliceTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return pdd && pdd->getOrientation()==visSurvey::PlaneDataDisplay::Timeslice
	? new uiODTimesliceTreeItem(visid) : 0;
}


uiODTimesliceParentTreeItem::uiODTimesliceParentTreeItem()
    : uiODTreeItem( "Timeslice" )
{}


bool uiODTimesliceParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild(new uiODTimesliceTreeItem(-1),false); );
}


uiODTimesliceTreeItem::uiODTimesliceTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 2 )
{
}
