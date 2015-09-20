/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodvw2dhor3dtreeitem.h"

#include "uiattribpartserv.h"
#include "uiflatviewer.h"
#include "uiflatviewwin.h"
#include "uiempartserv.h"
#include "uiflatviewstdcontrol.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"
#include "uimenu.h"
#include "uimpepartserv.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "ioobj.h"
#include "mouseevent.h"
#include "mpeengine.h"

#include "view2ddataman.h"
#include "view2dhorizon3d.h"


uiODVw2DHor3DParentTreeItem::uiODVw2DHor3DParentTreeItem()
    : uiODVw2DTreeItem( "Horizon 3D" )
{
}


uiODVw2DHor3DParentTreeItem::~uiODVw2DHor3DParentTreeItem()
{
}


bool uiODVw2DHor3DParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(m3Dots(uiStrings::sNew())), 0 );
    uiMenu* loadmenu = new uiMenu( m3Dots(uiStrings::sAdd()) );
    loadmenu->insertItem( new uiAction(tr("In all 2D Viewers")), 1 );
    loadmenu->insertItem( new uiAction(tr("Only in this 2D Viewer")), 2 );
    mnu.insertItem( loadmenu );
    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DHor3DParentTreeItem::handleSubMenu( int mnuid )
{
    handleStdSubMenu( mnuid );

    if ( mnuid == 0 )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet(
		applMgr()->attrServer()->curDescSet(false) );
	if ( !mps->addTracker( EM::Horizon3D::typeStr(), -1 ) )
	    return true;

	const int trackid = mps->activeTrackerID();
	const int emid = mps->getEMObjectID( trackid );
	applMgr()->viewer2DMgr().addNewTrackingHorizon3D( emid );
    }
    else if ( mnuid == 1 || mnuid==2 )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectHorizons( objs, false );
	TypeSet<EM::ObjectID> emids;
	for ( int idx=0; idx<objs.size(); idx++ )
	    emids += objs[idx]->id();
	if ( mnuid==1 )
	    applMgr()->viewer2DMgr().addHorizon3Ds( emids );
	else
	    addHorizon3Ds( emids );

	deepUnRef( objs );
    }

    return true;
}


void uiODVw2DHor3DParentTreeItem::getLoadedHorizon3Ds(
	TypeSet<EM::ObjectID>& emids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DHor3DTreeItem*,hor3dtreeitm,getChild(idx))
	if ( !hor3dtreeitm )
	    continue;
	emids.addIfNew( hor3dtreeitm->emObjectID() );
    }
}


void uiODVw2DHor3DParentTreeItem::removeHorizon3D( EM::ObjectID emid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor3DTreeItem*,hor3dtreeitm,getChild(idx))
	if ( !hor3dtreeitm || emid!=hor3dtreeitm->emObjectID() )
	    continue;
	removeChild( hor3dtreeitm );
    }
}


void uiODVw2DHor3DParentTreeItem::addHorizon3Ds(
	const TypeSet<EM::ObjectID>& emids )
{
    for ( int idx=0; idx<emids.size(); idx++ )
    {
	if ( MPE::engine().getTrackerByObject(emids[idx]) != -1 )
	{
	    EM::EMObject* emobj = EM::EMM().getObject( emids[idx] );
	    if ( !emobj || findChild(emobj->name()) )
		continue;

	    MPE::engine().addTracker( emobj );
	}

	addChld( new uiODVw2DHor3DTreeItem(emids[idx]), false, false);
    }
}


void uiODVw2DHor3DParentTreeItem::addNewTrackingHorizon3D( EM::ObjectID emid )
{
    uiODVw2DHor3DTreeItem* hortreeitem = new uiODVw2DHor3DTreeItem( emid );
    addChld( hortreeitem,false, false );
    viewer2D()->viewControl()->setEditMode( true );
    hortreeitem->select();
}


const char* uiODVw2DHor3DParentTreeItem::iconName() const
{ return "tree-horizon3d"; }


bool uiODVw2DHor3DParentTreeItem::init()
{ return uiODVw2DTreeItem::init(); }



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
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
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

    return true;
}


void uiODVw2DHor3DTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				emobj->preferredColor() );
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
    uitreeviewitem_->setSelected( true );

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

    return true;
}


bool uiODVw2DHor3DTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    uiAction* savemnu = new uiAction(m3Dots(uiStrings::sSave()));
    mnu.insertItem( savemnu, 0 );
    savemnu->setEnabled( applMgr()->EMServer()->isChanged(emid_) &&
			 applMgr()->EMServer()->isFullyLoaded(emid_) );
    mnu.insertItem( new uiAction(uiStrings::sSaveAs()), 1 );
    uiAction* cngsetup = new uiAction( sChangeSetup() );
    mnu.insertItem( cngsetup, 2 );
    cngsetup->setEnabled( MPE::engine().getTrackerByObject(emid_) > -1 );
    uiMenu* removemenu = new uiMenu( uiStrings::sRemove() );
    removemenu->insertItem( new uiAction(tr("From all 2D Viewers")), 3 );
    removemenu->insertItem( new uiAction(tr("Only from this 2D Viewer")), 4 );
    mnu.insertItem( removemenu );

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
	    const EM::SectionID sid = emobj->sectionID( emobj->nrSections()-1 );
	    applMgr()->mpeServer()->showSetupDlg( emid_, sid );
	}
    }
    else if ( mnuid == 3 )
	applMgr()->viewer2DMgr().removeHorizon3D( emid_ );
    else if ( mnuid==4 )
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


void uiODVw2DHor3DTreeItem::updateCS( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd )
	horview_->setTrcKeyZSampling( cs, upd );
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
    if ( !uitreeviewitem_->isSelected() || !horview_ )
	return;

    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    horview_->setSeedPicking( applMgr()->visServer()->isPicking() );
    horview_->setTrackerSetupActive(
	    applMgr()->visServer()->isTrackingSetupActive() );
}


void uiODVw2DHor3DTreeItem::mouseReleaseInVwrCB( CallBacker* )
{
}


uiTreeItem* uiODVw2DHor3DTreeItemFactory::createForVis(
				    const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet(const Vw2DHorizon3D*,obj,vwr2d.dataMgr()->getObject(id));
    return obj ? new uiODVw2DHor3DTreeItem(id,true) : 0;
}

