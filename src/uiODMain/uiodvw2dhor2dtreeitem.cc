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

uiODView2DHor2DParentTreeItem::uiODView2DHor2DParentTreeItem()
    : uiODView2DTreeItem( tr("2D Horizon") )
{
}


uiODView2DHor2DParentTreeItem::~uiODView2DHor2DParentTreeItem()
{
}


void uiODView2DHor2DParentTreeItem::getNonLoadedTrackedHor2Ds(
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


bool uiODView2DHor2DParentTreeItem::showSubMenu()
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


bool uiODView2DHor2DParentTreeItem::handleSubMenu( int mnuid )
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
	EM::ObjectID emid;
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
	applMgr()->EMServer()->selectHorizons( objs, true, getUiParent() );
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


void uiODView2DHor2DParentTreeItem::getHor2DVwr2DIDs(
	EM::ObjectID emid, TypeSet<Vis2DID>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODView2DHor2DTreeItem*,
			hor2dtreeitm,getChild(idx))
	if ( !hor2dtreeitm || hor2dtreeitm->emObjectID() != emid )
	    continue;

	vw2dobjids.addIfNew( hor2dtreeitm->vw2DObject()->id() );
    }
}


void uiODView2DHor2DParentTreeItem::getLoadedHorizon2Ds(
	TypeSet<EM::ObjectID>& emids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODView2DHor2DTreeItem*,
			hor2dtreeitm,getChild(idx))
	if ( !hor2dtreeitm )
	    continue;
	emids.addIfNew( hor2dtreeitm->emObjectID() );
    }
}


void uiODView2DHor2DParentTreeItem::removeHorizon2D( EM::ObjectID emid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor2DTreeItem*,hor2dtreeitm,getChild(idx))
	if ( !hor2dtreeitm || emid!=hor2dtreeitm->emObjectID() )
	    continue;
	removeChild( hor2dtreeitm );
    }
}


void uiODView2DHor2DParentTreeItem::addHorizon2Ds(
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

	uiODView2DHor2DTreeItem* childitem =
	    new uiODView2DHor2DTreeItem( emidstobeloaded[idx] );
	addChld( childitem, false, false );
    }
}


void uiODView2DHor2DParentTreeItem::setupTrackingHorizon2D( EM::ObjectID emid )
{
    TypeSet<EM::ObjectID> emidsloaded;
    getLoadedHorizon2Ds( emidsloaded );
    if ( !emidsloaded.isPresent(emid) )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor2DTreeItem*,hor2dtreeitm,getChild(idx))
	if ( hor2dtreeitm && emid==hor2dtreeitm->emObjectID() )
	{
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    hor2dtreeitm->select();
	}
    }

}


void uiODView2DHor2DParentTreeItem::addNewTrackingHorizon2D( EM::ObjectID emid )
{
    TypeSet<EM::ObjectID> emidsloaded;
    getLoadedHorizon2Ds( emidsloaded );
    if ( emidsloaded.isPresent(emid) )
	return;

    uiODView2DHor2DTreeItem* hortreeitem = new uiODView2DHor2DTreeItem( emid );
    const int trackid = applMgr()->mpeServer()->getTrackerID( emid );
    if ( trackid>=0 )
	MPE::engine().getEditor( emid, true );

    addChld( hortreeitem, false, false );
    if ( viewer2D() && viewer2D()->viewControl() )
	viewer2D()->viewControl()->setEditMode( true );

    hortreeitem->select();
}


const char* uiODView2DHor2DParentTreeItem::iconName() const
{ return "tree-horizon2d"; }


bool uiODView2DHor2DParentTreeItem::init()
{ return uiODView2DTreeItem::init(); }



uiODView2DHor2DTreeItem::uiODView2DHor2DTreeItem( const EM::ObjectID& emid )
    : uiODView2DTreeItem(uiString::emptyString())
    , emid_(emid)
    , horview_(0)
    , trackerefed_(false)
{
    if ( MPE::engine().hasTracker(emid_) )
	MPE::engine().refTracker( emid_ );
}


uiODView2DHor2DTreeItem::uiODView2DHor2DTreeItem( Vis2DID id, bool )
    : uiODView2DTreeItem(uiString::emptyString())
    , horview_(0)
    , trackerefed_(false)
{
    displayid_ = id;
}



uiODView2DHor2DTreeItem::~uiODView2DHor2DTreeItem()
{
     detachAllNotifiers();

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,uiODView2DHor2DTreeItem,emobjChangeCB) );

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


