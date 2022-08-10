/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "uiodvw2dhor3dtreeitem.h"

#include "uiattribpartserv.h"
#include "uicolor.h"
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
#include "uisellinest.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emtracker.h"
#include "ioman.h"
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
	TypeSet<EM::ObjectID>& emids )
{
    const int nrtracker = MPE::engine().nrTrackersAlive();
    TypeSet<EM::ObjectID> loadedemids;
    getLoadedHorizon3Ds( loadedemids );
    for ( int idx=0; idx<nrtracker; idx++ )
    {
	MPE::EMTracker* tracker = MPE::engine().getTracker( idx );
	if ( !tracker )
	    continue;

	EM::EMObject* emobj = tracker->emObject();
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

    TypeSet<EM::ObjectID> emids;
    getNonLoadedTrackedHor3Ds( emids );
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


bool uiODVw2DHor3DParentTreeItem::handleSubMenu( int mnuid )
{
    handleStdSubMenu( mnuid );

    if ( mnuid >= mNewIdx )
    {
	TypeSet<EM::ObjectID> emids;
	getNonLoadedTrackedHor3Ds( emids );
	const int emidx = mnuid - mNewIdx - 1;
	if ( emidx >= emids.size() )
	    return false;

	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet(
		applMgr()->attrServer()->curDescSet(false) );
	EM::ObjectID emid;
	if ( emids.validIdx(emidx) )
	    emid = emids[emidx];

	EM::EMObject* emobj = EM::EMM().getObject( emid );
	if ( !emobj )
	{
	// This will add the 3D scene item.
	    const bool res = mps->addTracker( EM::Horizon3D::typeStr(),
					      viewer2D()->getSyncSceneID() );
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
	applMgr()->viewer2DMgr().addNewTrackingHorizon3D(
		emid, viewer2D()->getSyncSceneID() );
	mps->enableTracking( trackid, true );
    }
    else if ( isAddItem(mnuid,true) || isAddItem(mnuid,false) )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectHorizons( objs, false, getUiParent() );
	TypeSet<EM::ObjectID> emids;
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
	EM::ObjectID emid, TypeSet<int>& vw2dobjids ) const
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
    TypeSet<EM::ObjectID> emidstobeloaded, emidsloaded;
    getLoadedHorizon3Ds( emidsloaded );
    for ( int idx=0; idx<emids.size(); idx++ )
    {
	if ( !emidsloaded.isPresent(emids[idx]) )
	    emidstobeloaded.addIfNew( emids[idx] );
    }

    for ( int idx=0; idx<emidstobeloaded.size(); idx++ )
    {
	const EM::ObjectID emid = emidstobeloaded[idx];
	const bool hastracker = MPE::engine().hasTracker( emid );
	if ( hastracker )
	{
	    EM::EMObject* emobj = EM::EMM().getObject( emid );
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

void uiODVw2DHor3DParentTreeItem::setupTrackingHorizon3D( EM::ObjectID emid )
{
    if ( viewer2D() && !viewer2D()->isVertical() &&
	 !viewer2D()->hasZAxisTransform() )
	return;

    TypeSet<EM::ObjectID> emidsloaded;
    getLoadedHorizon3Ds( emidsloaded );
    if ( !emidsloaded.isPresent(emid) )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor3DTreeItem*,hor3dtreeitm,getChild(idx))
	if ( hor3dtreeitm && emid==hor3dtreeitm->emObjectID() )
	{
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    hor3dtreeitm->select();
	    break;
	}
    }

}


void uiODVw2DHor3DParentTreeItem::addNewTrackingHorizon3D( EM::ObjectID emid )
{
    if ( viewer2D() && !viewer2D()->isVertical() &&
	 !viewer2D()->hasZAxisTransform() )
	return;

    TypeSet<EM::ObjectID> emidsloaded;
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



uiODVw2DHor3DTreeItem::uiODVw2DHor3DTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(uiString::emptyString())
    , emid_(emid)
    , horview_(0)
    , oldactivevolupdated_(false)
    , trackerefed_(false)
{
    if ( MPE::engine().hasTracker(emid_) )
	MPE::engine().refTracker( emid_ );
}


uiODVw2DHor3DTreeItem::uiODVw2DHor3DTreeItem( int id, bool )
    : uiODVw2DTreeItem(uiString::emptyString())
    , emid_(-1)
    , horview_(0)
    , oldactivevolupdated_(false)
    , trackerefed_(false)
{
    displayid_ = id;
}


uiODVw2DHor3DTreeItem::~uiODVw2DHor3DTreeItem()
{
    detachAllNotifiers();

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,uiODVw2DHor3DTreeItem,emobjChangeCB) );

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


bool uiODVw2DHor3DTreeItem::init()
{
    EM::EMObject* emobj = 0;
    if ( displayid_ < 0 )
    {
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	horview_ = View2D::Horizon3D::create( viewer2D()->viewwin(),
					  viewer2D()->dataEditor() );
	horview_->setEMObjectID( emid_ );
	viewer2D()->dataMgr()->addObject( horview_ );
	displayid_ = horview_->id();
    }
    else
    {
	mDynamicCastGet(View2D::Horizon3D*,hd,
		viewer2D()->getObject(displayid_))
	if ( !hd )
	    return false;
	emid_ = hd->getEMObjectID();
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	horview_ = hd;
    }

    if ( emobj )
	mAttachCB( emobj->change, uiODVw2DHor3DTreeItem::emobjChangeCB );

    displayMiniCtab();

    name_ = mToUiStringTodo(applMgr()->EMServer()->getName( emid_ ));
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
    uiODVw2DTreeItem::addKeyBoardEvent( emid_ );

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

bool uiODVw2DHor3DTreeItem::showSubMenu()
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
	lsfld->changed.notify( mCB(this,uiODVw2DHor3DTreeItem,propChgCB) );
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

	name_ = ems->getUiName( emid_ );
	const bool doremove = !applMgr()->viewer2DMgr().isItemPresent( parent_ )
			|| isRemoveItem(mnuid,false);
	if ( isRemoveItem(mnuid,true) )
	    applMgr()->viewer2DMgr().removeHorizon3D( emid_ );

	if ( doremove )
	    parent_->removeChild( this );
    }

    return true;
}


void uiODVw2DHor3DTreeItem::propChgCB( CallBacker* cb )
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


void uiODVw2DHor3DTreeItem::checkCB( CallBacker* )
{
    if ( horview_ )
	horview_->enablePainting( isChecked() );
}


bool uiODVw2DHor3DTreeItem::select()
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
    mDynamicCastGet(const View2D::Horizon3D*,obj,vwr2d.getObject(id));
    return obj ? new uiODVw2DHor3DTreeItem(id,true) : 0;
}
