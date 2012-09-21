/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodvw2dfaultsstreeitem.h"

#include "uiempartserv.h"
#include "uiflatviewstdcontrol.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uitreeview.h"
#include "pixmap.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "ioobj.h"
#include "randcolor.h"

#include "visvw2dfaultss3d.h"
#include "visvw2ddataman.h"

uiODVw2DFaultSSParentTreeItem::uiODVw2DFaultSSParentTreeItem()
    : uiODVw2DTreeItem( "FaultStickSet" )
{
}


uiODVw2DFaultSSParentTreeItem::~uiODVw2DFaultSSParentTreeItem()
{
}


bool uiODVw2DFaultSSParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&New"), 0 );
    mnu.insertItem( new uiMenuItem("&Load ..."), 1 );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DFaultSSParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid == 0 )
    {
	RefMan<EM::EMObject> emo =
	    	EM::EMM().createTempObject( EM::FaultStickSet::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	emo->setNewName();
	emo->setFullyLoaded( true );
	uiODVw2DFaultSSTreeItem* treeitem =
	    new uiODVw2DFaultSSTreeItem( emo->id() );
	addChld( treeitem, false, false );
	viewer2D()->viewControl()->setEditMode( true );
	treeitem->select();
    }
    else if ( mnuid == 1 )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaultStickSets( objs );

	for ( int idx=0; idx<objs.size(); idx++ )
	    addChld( new uiODVw2DFaultSSTreeItem(objs[idx]->id()),false,false);

	deepUnRef( objs );
    }

    return true;
}


bool uiODVw2DFaultSSParentTreeItem::init()
{
    return true;
}


void uiODVw2DFaultSSParentTreeItem::tempObjAddedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
    if ( !fss ) return;

    if ( findChild(applMgr()->EMServer()->getName(emid)) )
	return;

    addChld( new uiODVw2DFaultSSTreeItem(emid),false,false);
}



uiODVw2DFaultSSTreeItem::uiODVw2DFaultSSTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(0)
    , emid_(emid)
    , fssview_(0)
{}


uiODVw2DFaultSSTreeItem::uiODVw2DFaultSSTreeItem( int id, bool )
    : uiODVw2DTreeItem(0)
    , fssview_(0)
{
    displayid_ = id;
}


uiODVw2DFaultSSTreeItem::~uiODVw2DFaultSSTreeItem()
{
    NotifierAccess* deselnotify = fssview_ ? fssview_->deSelection() : 0;
    if ( deselnotify )
	deselnotify->remove( mCB(this,uiODVw2DFaultSSTreeItem,deSelCB) );

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
	emobj->change.remove( mCB(this,uiODVw2DFaultSSTreeItem,emobjChangeCB) );

    viewer2D()->dataMgr()->removeObject( fssview_ );
}


bool uiODVw2DFaultSSTreeItem::init()
{
    EM::EMObject* emobj = 0;
    if ( displayid_ < 0 )
    {
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;
	fssview_ = VW2DFaultSS3D::create( emid_, viewer2D()->viewwin(),
				     viewer2D()->dataEditor() );
    }
    else
    {
	mDynamicCastGet(VW2DFaultSS3D*,hd,
				viewer2D()->dataMgr()->getObject(displayid_))
	if ( !hd )
	    return false;
	emid_ = hd->emID();
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	fssview_ = hd;
    }

    emobj->change.notify( mCB(this,uiODVw2DFaultSSTreeItem,emobjChangeCB) );
    displayMiniCtab();
    name_ = applMgr()->EMServer()->getName( emid_ );
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DFaultSSTreeItem,checkCB) );

    fssview_->draw();

    if ( displayid_ < 0 )
	viewer2D()->dataMgr()->addObject( fssview_ );

    NotifierAccess* deselnotify =  fssview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODVw2DFaultSSTreeItem,deSelCB) );

    return true;
}


void uiODVw2DFaultSSTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );

    PtrMan<ioPixmap> pixmap = new ioPixmap( cPixmapWidth(), cPixmapHeight() );
    pixmap->fill( emobj->preferredColor() );

    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *pixmap );
}


void uiODVw2DFaultSSTreeItem::emobjChangeCB( CallBacker* cb )
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
	default: break;
    }
}


bool uiODVw2DFaultSSTreeItem::select()
{
    uitreeviewitem_->setSelected( true );

    viewer2D()->dataMgr()->setSelected( fssview_ );
    fssview_->selected();

    return true;
}


bool uiODVw2DFaultSSTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    uiMenuItem* savemnu = new uiMenuItem("&Save ... ");
    mnu.insertItem( savemnu, 0 );
    savemnu->setEnabled( applMgr()->EMServer()->isChanged(emid_) &&
	    		 applMgr()->EMServer()->isFullyLoaded(emid_) );
    mnu.insertItem( new uiMenuItem("&Save As ..."), 1 );
    mnu.insertItem( new uiMenuItem("&Remove"), 2 );

    const int mnuid = mnu.exec();
    if ( mnuid == 0 || mnuid == 1 )
    {
	bool savewithname = (mnuid == 1) ||
	    		    (EM::EMM().getMultiID( emid_ ).isEmpty());
	if ( !savewithname )
	{
	    PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid_) );
	    savewithname = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, savewithname );
	name_ = applMgr()->EMServer()->getName( emid_ );
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
    }
    else if ( mnuid == 2 )
	parent_->removeChild( this );

    return true;
}


void uiODVw2DFaultSSTreeItem::updateCS( const CubeSampling& cs, bool upd )
{
    if ( upd )
	fssview_->setCubeSampling( cs, upd );
}


void uiODVw2DFaultSSTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DFaultSSTreeItem::checkCB( CallBacker* )
{
    fssview_->enablePainting( isChecked() );
}


void uiODVw2DFaultSSTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    if ( emid != emid_ ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
    if ( !fss ) return;

    parent_->removeChild( this );
}


uiTreeItem* uiODVw2DFaultSSTreeItemFactory::createForVis( 
				const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet(const VW2DFaultSS3D*,obj,vwr2d.dataMgr()->getObject(id));
    return obj ? new uiODVw2DFaultSSTreeItem(id,true) : 0;
}
