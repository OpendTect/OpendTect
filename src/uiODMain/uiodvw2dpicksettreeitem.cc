/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
________________________________________________________________________

-*/

#include "uiodvw2dpicksettreeitem.h"

#include "picksetmanager.h"
#include "uiflatviewstdcontrol.h"
#include "uimenu.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodpicksettreeitem.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uipickpartserv.h"
#include "uipickpropdlg.h"
#include "uisetpickdirs.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uigraphicsview.h"
#include "keyboardevent.h"

#include "view2ddataman.h"
#include "view2dpickset.h"


uiODVw2DPickSetParentTreeItem::uiODVw2DPickSetParentTreeItem()
    : uiODVw2DParentTreeItem( uiStrings::sPickSet() )
{
    setObjectManager( &Pick::SetMGR() );
}


uiODVw2DPickSetParentTreeItem::~uiODVw2DPickSetParentTreeItem()
{
}


const char* uiODVw2DPickSetParentTreeItem::iconName() const
{ return "tree-pickset"; }


bool uiODVw2DPickSetParentTreeItem::init()
{ return uiODVw2DTreeItem::init(); }



bool uiODVw2DPickSetParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(m3Dots(uiStrings::sAdd())), 0 );
    mnu.insertItem( new uiAction(m3Dots(uiStrings::sNew())), 1 );
    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DPickSetParentTreeItem::handleSubMenu( int menuid )
{
    handleStdSubMenu( menuid );

    TypeSet<MultiID> setids;
    RefMan<Pick::Set> newps = 0;
    if ( menuid == 1  )
    {
	newps = applMgr()->pickServer()->createEmptySet( false );
	if ( !newps )
	    return false;

	setids += Pick::SetMGR().getID( *newps );
    }
    else if ( menuid == 0 &&
	      !applMgr()->pickServer()->loadSets(setids,false) )
	return false;

    if ( setids.isEmpty() )
	return true;

    addPickSets( setids );
    if ( newps )
	setActive( setids[0] );

    return true;
}


void uiODVw2DPickSetParentTreeItem::setActive( const MultiID& setid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DPickSetTreeItem*,picktreeitem,
			getChild(idx))
	if ( picktreeitem && picktreeitem->pickSetID() == setid )
	{
	    viewer2D()->viewControl()->setEditMode( true );
	    picktreeitem->select();
	    break;
	}
    }
}


void uiODVw2DPickSetParentTreeItem::objAddedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( MultiID,psid,cber );
    mEnsureExecutedInMainThreadWithCapsule(
	    uiODVw2DPickSetParentTreeItem::objAddedCB, cbercaps )
    if ( psid.isUdf() )
	return;

    TypeSet<MultiID> setids;
    setids += psid;
    addPickSets( setids );
}


void uiODVw2DPickSetParentTreeItem::objVanishedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( MultiID,psid,cber );
    mEnsureExecutedInMainThreadWithCapsule(
	    uiODVw2DPickSetParentTreeItem::objVanishedCB, cbercaps )
    if ( psid.isUdf() )
	return;

    removePickSet( psid );
}


void uiODVw2DPickSetParentTreeItem::objShownCB( CallBacker* cber )
{
    mCBCapsuleUnpack( MultiID,psid,cber );
    mEnsureExecutedInMainThreadWithCapsule(
	    uiODVw2DPickSetParentTreeItem::objShownCB, cbercaps )
    if ( psid.isUdf() )
	return;

    showHidePickSet( psid, true );
}


void uiODVw2DPickSetParentTreeItem::objHiddenCB( CallBacker* cber )
{
    mCBCapsuleUnpack( MultiID,psid,cber );
    mEnsureExecutedInMainThreadWithCapsule(
	    uiODVw2DPickSetParentTreeItem::objHiddenCB, cbercaps )
    if ( psid.isUdf() )
	return;

    showHidePickSet( psid, false );
}


