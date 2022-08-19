/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodvw2dfaulttreeitem.h"

#include "uicolor.h"
#include "uiempartserv.h"
#include "uiflatviewstdcontrol.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uisellinest.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "emeditor.h"
#include "emfault3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "randcolor.h"

#include "view2dfault.h"
#include "view2ddataman.h"


uiODView2DFaultParentTreeItem::uiODView2DFaultParentTreeItem()
    : uiODView2DTreeItem( uiStrings::sFault() )
{
}


uiODView2DFaultParentTreeItem::~uiODView2DFaultParentTreeItem()
{
}


bool uiODView2DFaultParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.addMenu( createAddMenu() );
    mnu.insertAction( new uiAction(uiStrings::sNew()), getNewItemID() );
    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODView2DFaultParentTreeItem::handleSubMenu( int mnuid )
{
    handleStdSubMenu( mnuid );

    if ( mnuid == getNewItemID() )
    {
	RefMan<EM::EMObject> emo =
			EM::EMM().createTempObject( EM::Fault3D::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( OD::getRandomColor(false) );
	emo->setNewName();
	emo->setFullyLoaded( true );
	addNewTempFault( emo->id() );
	applMgr()->viewer2DMgr().addNewTempFault(
		emo->id(), viewer2D()->getSyncSceneID() );
    }
    else if ( isAddItem(mnuid,true) || isAddItem(mnuid,false) )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaults( objs, false, getUiParent() );
	TypeSet<EM::ObjectID> emids;
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


const char* uiODView2DFaultParentTreeItem::iconName() const
{ return "tree-flt"; }


bool uiODView2DFaultParentTreeItem::init()
{ return uiODView2DTreeItem::init(); }



void uiODView2DFaultParentTreeItem::getFaultVwr2DIDs(
	EM::ObjectID emid, TypeSet<Vis2DID>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODView2DFaultTreeItem*,faultitem,getChild(idx))
	if ( !faultitem || faultitem->emObjectID() != emid )
	    continue;

	vw2dobjids.addIfNew( faultitem->vw2DObject()->id() );
    }
}


void uiODView2DFaultParentTreeItem::getLoadedFaults(
	TypeSet<EM::ObjectID>& emids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODView2DFaultTreeItem*,faultitem,getChild(idx))
	if ( !faultitem )
	    continue;
	emids.addIfNew( faultitem->emObjectID() );
    }
}


void uiODView2DFaultParentTreeItem::removeFault( EM::ObjectID emid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultTreeItem*,faultitem,getChild(idx))
	if ( !faultitem || emid!=faultitem->emObjectID() )
	    continue;
	removeChild( faultitem );
    }
}


void uiODView2DFaultParentTreeItem::addFaults(
					const TypeSet<EM::ObjectID>& emids )
{
    TypeSet<EM::ObjectID> emidstobeloaded, emidsloaded;
    getLoadedFaults( emidsloaded );
    for ( int idx=0; idx<emids.size(); idx++ )
    {
	if ( !emidsloaded.isPresent(emids[idx]) )
	    emidstobeloaded.addIfNew( emids[idx] );
    }

    for ( int idx=0; idx<emidstobeloaded.size(); idx++ )
    {
	const EM::ObjectID emid = emidstobeloaded[idx];
	const EM::EMObject* emobj = EM::EMM().getObject( emid );
	if ( !emobj || findChild(emobj->name()) )
	    continue;

	MPE::ObjectEditor* editor = MPE::engine().getEditor( emobj->id(),false);
	auto* childitem = new uiODView2DFaultTreeItem( emid );
	addChld( childitem, false, false);
	if ( editor )
	    editor->addUser();
    }
}


void uiODView2DFaultParentTreeItem::setupNewTempFault( EM::ObjectID emid )
{
    TypeSet<EM::ObjectID> emidsloaded;
    getLoadedFaults( emidsloaded );
    if ( !emidsloaded.isPresent(emid) )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultTreeItem*,flttreeitm,getChild(idx))
	if ( flttreeitm && emid==flttreeitm->emObjectID() )
	{
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    flttreeitm->select();
	    break;
	}
    }

}


void uiODView2DFaultParentTreeItem::addNewTempFault( EM::ObjectID emid )
{
    TypeSet<EM::ObjectID> emidsloaded;
    getLoadedFaults( emidsloaded );
    if ( emidsloaded.isPresent(emid) )
	return;

    auto* faulttreeitem = new uiODView2DFaultTreeItem( emid );
    addChld( faulttreeitem,false, false );
    viewer2D()->viewControl()->setEditMode( true );
    MPE::engine().getEditor( emid, true );
    faulttreeitem->select();
}


uiODView2DFaultTreeItem::uiODView2DFaultTreeItem( const EM::ObjectID& emid )
    : uiODView2DTreeItem(uiString::emptyString())
    , emid_(emid)
    , faultview_(0)
{}


uiODView2DFaultTreeItem::uiODView2DFaultTreeItem( Vis2DID id, bool )
    : uiODView2DTreeItem(uiString::emptyString())
    , faultview_(0)
{
    displayid_ = id;
}


uiODView2DFaultTreeItem::~uiODView2DFaultTreeItem()
{
    detachAllNotifiers();
    MPE::engine().removeEditor( emid_ );
    if ( faultview_ )
	viewer2D()->dataMgr()->removeObject( faultview_ );
}


