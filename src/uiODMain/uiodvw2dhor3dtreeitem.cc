/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: uiodvw2dhor3dtreeitem.cc,v 1.22 2012/02/16 05:05:37 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiodvw2dhor3dtreeitem.h"

#include "uiattribpartserv.h"
#include "uiflatviewer.h"
#include "uiflatviewwin.h"
#include "uiempartserv.h"
#include "uiflatviewstdcontrol.h"
#include "uigraphicsscene.h"
#include "uilistview.h"
#include "uirgbarraycanvas.h"
#include "uimenu.h"
#include "uimpepartserv.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uivispartserv.h"

#include "attribdatapack.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "ioobj.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "pixmap.h"

#include "visvw2ddataman.h"
#include "visvw2dhorizon3d.h"


uiODVw2DHor3DParentTreeItem::uiODVw2DHor3DParentTreeItem()
    : uiODVw2DTreeItem( "Horizon 3D" )
{
}


uiODVw2DHor3DParentTreeItem::~uiODVw2DHor3DParentTreeItem()
{
}


bool uiODVw2DHor3DParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&New"), 0 );
    mnu.insertItem( new uiMenuItem("&Load ..."), 1 );
    handleSubMenu( mnu.exec() );
    return true;
}


bool uiODVw2DHor3DParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid == 0 )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet(
				applMgr()->attrServer()->curDescSet(false) );
	if ( !mps->addTracker( EM::Horizon3D::typeStr(), -1 ) )
	    return true;

	const int trackid = mps->activeTrackerID();
	uiODVw2DHor3DTreeItem* hortreeitem = 
	    new uiODVw2DHor3DTreeItem( mps->getEMObjectID(trackid) );
	addChld( hortreeitem,false, false );
	viewer2D()->viewControl()->setEditMode( true );
	hortreeitem->select();
    }
    else if ( mnuid == 1 )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectHorizons( objs, false );

	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    if ( MPE::engine().getTrackerByObject(objs[idx]->id()) != -1 )
		MPE::engine().addTracker( objs[idx] );
	    addChld( new uiODVw2DHor3DTreeItem(objs[idx]->id()), false, false);
	}

	deepUnRef( objs );
    }

    return true;
}


bool uiODVw2DHor3DParentTreeItem::init()
{
    return true;
}


void uiODVw2DHor3DParentTreeItem::tempObjAddedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
    if ( !hor3d ) return;

    if ( findChild(applMgr()->EMServer()->getName(emid)) )
	return;

    addChld( new uiODVw2DHor3DTreeItem(emid), false, false);    
}



uiODVw2DHor3DTreeItem::uiODVw2DHor3DTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(0)
    , emid_(emid)
    , horview_(0)
    , oldactivevolupdated_(false)
    , trackerefed_(false)
{
    if ( MPE::engine().getTrackerByObject(emid_) != -1 )
    {
	MPE::engine().getEditor( emid_, true );
	trackerefed_ = true;
    }
}


uiODVw2DHor3DTreeItem::uiODVw2DHor3DTreeItem( int id, bool )
    : uiODVw2DTreeItem(0)
    , emid_(-1)
    , horview_(0)
    , oldactivevolupdated_(false)
    , trackerefed_(false)
{
    displayid_ = id;
}


uiODVw2DHor3DTreeItem::~uiODVw2DHor3DTreeItem()
{
    NotifierAccess* deselnotify = horview_ ? horview_->deSelection() : 0;
    if ( deselnotify )
	deselnotify->remove( mCB(this,uiODVw2DHor3DTreeItem,deSelCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonPressed.remove(
		mCB(this,uiODVw2DHor3DTreeItem,mousePressInVwrCB) );
	meh->buttonReleased.remove(
		mCB(this,uiODVw2DHor3DTreeItem,mouseReleaseInVwrCB) );
	meh->buttonReleased.remove(
		mCB(this,uiODVw2DHor3DTreeItem,msRelEvtCompletedInVwrCB) );
    }

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,uiODVw2DHor3DTreeItem,emobjChangeCB) );

	if ( trackerefed_ )
	{
	    const int trackeridx =
				MPE::engine().getTrackerByObject( emobj->id() );
	    if ( trackeridx >= 0 )
	    {
		MPE::engine().removeEditor( emobj->id() );
		MPE::engine().removeTracker( trackeridx );
	    }
	}
    }

    viewer2D()->dataMgr()->removeObject( horview_ );
}


