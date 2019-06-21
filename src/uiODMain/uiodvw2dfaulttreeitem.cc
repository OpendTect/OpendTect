/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2008
________________________________________________________________________

-*/

#include "uiodvw2dfaulttreeitem.h"

#include "uiempartserv.h"
#include "uiflatviewstdcontrol.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uitreeview.h"
#include "uistrings.h"
#include "uivispartserv.h"

#include "emeditor.h"
#include "emfault3d.h"
#include "emmanager.h"
#include "emtracker.h"
#include "emobject.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "randcolor.h"

#include "view2dfault.h"
#include "view2ddataman.h"


uiODVw2DFaultParentTreeItem::uiODVw2DFaultParentTreeItem()
    : uiODVw2DTreeItem( uiStrings::sFault() )
{
}


uiODVw2DFaultParentTreeItem::~uiODVw2DFaultParentTreeItem()
{
}


bool uiODVw2DFaultParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.addMenu( createAddMenu() );
    mnu.insertAction( new uiAction(uiStrings::sNew()), getNewItemID() );
    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DFaultParentTreeItem::handleSubMenu( int mnuid )
{
    handleStdSubMenu( mnuid );

    if ( mnuid == getNewItemID() )
    {
	RefMan<EM::Object> emo =
		EM::Flt3DMan().createTempObject( EM::Fault3D::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	emo->setNameToJustCreated();
	emo->setFullyLoaded( true );
	addNewTempFault( emo->id() );
	applMgr()->viewer2DMgr().addNewTempFault( emo->id(), -1 );
    }
    else if ( isAddItem(mnuid,true) || isAddItem(mnuid,false) )
    {
	ObjectSet<EM::Object> objs;
	applMgr()->EMServer()->selectFaults( objs );
	DBKeySet emids;
	for ( int idx=0; idx<objs.size(); idx++ )
	    emids += objs[idx]->id();

	if ( isAddItem(mnuid,true) )
	{
	    addFaults( emids );
	    applMgr()->viewer2DMgr().addFaults( emids );
	}
	else
	    addFaults( emids );

	deepUnRef( objs );
    }

    return true;
}


const char* uiODVw2DFaultParentTreeItem::iconName() const
{ return "tree-flt"; }


bool uiODVw2DFaultParentTreeItem::init()
{ return uiODVw2DTreeItem::init(); }



void uiODVw2DFaultParentTreeItem::getFaultVwr2DIDs(
	const DBKey& emid, TypeSet<int>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DFaultTreeItem*,faultitem,getChild(idx))
	if ( !faultitem || faultitem->emObjectID() != emid )
	    continue;

	vw2dobjids.addIfNew( faultitem->vw2DObject()->id() );
    }
}


void uiODVw2DFaultParentTreeItem::getLoadedFaults(
	DBKeySet& emids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DFaultTreeItem*,faultitem,getChild(idx))
	if ( !faultitem )
	    continue;
	emids.addIfNew( faultitem->emObjectID() );
    }
}


void uiODVw2DFaultParentTreeItem::removeFault( const DBKey& emid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DFaultTreeItem*,faultitem,getChild(idx))
	if ( !faultitem || emid!=faultitem->emObjectID() )
	    continue;
	removeChild( faultitem );
    }
}


void uiODVw2DFaultParentTreeItem::addFaults(const DBKeySet& emids)
{
    DBKeySet emidstobeloaded, emidsloaded;
    getLoadedFaults( emidsloaded );
    for ( int idx=0; idx<emids.size(); idx++ )
    {
	if ( !emidsloaded.isPresent(emids[idx]) )
	    emidstobeloaded.addIfNew( emids[idx] );
    }

    for ( int idx=0; idx<emidstobeloaded.size(); idx++ )
    {
	const DBKey emid = emidstobeloaded[idx];
	const EM::Object* emobj = EM::Flt3DMan().getObject( emid );
	if ( !emobj || findChild(emobj->name()) )
	    continue;

	MPE::ObjectEditor* editor = MPE::engine().getEditor( emobj->id(),false);
	uiODVw2DFaultTreeItem* childitem = new uiODVw2DFaultTreeItem( emid );
	addChld( childitem, false, false);
	if ( editor )
	    editor->addUser();
    }
}


void uiODVw2DFaultParentTreeItem::setupNewTempFault( const DBKey& emid )
{
    DBKeySet emidsloaded;
    getLoadedFaults( emidsloaded );
    if ( !emidsloaded.isPresent(emid) )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DFaultTreeItem*,flttreeitm,getChild(idx))
	if ( flttreeitm && emid==flttreeitm->emObjectID() )
	{
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    flttreeitm->select();
	    break;
	}
    }

}


void uiODVw2DFaultParentTreeItem::addNewTempFault( const DBKey& emid )
{
    DBKeySet emidsloaded;
    getLoadedFaults( emidsloaded );
    if ( emidsloaded.isPresent(emid) )
	return;

    uiODVw2DFaultTreeItem* faulttreeitem = new uiODVw2DFaultTreeItem( emid );
    addChld( faulttreeitem,false, false );
    viewer2D()->viewControl()->setEditMode( true );
    MPE::engine().getEditor( emid, true );
    faulttreeitem->select();
}



