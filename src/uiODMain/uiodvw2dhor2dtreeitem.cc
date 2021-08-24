/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "uiodvw2dhor2dtreeitem.h"

#include "uiattribpartserv.h"
#include "uicolor.h"
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
#include "uisellinest.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "emhorizon2d.h"
#include "emmanager.h"
#include "ioman.h"
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
	TypeSet<EM::ObjectID>& emids )
{
    const int nrtracker = MPE::engine().nrTrackersAlive();
    TypeSet<EM::ObjectID> loadedemids;
    getLoadedHorizon2Ds( loadedemids );
    for ( int idx=0; idx<nrtracker; idx++ )
    {
	MPE::EMTracker* tracker = MPE::engine().getTracker( idx );
	if ( !tracker )
	    continue;

	EM::EMObject* emobj = tracker->emObject();
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

    TypeSet<EM::ObjectID> emids;
    getNonLoadedTrackedHor2Ds( emids );
    if ( emids.isEmpty() )
    {
	uiAction* newmenu = new uiAction( m3Dots(tr("Track New")) );
	newmenu->setEnabled( cantrack );
	mnu.insertAction( newmenu, mNewIdx );
    }
    else
    {
	uiMenu* trackmenu = new uiMenu( tr("Track") );
	uiAction* newmenu = new uiAction( uiStrings::sNew() );
	newmenu->setEnabled( cantrack );
	trackmenu->insertAction( newmenu, mNewIdx );
	for ( int idx=0; idx<emids.size(); idx++ )
	{
	    const EM::EMObject* emobject = EM::EMM().getObject( emids[idx] );
	    uiAction* trackexistingmnu =
				new uiAction( toUiString(emobject->name()) );
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
	TypeSet<EM::ObjectID> emids;
	getNonLoadedTrackedHor2Ds( emids );
	const int emidx = mnuid - mNewIdx - 1;
	if ( emidx >= emids.size() )
	    return false;

	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet(
		applMgr()->attrServer()->curDescSet(true) );
	int emid = -1;
	if ( emids.validIdx(emidx) )
	    emid = emids[emidx];

	EM::EMObject* emobj = EM::EMM().getObject( emid );
	if ( emobj )
	    MPE::engine().setActiveTracker( emobj->id() );
	else if ( !mps->addTracker(EM::Horizon2D::typeStr(),
				   viewer2D()->getSyncSceneID()) )
	    return true;

	const int trackid = mps->activeTrackerID();
	emid = mps->getEMObjectID( trackid );
	addNewTrackingHorizon2D( emid );
	applMgr()->viewer2DMgr().addNewTrackingHorizon2D(
		emid, viewer2D()->getSyncSceneID() );
	mps->enableTracking( trackid, true );
    }
    else if ( isAddItem(mnuid,true) || isAddItem(mnuid,false) )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectHorizons( objs, true );
	TypeSet<EM::ObjectID> emids;
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
	EM::ObjectID emid, TypeSet<int>& vw2dobjids ) const
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
	TypeSet<EM::ObjectID>& emids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DHor2DTreeItem*,hor2dtreeitm,getChild(idx))
	if ( !hor2dtreeitm )
	    continue;
	emids.addIfNew( hor2dtreeitm->emObjectID() );
    }
}


void uiODVw2DHor2DParentTreeItem::removeHorizon2D( EM::ObjectID emid )
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
	const TypeSet<EM::ObjectID>& emids )
{
    TypeSet<EM::ObjectID> emidstobeloaded, emidsloaded;
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
	    EM::EMObject* emobj = EM::EMM().getObject( emidstobeloaded[idx] );
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


void uiODVw2DHor2DParentTreeItem::setupTrackingHorizon2D( EM::ObjectID emid )
{
    TypeSet<EM::ObjectID> emidsloaded;
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
	}
    }

}


void uiODVw2DHor2DParentTreeItem::addNewTrackingHorizon2D( EM::ObjectID emid )
{
    TypeSet<EM::ObjectID> emidsloaded;
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



uiODVw2DHor2DTreeItem::uiODVw2DHor2DTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(uiString::emptyString())
    , emid_(emid)
    , horview_(0)
    , trackerefed_(false)
{
    if ( MPE::engine().hasTracker(emid_) )
	MPE::engine().refTracker( emid_ );
}


uiODVw2DHor2DTreeItem::uiODVw2DHor2DTreeItem( int id, bool )
    : uiODVw2DTreeItem(uiString::emptyString())
    , emid_(-1)
    , horview_(0)
    , trackerefed_(false)
{
    displayid_ = id;
}



uiODVw2DHor2DTreeItem::~uiODVw2DHor2DTreeItem()
{
     detachAllNotifiers();

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,uiODVw2DHor2DTreeItem,emobjChangeCB) );

	EM::ObjectID emid = emobj->id();
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
    const Line2DInterSectionSet* intersectionset =
			ODMainWin()->viewer2DMgr().getLine2DInterSectionSet();
    EM::EMObject* emobj = 0;
    if ( displayid_ < 0 )
    {
	emobj = EM::EMM().getObject( emid_ );
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
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj )
	    return false;

	horview_ = hd;
    }

     if ( emobj )
	mAttachCB( emobj->change, uiODVw2DHor2DTreeItem::emobjChangeCB );

    displayMiniCtab();

    name_ = applMgr()->EMServer()->getUiName( emid_ );
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

    if ( viewer2D()->geomID() != Survey::GeometryManager::cUndefGeomID() )
	horview_->setGeomID( viewer2D()->geomID() );

    horview_->setLine2DInterSectionSet( intersectionset );

    horview_->draw();

    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	mAttachCB( deselnotify, uiODVw2DHor2DTreeItem::deSelCB );
    uiODVw2DTreeItem::addKeyBoardEvent( emid_ );
    return true;
}


