/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: uiodvw2dhor2dtreeitem.cc,v 1.1 2010-06-24 08:57:00 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dhor2dtreeitem.h"

#include "uiflatviewer.h"
#include "uiflatviewwin.h"
#include "uiempartserv.h"
#include "uigraphicsscene.h"
#include "uimenu.h"
#include "uilistview.h"
#include "uirgbarraycanvas.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uivispartserv.h"

#include "emhorizon2d.h"
#include "emmanager.h"
#include "emobject.h"
#include "mouseevent.h"

#include "visseis2ddisplay.h"
#include "visvw2ddataman.h"
#include "visvw2dhorizon2d.h"


uiODVw2DHor2DParentTreeItem::uiODVw2DHor2DParentTreeItem()
    : uiODVw2DTreeItem( "Horizon 2D" )
{
}


uiODVw2DHor2DParentTreeItem::~uiODVw2DHor2DParentTreeItem()
{
    applMgr()->EMServer()->tempobjAdded.remove(
	    mCB(this,uiODVw2DHor2DParentTreeItem,tempObjAddedCB) );
}


bool uiODVw2DHor2DParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Load ..."), 0 );
    handleSubMenu( mnu.exec() );
    return true;
}


bool uiODVw2DHor2DParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid == 0 )
    {
	TypeSet<MultiID> sortedmids;
	EM::EMM().sortedHorizonsList( sortedmids, true );

	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectHorizons( objs, true );

	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODVw2DHor2DTreeItem(objs[idx]->id()), false, false);

	deepUnRef( objs );
    }

    return true;
}


bool uiODVw2DHor2DParentTreeItem::init()
{
    applMgr()->EMServer()->tempobjAdded.notify(
	    mCB(this,uiODVw2DHor2DParentTreeItem,tempObjAddedCB) );

    return true;
}


void uiODVw2DHor2DParentTreeItem::tempObjAddedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj);
    if ( !hor2d ) return;

    addChild( new uiODVw2DHor2DTreeItem(emid), false, false);
}


uiODVw2DHor2DTreeItem::uiODVw2DHor2DTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(0)
    , horview_(0)
    , emid_( emid )
{}


uiODVw2DHor2DTreeItem::~uiODVw2DHor2DTreeItem()
{
    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	deselnotify->remove( mCB(this,uiODVw2DHor2DTreeItem,deSelCB) );

    applMgr()->EMServer()->tempobjAbtToDel.remove(
	    mCB(this,uiODVw2DHor2DTreeItem,emobjAbtToDelCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonReleased.remove(
		mCB(this,uiODVw2DHor2DTreeItem,musReleaseInVwrCB) );
    }

    viewer2D()->dataMgr()->removeObject( horview_ );
}


bool uiODVw2DHor2DTreeItem::init()
{
    name_ = applMgr()->EMServer()->getName( emid_ );
    uilistviewitem_->setCheckable(true);
    uilistviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DHor2DTreeItem,checkCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh = 
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonReleased.notify(
		mCB(this,uiODVw2DHor2DTreeItem,musReleaseInVwrCB) );
    }

    horview_ = new Vw2DHorizon2D( emid_, viewer2D()->viewwin(),
	    			  viewer2D()->dataEditor() );
    horview_->setSelSpec( &viewer2D()->selSpec(true), true );
    horview_->setSelSpec( &viewer2D()->selSpec(false), false );
    horview_->setLineName(
	    applMgr()->visServer()->getObjectName(viewer2D()->visid_) );
    
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
	    applMgr()->visServer()->getObject(viewer2D()->visid_));
    if ( s2d )
	horview_->setLineSetID( s2d->lineSetID() );

    horview_->draw();
    viewer2D()->dataMgr()->addObject( horview_ );

    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODVw2DHor2DTreeItem,deSelCB) );

    applMgr()->EMServer()->tempobjAbtToDel.notify(
	    mCB(this,uiODVw2DHor2DTreeItem,emobjAbtToDelCB) );
    
    return true;
}


bool uiODVw2DHor2DTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Remove ..."), 0 );

    if (  mnu.exec() == 0 )
    {
	parent_->removeChild( this );
    }

    return true;
}


bool uiODVw2DHor2DTreeItem::select()
{
    if ( !uilistviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( horview_ );
    horview_->selected();
    return true;
}


void uiODVw2DHor2DTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DHor2DTreeItem::checkCB( CallBacker* )
{
    horview_->enablePainting( isChecked() );
}


void uiODVw2DHor2DTreeItem::updateSelSpec( const Attrib::SelSpec* selspec,
					   bool wva )
{
    horview_->setSelSpec( selspec, wva );
}


void uiODVw2DHor2DTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj);
    if ( !hor2d ) return;

    if ( emid != emid_ ) return;

    parent_->removeChild( this );
}


void uiODVw2DHor2DTreeItem::musReleaseInVwrCB( CallBacker* )
{
    if ( !uilistviewitem_->isSelected() )
	return;

    horview_->setSeedPicking( applMgr()->visServer()->isPicking() );
    horview_->setTrackerSetupActive(
	    		applMgr()->visServer()->isTrackingSetupActive() );
}