bool uiODVw2DHor3DTreeItem::init()
{
    EM::EMObject* emobj = 0;
    if ( displayid_ < 0 )
    {
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	horview_ = Vw2DHorizon3D::create( emid_, viewer2D()->viewwin(),
				      viewer2D()->dataEditor() );
    }
    else
    {
	mDynamicCastGet(Vw2DHorizon3D*,hd,
		viewer2D()->dataMgr()->getObject(displayid_))
	if ( !hd )
	    return false;
	emid_ = hd->emID();
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	horview_ = hd;
    }
    emobj->change.notify( mCB(this,uiODVw2DHor3DTreeItem,emobjChangeCB) );
    displayMiniCtab();

    name_ = applMgr()->EMServer()->getName( emid_ );
    uilistviewitem_->setCheckable(true);
    uilistviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DHor3DTreeItem,checkCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
			&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonPressed.notify(
		mCB(this,uiODVw2DHor3DTreeItem,mousePressInVwrCB) );
	meh->buttonReleased.notify(
		mCB(this,uiODVw2DHor3DTreeItem,mouseReleaseInVwrCB) );
    }


    horview_->setSelSpec( &viewer2D()->selSpec(true), true );
    horview_->setSelSpec( &viewer2D()->selSpec(false), false );
    horview_->draw();

    if ( displayid_ < 0 )
	viewer2D()->dataMgr()->addObject( horview_ );

    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODVw2DHor3DTreeItem,deSelCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
		    &vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonReleased.notify(
		mCB(this,uiODVw2DHor3DTreeItem,msRelEvtCompletedInVwrCB) );
    }

    return true;
}


void uiODVw2DHor3DTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );

    PtrMan<ioPixmap> pixmap = new ioPixmap( cPixmapWidth(), cPixmapHeight() );
    pixmap->fill( emobj->preferredColor() );
    uilistviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *pixmap );
}


void uiODVw2DHor3DTreeItem::emobjChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const EM::EMObjectCallbackData&,
	    			cbdata, caller, cb );
    mDynamicCastGet(EM::EMObject*,emobject,caller);
    if ( !emobject ) return;

    switch( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	    break;
	case EM::EMObjectCallbackData::PrefColorChange:
	    {
		displayMiniCtab();
		break;
	    }
	default: break;
    }
}


bool uiODVw2DHor3DTreeItem::select()
{
    uilistviewitem_->setSelected( true );

    if ( !trackerefed_ )
    {
	if (  MPE::engine().getTrackerByObject(emid_) != -1 )
	{
	    MPE::engine().addTracker( EM::EMM().getObject(emid_) );
	    MPE::engine().getEditor( emid_, true );
	    trackerefed_ = true;
	}
    }

    viewer2D()->dataMgr()->setSelected( horview_ );
    horview_->selected( isChecked() );
    
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonReleased.remove(
		mCB(this,uiODVw2DHor3DTreeItem,msRelEvtCompletedInVwrCB) );
    }


    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonReleased.notify(
		mCB(this,uiODVw2DHor3DTreeItem,msRelEvtCompletedInVwrCB) );
    }


    return true;
}


