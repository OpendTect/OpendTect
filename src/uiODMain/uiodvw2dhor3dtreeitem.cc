/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "uiodvw2dhor3dtreeitem.h"

#include "uiattribpartserv.h"
#include "uiflatviewer.h"
#include "uiflatviewwin.h"
#include "uiempartserv.h"
#include "uiflatviewstdcontrol.h"
#include "uigraphicsscene.h"
#include "uimenu.h"
#include "uimpe.h"
#include "uimpepartserv.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uirgbarraycanvas.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emtracker.h"
#include "ioobj.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "view2ddataman.h"
#include "view2dhorizon3d.h"

#define mNewIdx		10

uiODVw2DHor3DParentTreeItem::uiODVw2DHor3DParentTreeItem()
    : uiODVw2DTreeItem( tr("3D Horizon") )
{
}


uiODVw2DHor3DParentTreeItem::~uiODVw2DHor3DParentTreeItem()
{
}


void uiODVw2DHor3DParentTreeItem::getNonLoadedTrackedHor3Ds(
	DBKeySet& emids )
{
    const int nrtracker = MPE::engine().nrTrackersAlive();
    DBKeySet loadedemids;
    getLoadedHorizon3Ds( loadedemids );
    for ( int idx=0; idx<nrtracker; idx++ )
    {
	MPE::EMTracker* tracker = MPE::engine().getTracker( idx );
	if ( !tracker )
	    continue;

	EM::Object* emobj = tracker->emObject();
	mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
	if ( !hor3d || loadedemids.isPresent(emobj->id()) )
	    continue;

	emids.addIfNew( emobj->id() );
    }
}


bool uiODVw2DHor3DParentTreeItem::showSubMenu()
{
    const bool cantrack =
	!viewer2D()->hasZAxisTransform() && viewer2D()->isVertical();

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.addMenu( createAddMenu() );

    DBKeySet emids;
    getNonLoadedTrackedHor3Ds( emids );
    if ( emids.isEmpty() )
    {
	uiAction* newact = new uiAction( m3Dots(tr("Track New")) );
	newact->setEnabled( cantrack );
	mnu.insertAction( newact, mNewIdx );
    }
    else
    {
	uiMenu* trackmenu = new uiMenu( uiStrings::sTrack() );
	uiAction* newact = new uiAction( uiStrings::sNew() );
	newact->setEnabled( cantrack );
	trackmenu->insertAction( newact, mNewIdx );
	for ( int idx=0; idx<emids.size(); idx++ )
	{
	    const EM::Object* emobject =
			EM::Hor3DMan().getObject( emids[idx] );
	    uiAction* trackexistingmnu = new uiAction(
			toUiString(emobject->name()) );
	    trackexistingmnu->setEnabled( cantrack );
	    trackmenu->insertAction( trackexistingmnu, mNewIdx + idx + 1 );
	}

	mnu.addMenu( trackmenu );
    }

    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DHor3DParentTreeItem::handleSubMenu( int mnuid )
{
    handleStdSubMenu( mnuid );

    if ( mnuid >= mNewIdx )
    {
	DBKeySet emids;
	getNonLoadedTrackedHor3Ds( emids );
	const int emidx = mnuid - mNewIdx - 1;
	if ( emidx >= emids.size() )
	    return false;

	uiMPEPartServer* mps = applMgr()->mpeServer();
	DBKey emid = DBKey::getInvalid();
	if ( emids.validIdx(emidx) )
	    emid = emids[emidx];

	EM::Object* emobj = EM::Hor3DMan().getObject( emid );
	if ( !emobj )
	{
	// This will add the 3D scene item.
	    const bool res = mps->addTracker( EM::Horizon3D::typeStr(), -1 );
	    if ( !res )
		return true;
	}

	const MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
	if ( !tracker )
	    return false;

	emid = tracker->objectID();
	const int trackid = MPE::engine().getTrackerByObject( emid );
	if ( mps->getSetupGroup() )
	    mps->getSetupGroup()->setTrackingMethod(
						EventTracker::AdjacentParent );
	addNewTrackingHorizon3D( emid );
	applMgr()->viewer2DMgr().addNewTrackingHorizon3D( emid, -1 );
	mps->enableTracking( trackid, true );
    }
    else if ( isAddItem(mnuid,true) || isAddItem(mnuid,false) )
    {
	ObjectSet<EM::Object> objs;
	applMgr()->EMServer()->selectHorizons( objs, false );
	DBKeySet emids;
	for ( int idx=0; idx<objs.size(); idx++ )
	    emids += objs[idx]->id();

	if ( isAddItem(mnuid,true) )
	{
	    addHorizon3Ds( emids );
	    applMgr()->viewer2DMgr().addHorizon3Ds( emids );
	}
	else
	    addHorizon3Ds( emids );

	deepUnRef( objs );
    }

    return true;
}


void uiODVw2DHor3DParentTreeItem::getHor3DVwr2DIDs(
	const DBKey& emid, TypeSet<int>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DHor3DTreeItem*,hor3dtreeitm,getChild(idx))
	if ( !hor3dtreeitm || hor3dtreeitm->emObjectID() != emid )
	    continue;

	vw2dobjids.addIfNew( hor3dtreeitm->vw2DObject()->id() );
    }
}


