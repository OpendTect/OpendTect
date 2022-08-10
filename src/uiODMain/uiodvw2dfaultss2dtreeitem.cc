/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
________________________________________________________________________

-*/

#include "uiodvw2dfaultss2dtreeitem.h"

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
#include "emfaultstickset.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "randcolor.h"

#include "visseis2ddisplay.h"
#include "view2ddataman.h"
#include "view2dfaultss2d.h"


uiODVw2DFaultSS2DParentTreeItem::uiODVw2DFaultSS2DParentTreeItem()
    : uiODVw2DTreeItem( uiStrings::sFaultStickSet() )
{}


uiODVw2DFaultSS2DParentTreeItem::~uiODVw2DFaultSS2DParentTreeItem()
{
}


bool uiODVw2DFaultSS2DParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.addMenu( createAddMenu() );
    mnu.insertAction( new uiAction(uiStrings::sNew()), getNewItemID() );
    insertStdSubMenu( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DFaultSS2DParentTreeItem::handleSubMenu( int mnuid )
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
	addNewTempFaultSS2D( emo->id() );
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
	    addFaultSS2Ds( emids );
	    applMgr()->viewer2DMgr().addFaultSS2Ds( emids );
	    applMgr()->viewer2DMgr().addFaultSSs( emids );
	}
	else
	    addFaultSS2Ds( emids );

	deepUnRef( objs );
    }

    return true;
}


const char* uiODVw2DFaultSS2DParentTreeItem::iconName() const
{ return "tree-fltss"; }


bool uiODVw2DFaultSS2DParentTreeItem::init()
{ return uiODVw2DTreeItem::init(); }


void uiODVw2DFaultSS2DParentTreeItem::getFaultSS2DVwr2DIDs(
	EM::ObjectID emid, TypeSet<int>& vw2dobjids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DFaultSS2DTreeItem*,faultssitem,
			getChild(idx))
	if ( !faultssitem || faultssitem->emObjectID() != emid )
	    continue;

	vw2dobjids.addIfNew( faultssitem->vw2DObject()->id() );
    }
}


void uiODVw2DFaultSS2DParentTreeItem::getLoadedFaultSS2Ds(
	TypeSet<EM::ObjectID>& emids ) const
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(const uiODVw2DFaultSS2DTreeItem*,fss2ditm,getChild(idx))
	if ( !fss2ditm )
	    continue;
	emids.addIfNew( fss2ditm->emObjectID() );
    }
}


void uiODVw2DFaultSS2DParentTreeItem::removeFaultSS2D( EM::ObjectID emid )
{
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DFaultSS2DTreeItem*,faultitem,getChild(idx))
	if ( !faultitem || emid!=faultitem->emObjectID() )
	    continue;
	removeChild( faultitem );
    }
}


void uiODVw2DFaultSS2DParentTreeItem::addFaultSS2Ds(
					const TypeSet<EM::ObjectID>& emids )
{
    TypeSet<EM::ObjectID> emidstobeloaded, emidsloaded;
    getLoadedFaultSS2Ds( emidsloaded );
    for ( int idx=0; idx<emids.size(); idx++ )
    {
	if ( !emidsloaded.isPresent(emids[idx]) )
	    emidstobeloaded.addIfNew( emids[idx] );
    }

    for ( int idx=0; idx<emidstobeloaded.size(); idx++ )
    {
	const EM::EMObject* emobj = EM::EMM().getObject( emidstobeloaded[idx] );
	if ( !emobj || findChild(emobj->name()) )
	    continue;

	MPE::ObjectEditor* editor =
	    MPE::engine().getEditor( emobj->id(), false );
	uiODVw2DFaultSS2DTreeItem* childitem =
	    new uiODVw2DFaultSS2DTreeItem( emidstobeloaded[idx] );
	addChld( childitem, false, false );
	if ( editor )
	    editor->addUser();
    }
}


void uiODVw2DFaultSS2DParentTreeItem::setupNewTempFaultSS2D( EM::ObjectID emid )
{
    TypeSet<EM::ObjectID> emidsloaded;
    getLoadedFaultSS2Ds( emidsloaded );
    if ( !emidsloaded.isPresent(emid) )
	return;

    bool hasfss2d = false;
    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DFaultSS2DTreeItem*,fss2dtreeitm,getChild(idx))
	if ( !fss2dtreeitm || emid!=fss2dtreeitm->emObjectID() )
	    continue;

	hasfss2d = true;
	fss2dtreeitm->select();
    }

    if ( viewer2D() && viewer2D()->viewControl() && hasfss2d )
	viewer2D()->viewControl()->setEditMode( true );
}


