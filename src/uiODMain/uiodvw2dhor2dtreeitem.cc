/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "uiodvw2dhor2dtreeitem.h"

#include "uiattribpartserv.h"
#include "uiflatviewer.h"
#include "uiflatviewwin.h"
#include "uiempartserv.h"
#include "uiflatviewstdcontrol.h"
#include "uigraphicsscene.h"
#include "uimenu.h"
#include "uimpepartserv.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uirgbarraycanvas.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "emhorizon2d.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioobj.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "view2ddataman.h"
#include "view2dhorizon2d.h"

#define mNewIdx		10

uiODVw2DHor2DParentTreeItem::uiODVw2DHor2DParentTreeItem()
    : uiODVw2DTreeItem( tr("2D Horizon") )
{
}


uiODVw2DHor2DParentTreeItem::~uiODVw2DHor2DParentTreeItem()
{
}


void uiODVw2DHor2DParentTreeItem::getNonLoadedTrackedHor2Ds(
	DBKeySet& emids )
{
    const int nrtracker = MPE::engine().nrTrackersAlive();
    DBKeySet loadedemids;
    getLoadedHorizon2Ds( loadedemids );
    for ( int idx=0; idx<nrtracker; idx++ )
    {
	MPE::EMTracker* tracker = MPE::engine().getTracker( idx );
	if ( !tracker )
	    continue;

	EM::Object* emobj = tracker->emObject();
	mDynamicCastGet(EM::Horizon2D*,hor2d,emobj);
	if ( !hor2d || loadedemids.isPresent(emobj->id()) )
	    continue;

	emids.addIfNew( emobj->id() );
    }
}


bool uiODVw2DHor2DParentTreeItem::showSubMenu()
{
    const bool cantrack = !viewer2D()->hasZAxisTransform();

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.addMenu( createAddMenu() );

    DBKeySet emids;
    getNonLoadedTrackedHor2Ds( emids );
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
			EM::Hor2DMan().getObject( emids[idx] );
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


bool uiODVw2DHor2DParentTreeItem::handleSubMenu( int mnuid )
{
    handleStdSubMenu( mnuid );

    if ( mnuid >= mNewIdx )
    {
	DBKeySet emids;
	getNonLoadedTrackedHor2Ds( emids );
	const int emidx = mnuid - mNewIdx - 1;
	if ( emidx >= emids.size() )
	    return false;

	uiMPEPartServer* mps = applMgr()->mpeServer();
	DBKey emid = DBKey::getInvalid();
	if ( emids.validIdx(emidx) )
	    emid = emids[emidx];

	EM::Object* emobj = EM::Hor2DMan().getObject( emid );
	if ( emobj )
	    MPE::engine().setActiveTracker( emobj->id() );
	else if ( !mps->addTracker(EM::Horizon2D::typeStr(),-1) )
	    return true;

	const int trackid = mps->activeTrackerID();
	emid = mps->getEMObjectID( trackid );
	addNewTrackingHorizon2D( emid );
	applMgr()->viewer2DMgr().addNewTrackingHorizon2D( emid,-1 );
	mps->enableTracking( trackid, true );
    }
    else if ( isAddItem(mnuid,true) || isAddItem(mnuid,false) )
    {
	ObjectSet<EM::Object> objs;
	applMgr()->EMServer()->selectHorizons( objs, true );
	DBKeySet emids;
	for ( int idx=0; idx<objs.size(); idx++ )
	    emids += objs[idx]->id();

	if ( isAddItem(mnuid,true) )
	{
	    addHorizon2Ds( emids );
	    applMgr()->viewer2DMgr().addHorizon2Ds( emids );
	}
	else
	    addHorizon2Ds( emids );

	deepUnRef( objs );
    }

    return true;
}


void uiODVw2DHor2DParentTreeItem::getHor2DVwr2DIDs(
	const DBKey& emid, TypeSet<int>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DHor2DTreeItem*,hor2dtreeitm,getChild(idx))
	if ( !hor2dtreeitm || hor2dtreeitm->emObjectID() != emid )
	    continue;

	vw2dobjids.addIfNew( hor2dtreeitm->vw2DObject()->id() );
    }
}


void uiODVw2DHor2DParentTreeItem::getLoadedHorizon2Ds(
	DBKeySet& emids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DHor2DTreeItem*,hor2dtreeitm,getChild(idx))
	if ( !hor2dtreeitm )
	    continue;
	emids.addIfNew( hor2dtreeitm->emObjectID() );
    }
}