void uiODVw2DHor3DParentTreeItem::getLoadedHorizon3Ds(
	DBKeySet& emids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DHor3DTreeItem*,hor3dtreeitm,getChild(idx))
	if ( !hor3dtreeitm )
	    continue;
	emids.addIfNew( hor3dtreeitm->emObjectID() );
    }
}


void uiODVw2DHor3DParentTreeItem::removeHorizon3D( const DBKey& emid )
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
	const DBKeySet& emids )
{
    DBKeySet emidstobeloaded, emidsloaded;
    getLoadedHorizon3Ds( emidsloaded );
    for ( int idx=0; idx<emids.size(); idx++ )
    {
	if ( !emidsloaded.isPresent(emids[idx]) )
	    emidstobeloaded.addIfNew( emids[idx] );
    }

    for ( int idx=0; idx<emidstobeloaded.size(); idx++ )
    {
	const DBKey emid = emidstobeloaded[idx];
	const bool hastracker = MPE::engine().hasTracker( emid );
	if ( hastracker )
	{
	    EM::Object* emobj = EM::Hor3DMan().getObject( emid );
	    if ( !emobj || findChild(emobj->name()) )
		continue;

	    MPE::engine().getEditor( emid, true );
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    const int trackeridx = MPE::engine().getTrackerByObject( emid );
	    applMgr()->mpeServer()->enableTracking( trackeridx, true );
	}

	uiODVw2DHor3DTreeItem* childitem =
	    new uiODVw2DHor3DTreeItem( emidstobeloaded[idx] );
	addChld( childitem, false, false);
    }
}

void uiODVw2DHor3DParentTreeItem::setupTrackingHorizon3D( const DBKey& emid )
{
    if ( viewer2D() && !viewer2D()->isVertical() &&
	 !viewer2D()->hasZAxisTransform() )
	return;

    DBKeySet emidsloaded;
    getLoadedHorizon3Ds( emidsloaded );
    if ( !emidsloaded.isPresent(emid) )
	return;

    bool hashorizon = false;
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor3DTreeItem*,hor3dtreeitm,getChild(idx))
	if ( !hor3dtreeitm || emid!=hor3dtreeitm->emObjectID() )
	    continue;

	hashorizon = true;
	hor3dtreeitm->select();
    }

    if ( viewer2D() && viewer2D()->viewControl() && hashorizon )
	viewer2D()->viewControl()->setEditMode( true );
}


void uiODVw2DHor3DParentTreeItem::addNewTrackingHorizon3D( const DBKey& emid )
{
    if ( viewer2D() && !viewer2D()->isVertical() &&
	 !viewer2D()->hasZAxisTransform() )
	return;

    DBKeySet emidsloaded;
    getLoadedHorizon3Ds( emidsloaded );
    if ( emidsloaded.isPresent(emid) )
	return;

    uiODVw2DHor3DTreeItem* hortreeitem = new uiODVw2DHor3DTreeItem( emid );
    const int trackid = applMgr()->mpeServer()->getTrackerID( emid );
    if ( trackid>=0 )
	MPE::engine().getEditor( emid, true );

    addChld( hortreeitem, false, false );
    if ( viewer2D() && viewer2D()->viewControl() )
	viewer2D()->viewControl()->setEditMode( true );

    hortreeitem->select();
}


