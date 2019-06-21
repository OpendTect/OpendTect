/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
________________________________________________________________________

-*/

#include "uiodvw2dfaultsstreeitem.h"

#include "uiempartserv.h"
#include "uiflatviewstdcontrol.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uipixmap.h"
#include "uitreeview.h"
#include "uistrings.h"
#include "uivispartserv.h"

#include "emeditor.h"
#include "emfaultstickset.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "randcolor.h"

#include "view2dfaultss3d.h"
#include "view2ddataman.h"


uiODVw2DFaultSSParentTreeItem::uiODVw2DFaultSSParentTreeItem()
    : uiODVw2DTreeItem( uiStrings::sFaultStickSet())
{
}


uiODVw2DFaultSSParentTreeItem::~uiODVw2DFaultSSParentTreeItem()
{
}


bool uiODVw2DFaultSSParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.addMenu( createAddMenu() );
    mnu.insertAction( new uiAction(uiStrings::sNew()), getNewItemID() );
    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DFaultSSParentTreeItem::handleSubMenu( int mnuid )
{
    handleStdSubMenu( mnuid );

    if ( mnuid == getNewItemID() )
    {
	RefMan<EM::Object> emo =
		EM::FSSMan().createTempObject( EM::FaultStickSet::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	emo->setNameToJustCreated();
	emo->setFullyLoaded( true );
	addNewTempFaultSS( emo->id() );
	applMgr()->viewer2DMgr().addNewTempFaultSS( emo->id(), -1 );
	applMgr()->viewer2DMgr().addNewTempFaultSS2D( emo->id(), -1 );
    }
    else if ( isAddItem(mnuid,true) || isAddItem(mnuid,false) )
    {
	ObjectSet<EM::Object> objs;
	applMgr()->EMServer()->selectFaultStickSets( objs );
	DBKeySet emids;
	for ( int idx=0; idx<objs.size(); idx++ )
	    emids += objs[idx]->id();

	if ( isAddItem(mnuid,true) )
	{
	    addFaultSSs( emids );
	    applMgr()->viewer2DMgr().addFaultSSs( emids );
	    applMgr()->viewer2DMgr().addFaultSS2Ds( emids );
	}
	else
	    addFaultSSs( emids );

	deepUnRef( objs );
    }

    return true;
}


const char* uiODVw2DFaultSSParentTreeItem::iconName() const
{ return "tree-fltss"; }


bool uiODVw2DFaultSSParentTreeItem::init()
{ return uiODVw2DTreeItem::init(); }


void uiODVw2DFaultSSParentTreeItem::getFaultSSVwr2DIDs(
	const DBKey& emid, TypeSet<int>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DFaultSSTreeItem*,faultssitem,
			getChild(idx))
	if ( !faultssitem || faultssitem->emObjectID() != emid )
	    continue;

	vw2dobjids.addIfNew( faultssitem->vw2DObject()->id() );
    }
}


void uiODVw2DFaultSSParentTreeItem::getLoadedFaultSSs(
	DBKeySet& emids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DFaultSSTreeItem*,faultitem,getChild(idx))
	if ( !faultitem )
	    continue;
	emids.addIfNew( faultitem->emObjectID() );
    }
}


void uiODVw2DFaultSSParentTreeItem::removeFaultSS( const DBKey& emid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DFaultSSTreeItem*,faultitem,getChild(idx))
	if ( !faultitem || emid!=faultitem->emObjectID() )
	    continue;
	removeChild( faultitem );
    }
}


void uiODVw2DFaultSSParentTreeItem::addFaultSSs(
					const DBKeySet& emids )
{
    DBKeySet emidstobeloaded, emidsloaded;
    getLoadedFaultSSs( emidsloaded );
    for ( int idx=0; idx<emids.size(); idx++ )
    {
	if ( !emidsloaded.isPresent(emids[idx]) )
	    emidstobeloaded.addIfNew( emids[idx] );
    }

    for ( int idx=0; idx<emidstobeloaded.size(); idx++ )
    {
	const EM::Object* emobj =
	    EM::FSSMan().getObject( emidstobeloaded[idx] );
	if ( !emobj || findChild(emobj->name()) )
	    continue;

	MPE::ObjectEditor* editor =
	    MPE::engine().getEditor( emobj->id(), false );
	uiODVw2DFaultSSTreeItem* childitem =
	    new uiODVw2DFaultSSTreeItem( emidstobeloaded[idx] );
	addChld( childitem ,false, false );
	if ( editor )
	    editor->addUser();
    }
}


void uiODVw2DFaultSSParentTreeItem::setupNewTempFaultSS( const DBKey& emid )
{
    DBKeySet emidsloaded;
    getLoadedFaultSSs( emidsloaded );
    if ( !emidsloaded.isPresent(emid) )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DFaultSSTreeItem*,fltsstreeitm,getChild(idx))
	if ( fltsstreeitm && emid==fltsstreeitm->emObjectID() )
	{
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    fltsstreeitm->select();
	    break;
	}
    }

}


