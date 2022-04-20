/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
________________________________________________________________________

-*/

#include "uiodvw2dpicksettreeitem.h"

#include "pickset.h"
#include "uiflatviewstdcontrol.h"
#include "uimenu.h"
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
#include "view2ddataman.h"
#include "view2dpickset.h"

#include "uivispartserv.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uigraphicsview.h"


uiODVw2DPickSetParentTreeItem::uiODVw2DPickSetParentTreeItem()
    : uiODVw2DTreeItem( uiStrings::sPointSet() )
    , picksetmgr_(Pick::Mgr())
{
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
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), 0 );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sNew())), 1 );
    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DPickSetParentTreeItem::handleSubMenu( int menuid )
{
    handleStdSubMenu( menuid );

    TypeSet<MultiID> pickmidstoadd;
    const Pick::Set* newps = 0;
    if ( menuid == 1  )
    {
	newps = applMgr()->pickServer()->createEmptySet( false );
	if ( !newps )
	    return false;

	pickmidstoadd += picksetmgr_.get( *newps );
    }
    else if ( menuid == 0 &&
	      !applMgr()->pickServer()->loadSets(pickmidstoadd,false) )
	return false;

    if ( !pickmidstoadd.isEmpty() )
	addPickSets( pickmidstoadd );

    if ( newps )
    {
	const MultiID& newpickmid = picksetmgr_.get( *newps );
	for ( int idx=0; idx<nrChildren(); idx++ )
	{
	    mDynamicCastGet(uiODVw2DPickSetTreeItem*,picktreeitem,
			    getChild(idx))
	    if ( picktreeitem && picktreeitem->pickMultiID() == newpickmid )
	    {
		viewer2D()->viewControl()->setEditMode( true );
		picktreeitem->select();
		break;
	    }

	}
    }

    return true;
}


void uiODVw2DPickSetParentTreeItem::getPickSetVwr2DIDs(
	const MultiID& mid, TypeSet<int>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DPickSetTreeItem*,picktreeitem,
			getChild(idx))
	if ( !picktreeitem || picktreeitem->pickMultiID() != mid )
	    continue;

	vw2dobjids.addIfNew( picktreeitem->vw2DObject()->id() );
    }
}


void uiODVw2DPickSetParentTreeItem::removePickSet( const MultiID& mid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DPickSetTreeItem*,pickitm,getChild(idx))
	if ( !pickitm || mid!=pickitm->pickMultiID() )
	    continue;

	removeChild( pickitm );
    }
}


void uiODVw2DPickSetParentTreeItem::getLoadedPickSets(
	TypeSet<MultiID>& picks ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DPickSetTreeItem*,pickitm,getChild(idx))
	if ( !pickitm )
	    continue;

	picks.addIfNew( pickitm->pickMultiID() );
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
	if ( picktreeitm && pickid==picktreeitm->pickMultiID() )
	{
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    picktreeitm->select();
	}
    }

}


void uiODVw2DPickSetParentTreeItem::addPickSets(
	const TypeSet<MultiID>& pickids )
{
    TypeSet<MultiID> pickidstobeloaded, pickidsloaded;
    getLoadedPickSets( pickidsloaded );
    for ( int idx=0; idx<pickids.size(); idx++ )
    {
	if ( !pickidsloaded.isPresent(pickids[idx]) )
	    pickidstobeloaded.addIfNew( pickids[idx] );
    }
    for ( int idx=0; idx<pickidstobeloaded.size(); idx++ )
    {
	const int picksetidx = picksetmgr_.indexOf( pickidstobeloaded[idx] );
	if ( picksetidx<0 )
	    continue;

	ConstRefMan<Pick::Set> ps = picksetmgr_.get( picksetidx );
	if ( findChild(ps->name()) )
	    continue;

	uiODVw2DPickSetTreeItem* childitm =
	    new uiODVw2DPickSetTreeItem( picksetidx );
	addChld( childitm, false, false);
    }
}


uiODVw2DPickSetTreeItem::uiODVw2DPickSetTreeItem( int picksetid )
    : uiODVw2DTreeItem(uiString::emptyString())
    , picksetmgr_(Pick::Mgr())
    , pickset_(Pick::Mgr().get(picksetid))
{
    mAttachCB( picksetmgr_.setToBeRemoved,
	       uiODVw2DPickSetTreeItem::removePickSetCB );
    mAttachCB( picksetmgr_.setDispChanged,
	       uiODVw2DPickSetTreeItem::displayChangedCB );
}


uiODVw2DPickSetTreeItem::uiODVw2DPickSetTreeItem( int id, bool )
    : uiODVw2DTreeItem(uiString::emptyString())
    , picksetmgr_(Pick::Mgr())
    , pickset_(applMgr()->pickServer()->createEmptySet(false))
{
    displayid_ = id;
    mAttachCB( picksetmgr_.setToBeRemoved,
	       uiODVw2DPickSetTreeItem::removePickSetCB );
    mAttachCB( picksetmgr_.setDispChanged,
	       uiODVw2DPickSetTreeItem::displayChangedCB );
}


