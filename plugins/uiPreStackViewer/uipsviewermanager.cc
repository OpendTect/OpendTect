/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		5-11-2007
 RCS:		$Id: uipsviewermanager.cc,v 1.1 2007-09-12 16:04:33 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uipsviewermanager.h"
#include "bufstringset.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uipsviewersetting.h"
#include "uiseispartserv.h"
#include "uivispartserv.h"
#include "visdatagroup.h"
#include "visplanedatadisplay.h"
#include "visprestackviewer.h"


namespace PreStackView
{

uiPSViewerMgr::uiPSViewerMgr()
    : selectpsdatamenuitem_( "Display 3D PS Gather" )
    , proptymenuitem_( "Properties" )				 
    , removemenuitem_( "Remove") 
    , visserv_( ODMainWin()->applMgr().visServer() )  	      
{
    visserv_->removeAllNotifier().notify( mCB(this,uiPSViewerMgr,removeAllCB) );
    RefMan<MenuHandler> menuhandler = visserv_->getMenuHandler();
    
    menuhandler->createnotifier.notify( mCB(this,uiPSViewerMgr,createMenuCB) );
    menuhandler->handlenotifier.notify( mCB(this,uiPSViewerMgr,handleMenuCB) );
}


uiPSViewerMgr::~uiPSViewerMgr()
{
    visserv_->removeAllNotifier().remove( mCB(this,uiPSViewerMgr,removeAllCB) );
    RefMan<MenuHandler> menuhandler = visserv_->getMenuHandler(); 
    
    menuhandler->createnotifier.remove( mCB(this,uiPSViewerMgr,createMenuCB) );
    menuhandler->handlenotifier.remove( mCB(this,uiPSViewerMgr,handleMenuCB) );

    removeAllCB( 0 );
}    


void uiPSViewerMgr::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet( uiMenuHandler*, menu, cb );
    
    RefMan<visBase::DataObject> dataobj = visserv_->getObject( menu->menuID() );

    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, dataobj.ptr() );
    if ( pdd && pdd->getOrientation()!=visSurvey::PlaneDataDisplay::Timeslice &&
	 menu->getMenuType()==uiMenuHandler::fromScene )
    {
	uiSeisPartServer* seisserv = ODMainWin()->applMgr().seisServer();
	if ( seisserv->getStoredGathersList().size() )
	{
	    selectpsdatamenuitem_.removeItems();
	    selectpsdatamenuitem_.createItems(seisserv->getStoredGathersList());
	    mAddMenuItem( menu, &selectpsdatamenuitem_, true, false );
	}
    }
    else
	mResetMenuItem( &selectpsdatamenuitem_ );

    mDynamicCastGet( PreStackView::PreStackViewer*, psv, dataobj.ptr() );
    if ( psv )
    { 
	mAddMenuItem( menu, &proptymenuitem_, true, false );
    	mAddMenuItem( menu, &removemenuitem_, true, false ); 
    }
    else
    {
	mResetMenuItem( &proptymenuitem_ );
	mResetMenuItem( &removemenuitem_ );
    }
}


void uiPSViewerMgr::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiMenuHandler*,menu,caller );

    if ( menu->isHandled() )
	return;

    const TypeSet<int>* path = menu->getPath();
    if ( !path ) return;

    TypeSet<int> sceneids;
    visserv_->getChildIds( -1, sceneids );

    sceneids.createIntersection( *path );
    if ( sceneids.size()!=1 )
	return;

    const int mnuidx = selectpsdatamenuitem_.itemIndex( mnuid );
    if ( mnuidx>=0 )
    {
	PtrMan<IOObj> ioobj = IOM().getLocal(
		selectpsdatamenuitem_.getItem(mnuidx)->text );
	if ( !ioobj )
	    return;

	menu->setIsHandled( true );

	const Coord3 pickedpos = menu->getPickedPos();
	if ( !pickedpos.isDefined() )
	    return;

	const BinID bid = SI().transform( pickedpos );

	RefMan<visBase::DataObject> dataobj = visserv_->
	    getObject( menu->menuID() );
	mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, dataobj.ptr() );

	PreStackViewer* viewer = PreStackViewer::create();
	viewer->ref();
	viewer->setMultiID( ioobj->key() );
	viewer->setSectionDisplay( pdd ); 
	
	visserv_->addObject( viewer, sceneids[0], false );
	viewers_ += viewer;

	viewer->setPosition( bid );
	
	if ( viewer->getScene() )
	    viewer->getScene()->change.notifyIfNotNotified( mCB( this, 
			uiPSViewerMgr,sceneChangeCB ) );
    }
    else if ( mnuid==removemenuitem_.id )
    {
 	RefMan<visBase::DataObject> dataobj = visserv_->
	    getObject( menu->menuID() );
	mDynamicCastGet( PreStackView::PreStackViewer*, psv, dataobj.ptr() );
	if ( !psv ) return;

	menu->setIsHandled( true );
	
	visserv_->removeObject( psv, sceneids[0] );
	viewers_ -= psv;
	psv->unRef();
    }
    else if ( mnuid==proptymenuitem_.id )
    {
	menu->setIsHandled( true );
	RefMan<visBase::DataObject> dataobj = visserv_->
	    getObject( menu->menuID() );
	mDynamicCastGet( PreStackView::PreStackViewer*, psv, dataobj.ptr() );
	if ( !psv ) return;

	uiPSViewerSetting dlg( menu->getParent(), *psv );
	dlg.go(); 
	if ( !dlg.acceptOK() )
	    return;
    }
}


void uiPSViewerMgr::sceneChangeCB( CallBacker* )
{ 
    for ( int idx = 0; idx<viewers_.size(); idx++ )
    {
	PreStackView::PreStackViewer* psv = viewers_[idx];

	visBase::Scene* scene = psv->getScene();
	if ( !scene || scene->getFirstIdx( psv->getSectionDisplay() )==-1 )
	{
	    viewers_.remove( idx );
	    if ( scene ) visserv_->removeObject( psv, scene->id() );
	    psv->unRef();
	    idx--;
	}
    }
}


void  uiPSViewerMgr::removeAllCB( CallBacker* )
{
    deepUnRef( viewers_ );
}    


}; // Namespace
