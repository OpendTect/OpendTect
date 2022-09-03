/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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


uiODView2DPickSetParentTreeItem::uiODView2DPickSetParentTreeItem()
    : uiODView2DTreeItem( uiStrings::sPointSet() )
    , picksetmgr_(Pick::Mgr())
{
}


uiODView2DPickSetParentTreeItem::~uiODView2DPickSetParentTreeItem()
{
}


const char* uiODView2DPickSetParentTreeItem::iconName() const
{ return "tree-pickset"; }


bool uiODView2DPickSetParentTreeItem::init()
{ return uiODView2DTreeItem::init(); }



bool uiODView2DPickSetParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), 0 );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sNew())), 1 );
    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODView2DPickSetParentTreeItem::handleSubMenu( int menuid )
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
	    mDynamicCastGet(uiODView2DPickSetTreeItem*,picktreeitem,
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


void uiODView2DPickSetParentTreeItem::getPickSetVwr2DIDs(
	const MultiID& mid, TypeSet<Vis2DID>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODView2DPickSetTreeItem*,picktreeitem,
			getChild(idx))
	if ( !picktreeitem || picktreeitem->pickMultiID() != mid )
	    continue;

	vw2dobjids.addIfNew( picktreeitem->vw2DObject()->id() );
    }
}


void uiODView2DPickSetParentTreeItem::removePickSet( const MultiID& mid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DPickSetTreeItem*,pickitm,getChild(idx))
	if ( !pickitm || mid!=pickitm->pickMultiID() )
	    continue;

	removeChild( pickitm );
    }
}


void uiODView2DPickSetParentTreeItem::getLoadedPickSets(
	TypeSet<MultiID>& picks ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODView2DPickSetTreeItem*,pickitm,getChild(idx))
	if ( !pickitm )
	    continue;

	picks.addIfNew( pickitm->pickMultiID() );
    }
}


void uiODView2DPickSetParentTreeItem::setupNewPickSet(
	const MultiID& pickid )
{
    TypeSet<MultiID> pickidsloaded;
    getLoadedPickSets( pickidsloaded );
    if ( !pickidsloaded.isPresent(pickid) )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DPickSetTreeItem*,picktreeitm,getChild(idx))
	if ( picktreeitm && pickid==picktreeitm->pickMultiID() )
	{
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    picktreeitm->select();
	}
    }

}


void uiODView2DPickSetParentTreeItem::addPickSets(
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
	if ( findChild(ps->name().buf()) )
	    continue;

	uiODView2DPickSetTreeItem* childitm =
	    new uiODView2DPickSetTreeItem( picksetidx );
	addChld( childitm, false, false);
    }
}


uiODView2DPickSetTreeItem::uiODView2DPickSetTreeItem( int picksetid )
    : uiODView2DTreeItem(uiString::emptyString())
    , picksetmgr_(Pick::Mgr())
    , pickset_(Pick::Mgr().get(picksetid))
{
    mAttachCB( picksetmgr_.setToBeRemoved,
	       uiODView2DPickSetTreeItem::removePickSetCB );
    mAttachCB( picksetmgr_.setDispChanged,
	       uiODView2DPickSetTreeItem::displayChangedCB );
}


uiODView2DPickSetTreeItem::uiODView2DPickSetTreeItem( Vis2DID id, bool )
    : uiODView2DTreeItem(uiString::emptyString())
    , picksetmgr_(Pick::Mgr())
    , pickset_(applMgr()->pickServer()->createEmptySet(false))
{
    displayid_ = id;
    mAttachCB( picksetmgr_.setToBeRemoved,
	       uiODView2DPickSetTreeItem::removePickSetCB );
    mAttachCB( picksetmgr_.setDispChanged,
	       uiODView2DPickSetTreeItem::displayChangedCB );
}


uiODView2DPickSetTreeItem::~uiODView2DPickSetTreeItem()
{
    detachAllNotifiers();
    if ( vw2dpickset_ )
	viewer2D()->dataMgr()->removeObject( vw2dpickset_ );
}


bool uiODView2DPickSetTreeItem::init()
{
    const int picksetidx = picksetmgr_.indexOf( pickset_->name().buf() );
    if ( !displayid_.isValid() )
    {
	if ( picksetidx < 0 )
	    return false;

	vw2dpickset_ = View2D::PickSet::create( viewer2D()->viewwin(),
					    viewer2D()->dataEditor() );
	vw2dpickset_->setPickSet( *pickset_ );
	viewer2D()->dataMgr()->addObject( vw2dpickset_ );
	displayid_ = vw2dpickset_->id();
    }
    else
    {
	mDynamicCastGet(View2D::PickSet*,pickdisplay,
			viewer2D()->getObject(displayid_))
	if ( !pickdisplay )
	    return false;

	pickset_ = picksetmgr_.get( pickdisplay->pickSetID() );
	vw2dpickset_ = pickdisplay;
    }

    name_ = mToUiStringTodo(pickset_->name());
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    displayMiniCtab();
    mAttachCB( checkStatusChange(), uiODView2DPickSetTreeItem::checkCB );
    vw2dpickset_->drawAll();

    for ( int ivwr = 0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(ivwr);
	mAttachCB(vwr.rgbCanvas().getKeyboardEventHandler().keyPressed,
	    uiODView2DPickSetTreeItem::keyPressedCB);
    }

    return true;
}


const MultiID& uiODView2DPickSetTreeItem::pickMultiID() const
{
    return picksetmgr_.get( *pickset_ );
}


void uiODView2DPickSetTreeItem::displayChangedCB( CallBacker* )
{
    if ( vw2dpickset_ )
	vw2dpickset_->drawAll();
    displayMiniCtab();
}


void uiODView2DPickSetTreeItem::displayMiniCtab()
{
    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				pickset_->disp_.color_ );
}


bool uiODView2DPickSetTreeItem::select()
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

bool uiODView2DPickSetTreeItem::showSubMenu()
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


void uiODView2DPickSetTreeItem::removePickSetCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( ps != pickset_ )
	return;

    if ( vw2dpickset_ )
	vw2dpickset_->clearPicks();

    parent_->removeChild( this );
}


void uiODView2DPickSetTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODView2DPickSetTreeItem::checkCB( CallBacker* )
{
    if ( vw2dpickset_ )
	vw2dpickset_->enablePainting( isChecked() );
}


void uiODView2DPickSetTreeItem::keyPressedCB( CallBacker* cb )
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


uiTreeItem* uiODView2DPickSetTreeItemFactory::createForVis(
				const uiODViewer2D& vwr2d, Vis2DID id ) const
{
    mDynamicCastGet(const View2D::PickSet*,obj,vwr2d.getObject(id));
    return obj ? new uiODView2DPickSetTreeItem(id,false) : nullptr;
}
