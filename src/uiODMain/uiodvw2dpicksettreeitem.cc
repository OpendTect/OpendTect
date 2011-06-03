/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
 RCS:		$Id: uiodvw2dpicksettreeitem.cc,v 1.4 2011-06-03 14:10:26 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiodvw2dpicksettreeitem.h"

#include "pickset.h"
#include "pixmap.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodpicksettreeitem.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uipickpartserv.h"
#include "uipickpropdlg.h"
#include "uisetpickdirs.h"
#include "visvw2ddataman.h"
#include "visvw2dpickset.h"


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
{
    return true;
}


bool uiODVw2DPickSetParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&New"), 0 );
    mnu.insertItem( new uiMenuItem("&Load ..."), 1 );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DPickSetParentTreeItem::handleSubMenu( int menuid )
{
    return menuid == 0 ? applMgr()->pickServer()->createEmptySet( false )
	 : menuid == 1 ? applMgr()->pickServer()->loadSets( false ) : false;
}


void uiODVw2DPickSetParentTreeItem::pickSetAdded( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb);
    if ( ps ) 
    {
	const int picksetidx = picksetmgr_.indexOf(*ps);
	addChld( new uiODVw2DPickSetTreeItem(picksetidx), false, false);
    }
}


uiODVw2DPickSetTreeItem::uiODVw2DPickSetTreeItem( int picksetid )
    : uiODVw2DTreeItem(0)
    , pickset_(Pick::Mgr().get(picksetid))
    , picksetmgr_(Pick::Mgr())
    , vw2dpickset_(0)
    , setidx_(-1)
{
    picksetmgr_.setToBeRemoved.notify( 
	mCB(this,uiODVw2DPickSetTreeItem,removePickSetCB) );
    picksetmgr_.setDispChanged.notify(
	mCB(this,uiODVw2DPickSetTreeItem,displayChangedCB) );
}


uiODVw2DPickSetTreeItem::~uiODVw2DPickSetTreeItem()
{
    picksetmgr_.setToBeRemoved.remove(
	mCB(this,uiODVw2DPickSetTreeItem,removePickSetCB) );
    delete vw2dpickset_;
    checkStatusChange()->remove( mCB(this,uiODVw2DPickSetTreeItem,checkCB) );
    picksetmgr_.setDispChanged.remove(
	mCB(this,uiODVw2DPickSetTreeItem,displayChangedCB) );
}


bool uiODVw2DPickSetTreeItem::init()
{
    name_ = pickset_.name();
    uilistviewitem_->setCheckable(true);
    uilistviewitem_->setChecked( true );
    displayMiniCtab();
    checkStatusChange()->notify( mCB(this,uiODVw2DPickSetTreeItem,checkCB) );
    if ( !vw2dpickset_ )
	vw2dpickset_ = VW2DPickSet::create( picksetmgr_.indexOf(pickset_), 
			    viewer2D()->viewwin(),viewer2D()->dataEditor() );

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

    PtrMan<ioPixmap> pixmap = new ioPixmap( cPixmapWidth(), cPixmapHeight() );
    pixmap->fill( pickset_.disp_.color_ );
    uilistviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *pixmap );
}


bool uiODVw2DPickSetTreeItem::select()
{
    if ( !uilistviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( vw2dpickset_ );
    vw2dpickset_->selected();
    return true;
}


bool uiODVw2DPickSetTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );

    mnu.insertItem( new uiMenuItem("&Properties ..."), 0 );
    mnu.insertItem( new uiMenuItem("Set &direction ..."), 1 );
    mnu.insertItem( new uiMenuItem("&Save ... "), 2 );
    mnu.insertItem( new uiMenuItem("&Save As ..."), 3 );
    mnu.insertItem( new uiMenuItem("&Remove"), 4 );

    const int mnuid = mnu.exec();
    switch ( mnuid )
    {
	case 0:{ 
	    uiPickPropDlg dlg( getUiParent(), pickset_, NULL );
	    dlg.go();
	    }break;
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


uiTreeItem* 
	uiODVw2DPickSetTreeItemFactory::createForVis( int vwridx, int id ) const
{
    const uiODViewer2D* vwr2d = ODMainWin()->viewer2DMgr().getViewer2D(vwridx);
    if ( !vwr2d ) return 0;
    mDynamicCastGet(const VW2DPickSet*,obj,vwr2d->dataMgr()->getObject(id));
    return obj ? new uiODVw2DPickSetTreeItem(id) : 0;
}