const char* uiODVw2DHor3DParentTreeItem::iconName() const
{ return "tree-horizon3d"; }


bool uiODVw2DHor3DParentTreeItem::init()
{ return uiODVw2DTreeItem::init(); }


// uiODVw2DHor3DTreeItem
uiODVw2DHor3DTreeItem::uiODVw2DHor3DTreeItem( const DBKey& emid )
    : uiODVw2DEMTreeItem(emid)
    , horview_(0)
{
    if ( MPE::engine().hasTracker(emid_) )
	MPE::engine().refTracker( emid_ );
}


uiODVw2DHor3DTreeItem::uiODVw2DHor3DTreeItem( int id, bool )
    : uiODVw2DEMTreeItem(DBKey::getInvalid())
    , horview_(0)
{
    displayid_ = id;
}


uiODVw2DHor3DTreeItem::~uiODVw2DHor3DTreeItem()
{
    detachAllNotifiers();

    EM::Object* emobj = EM::Hor3DMan().getObject( emid_ );
    if ( emobj )
    {
	emobj->objectChanged().remove(
		mCB(this,uiODVw2DHor3DTreeItem,emobjChangeCB) );

	DBKey emid = emobj->id();
	if ( MPE::engine().hasTracker(emid) )
	{
	    MPE::engine().removeEditor( emid );
	    MPE::engine().unRefTracker( emid );
	}
    }

    if ( horview_ )
	viewer2D()->dataMgr()->removeObject( horview_ );
}


bool uiODVw2DHor3DTreeItem::init()
{
    EM::Object* emobj = 0;
    if ( displayid_ < 0 )
    {
	emobj = EM::Hor3DMan().getObject( emid_ );
	if ( !emobj ) return false;

	horview_ = Vw2DHorizon3D::create( emid_, viewer2D()->viewwin(),
				      viewer2D()->dataEditor() );
	viewer2D()->dataMgr()->addObject( horview_ );
	displayid_ = horview_->id();
    }
    else
    {
	mDynamicCastGet(Vw2DHorizon3D*,hd,
		viewer2D()->dataMgr()->getObject(displayid_))
	if ( !hd )
	    return false;
	emid_ = hd->emID();
	emobj = EM::Hor3DMan().getObject( emid_ );
	if ( !emobj ) return false;

	horview_ = hd;
    }

    if ( emobj )
	mAttachCB( emobj->objectChanged(),
		uiODVw2DHor3DTreeItem::emobjChangeCB );

    displayMiniCtab();

    name_ = toUiString( emid_.name() );
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DHor3DTreeItem,checkCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );

	mAttachCB( vwr.rgbCanvas().scene().getMouseEventHandler().buttonPressed,
	    uiODVw2DHor3DTreeItem::mouseReleaseInVwrCB );

	mAttachCB( vwr.rgbCanvas().scene().getMouseEventHandler().buttonPressed,
	uiODVw2DHor3DTreeItem::mousePressInVwrCB );

    }

    horview_->setSelSpec( &viewer2D()->selSpec(true), true );
    horview_->setSelSpec( &viewer2D()->selSpec(false), false );
    horview_->draw();

    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	mAttachCB( deselnotify, uiODVw2DHor3DTreeItem::deSelCB );

    uiODVw2DTreeItem::addKeyBoardEvent();
    return true;
}


#define mPropID		0
#define mStartID	1
#define mSettsID	2
#define mSaveID		3
#define mSaveAsID	4