bool uiODView2DHor2DTreeItem::init()
{
    const Line2DInterSectionSet* intersectionset =
			ODMainWin()->viewer2DMgr().getLine2DInterSectionSet();
    EM::EMObject* emobj = nullptr;
    if ( !displayid_.isValid() )
    {
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	horview_ = View2D::Horizon2D::create( viewer2D()->viewwin(),
					  viewer2D()->dataEditor() );
	horview_->setEMObjectID( emid_ );
	viewer2D()->dataMgr()->addObject( horview_ );
	displayid_ = horview_->id();
    }
    else
    {
	mDynamicCastGet(View2D::Horizon2D*,hd,
			    viewer2D()->getObject(displayid_))
	if ( !hd )
	    return false;
	emid_ = hd->getEMObjectID();
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj )
	    return false;

	horview_ = hd;
    }

     if ( emobj )
	mAttachCB( emobj->change, uiODView2DHor2DTreeItem::emobjChangeCB );

    displayMiniCtab();

    name_ = applMgr()->EMServer()->getUiName( emid_ );
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODView2DHor2DTreeItem,checkCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	mAttachCB( vwr.rgbCanvas().scene().getMouseEventHandler().buttonPressed,
	    uiODView2DHor2DTreeItem::mouseReleaseInVwrCB );

	mAttachCB( vwr.rgbCanvas().scene().getMouseEventHandler().buttonPressed,
	uiODView2DHor2DTreeItem::mousePressInVwrCB );
    }

    horview_->setSelSpec( &viewer2D()->selSpec(true), true );
    horview_->setSelSpec( &viewer2D()->selSpec(false), false );

    if ( viewer2D()->geomID() != Survey::GeometryManager::cUndefGeomID() )
	horview_->setGeomID( viewer2D()->geomID() );

    horview_->setLine2DInterSectionSet( intersectionset );

    horview_->draw();

    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	mAttachCB( deselnotify, uiODView2DHor2DTreeItem::deSelCB );
    uiODView2DTreeItem::addKeyBoardEvent( emid_ );
    return true;
}


void uiODView2DHor2DTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				emobj->preferredColor() );
}


void uiODView2DHor2DTreeItem::emobjChangeCB( CallBacker* cb )
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


#define mPropID		0
#define mStartID	1
#define mSettsID	2
#define mSaveID		3
#define mSaveAsID	4

bool uiODView2DHor2DTreeItem::showSubMenu()
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
	lsfld->changed.notify( mCB(this,uiODView2DHor2DTreeItem,propChgCB) );
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
	mps->useSavedSetupDlg( emid_ );
	if ( viewer2D() && viewer2D()->viewControl() )
	    viewer2D()->viewControl()->setEditMode( true );
    }
    else if ( mnuid == mSettsID )
    {
	mps->showSetupDlg( emid_ );
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


void uiODView2DHor2DTreeItem::propChgCB( CallBacker* cb )
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


bool uiODView2DHor2DTreeItem::select()
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


void uiODView2DHor2DTreeItem::deSelCB( CallBacker* )
{
    const int trackeridx =
	MPE::engine().getTrackerByObject( emid_ );
    applMgr()->mpeServer()->enableTracking( trackeridx, false );

    //TODO handle on/off MOUSEEVENT
}


void uiODView2DHor2DTreeItem::checkCB( CallBacker* )
{
    if ( horview_ )
	horview_->enablePainting( isChecked() );
}


void uiODView2DHor2DTreeItem::updateSelSpec( const Attrib::SelSpec* selspec,
					   bool wva )
{
    if ( horview_ )
	horview_->setSelSpec( selspec, wva );
}


void uiODView2DHor2DTreeItem::emobjAbtToDelCB( CallBacker* cb )
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


void uiODView2DHor2DTreeItem::mousePressInVwrCB( CallBacker* )
{
    if ( !uitreeviewitem_->isSelected() || !horview_ )
	return;

    horview_->setSeedPicking( applMgr()->visServer()->isPicking() );
    horview_->setTrackerSetupActive(
			applMgr()->visServer()->isTrackingSetupActive() );
}


void uiODView2DHor2DTreeItem::mouseReleaseInVwrCB( CallBacker* )
{
}


uiTreeItem* uiODView2DHor2DTreeItemFactory::createForVis(
				const uiODViewer2D& vwr2d, Vis2DID id ) const
{
    mDynamicCastGet(const View2D::Horizon2D*,obj,vwr2d.getObject(id));
    return obj ? new uiODView2DHor2DTreeItem(id,true) : nullptr;
}
