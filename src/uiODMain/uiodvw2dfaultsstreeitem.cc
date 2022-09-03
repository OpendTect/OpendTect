/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodvw2dfaultsstreeitem.h"

#include "uicolor.h"
#include "uiempartserv.h"
#include "uiflatviewstdcontrol.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uipixmap.h"
#include "uisellinest.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "emeditor.h"
#include "emfaultstickset.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "randcolor.h"

#include "view2dfaultss3d.h"
#include "view2ddataman.h"


uiODView2DFaultSSParentTreeItem::uiODView2DFaultSSParentTreeItem()
    : uiODView2DTreeItem( uiStrings::sFaultStickSet())
{
}


uiODView2DFaultSSParentTreeItem::~uiODView2DFaultSSParentTreeItem()
{
}


bool uiODView2DFaultSSParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.addMenu( createAddMenu() );
    mnu.insertAction( new uiAction(uiStrings::sNew()), getNewItemID() );
    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODView2DFaultSSParentTreeItem::handleSubMenu( int mnuid )
{
    handleStdSubMenu( mnuid );

    if ( mnuid == getNewItemID() )
    {
	RefMan<EM::EMObject> emo =
		EM::EMM().createTempObject( EM::FaultStickSet::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( OD::getRandomColor(false) );
	emo->setNewName();
	emo->setFullyLoaded( true );
	addNewTempFaultSS( emo->id() );
	applMgr()->viewer2DMgr().addNewTempFaultSS(
		emo->id(), viewer2D()->getSyncSceneID() );
	applMgr()->viewer2DMgr().addNewTempFaultSS2D(
		emo->id(), viewer2D()->getSyncSceneID() );
    }
    else if ( isAddItem(mnuid,true) || isAddItem(mnuid,false) )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaultStickSets( objs, getUiParent() );
	TypeSet<EM::ObjectID> emids;
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


const char* uiODView2DFaultSSParentTreeItem::iconName() const
{ return "tree-fltss"; }


bool uiODView2DFaultSSParentTreeItem::init()
{ return uiODView2DTreeItem::init(); }


void uiODView2DFaultSSParentTreeItem::getFaultSSVwr2DIDs(
	EM::ObjectID emid, TypeSet<Vis2DID>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODView2DFaultSSTreeItem*,faultssitem,
			getChild(idx))
	if ( !faultssitem || faultssitem->emObjectID() != emid )
	    continue;

	vw2dobjids.addIfNew( faultssitem->vw2DObject()->id() );
    }
}


void uiODView2DFaultSSParentTreeItem::getLoadedFaultSSs(
	TypeSet<EM::ObjectID>& emids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODView2DFaultSSTreeItem*,faultitem,
			getChild(idx))
	if ( !faultitem )
	    continue;
	emids.addIfNew( faultitem->emObjectID() );
    }
}


void uiODView2DFaultSSParentTreeItem::removeFaultSS( EM::ObjectID emid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSSTreeItem*,faultitem,getChild(idx))
	if ( !faultitem || emid!=faultitem->emObjectID() )
	    continue;
	removeChild( faultitem );
    }
}


void uiODView2DFaultSSParentTreeItem::addFaultSSs(
					const TypeSet<EM::ObjectID>& emids )
{
    TypeSet<EM::ObjectID> emidstobeloaded, emidsloaded;
    getLoadedFaultSSs( emidsloaded );
    for ( int idx=0; idx<emids.size(); idx++ )
    {
	if ( !emidsloaded.isPresent(emids[idx]) )
	    emidstobeloaded.addIfNew( emids[idx] );
    }

    for ( int idx=0; idx<emidstobeloaded.size(); idx++ )
    {
	const EM::EMObject* emobj = EM::EMM().getObject( emidstobeloaded[idx] );
	if ( !emobj || findChild(emobj->name().buf()) )
	    continue;

	MPE::ObjectEditor* editor =
	    MPE::engine().getEditor( emobj->id(), false );
	uiODView2DFaultSSTreeItem* childitem =
	    new uiODView2DFaultSSTreeItem( emidstobeloaded[idx] );
	addChld( childitem ,false, false );
	if ( editor )
	    editor->addUser();
    }
}


void uiODView2DFaultSSParentTreeItem::setupNewTempFaultSS( EM::ObjectID emid )
{
    TypeSet<EM::ObjectID> emidsloaded;
    getLoadedFaultSSs( emidsloaded );
    if ( !emidsloaded.isPresent(emid) )
	return;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSSTreeItem*,fltsstreeitm,getChild(idx))
	if ( fltsstreeitm && emid==fltsstreeitm->emObjectID() )
	{
	    if ( viewer2D() && viewer2D()->viewControl() )
		viewer2D()->viewControl()->setEditMode( true );
	    fltsstreeitm->select();
	    break;
	}
    }

}