void uiODVw2DPickSetParentTreeItem::objOrphanedCB( CallBacker* cber )
{
    mCBCapsuleUnpack( MultiID,psid,cber );
    mEnsureExecutedInMainThreadWithCapsule(
	    uiODVw2DPickSetParentTreeItem::objOrphanedCB, cbercaps )
    if ( psid.isUdf() )
	return;
    //TODO do something when we have clearer idea what to do when it happens
}


void uiODVw2DPickSetParentTreeItem::getPickSetVwr2DIDs(
	const MultiID& mid, TypeSet<int>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DPickSetTreeItem*,picktreeitem,
			getChild(idx))
	if ( !picktreeitem || picktreeitem->pickSetID() != mid )
	    continue;

	vw2dobjids.addIfNew( picktreeitem->vw2DObject()->id() );
    }
}


void uiODVw2DPickSetParentTreeItem::showHidePickSet( const MultiID& mid,
						     bool show )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DPickSetTreeItem*,pickitm,getChild(idx))
	if ( !pickitm || mid!=pickitm->pickSetID() )
	    continue;

	pickitm->setChecked( show, true );
    }
}


void uiODVw2DPickSetParentTreeItem::removePickSet( const MultiID& mid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DPickSetTreeItem*,pickitm,getChild(idx))
	if ( !pickitm || mid!=pickitm->pickSetID() )
	    continue;

	removeChild( pickitm );
    }
}


void uiODVw2DPickSetParentTreeItem::getLoadedPickSets(
	TypeSet<MultiID>& psids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DPickSetTreeItem*,pickitm,getChild(idx))
	if ( !pickitm )
	    continue;

	psids.addIfNew( pickitm->pickSetID() );
    }
}


void uiODVw2DPickSetParentTreeItem::setupNewPickSet(
	const MultiID& pickid )
{
    TypeSet<MultiID> pickidsloaded;
    getLoadedPickSets( pickidsloaded );
    if ( !pickidsloaded.isPresent(pickid) )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DPickSetTreeItem*,picktreeitm,getChild(idx))
	if ( picktreeitm && pickid==picktreeitm->pickSetID() )
	{
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    picktreeitm->select();
	    break;
	}
    }

}


void uiODVw2DPickSetParentTreeItem::addPickSets(
	const TypeSet<MultiID>& setids )
{
    TypeSet<MultiID> setidstobeloaded, setidsloaded;
    getLoadedPickSets( setidsloaded );
    for ( int idx=0; idx<setids.size(); idx++ )
    {
	if ( !setidsloaded.isPresent(setids[idx]) )
	    setidstobeloaded.addIfNew( setids[idx] );
    }

    for ( int idx=0; idx<setidstobeloaded.size(); idx++ )
    {
	RefMan<Pick::Set> ps = Pick::SetMGR().fetchForEdit(
						setidstobeloaded[idx] );
	if ( !ps )
	    continue;

	uiODVw2DPickSetTreeItem* childitm =
			    new uiODVw2DPickSetTreeItem( *ps );
	addChld( childitm, false, false);
    }
}


uiODVw2DPickSetTreeItem::uiODVw2DPickSetTreeItem( Pick::Set& ps )
    : uiODVw2DTreeItem(uiString::emptyString())
    , pickset_(ps)
    , vw2dpickset_(0)
{
    pickset_.ref();
}


bool uiODVw2DPickSetTreeItem::init()
{
    const MultiID setid = Pick::SetMGR().getID( pickset_ );
    vw2dpickset_ =
	VW2DPickSet::create( setid.leafID(), viewer2D()->viewwin(),
			     viewer2D()->dataEditor() );
    if ( !vw2dpickset_ )
	{ pErrMsg("Factory returns null"); return false; }

    viewer2D()->dataMgr()->addObject( vw2dpickset_ );
    displayid_ = vw2dpickset_->id();

    name_ = toUiString( pickset_.name() );
    uitreeviewitem_->setCheckable( true );
    uitreeviewitem_->setChecked( true );
    displayMiniCtab();
    vw2dpickset_->drawAll();

    addKeyBoardEvent();
    mAttachCB( checkStatusChange(), uiODVw2DPickSetTreeItem::checkCB );
    mAttachCB( pickset_.objectChanged(),
	       uiODVw2DPickSetTreeItem::setChangedCB );
    return true;
}


