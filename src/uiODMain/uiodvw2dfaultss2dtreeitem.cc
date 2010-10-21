/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dfaultss2dtreeitem.cc,v 1.11 2010-10-21 09:42:25 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dfaultss2dtreeitem.h"

#include "uiempartserv.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uivispartserv.h"
#include "uiodviewer2dmgr.h"
#include "pixmap.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "ioobj.h"
#include "randcolor.h"

#include "visseis2ddisplay.h"
#include "visvw2ddataman.h"
#include "visvw2dfaultss2d.h"


uiODVw2DFaultSS2DParentTreeItem::uiODVw2DFaultSS2DParentTreeItem()
    : uiODVw2DTreeItem( "FaultStickSet" )
{}


uiODVw2DFaultSS2DParentTreeItem::~uiODVw2DFaultSS2DParentTreeItem()
{
}


bool uiODVw2DFaultSS2DParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&New"), 0 );
    mnu.insertItem( new uiMenuItem("&Load ..."), 1 );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DFaultSS2DParentTreeItem::handleSubMenu( int mnuid )
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
	addChild( new uiODVw2DFaultSS2DTreeItem(emo->id()), false, false );
    }
    else if ( mnuid == 1 )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaultStickSets( objs );
	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODVw2DFaultSS2DTreeItem(
					objs[idx]->id()), false, false);

	deepUnRef( objs );
    }

    return true;
}


bool uiODVw2DFaultSS2DParentTreeItem::init()
{
    return true;
}


void uiODVw2DFaultSS2DParentTreeItem::tempObjAddedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
    if ( !fss ) return;

    if ( findChild(applMgr()->EMServer()->getName(emid)) )
	return;

    addChild( new uiODVw2DFaultSS2DTreeItem(emid),false,false);
}



uiODVw2DFaultSS2DTreeItem::uiODVw2DFaultSS2DTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(0)
    , emid_(emid)
    , fssview_(0)
{}


uiODVw2DFaultSS2DTreeItem::~uiODVw2DFaultSS2DTreeItem()
{
    NotifierAccess* deselnotify = fssview_->deSelection();
    if ( deselnotify )
	deselnotify->remove( mCB(this,uiODVw2DFaultSS2DTreeItem,deSelCB) );

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->change.remove(
		mCB(this,uiODVw2DFaultSS2DTreeItem,emobjChangeCB) );
	emobj->unRef();
    }

    viewer2D()->dataMgr()->removeObject( fssview_ );
}


bool uiODVw2DFaultSS2DTreeItem::init()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    emobj->ref();
    emobj->change.notify( mCB(this,uiODVw2DFaultSS2DTreeItem,emobjChangeCB) );
    displayMiniCtab();

    name_ = applMgr()->EMServer()->getName( emid_ );
    uilistviewitem_->setCheckable(true);
    uilistviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DFaultSS2DTreeItem,checkCB) );

    fssview_ = new VW2DFautSS2D( emid_, viewer2D()->viewwin(),
	    			 viewer2D()->dataEditor() );

    if ( !viewer2D()->lineSetID().isEmpty() )
	fssview_->setLineSetID( viewer2D()->lineSetID() );

    fssview_->draw();
    viewer2D()->dataMgr()->addObject( fssview_ );

    NotifierAccess* deselnotify =  fssview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODVw2DFaultSS2DTreeItem,deSelCB) );

    return true;
}


void uiODVw2DFaultSS2DTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );

    PtrMan<ioPixmap> pixmap = new ioPixmap( cPixmapWidth(), cPixmapHeight() );
    pixmap->fill( emobj->preferredColor() );
    uilistviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *pixmap );
}


void uiODVw2DFaultSS2DTreeItem::emobjChangeCB( CallBacker* cb )
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


bool uiODVw2DFaultSS2DTreeItem::select()
{
    if ( !uilistviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( fssview_ );
    fssview_->selected();
    return true;
}


bool uiODVw2DFaultSS2DTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    uiMenuItem* savemnu = new uiMenuItem("&Save ... ");
    mnu.insertItem( savemnu, 0 );
    savemnu->setEnabled( applMgr()->EMServer()->isChanged(emid_) );
    mnu.insertItem( new uiMenuItem("&Remove"), 1 );

    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
    {
	bool savewithname = EM::EMM().getMultiID( emid_ ).isEmpty();
	if ( !savewithname )
	{
	    PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid_) );
	    savewithname = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, savewithname );
	name_ = applMgr()->EMServer()->getName( emid_ );
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
    }
    else if ( mnuid == 1 )
	parent_->removeChild( this );

    return true;
}


void uiODVw2DFaultSS2DTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DFaultSS2DTreeItem::checkCB( CallBacker* )
{
    fssview_->enablePainting( isChecked() );
}


void uiODVw2DFaultSS2DTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    if ( emid != emid_ ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
    if ( !fss ) return;

    parent_->removeChild( this );
}