void uiODVw2DFaultSSParentTreeItem::addNewTempFaultSS( const DBKey& emid )
{
    DBKeySet emidsloaded;
    getLoadedFaultSSs( emidsloaded );
    if ( emidsloaded.isPresent(emid) )
	return;

    uiODVw2DFaultSSTreeItem* faulttreeitem = new uiODVw2DFaultSSTreeItem(emid);
    addChld( faulttreeitem,false, false );
    if ( viewer2D() && viewer2D()->viewControl() )
	viewer2D()->viewControl()->setEditMode( true );
    faulttreeitem->select();
}



// uiODVw2DFaultSSTreeItem
uiODVw2DFaultSSTreeItem::uiODVw2DFaultSSTreeItem( const DBKey& emid )
    : uiODVw2DEMTreeItem( emid )
    , fssview_(0)
{}


uiODVw2DFaultSSTreeItem::uiODVw2DFaultSSTreeItem( int id, bool )
    : uiODVw2DEMTreeItem( DBKey::getInvalid() )
    , fssview_(0)
{
    displayid_ = id;
}


uiODVw2DFaultSSTreeItem::~uiODVw2DFaultSSTreeItem()
{
    detachAllNotifiers();
    MPE::engine().removeEditor( emid_ );
    if ( fssview_ )
	viewer2D()->dataMgr()->removeObject( fssview_ );
}


bool uiODVw2DFaultSSTreeItem::init()
{
    EM::Object* emobj = 0;
    if ( displayid_ < 0 )
    {
	emobj = EM::FSSMan().getObject( emid_ );
	if ( !emobj ) return false;
	fssview_ = VW2DFaultSS3D::create( emid_, viewer2D()->viewwin(),
				     viewer2D()->dataEditor() );
	viewer2D()->dataMgr()->addObject( fssview_ );
	displayid_ = fssview_->id();
    }
    else
    {
	mDynamicCastGet(VW2DFaultSS3D*,hd,
			viewer2D()->dataMgr()->getObject(displayid_))
	if ( !hd )
	    return false;
	emid_ = hd->emID();
	emobj = EM::FSSMan().getObject( emid_ );
	if ( !emobj ) return false;

	fssview_ = hd;
    }

    mAttachCB( emobj->objectChanged(), uiODVw2DFaultSSTreeItem::emobjChangeCB );
    displayMiniCtab();
    name_ = toUiString( emid_.name() );
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DFaultSSTreeItem,checkCB) );

    fssview_->draw();

    if ( viewer2D() && viewer2D()->viewControl() )
	mAttachCB( viewer2D()->viewControl()->editPushed(),
		   uiODVw2DFaultSSTreeItem::enableKnotsCB );

    NotifierAccess* deselnotify =  fssview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODVw2DFaultSSTreeItem,deSelCB) );

    uiODVw2DTreeItem::addKeyBoardEvent();
    return true;
}


void uiODVw2DFaultSSTreeItem::enableKnotsCB( CallBacker* )
{
    if ( fssview_ && viewer2D()->dataMgr()->selectedID() == fssview_->id() )
	fssview_->selected();
}


bool uiODVw2DFaultSSTreeItem::select()
{
    if ( uitreeviewitem_->treeView() )
	uitreeviewitem_->treeView()->clearSelection();

    uitreeviewitem_->setSelected( true );
    if ( fssview_ )
    {
	viewer2D()->dataMgr()->setSelected( fssview_ );
	fssview_->selected();
    }

    return true;
}


#define mPropID		0
#define mSaveID		1
#define mSaveAsID	2

bool uiODVw2DFaultSSTreeItem::showSubMenu()
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
    else if ( mnuid== mSaveAsID )
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
	{
	    applMgr()->viewer2DMgr().removeFaultSS2D( emid_ );
	    applMgr()->viewer2DMgr().removeFaultSS( emid_ );
	}

	if ( doremove )
	    parent_->removeChild( this );
    }

    return true;
}


void uiODVw2DFaultSSTreeItem::updateCS( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd && fssview_ )
	fssview_->setTrcKeyZSampling( cs, upd );
}


void uiODVw2DFaultSSTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DFaultSSTreeItem::checkCB( CallBacker* )
{
    if ( fssview_ )
	fssview_->enablePainting( isChecked() );
}


void uiODVw2DFaultSSTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const DBKey&, emid, cb );
    if ( emid != emid_ ) return;

    EM::Object* emobj = EM::FSSMan().getObject( emid );
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


const Vw2DDataObject* uiODVw2DFaultSSTreeItem::vw2DObject() const
{ return fssview_; }
