/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodvw2dpicksettreeitem.h"

#include "pickset.h"
#include "uimenu.h"
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


uiODVw2DPickSetParentTreeItem::uiODVw2DPickSetParentTreeItem()
    : uiODVw2DTreeItem( "PickSet" )
    , picksetmgr_(Pick::Mgr())
{
    picksetmgr_.setAdded.notify(
	mCB(this,uiODVw2DPickSetParentTreeItem,pickSetAdded) );
}


uiODVw2DPickSetParentTreeItem::~uiODVw2DPickSetParentTreeItem()
{
    picksetmgr_.setAdded.remove(
	mCB(this,uiODVw2DPickSetParentTreeItem,pickSetAdded) );
}


bool uiODVw2DPickSetParentTreeItem::init()
{ return true; }


bool uiODVw2DPickSetParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sNew(false)), 0 );
    mnu.insertItem( new uiAction(uiStrings::sLoad(false)), 1 );
    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DPickSetParentTreeItem::handleSubMenu( int menuid )
{
    handleStdSubMenu( menuid );

    if ( menuid == 0  )
	return applMgr()->pickServer()->createEmptySet( false );
    else if ( menuid == 1 )
    {
	TypeSet<MultiID> mids;
	if ( !applMgr()->pickServer()->loadSets(mids,false) )
	    return false;

	for ( int idx=0; idx<mids.size(); idx++ )
	{
	    const int index = picksetmgr_.indexOf( mids[idx] );
	    if ( index < 0 ) continue;
	    Pick::Set& ps = picksetmgr_.get( index );
	    pickSetAdded( &ps );
	}
    }

    return true;
}


void uiODVw2DPickSetParentTreeItem::pickSetAdded( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb);
    if ( !ps || findChild(ps->name()) )
	return;

    const int picksetidx = picksetmgr_.indexOf(*ps);
    addChld( new uiODVw2DPickSetTreeItem(picksetidx), false, false);
}


uiODVw2DPickSetTreeItem::uiODVw2DPickSetTreeItem( int picksetid )
    : uiODVw2DTreeItem(0)
    , pickset_(Pick::Mgr().get(picksetid))
    , picksetmgr_(Pick::Mgr())
    , vw2dpickset_(0)
    , setidx_(-1)
{
    mAttachCB( picksetmgr_.setToBeRemoved,
	       uiODVw2DPickSetTreeItem::removePickSetCB );
    mAttachCB( picksetmgr_.setDispChanged,
	       uiODVw2DPickSetTreeItem::displayChangedCB );
}


uiODVw2DPickSetTreeItem::~uiODVw2DPickSetTreeItem()
{
    detachAllNotifiers();
    viewer2D()->dataMgr()->removeObject( vw2dpickset_ );
}


bool uiODVw2DPickSetTreeItem::init()
{
    name_ = pickset_.name();
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    displayMiniCtab();
    mAttachCB( checkStatusChange(), uiODVw2DPickSetTreeItem::checkCB );
    if ( !vw2dpickset_ )
    {
	vw2dpickset_ = VW2DPickSet::create( picksetmgr_.indexOf(pickset_),
			    viewer2D()->viewwin(),viewer2D()->dataEditor() );
	viewer2D()->dataMgr()->addObject( vw2dpickset_ );
    }

    vw2dpickset_->drawAll();
    return true;
}


void uiODVw2DPickSetTreeItem::displayChangedCB( CallBacker* )
{
    vw2dpickset_->drawAll();
    displayMiniCtab();
}


void uiODVw2DPickSetTreeItem::displayMiniCtab()
{
    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				pickset_.disp_.color_ );
}


bool uiODVw2DPickSetTreeItem::select()
{
    if ( !uitreeviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( vw2dpickset_ );
    vw2dpickset_->selected();
    return true;
}


bool uiODVw2DPickSetTreeItem::showSubMenu()
{
    const int setidx = Pick::Mgr().indexOf( pickset_ );
    const bool changed = setidx < 0 || Pick::Mgr().isChanged(setidx);

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction( uiStrings::sProperties( false )), 0 );
    mnu.insertItem( new uiAction(tr("Set &direction ...")), 1 );
    uiAction* saveitm = new uiAction( uiStrings::sSave(false) );
    mnu.insertItem( saveitm, 2 );
    saveitm->setEnabled( changed );
    mnu.insertItem( new uiAction( uiStrings::sSaveAs(true) ), 3 );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true) ), 4 );

    const int mnuid = mnu.exec();
    switch ( mnuid )
    {
	case 0:{
	    uiPickPropDlg dlg( getUiParent(), pickset_, 0 );
	    dlg.go();
	    } break;
	case 1:
	    applMgr()->setPickSetDirs( pickset_ );
	    break;
	case 2:
	    applMgr()->storePickSet( pickset_ );
	    break;
	case 3:
	    applMgr()->storePickSetAs( pickset_ );
	    break;
	case 4:
	    setidx_ = picksetmgr_.indexOf( pickset_ );
	    if ( setidx_ >= 0 )
		picksetmgr_.set( picksetmgr_.id(setidx_), 0 );
	    break;
    }

    return true;
}


void uiODVw2DPickSetTreeItem::removePickSetCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( ps != &pickset_ )
	return;

    vw2dpickset_->clearPicks();
    parent_->removeChild( this );
}


void uiODVw2DPickSetTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DPickSetTreeItem::checkCB( CallBacker* )
{
    vw2dpickset_->enablePainting( isChecked() );
}


uiTreeItem* uiODVw2DPickSetTreeItemFactory::createForVis(
				    const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet(const VW2DPickSet*,obj,vwr2d.dataMgr()->getObject(id));
    return obj ? new uiODVw2DPickSetTreeItem(id) : 0;
}