void uiODVw2DHor2DParentTreeItem::removeHorizon2D( const DBKey& emid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor2DTreeItem*,hor2dtreeitm,getChild(idx))
	if ( !hor2dtreeitm || emid!=hor2dtreeitm->emObjectID() )
	    continue;
	removeChild( hor2dtreeitm );
    }
}


void uiODVw2DHor2DParentTreeItem::addHorizon2Ds(
	const DBKeySet& emids )
{
    DBKeySet emidstobeloaded, emidsloaded;
    getLoadedHorizon2Ds( emidsloaded );
    for ( int idx=0; idx<emids.size(); idx++ )
    {
	if ( !emidsloaded.isPresent(emids[idx]) )
	    emidstobeloaded.addIfNew( emids[idx] );
    }

    for ( int idx=0; idx<emidstobeloaded.size(); idx++ )
    {
	const bool hastracker =
	    MPE::engine().hasTracker( emidstobeloaded[idx] );
	if ( hastracker )
	{
	    EM::Object* emobj =
			EM::Hor2DMan().getObject( emidstobeloaded[idx] );
	    if ( !emobj || findChild(emobj->name()) )
		continue;

	    MPE::engine().getEditor( emobj->id(), true );
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    const int trackeridx =
		MPE::engine().getTrackerByObject( emidstobeloaded[idx] );
	    applMgr()->mpeServer()->enableTracking( trackeridx, true );
	}

	uiODVw2DHor2DTreeItem* childitem =
	    new uiODVw2DHor2DTreeItem( emidstobeloaded[idx] );
	addChld( childitem, false, false );
    }
}


void uiODVw2DHor2DParentTreeItem::setupTrackingHorizon2D( const DBKey& emid )
{
    DBKeySet emidsloaded;
    getLoadedHorizon2Ds( emidsloaded );
    if ( !emidsloaded.isPresent(emid) )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor2DTreeItem*,hor2dtreeitm,getChild(idx))
	if ( hor2dtreeitm && emid==hor2dtreeitm->emObjectID() )
	{
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    hor2dtreeitm->select();
	    break;
	}
    }

}


void uiODVw2DHor2DParentTreeItem::addNewTrackingHorizon2D( const DBKey& emid )
{
    DBKeySet emidsloaded;
    getLoadedHorizon2Ds( emidsloaded );
    if ( emidsloaded.isPresent(emid) )
	return;

    uiODVw2DHor2DTreeItem* hortreeitem = new uiODVw2DHor2DTreeItem( emid );
    const int trackid = applMgr()->mpeServer()->getTrackerID( emid );
    if ( trackid>=0 )
	MPE::engine().getEditor( emid, true );

    addChld( hortreeitem, false, false );
    if ( viewer2D() && viewer2D()->viewControl() )
	viewer2D()->viewControl()->setEditMode( true );

    hortreeitem->select();
}


const char* uiODVw2DHor2DParentTreeItem::iconName() const
{ return "tree-horizon2d"; }


bool uiODVw2DHor2DParentTreeItem::init()
{ return uiODVw2DTreeItem::init(); }


// uiODVw2DHor2DTreeItem
uiODVw2DHor2DTreeItem::uiODVw2DHor2DTreeItem( const DBKey& emid )
    : uiODVw2DEMTreeItem(emid)
    , horview_(0)
{
    if ( MPE::engine().hasTracker(emid_) )
	MPE::engine().refTracker( emid_ );
}


uiODVw2DHor2DTreeItem::uiODVw2DHor2DTreeItem( int id, bool )
    : uiODVw2DEMTreeItem(DBKey::getInvalid())
    , horview_(0)
{
    displayid_ = id;
}



