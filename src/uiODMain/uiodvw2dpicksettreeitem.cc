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
    : uiODVw2DTreeItem( uiStrings::sPickSet() )
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
    mnu.insertItem( new uiAction(m3Dots(uiStrings::sNew())), 0 );
    mnu.insertItem( new uiAction(m3Dots(uiStrings::sAdd())), 1 );
    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DPickSetParentTreeItem::handleSubMenu( int menuid )
{
    handleStdSubMenu( menuid );

    TypeSet<MultiID> pickmidstoadd;
    if ( menuid == 0  )
    {
	const Pick::Set* newps =
	    applMgr()->pickServer()->createEmptySet( false );
	if ( !newps )
	    return false;

	pickmidstoadd += picksetmgr_.get( *newps );
    }
    else if ( menuid == 1 &&
	      !applMgr()->pickServer()->loadSets(pickmidstoadd,false) )
	return false;

    if ( !pickmidstoadd.isEmpty() )
	addPickSets( pickmidstoadd );

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

	const Pick::Set& ps = picksetmgr_.get( picksetidx );
	if ( findChild(ps.name()) )
	    continue;

	addChld( new uiODVw2DPickSetTreeItem(picksetidx), false, false);
    }
}


uiODVw2DPickSetTreeItem::uiODVw2DPickSetTreeItem( int picksetid )
    : uiODVw2DTreeItem(uiString::emptyString())
    , pickset_(* new Pick::Set(Pick::Mgr().get(picksetid)))
    , picksetmgr_(Pick::Mgr())
    , vw2dpickset_(0)
{
    mAttachCB( picksetmgr_.setToBeRemoved,
	       uiODVw2DPickSetTreeItem::removePickSetCB );
    mAttachCB( picksetmgr_.setDispChanged,
	       uiODVw2DPickSetTreeItem::displayChangedCB );
}


uiODVw2DPickSetTreeItem::uiODVw2DPickSetTreeItem( int id, bool )
    : uiODVw2DTreeItem(uiString::emptyString())
    , pickset_( *new Pick::Set(""))
    , picksetmgr_(Pick::Mgr())
    , vw2dpickset_(0)
{
    displayid_ = id;
}


uiODVw2DPickSetTreeItem::~uiODVw2DPickSetTreeItem()
{
    detachAllNotifiers();
    delete &pickset_;
    if ( vw2dpickset_ )
	viewer2D()->dataMgr()->removeObject( vw2dpickset_ );
}


bool uiODVw2DPickSetTreeItem::init()
{
    const int picksetidx = picksetmgr_.indexOf( pickset_.name() );
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
	delete &pickset_;
	pickset_ = *new Pick::Set();
	mDynamicCastGet(VW2DPickSet*,pickdisplay,
			viewer2D()->dataMgr()->getObject(displayid_))
	if ( !pickdisplay )
	    return false;

	pickset_ = picksetmgr_.get( pickdisplay->pickSetID() );
	vw2dpickset_ = pickdisplay;
    }

    name_ = mToUiStringTodo(pickset_.name());
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    displayMiniCtab();
    mAttachCB( checkStatusChange(), uiODVw2DPickSetTreeItem::checkCB );
    vw2dpickset_->drawAll();
    return true;
}


const MultiID& uiODVw2DPickSetTreeItem::pickMultiID() const
{
    return picksetmgr_.get( pickset_ );
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
				pickset_.disp_.color_ );
}


bool uiODVw2DPickSetTreeItem::select()
{
    if ( !uitreeviewitem_->isSelected() )
	return false;

    if ( vw2dpickset_ )
    {
	viewer2D()->dataMgr()->setSelected( vw2dpickset_ );
	vw2dpickset_->selected();
    }
    return true;
}


bool uiODVw2DPickSetTreeItem::showSubMenu()
{
    const int setidx = Pick::Mgr().indexOf( pickset_ );
    const bool changed = setidx < 0 || Pick::Mgr().isChanged(setidx);

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction( m3Dots(uiStrings::sProperties())), 0 );
    mnu.insertItem( new uiAction(m3Dots(tr("Set &direction"))), 1 );
    uiAction* saveitm = new uiAction( m3Dots(uiStrings::sSave()) );
    mnu.insertItem( saveitm, 2 );
    saveitm->setEnabled( changed );
    mnu.insertItem( new uiAction( uiStrings::sSaveAs() ), 3 );
    mnu.insertItem( new uiAction(uiStrings::sRemove() ), 4 );

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
	    parent_->removeChild( this );
	    break;
    }

    return true;
}


void uiODVw2DPickSetTreeItem::removePickSetCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( ps != &pickset_ )
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


uiTreeItem* uiODVw2DPickSetTreeItemFactory::createForVis(
				    const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet(const VW2DPickSet*,obj,vwr2d.dataMgr()->getObject(id));
    return obj ? new uiODVw2DPickSetTreeItem(id,false) : 0;
}