void uiODView2DFaultSSParentTreeItem::addNewTempFaultSS( EM::ObjectID emid )
{
    TypeSet<EM::ObjectID> emidsloaded;
    getLoadedFaultSSs( emidsloaded );
    if ( emidsloaded.isPresent(emid) )
	return;

    auto* faulttreeitem = new uiODView2DFaultSSTreeItem(emid);
    addChld( faulttreeitem,false, false );
    if ( viewer2D() && viewer2D()->viewControl() )
	viewer2D()->viewControl()->setEditMode( true );
    faulttreeitem->select();
}


uiODView2DFaultSSTreeItem::uiODView2DFaultSSTreeItem( const EM::ObjectID& emid )
    : uiODView2DTreeItem(uiString::emptyString())
    , emid_(emid)
    , fssview_(0)
{}


uiODView2DFaultSSTreeItem::uiODView2DFaultSSTreeItem( Vis2DID id, bool )
    : uiODView2DTreeItem(uiString::emptyString())
    , fssview_(0)
{
    displayid_ = id;
}


uiODView2DFaultSSTreeItem::~uiODView2DFaultSSTreeItem()
{
    detachAllNotifiers();
    MPE::engine().removeEditor( emid_ );
    if ( fssview_ )
	viewer2D()->dataMgr()->removeObject( fssview_ );
}


bool uiODView2DFaultSSTreeItem::init()
{
    EM::EMObject* emobj = nullptr;
    if ( !displayid_.isValid() )
    {
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj )
	    return false;

	fssview_ = View2D::FaultSS3D::create( viewer2D()->viewwin(),
					  viewer2D()->dataEditor() );
	fssview_->setEMObjectID( emid_ );
	viewer2D()->dataMgr()->addObject( fssview_ );
	displayid_ = fssview_->id();
    }
    else
    {
	mDynamicCastGet(View2D::FaultSS3D*,hd,
			viewer2D()->getObject(displayid_))
	if ( !hd )
	    return false;
	emid_ = hd->getEMObjectID();
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	fssview_ = hd;
    }

    mAttachCB( emobj->change, uiODView2DFaultSSTreeItem::emobjChangeCB );
    displayMiniCtab();
    name_ = applMgr()->EMServer()->getUiName( emid_ );
    uitreeviewitem_->setCheckable(true);
    uitreeviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODView2DFaultSSTreeItem,checkCB) );

    fssview_->draw();

    if ( viewer2D() && viewer2D()->viewControl() )
	mAttachCB( viewer2D()->viewControl()->editPushed(),
		   uiODView2DFaultSSTreeItem::enableKnotsCB );

    NotifierAccess* deselnotify =  fssview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODView2DFaultSSTreeItem,deSelCB) );

    uiODView2DTreeItem::addKeyBoardEvent( emid_ );

    return true;
}


void uiODView2DFaultSSTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				emobj->preferredColor() );
}


void uiODView2DFaultSSTreeItem::emobjChangeCB( CallBacker* cb )
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


void uiODView2DFaultSSTreeItem::enableKnotsCB( CallBacker* )
{
    if ( fssview_ && viewer2D()->dataMgr()->selectedID() == fssview_->id() )
	fssview_->selected();
}


bool uiODView2DFaultSSTreeItem::select()
{
    if ( uitreeviewitem_->treeView() )
	uitreeviewitem_->treeView()->deselectAll();

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

bool uiODView2DFaultSSTreeItem::showSubMenu()
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
	lsfld->changed.notify( mCB(this,uiODView2DFaultSSTreeItem,propChgCB) );
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
	{
	    applMgr()->viewer2DMgr().removeFaultSS2D( emid_ );
	    applMgr()->viewer2DMgr().removeFaultSS( emid_ );
	}
	if ( doremove )
	    parent_->removeChild( this );
    }

    return true;
}


void uiODView2DFaultSSTreeItem::propChgCB( CallBacker* cb )
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


void uiODView2DFaultSSTreeItem::updateCS( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd && fssview_ )
	fssview_->setTrcKeyZSampling( cs, upd );
}


void uiODView2DFaultSSTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODView2DFaultSSTreeItem::checkCB( CallBacker* )
{
    if ( fssview_ )
	fssview_->enablePainting( isChecked() );
}


void uiODView2DFaultSSTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    if ( emid != emid_ ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
    if ( !fss ) return;

    parent_->removeChild( this );
}


uiTreeItem* uiODView2DFaultSSTreeItemFactory::createForVis(
				const uiODViewer2D& vwr2d, Vis2DID id ) const
{
    mDynamicCastGet(const View2D::FaultSS3D*,obj,vwr2d.getObject(id));
    return obj ? new uiODView2DFaultSSTreeItem(id,true) : 0;
}