uiODVw2DPickSetTreeItem::~uiODVw2DPickSetTreeItem()
{
    detachAllNotifiers();
    if ( vw2dpickset_ )
	viewer2D()->dataMgr()->removeObject( vw2dpickset_ );
    pickset_.unRef();
}


MultiID uiODVw2DPickSetTreeItem::pickSetID() const
{
    return Pick::SetMGR().getID( pickset_ );
}


void uiODVw2DPickSetTreeItem::setChangedCB( CallBacker* )
{
    displayMiniCtab();
}


void uiODVw2DPickSetTreeItem::displayMiniCtab()
{
    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				pickset_.dispColor() );
}


bool uiODVw2DPickSetTreeItem::select()
{
    if ( uitreeviewitem_->treeView() )
	uitreeviewitem_->treeView()->clearSelection();

    uitreeviewitem_->setSelected( true );
    if ( vw2dpickset_ )
    {
	viewer2D()->dataMgr()->setSelected( vw2dpickset_ );
	vw2dpickset_->selected();
    }
    return true;
}

#define mPropID		0
#define mSaveID		1
#define mSaveAsID	2
#define mRemoveID	3
#define mDirectionID	4

bool uiODVw2DPickSetTreeItem::showSubMenu()
{
    const bool haschanged = Pick::SetMGR().needsSave( pickset_ );

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    addAction( mnu, m3Dots(uiStrings::sProperties()), mPropID, "disppars" );
    addAction( mnu, m3Dots(tr("Set Directions")), mDirectionID );
    addAction( mnu, uiStrings::sSave(), mSaveID, "save", haschanged );
    addAction( mnu, m3Dots(uiStrings::sSaveAs()), mSaveAsID, "saveas", true );
    addAction( mnu, uiStrings::sRemove(), mRemoveID, "remove" );

    const int mnuid = mnu.exec();
    switch ( mnuid )
    {
	case mPropID:
	{
	    uiPickPropDlg dlg( getUiParent(), pickset_, 0 );
	    dlg.go();
	} break;
	case mDirectionID:
	    applMgr()->setPickSetDirs( pickset_ );
	    break;
	case mSaveID:
	    doSave();
	    break;
	case mSaveAsID:
	    doSaveAs();
	    break;
	case mRemoveID:
	{
	    if ( Pick::SetMGR().needsSave(pickset_) )
	    {
		const int res = uiMSG().askSave(
		    tr("PickSet '%1' has been modified. "
		       "Do you want to save it?").arg( pickset_.name() ) );
		if ( res < 0 )
		    return false;
		else if ( res == 1 )
		    doSave();
	    }

	    parent_->removeChild( this );
	} break;
    }

    return true;
}


void uiODVw2DPickSetTreeItem::doSave()
{
    applMgr()->storePickSet( pickset_ );
}


void uiODVw2DPickSetTreeItem::doSaveAs()
{
    applMgr()->storePickSetAs( pickset_ );
}


void uiODVw2DPickSetTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DPickSetTreeItem::enableDisplay( bool yn )
{
    if ( vw2dpickset_ )
	vw2dpickset_->enablePainting( yn );
}


void uiODVw2DPickSetTreeItem::checkCB( CallBacker* )
{
    enableDisplay( isChecked() );
}


uiTreeItem* uiODVw2DPickSetTreeItemFactory::createForVis(
				    const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet( const VW2DPickSet*, vw2dps,
			vwr2d.dataMgr()->getObject(id) );
    if ( !vw2dps )
	return 0;

    RefMan<Pick::Set> ps = Pick::SetMGR().fetchForEdit( vw2dps->pickSetID() );
    if ( !ps )
	{ pErrMsg("Refcount should prevent this"); return 0; }

    return new uiODVw2DPickSetTreeItem( *ps );
}