bool uiODVw2DHor3DTreeItem::showSubMenu()
{
    uiEMPartServer* ems = applMgr()->EMServer();
    uiMPEPartServer* mps = applMgr()->mpeServer();
    uiVisPartServer* vps = applMgr()->visServer();
    if ( !ems || !mps || !vps ) return false;

    uiMenu mnu( getUiParent(), uiStrings::sAction() );

    addAction( mnu, m3Dots(uiStrings::sProperties()), mPropID, "disppars",true);

    uiMenu* trackmnu = new uiMenu( uiStrings::sTracking() );
    mnu.addMenu( trackmnu );
    const bool hastracker = MPE::engine().getTrackerByObject(emid_) > -1;
    addAction( *trackmnu, m3Dots(tr("Start Tracking")), mStartID,
		0, !hastracker );
    addAction( *trackmnu, m3Dots(tr("Change Settings")), mSettsID,
		"seedpicksettings", hastracker );

    const bool haschanged = ems->isChanged(emid_) && ems->isFullyLoaded(emid_);
    addAction( mnu, uiStrings::sSave(), mSaveID, "save", haschanged );
    addAction( mnu, m3Dots(uiStrings::sSaveAs()), mSaveAsID, "saveas", true );

    uiMenu* removemenu = createRemoveMenu();
    mnu.addMenu( removemenu );

    const int mnuid = mnu.exec();
    if ( mnuid == mPropID )
	showPropDlg();
    else if ( mnuid == mSaveID )
	doSave();
    else if ( mnuid == mSaveAsID )
	doSaveAs();
    else if ( mnuid == mStartID )
    {
	const EM::Object* emobj = EM::Hor3DMan().getObject( emid_ );
	if ( !emobj || mps->addTracker(emid_)==-1 )
	    return false;

	MPE::engine().setActiveTracker( emid_ );
	mps->useSavedSetupDlg( emid_ );
	if ( viewer2D() && viewer2D()->viewControl() )
	    viewer2D()->viewControl()->setEditMode( true );
    }
    else if ( mnuid == mSettsID )
    {
	EM::Object* emobj = EM::Hor3DMan().getObject( emid_ );
	if ( emobj )
	    mps->showSetupDlg( emid_ );
    }
    else if ( isRemoveItem(mnuid,false) || isRemoveItem(mnuid,true) )
    {
	if ( !ems->askUserToSave(emid_,true) )
	    return true;

	const int trackerid = mps->getTrackerID( emid_ );
	if ( trackerid>= 0 )
	    renameVisObj();

	name_ = toUiString( emid_.name() );
	if ( isRemoveItem(mnuid,true) )
	    applMgr()->viewer2DMgr().removeHorizon3D( emid_ );

	const bool doremove = !applMgr()->viewer2DMgr().isItemPresent( parent_ )
			|| isRemoveItem(mnuid,false);
	if ( doremove )
	    parent_->removeChild( this );
    }

    return true;
}


void uiODVw2DHor3DTreeItem::checkCB( CallBacker* )
{
    if ( horview_ )
	horview_->enablePainting( isChecked() );
}


bool uiODVw2DHor3DTreeItem::select()
{
    if ( uitreeviewitem_->treeView() )
	uitreeviewitem_->treeView()->clearSelection();

    uitreeviewitem_->setSelected( true );

    if ( horview_ )
    {
	viewer2D()->dataMgr()->setSelected( horview_ );
	horview_->selected( isChecked() );
    }

    const int trackeridx =
	MPE::engine().getTrackerByObject( emid_ );
    applMgr()->mpeServer()->enableTracking( trackeridx, true );
    return true;
}


void uiODVw2DHor3DTreeItem::deSelCB( CallBacker* )
{
    const int trackeridx =
	MPE::engine().getTrackerByObject( emid_ );
    applMgr()->mpeServer()->enableTracking( trackeridx, false );

    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DHor3DTreeItem::updateSelSpec( const Attrib::SelSpec* selspec,
					   bool wva )
{
    if ( horview_ )
	horview_->setSelSpec( selspec, wva );
}


void uiODVw2DHor3DTreeItem::updateCS( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd && horview_ )
	horview_->setTrcKeyZSampling( cs, upd );
}


void uiODVw2DHor3DTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const DBKey&, emid, cb );
    if ( emid != emid_ ) return;

    EM::Object* emobj = EM::Hor3DMan().getObject( emid );
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

const Vw2DDataObject* uiODVw2DHor3DTreeItem::vw2DObject() const
{ return horview_; }