void uiODVw2DFaultSS2DParentTreeItem::addNewTempFaultSS2D( EM::ObjectID emid )
{
    TypeSet<EM::ObjectID> emidsloaded;
    getLoadedFaultSS2Ds( emidsloaded );
    if ( emidsloaded.isPresent(emid) )
	return;

    uiODVw2DFaultSS2DTreeItem* fss2ditem = new uiODVw2DFaultSS2DTreeItem( emid);
    addChld( fss2ditem,false, false );
    if ( viewer2D() && viewer2D()->viewControl() )
	viewer2D()->viewControl()->setEditMode( true );
    fss2ditem->select();
}


void uiODVw2DFaultSS2DParentTreeItem::tempObjAddedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
    if ( !fss ) return;

    if ( findChild(applMgr()->EMServer()->getUiName(emid).getFullString()) )
	return;

    addChld( new uiODVw2DFaultSS2DTreeItem(emid),false,false);
}



uiODVw2DFaultSS2DTreeItem::uiODVw2DFaultSS2DTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(uiString::emptyString())
    , emid_(emid)
    , fssview_(0)
{
}


uiODVw2DFaultSS2DTreeItem::uiODVw2DFaultSS2DTreeItem( int id, bool )
    : uiODVw2DTreeItem(uiString::emptyString())
    , emid_(-1)
    , fssview_(0)
{
    displayid_ = id;
}


uiODVw2DFaultSS2DTreeItem::~uiODVw2DFaultSS2DTreeItem()
{
    detachAllNotifiers();
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
	emobj->unRef();

    if ( fssview_ )
	viewer2D()->dataMgr()->removeObject( fssview_ );
}


bool uiODVw2DFaultSS2DTreeItem::init()
{
    uitreeviewitem_->setCheckable(true);
    EM::EMObject* emobj = 0;
    if ( displayid_ < 0 )
    {
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	fssview_ = View2D::FaultSS2D::create( viewer2D()->viewwin(),
					  viewer2D()->dataEditor() );
	fssview_->setEMObjectID( emid_ );
	viewer2D()->dataMgr()->addObject( fssview_ );
	displayid_ = fssview_->id();
    }
    else
    {
	mDynamicCastGet(View2D::FaultSS2D*,fd,
			viewer2D()->getObject(displayid_))
	if ( !fd )
	    return false;

	emid_ = fd->getEMObjectID();
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	fssview_ = fd;
    }

    emobj->ref();
    mAttachCB( emobj->change, uiODVw2DFaultSS2DTreeItem::emobjChangeCB );

    name_ = applMgr()->EMServer()->getUiName( emid_ );
    uitreeviewitem_->setChecked( true );
    mAttachCB( checkStatusChange(), uiODVw2DFaultSS2DTreeItem::checkCB );

    displayMiniCtab();

    if ( viewer2D()->geomID() != Survey::GeometryManager::cUndefGeomID() )
	fssview_->setGeomID( viewer2D()->geomID() );

    mAttachCB( fssview_->deSelection(), uiODVw2DFaultSS2DTreeItem::deSelCB );

    fssview_->draw();

    if ( viewer2D() && viewer2D()->viewControl() )
	mAttachCB( viewer2D()->viewControl()->editPushed(),
		   uiODVw2DFaultSS2DTreeItem::enableKnotsCB );

    uiODVw2DTreeItem::addKeyBoardEvent( emid_ );

    return true;
}


void uiODVw2DFaultSS2DTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				emobj->preferredColor() );
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


void uiODVw2DFaultSS2DTreeItem::enableKnotsCB( CallBacker* )
{
    if ( fssview_ && viewer2D()->dataMgr()->selectedID() == fssview_->id() )
	fssview_->selected();
}


bool uiODVw2DFaultSS2DTreeItem::select()
{
    if ( uitreeviewitem_->treeView() )
	uitreeviewitem_->treeView()->deselectAll();

    uitreeviewitem_->setSelected( true);
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

bool uiODVw2DFaultSS2DTreeItem::showSubMenu()
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
	lsfld->changed.notify( mCB(this,uiODVw2DFaultSS2DTreeItem,propChgCB) );
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
	    applMgr()->viewer2DMgr().removeFaultSS( emid_ );
	    applMgr()->viewer2DMgr().removeFaultSS2D( emid_ );
	}
	if ( doremove )
	    parent_->removeChild( this );
    }

    return true;
}


void uiODVw2DFaultSS2DTreeItem::propChgCB( CallBacker* cb )
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


void uiODVw2DFaultSS2DTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DFaultSS2DTreeItem::checkCB( CallBacker* )
{
    if ( fssview_ )
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


uiTreeItem* uiODVw2DFaultSS2DTreeItemFactory::createForVis(
				const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet(const View2D::FaultSS2D*,obj,vwr2d.getObject(id));
    return obj ? new uiODVw2DFaultSS2DTreeItem(id,true) : 0;
}