bool uiODView2DFaultTreeItem::init()
{
    EM::EMObject* emobj = nullptr;
    if ( !displayid_.isValid() )
    {
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	faultview_ = View2D::Fault::create( viewer2D()->viewwin(),
					viewer2D()->dataEditor() );
	faultview_->setEMObjectID( emid_ );
	viewer2D()->dataMgr()->addObject( faultview_ );
	displayid_ = faultview_->id();
    }
    else
    {
	mDynamicCastGet(View2D::Fault*,hd,
			viewer2D()->getObject(displayid_))
	if ( !hd )
	    return false;

	emid_ = hd->getEMObjectID();
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	faultview_ = hd;
    }

    mAttachCB( emobj->change, uiODView2DFaultTreeItem::emobjChangeCB );
    displayMiniCtab();

    name_ = applMgr()->EMServer()->getUiName( emid_ );
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODView2DFaultTreeItem,checkCB) );

    faultview_->draw();

    mAttachCB( viewer2D()->viewControl()->editPushed(),
	       uiODView2DFaultTreeItem::enableKnotsCB );

    NotifierAccess* deselnotify = faultview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODView2DFaultTreeItem,deSelCB) );

    uiODView2DTreeItem::addKeyBoardEvent( emid_ );

    return true;
}


void uiODView2DFaultTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				emobj->preferredColor() );
}


void uiODView2DFaultTreeItem::emobjChangeCB( CallBacker* cb )
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
	case EM::EMObjectCallbackData::NameChange:
	{
	    name_ = mToUiStringTodo(applMgr()->EMServer()->getName( emid_ ));
	    uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
	    break;
	}
	default: break;
    }
}


void uiODView2DFaultTreeItem::enableKnotsCB( CallBacker* )
{
    if ( faultview_ && viewer2D()->dataMgr()->selectedID() == faultview_->id() )
	faultview_->selected();
}


bool uiODView2DFaultTreeItem::select()
{
    if ( uitreeviewitem_->treeView() )
	uitreeviewitem_->treeView()->deselectAll();

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

bool uiODView2DFaultTreeItem::showSubMenu()
{
    uiEMPartServer* ems = applMgr()->EMServer();
    const EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !ems || !emobj )
	return false;

    uiMenu mnu( getUiParent(), uiStrings::sAction() );

    addAction( mnu, m3Dots(uiStrings::sProperties()), mPropID, "disppars",true);

    const bool haschanged = ems->isChanged( emid_ );
    addAction( mnu, uiStrings::sSave(), mSaveID, "save", haschanged );
    addAction( mnu, m3Dots(uiStrings::sSaveAs()), mSaveAsID, "saveas", true );

    uiMenu* removemenu = createRemoveMenu();
    mnu.addMenu( removemenu );

    const int mnuid = mnu.exec();
    if ( mnuid == mPropID )
    {
	uiDialog dlg( getUiParent(), uiDialog::Setup(uiStrings::sProperties(),
					mNoDlgTitle,mNoHelpKey) );
	dlg.setCtrlStyle( uiDialog::CloseOnly );
	uiSelLineStyle::Setup lssu;
	lssu.drawstyle(false);
	OD::LineStyle ls = emobj->preferredLineStyle();
	ls.color_ = emobj->preferredColor();
	uiSelLineStyle* lsfld = new uiSelLineStyle( &dlg, ls, lssu );
	lsfld->changed.notify( mCB(this,uiODView2DFaultTreeItem,propChgCB) );
	dlg.go();

    }
    else if ( mnuid==mSaveID || mnuid==mSaveAsID )
    {
	if ( mnuid==mSaveID )
	    doSave();
	if ( mnuid==mSaveAsID )
	    doSaveAs();
    }
    else if ( isRemoveItem(mnuid,false) || isRemoveItem(mnuid,true) )
    {
	if ( !applMgr()->EMServer()->askUserToSave(emid_,true) )
	    return true;

	name_ = applMgr()->EMServer()->getUiName( emid_ );
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


void uiODView2DFaultTreeItem::propChgCB( CallBacker* cb )
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    mDynamicCastGet(uiColorInput*,colfld,cb)
    if ( colfld )
    {
	emobj->setPreferredColor( colfld->color() );
	return;
    }

    OD::LineStyle ls = emobj->preferredLineStyle();
    mDynamicCastGet(uiSpinBox*,szfld,cb)
    if ( szfld )
    {
	ls.width_ = szfld->getIntValue();
	emobj->setPreferredLineStyle( ls );
    }
}


void uiODView2DFaultTreeItem::updateCS( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd && faultview_ )
	faultview_->draw();
}


void uiODView2DFaultTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODView2DFaultTreeItem::checkCB( CallBacker* )
{
    if ( faultview_ )
	faultview_->enablePainting( isChecked() );
}


void uiODView2DFaultTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    if ( emid != emid_ ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,f3d,emobj);
    if ( !f3d ) return;

    parent_->removeChild( this );
}


uiTreeItem* uiODView2DFaultTreeItemFactory::createForVis(
				const uiODViewer2D& vwr2d, Vis2DID id ) const
{
    mDynamicCastGet(const View2D::Fault*,obj,vwr2d.getObject(id));
    return obj ? new uiODView2DFaultTreeItem(id,true) : 0;
}
