/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dfaultsstreeitem.cc,v 1.1 2010-06-24 08:57:00 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dfaultsstreeitem.h"

#include "uiempartserv.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "emobject.h"

#include "visvw2dfaultss3d.h"
#include "visvw2ddataman.h"

uiODVw2DFaultSSParentTreeItem::uiODVw2DFaultSSParentTreeItem()
    : uiODVw2DTreeItem( "FaultStickSet" )
{
}


uiODVw2DFaultSSParentTreeItem::~uiODVw2DFaultSSParentTreeItem()
{
    applMgr()->EMServer()->tempobjAdded.remove(
	    mCB(this,uiODVw2DFaultSSParentTreeItem,tempObjAddedCB) );
}


bool uiODVw2DFaultSSParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Load ..."), 0 );
    handleSubMenu( mnu.exec() );
    
    return true;
}


bool uiODVw2DFaultSSParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid == 0 )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaultStickSets( objs );

	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODVw2DFaultSSTreeItem(objs[idx]->id()),false,false);

	deepUnRef( objs );
    }

    return true;
}


bool uiODVw2DFaultSSParentTreeItem::init()
{
    applMgr()->EMServer()->tempobjAdded.notify(
	    mCB(this,uiODVw2DFaultSSParentTreeItem,tempObjAddedCB) );

    return true;
}


void uiODVw2DFaultSSParentTreeItem::tempObjAddedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
    if ( !fss ) return;

    addChild( new uiODVw2DFaultSSTreeItem(emid),false,false);
}


uiODVw2DFaultSSTreeItem::uiODVw2DFaultSSTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(0)
    , emid_(emid)
    , fssview_(0)
{}


uiODVw2DFaultSSTreeItem::~uiODVw2DFaultSSTreeItem()
{
    NotifierAccess* deselnotify = fssview_->deSelection();
    if ( deselnotify )
	deselnotify->remove( mCB(this,uiODVw2DFaultSSTreeItem,deSelCB) );

    applMgr()->EMServer()->tempobjAbtToDel.remove(
	    mCB(this,uiODVw2DFaultSSTreeItem,emobjAbtToDelCB) );

    viewer2D()->dataMgr()->removeObject( fssview_ );
}


bool uiODVw2DFaultSSTreeItem::init()
{
    name_ = applMgr()->EMServer()->getName( emid_ );
    uilistviewitem_->setCheckable(true);
    uilistviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DFaultSSTreeItem,checkCB) );

    fssview_ = new VW2DFautSS3D( emid_, viewer2D()->viewwin(),
	    			 viewer2D()->dataEditor() );
    fssview_->draw();
    viewer2D()->dataMgr()->addObject( fssview_ );

    NotifierAccess* deselnotify =  fssview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODVw2DFaultSSTreeItem,deSelCB) );

    applMgr()->EMServer()->tempobjAbtToDel.notify(
	    mCB(this,uiODVw2DFaultSSTreeItem,emobjAbtToDelCB) );

    return true;
}


bool uiODVw2DFaultSSTreeItem::select()
{
    if ( !uilistviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( fssview_ );
    fssview_->selected();

    return true;
}


bool uiODVw2DFaultSSTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Remove ..."), 0 );

    if (  mnu.exec() == 0 )
    {
	parent_->removeChild( this );
    }

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

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

     mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
     if ( !fss ) return;

     if ( emid != emid_ ) return;

     parent_->removeChild( this );
}