uiODVw2DPickSetTreeItem::~uiODVw2DPickSetTreeItem()
{
    detachAllNotifiers();
    if ( vw2dpickset_ )
	viewer2D()->dataMgr()->removeObject( vw2dpickset_ );
}


bool uiODVw2DPickSetTreeItem::init()
{
    const int picksetidx = picksetmgr_.indexOf( pickset_->name() );
    if ( displayid_ < 0 )
    {
	if ( picksetidx < 0 )
	    return false;

	vw2dpickset_ =
	    VW2DPickSet::create( picksetidx, viewer2D()->viewwin(),
				 viewer2D()->dataEditor() );
	viewer2D()->dataMgr()->addObject( vw2dpickset_ );
	displayid_ = vw2dpickset_->id();
    }
    else
    {
	mDynamicCastGet(VW2DPickSet*,pickdisplay,
			viewer2D()->dataMgr()->getObject(displayid_))
	if ( !pickdisplay )
	    return false;

	pickset_ = picksetmgr_.get( pickdisplay->pickSetID() );
	vw2dpickset_ = pickdisplay;
    }

    name_ = mToUiStringTodo(pickset_->name());
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    displayMiniCtab();
    mAttachCB( checkStatusChange(), uiODVw2DPickSetTreeItem::checkCB );
    vw2dpickset_->drawAll();

    for ( int ivwr = 0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(ivwr);
	mAttachCB(vwr.rgbCanvas().getKeyboardEventHandler().keyPressed,
	    uiODVw2DPickSetTreeItem::keyPressedCB);
    }

    return true;
}


const MultiID& uiODVw2DPickSetTreeItem::pickMultiID() const
{
    return picksetmgr_.get( *pickset_ );
}


void uiODVw2DPickSetTreeItem::displayChangedCB( CallBacker* )
{
    if ( vw2dpickset_ )
	vw2dpickset_->drawAll();
    displayMiniCtab();
}


void uiODVw2DPickSetTreeItem::displayMiniCtab()
{
    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				pickset_->disp_.color_ );
}


bool uiODVw2DPickSetTreeItem::select()
{
    if ( uitreeviewitem_->treeView() )
	uitreeviewitem_->treeView()->deselectAll();

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
    const int setidx = Pick::Mgr().indexOf( *pickset_ );
    const bool haschanged = setidx < 0 || Pick::Mgr().isChanged(setidx);

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
	    uiPickPropDlg dlg( getUiParent(), *pickset_, 0 );
	    dlg.go();
	} break;
	case mDirectionID:
	    applMgr()->setPickSetDirs( *pickset_ );
	    break;
	case mSaveID:
	    applMgr()->storePickSet( *pickset_ );
	    break;
	case mSaveAsID:
	    applMgr()->storePickSetAs( *pickset_ );
	    break;
	case mRemoveID:
	{
	    const int picksetidx  = picksetmgr_.indexOf( *pickset_ );
	    if ( picksetidx>=0 )
	    {
		if ( picksetmgr_.isChanged(picksetidx) )
		{
		    const int res = uiMSG().askSave(
			tr("PointSet '%1' has been modified. "
			   "Do you want to save it?").arg(pickset_->name()) );
		    if ( res==-1 )
			return false;
		    else if ( res==1 )
			applMgr()->storePickSet( *pickset_ );
		}

		parent_->removeChild( this );
	    }
	} break;
    }

    return true;
}


void uiODVw2DPickSetTreeItem::removePickSetCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( ps != pickset_ )
	return;

    if ( vw2dpickset_ )
	vw2dpickset_->clearPicks();

    parent_->removeChild( this );
}


void uiODVw2DPickSetTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DPickSetTreeItem::checkCB( CallBacker* )
{
    if ( vw2dpickset_ )
	vw2dpickset_->enablePainting( isChecked() );
}


void uiODVw2DPickSetTreeItem::keyPressedCB( CallBacker* cb )
{
    if ( !uitreeviewitem_->isSelected() )
	return;

    mDynamicCastGet(const KeyboardEventHandler*,keh,cb)
    if ( !keh || !keh->hasEvent() ) return;

    if ( KeyboardEvent::isSave(keh->event()) )
	applMgr()->storePickSet( *pickset_ );

    if ( KeyboardEvent::isSaveAs(keh->event()) )
	applMgr()->storePickSetAs( *pickset_ );
}


uiTreeItem* uiODVw2DPickSetTreeItemFactory::createForVis(
				    const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet(const VW2DPickSet*,obj,vwr2d.dataMgr()->getObject(id));
    return obj ? new uiODVw2DPickSetTreeItem(id,false) : 0;
}