uiODVw2DHor2DTreeItem::~uiODVw2DHor2DTreeItem()
{
    detachAllNotifiers();

    EM::Object* emobj = EM::Hor2DMan().getObject( emid_ );
    if ( emobj )
    {
	emobj->objectChanged().remove(
		mCB(this,uiODVw2DHor2DTreeItem,emobjChangeCB) );

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


bool uiODVw2DHor2DTreeItem::init()
{
    EM::Object* emobj = 0;
    if ( displayid_ < 0 )
    {
	emobj = EM::Hor2DMan().getObject( emid_ );
	if ( !emobj ) return false;

	horview_ = Vw2DHorizon2D::create( emid_, viewer2D()->viewwin(),
				      viewer2D()->dataEditor() );
	viewer2D()->dataMgr()->addObject( horview_ );
	displayid_ = horview_->id();
    }
    else
    {
	mDynamicCastGet(Vw2DHorizon2D*,hd,
			    viewer2D()->dataMgr()->getObject(displayid_))
	if ( !hd )
	    return false;
	emid_ = hd->emID();
	emobj = EM::Hor2DMan().getObject( emid_ );
	if ( !emobj )
	    return false;

	horview_ = hd;
    }

    if ( emobj )
	mAttachCB( emobj->objectChanged(),
		uiODVw2DHor2DTreeItem::emobjChangeCB );

    displayMiniCtab();

    name_ = toUiString( emid_.name() );
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DHor2DTreeItem,checkCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	mAttachCB( vwr.rgbCanvas().scene().getMouseEventHandler().buttonPressed,
	    uiODVw2DHor2DTreeItem::mouseReleaseInVwrCB );

	mAttachCB( vwr.rgbCanvas().scene().getMouseEventHandler().buttonPressed,
	uiODVw2DHor2DTreeItem::mousePressInVwrCB );

    }

    horview_->setSelSpec( &viewer2D()->selSpec(true), true );
    horview_->setSelSpec( &viewer2D()->selSpec(false), false );

    if ( viewer2D()->geomID().isValid() )
	horview_->setGeomID( viewer2D()->geomID() );

    horview_->draw();

    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	mAttachCB( deselnotify, uiODVw2DHor2DTreeItem::deSelCB );

    uiODVw2DTreeItem::addKeyBoardEvent();
    return true;
}


#define mPropID		0
#define mStartID	1
#define mSettsID	2
#define mSaveID		3
#define mSaveAsID	4

bool uiODVw2DHor2DTreeItem::showSubMenu()
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
	const EM::Object* emobj = EM::Hor2DMan().getObject( emid_ );
	if ( !emobj || mps->addTracker(emid_)==-1 )
	    return false;

	MPE::engine().setActiveTracker( emid_ );
	mps->useSavedSetupDlg( emid_ );
	if ( viewer2D() && viewer2D()->viewControl() )
	    viewer2D()->viewControl()->setEditMode( true );
    }
    else if ( mnuid == mSettsID )
    {
	EM::Object* emobj = EM::Hor2DMan().getObject( emid_ );
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
	bool doremove = !applMgr()->viewer2DMgr().isItemPresent( parent_ ) ||
		isRemoveItem(mnuid,false);
	if ( isRemoveItem(mnuid,true) )
	    applMgr()->viewer2DMgr().removeHorizon2D( emid_ );

	if ( doremove )
	    parent_->removeChild( this );
    }

    return true;
}


bool uiODVw2DHor2DTreeItem::select()
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


void uiODVw2DHor2DTreeItem::deSelCB( CallBacker* )
{
    const int trackeridx =
	MPE::engine().getTrackerByObject( emid_ );
    applMgr()->mpeServer()->enableTracking( trackeridx, false );

    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DHor2DTreeItem::checkCB( CallBacker* )
{
    if ( horview_ )
	horview_->enablePainting( isChecked() );
}


void uiODVw2DHor2DTreeItem::updateSelSpec( const Attrib::SelSpec* selspec,
					   bool wva )
{
    if ( horview_ )
	horview_->setSelSpec( selspec, wva );
}


void uiODVw2DHor2DTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const DBKey&,emid,cb);
    if ( emid != emid_ ) return;

    EM::Object* emobj = EM::Hor2DMan().getObject( emid );
    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj);
    if ( !hor2d )
	return;

    parent_->removeChild( this );
}


void uiODVw2DHor2DTreeItem::mousePressInVwrCB( CallBacker* )
{
    if ( !uitreeviewitem_->isSelected() || !horview_ )
	return;

    horview_->setSeedPicking( applMgr()->visServer()->isPicking() );
    horview_->setTrackerSetupActive(
			applMgr()->visServer()->isTrackingSetupActive() );
}


void uiODVw2DHor2DTreeItem::mouseReleaseInVwrCB( CallBacker* )
{
}


uiTreeItem* uiODVw2DHor2DTreeItemFactory::createForVis(
				    const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet(const Vw2DHorizon2D*,obj,vwr2d.dataMgr()->getObject(id));
    return obj ? new uiODVw2DHor2DTreeItem(id,true) : 0;
}

const Vw2DDataObject* uiODVw2DHor2DTreeItem::vw2DObject() const
{ return horview_; }