bool uiODVw2DHor3DTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    uiMenuItem* savemnu = new uiMenuItem("&Save ... ");
    mnu.insertItem( savemnu, 0 );
    savemnu->setEnabled( applMgr()->EMServer()->isChanged(emid_) &&
	    		 applMgr()->EMServer()->isFullyLoaded(emid_) );
    mnu.insertItem( new uiMenuItem("&Save As ..."), 1 );
    uiMenuItem* cngsetup = new uiMenuItem( "Change setup..." );
    mnu.insertItem( cngsetup, 2 );
    cngsetup->setEnabled( MPE::engine().getTrackerByObject(emid_) > -1 );
    mnu.insertItem( new uiMenuItem("&Remove"), 3 );

    applMgr()->mpeServer()->setCurrentAttribDescSet(
	    			applMgr()->attrServer()->curDescSet(false) );
    applMgr()->mpeServer()->setCurrentAttribDescSet(
	    			applMgr()->attrServer()->curDescSet(true) );

    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
    {
	bool savewithname = EM::EMM().getMultiID( emid_ ).isEmpty();
	if ( !savewithname )
	{
	    PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid_) );
	    savewithname = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, savewithname );
	const MultiID mid = applMgr()->EMServer()->getStorageID(emid_);
	applMgr()->mpeServer()->saveSetup( mid );
	name_ = applMgr()->EMServer()->getName( emid_ );
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
    }
    else if ( mnuid == 1 )
    {
	const MultiID oldmid = applMgr()->EMServer()->getStorageID(emid_);
	applMgr()->mpeServer()->prepareSaveSetupAs( oldmid );

	MultiID storedmid;
	applMgr()->EMServer()->storeObject( emid_, true, storedmid );
	name_ = applMgr()->EMServer()->getName( emid_ );

	const MultiID midintree = applMgr()->EMServer()->getStorageID(emid_);
	EM::EMM().getObject(emid_)->setMultiID( storedmid);
	applMgr()->mpeServer()->saveSetupAs( storedmid );
	EM::EMM().getObject(emid_)->setMultiID( midintree );

	uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
    }
    else if ( mnuid == 2 )
    {
	EM::EMObject* emobj = EM::EMM().getObject( emid_ );
	if ( emobj )
	{
	    const int sectionid = emobj->sectionID( emobj->nrSections()-1 );
	    applMgr()->mpeServer()->showSetupDlg( emid_, sectionid );
	}
    }
    else if ( mnuid == 3 )
	parent_->removeChild( this );

    return true;
}


void uiODVw2DHor3DTreeItem::checkCB( CallBacker* )
{
    horview_->enablePainting( isChecked() );
}


void uiODVw2DHor3DTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DHor3DTreeItem::updateSelSpec( const Attrib::SelSpec* selspec,
					   bool wva )
{
    horview_->setSelSpec( selspec, wva );
}


void uiODVw2DHor3DTreeItem::updateCS( const CubeSampling& cs, bool upd )
{
    if ( upd )
	horview_->setCubeSampling( cs, upd );
}


void uiODVw2DHor3DTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    if ( emid != emid_ ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
    if ( !hor3d ) return;

    parent_->removeChild( this );
}


void uiODVw2DHor3DTreeItem::mousePressInVwrCB( CallBacker* )
{
    if ( !uilistviewitem_->isSelected() || !horview_ )
	return;

    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    horview_->setSeedPicking( applMgr()->visServer()->isPicking() );
    horview_->setTrackerSetupActive(
	    applMgr()->visServer()->isTrackingSetupActive() );
}


void uiODVw2DHor3DTreeItem::mouseReleaseInVwrCB( CallBacker* )
{
    if ( !uilistviewitem_->isSelected() || !horview_ )
	return;

    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    if ( !applMgr()->visServer()->isPicking() )
	return;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( 0 );
    const FlatDataPack* fdp = vwr.pack( true );
    if ( !fdp )
	fdp = vwr.pack( false );
    if ( !fdp ) return;

    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
    if ( !dp3d ) return;

    if ( MPE::engine().activeVolume() != dp3d->cube().cubeSampling() )
    {
	applMgr()->visServer()->updateOldActiVolInuiMPEMan();
	oldactivevolupdated_ = true;
    }

    applMgr()->visServer()->turnQCPlaneOff();
}


void uiODVw2DHor3DTreeItem::msRelEvtCompletedInVwrCB( CallBacker* )
{
    if ( !uilistviewitem_->isSelected() || !horview_ ||
	 !viewer2D()->viewwin()->nrViewers() )
	return;

    if ( oldactivevolupdated_ )
       applMgr()->visServer()->restoreActiveVolInuiMPEMan();

    oldactivevolupdated_ = false;
}


uiTreeItem* uiODVw2DHor3DTreeItemFactory::createForVis( 
				    const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet(const Vw2DHorizon3D*,obj,vwr2d.dataMgr()->getObject(id));
    return obj ? new uiODVw2DHor3DTreeItem(id,true) : 0;
}