// uiODVw2DFaultTreeItem
uiODVw2DFaultTreeItem::uiODVw2DFaultTreeItem( const DBKey& emid )
    : uiODVw2DEMTreeItem(emid)
    , faultview_(0)
{}


uiODVw2DFaultTreeItem::uiODVw2DFaultTreeItem( int id, bool )
    : uiODVw2DEMTreeItem(DBKey::getInvalid())
    , faultview_(0)
{
    displayid_ = id;
}


uiODVw2DFaultTreeItem::~uiODVw2DFaultTreeItem()
{
    detachAllNotifiers();
    MPE::engine().removeEditor( emid_ );
    if ( faultview_ )
	viewer2D()->dataMgr()->removeObject( faultview_ );
}


bool uiODVw2DFaultTreeItem::init()
{
    EM::Object* emobj = 0;
    if ( displayid_ < 0 )
    {
	emobj = EM::Flt3DMan().getObject( emid_ );
	if ( !emobj ) return false;

	faultview_ = VW2DFault::create( emid_, viewer2D()->viewwin(),
				   viewer2D()->dataEditor() );
	viewer2D()->dataMgr()->addObject( faultview_ );
	displayid_ = faultview_->id();
    }
    else
    {
	mDynamicCastGet(VW2DFault*,hd,
			viewer2D()->dataMgr()->getObject(displayid_))
	if ( !hd )
	    return false;
	emid_ = hd->emID();
	emobj = EM::Flt3DMan().getObject( emid_ );
	if ( !emobj ) return false;

	faultview_ = hd;
    }

    mAttachCB( emobj->objectChanged(), uiODVw2DFaultTreeItem::emobjChangeCB );
    displayMiniCtab();

    name_ = toUiString( emid_.name() );
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DFaultTreeItem,checkCB) );

    faultview_->draw();

    mAttachCB( viewer2D()->viewControl()->editPushed(),
	       uiODVw2DFaultTreeItem::enableKnotsCB );

    NotifierAccess* deselnotify = faultview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODVw2DFaultTreeItem,deSelCB) );

    uiODVw2DTreeItem::addKeyBoardEvent();
    return true;
}


void uiODVw2DFaultTreeItem::enableKnotsCB( CallBacker* )
{
    if ( faultview_ && viewer2D()->dataMgr()->selectedID() == faultview_->id() )
	faultview_->selected();
}


bool uiODVw2DFaultTreeItem::select()
{
    if ( uitreeviewitem_->treeView() )
	uitreeviewitem_->treeView()->clearSelection();

    uitreeviewitem_->setSelected( true );
    if ( faultview_ )
    {
	viewer2D()->dataMgr()->setSelected( faultview_ );
	faultview_->selected();
    }

    return true;
}


#define mPropID		0
#define mSaveID		1
#define mSaveAsID	2

bool uiODVw2DFaultTreeItem::showSubMenu()
{
    uiEMPartServer* ems = applMgr()->EMServer();
    uiMenu mnu( getUiParent(), uiStrings::sAction() );

    addAction( mnu, m3Dots(uiStrings::sProperties()), mPropID, "disppars",true);

    const bool haschanged = ems->isChanged( emid_ );
    addAction( mnu, uiStrings::sSave(), mSaveID, "save", haschanged );
    addAction( mnu, m3Dots(uiStrings::sSaveAs()), mSaveAsID, "saveas", true );

    uiMenu* removemenu = createRemoveMenu();
    mnu.addMenu( removemenu );

    const int mnuid = mnu.exec();
    if ( mnuid == mPropID )
	showPropDlg();
    else if ( mnuid==mSaveID )
	doSave();
    else if ( mnuid==mSaveAsID )
	doSaveAs();
    else if ( isRemoveItem(mnuid,false) || isRemoveItem(mnuid,true) )
    {
	if ( !applMgr()->EMServer()->askUserToSave(emid_,true) )
	    return true;

	name_ = toUiString( emid_.name() );
	renameVisObj();
	bool doremove = !applMgr()->viewer2DMgr().isItemPresent( parent_ ) ||
		isRemoveItem(mnuid,false);
	if ( isRemoveItem(mnuid,true) )
	    applMgr()->viewer2DMgr().removeFault( emid_ );

	if ( doremove )
	    parent_->removeChild( this );
    }

    return true;
}


void uiODVw2DFaultTreeItem::updateCS( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd && faultview_ )
	faultview_->draw();
}


void uiODVw2DFaultTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DFaultTreeItem::checkCB( CallBacker* )
{
    if ( faultview_ )
	faultview_->enablePainting( isChecked() );
}


void uiODVw2DFaultTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const DBKey&, emid, cb );
    if ( emid != emid_ ) return;

    EM::Object* emobj = EM::Flt3DMan().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,f3d,emobj);
    if ( !f3d ) return;

    parent_->removeChild( this );
}


uiTreeItem* uiODVw2DFaultTreeItemFactory::createForVis(
				    const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet(const VW2DFault*,obj,vwr2d.dataMgr()->getObject(id));
    return obj ? new uiODVw2DFaultTreeItem(id,true) : 0;
}

const Vw2DDataObject* uiODVw2DFaultTreeItem::vw2DObject() const
{ return faultview_; }