void uiODVw2DHor2DTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				emobj->preferredColor() );
}


void uiODVw2DHor2DTreeItem::emobjChangeCB( CallBacker* cb )
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
	case EM::EMObjectCallbackData::NameChange:
	{
	    name_ = mToUiStringTodo(applMgr()->EMServer()->getName( emid_ ));
	    uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
	    break;
	}
	default: break;
    }
}


void uiODVw2DHor2DTreeItem::renameVisObj()
{
    const MultiID midintree = applMgr()->EMServer()->getStorageID(emid_);
    TypeSet<int> visobjids;
    applMgr()->visServer()->findObject( midintree, visobjids );
    for ( int idx=0; idx<visobjids.size(); idx++ )
	applMgr()->visServer()->setUiObjectName( visobjids[idx], name_ );
    applMgr()->visServer()->triggerTreeUpdate();
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
    const EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !ems || !mps || !vps || !emobj ) return false;

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

    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(false) );
    mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet(true) );

    const int mnuid = mnu.exec();
    if ( mnuid == mPropID )
    {
	uiDialog dlg( getUiParent(), uiDialog::Setup(uiStrings::sProperties(),
					mNoDlgTitle,mNoHelpKey) );
	dlg.setCtrlStyle( uiDialog::CloseOnly );
	uiSelLineStyle::Setup lssu;
	lssu.drawstyle(false);
	OD::LineStyle ls = emobj->preferredLineStyle();
	ls.color_ = emobj->preferredColor();
	uiSelLineStyle* lsfld = new uiSelLineStyle( &dlg, ls, lssu );
	lsfld->changed.notify( mCB(this,uiODVw2DHor2DTreeItem,propChgCB) );
	dlg.go();
    }
    else if ( mnuid == mSaveID )
    {
	doSave();
    }
    else if ( mnuid == mSaveAsID )
    {
	doSaveAs();
    }
    else if ( mnuid == mStartID )
    {
	if ( mps->addTracker(emid_,Coord3::udf())==-1 )
	    return false;

	MPE::engine().setActiveTracker( emid_ );
	const EM::SectionID sid = emobj->sectionID( 0 );
	mps->useSavedSetupDlg( emid_, sid );
	if ( viewer2D() && viewer2D()->viewControl() )
	    viewer2D()->viewControl()->setEditMode( true );
    }
    else if ( mnuid == mSettsID )
    {
	const EM::SectionID sid = emobj->sectionID( 0 );
	mps->showSetupDlg( emid_, sid );
    }
    else if ( isRemoveItem(mnuid,false) || isRemoveItem(mnuid,true) )
    {
	if ( !ems->askUserToSave(emid_,true) )
	    return true;

	const int trackerid = mps->getTrackerID( emid_ );
	if ( trackerid>= 0 )
	    renameVisObj();
	name_ = mToUiStringTodo(applMgr()->EMServer()->getName( emid_ ));
	bool doremove = !applMgr()->viewer2DMgr().isItemPresent( parent_ ) ||
		isRemoveItem(mnuid,false);
	if ( isRemoveItem(mnuid,true) )
	    applMgr()->viewer2DMgr().removeHorizon2D( emid_ );
	if ( doremove )
	    parent_->removeChild( this );
    }

    return true;
}


void uiODVw2DHor2DTreeItem::propChgCB( CallBacker* cb )
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    mDynamicCastGet(uiColorInput*,colfld,cb)
    if ( colfld )
    {
	emobj->setPreferredColor( colfld->color() );
	return;
    }

    OD::LineStyle ls = emobj->preferredLineStyle();
    mDynamicCastGet(uiSpinBox*,szfld,cb)
    if ( szfld )
    {
	ls.width_ = szfld->getIntValue();
	emobj->setPreferredLineStyle( ls );
    }
}


bool uiODVw2DHor2DTreeItem::select()
{
    if ( uitreeviewitem_->treeView() )
	uitreeviewitem_->treeView()->deselectAll();

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
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    if ( emid != emid_ ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj);
    if ( !hor2d ) return;

    if ( emid != emid_ ) return;

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
