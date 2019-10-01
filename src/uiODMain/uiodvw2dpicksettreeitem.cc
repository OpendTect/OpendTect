/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
________________________________________________________________________

-*/

#include "uiodvw2dpicksettreeitem.h"

#include "uiflatviewstdcontrol.h"
#include "uimenu.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodpicksettreeitem.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uipickpartserv.h"
#include "uipickpropdlg.h"
#include "uisetpickdirs.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uigraphicsview.h"

#include "keyboardevent.h"
#include "picksetmanager.h"
#include "view2ddataman.h"
#include "view2dpickset.h"


uiODVw2DPickSetParentTreeItem::uiODVw2DPickSetParentTreeItem()
    : uiODVw2DParentTreeItem( uiStrings::sPointSet() )
{
}


uiODVw2DPickSetParentTreeItem::~uiODVw2DPickSetParentTreeItem()
{
}


const char* uiODVw2DPickSetParentTreeItem::iconName() const
{ return "tree-pickset"; }


const char* uiODVw2DPickSetParentTreeItem::childObjTypeKey() const
{ return Pick::SetPresentationInfo::sFactoryKey(); }


bool uiODVw2DPickSetParentTreeItem::init()
{ return uiODVw2DParentTreeItem::init(); }



bool uiODVw2DPickSetParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), 0 );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sNew())), 1 );
    addStandardItems( mnu );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DPickSetParentTreeItem::handleSubMenu( int menuid )
{
    handleStandardItems( menuid );

    DBKeySet setids;
    RefMan<Pick::Set> newps = 0;
    if ( menuid == 1  )
    {
	newps = applMgr()->pickServer()->createEmptySet( false );
	if ( !newps )
	    return false;

	setids += Pick::SetMGR().getID( *newps );
    }
    else if ( menuid == 0 &&
	      !applMgr()->pickServer()->loadSets(setids,false) )
	return false;

    if ( setids.isEmpty() )
	return true;


    Presentation::ObjInfoSet prinfos;
    for ( int idx=0; idx<setids.size(); idx++ )
	prinfos.add( new Pick::SetPresentationInfo(setids[idx]) );

    addChildren( prinfos );
    for ( int idx=0; idx<prinfos.size(); idx++ )
	emitChildPrRequest( *prinfos.get(idx), Presentation::Add );

    if ( viewer2D() && viewer2D()->viewControl() )
	viewer2D()->viewControl()->setEditMode(
		newps && selectChild(*prinfos.get(0)) );


    return true;
}


uiPresManagedTreeItem* uiODVw2DPickSetParentTreeItem::addChildItem(
	const Presentation::ObjInfo& prinfo )
{
    mDynamicCastGet(const Pick::SetPresentationInfo*,pickprinfo,&prinfo);
    if ( !pickprinfo )
	return 0;

    RefMan<Pick::Set> ps = Pick::SetMGR().fetchForEdit( pickprinfo->storedID());
    ps.setNoDelete( true );
    if ( !ps || ps->isPolygon() )
	return 0;

    uiODVw2DPickSetTreeItem* childitm = new uiODVw2DPickSetTreeItem( *ps );
    addChld( childitm, false, false);
    return childitm;
}


uiODVw2DPickSetTreeItem::uiODVw2DPickSetTreeItem( Pick::Set& ps )
    : uiODVw2DTreeItem(uiString::empty())
    , pickset_(ps)
    , vw2dpickset_(0)
{
    pickset_.ref();
    storedid_ = Pick::SetMGR().getID( pickset_ );
}


bool uiODVw2DPickSetTreeItem::init()
{
    vw2dpickset_ =
	VW2DPickSet::create( storedid_, viewer2D()->viewwin(),
			     viewer2D()->dataEditor() );
    if ( !vw2dpickset_ )
	{ pErrMsg("Factory returns null"); return false; }

    viewer2D()->dataMgr()->addObject( vw2dpickset_ );
    displayid_ = vw2dpickset_->id();

    name_ = toUiString( pickset_.name() );
    uitreeviewitem_->setCheckable( true );
    uitreeviewitem_->setChecked( true );
    displayMiniCtab();
    vw2dpickset_->draw();

    addKeyBoardEvent();
    mAttachCB( checkStatusChange(), uiODVw2DPickSetTreeItem::checkCB );
    mAttachCB( pickset_.objectChanged(),
	       uiODVw2DPickSetTreeItem::setChangedCB );
    return true;
}


uiODVw2DPickSetTreeItem::~uiODVw2DPickSetTreeItem()
{
    detachAllNotifiers();
    if ( vw2dpickset_ )
	viewer2D()->dataMgr()->removeObject( vw2dpickset_ );
    pickset_.unRef();
}



Presentation::ObjInfo* uiODVw2DPickSetTreeItem::getObjPrInfo() const
{
    Pick::SetPresentationInfo* psprinfo = new Pick::SetPresentationInfo;
    psprinfo->setStoredID( storedid_ );
    return psprinfo;
}


void uiODVw2DPickSetTreeItem::setChangedCB( CallBacker* )
{
    displayMiniCtab();
}


void uiODVw2DPickSetTreeItem::displayMiniCtab()
{
    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				pickset_.dispColor() );
}


bool uiODVw2DPickSetTreeItem::select()
{
    if ( uitreeviewitem_->treeView() )
	uitreeviewitem_->treeView()->clearSelection();

    uitreeviewitem_->setSelected( true );
    if ( vw2dpickset_ )
    {
	viewer2D()->dataMgr()->setSelected( vw2dpickset_ );
	vw2dpickset_->selected();
    }
    return true;
}

#define mPropID		0
#define mSaveID		1
#define mSaveAsID	2
#define mRemoveID	3
#define mDirectionID	4

bool uiODVw2DPickSetTreeItem::showSubMenu()
{
    const bool haschanged = Pick::SetMGR().needsSave( pickset_ );

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    addAction( mnu, m3Dots(uiStrings::sProperties()), mPropID, "disppars" );
    addAction( mnu, m3Dots(tr("Set Directions")), mDirectionID );
    addAction( mnu, uiStrings::sSave(), mSaveID, "save", haschanged );
    addAction( mnu, m3Dots(uiStrings::sSaveAs()), mSaveAsID, "saveas", true );
    addAction( mnu, uiStrings::sRemove(), mRemoveID, "remove" );

    const int mnuid = mnu.exec();
    switch ( mnuid )
    {
	case mPropID:
	{
	    uiPickPropDlg dlg( getUiParent(), pickset_, 0 );
	    dlg.go();
	} break;
	case mDirectionID:
	    applMgr()->setPickSetDirs( pickset_ );
	    break;
	case mSaveID:
	    doSave();
	    break;
	case mSaveAsID:
	    doSaveAs();
	    break;
	case mRemoveID:
	{
	    if ( Pick::SetMGR().needsSave(pickset_) )
	    {
		const int res = mTIUiMsg().askSave(
		    tr("PointSet '%1' has been modified. "
		       "Do you want to save it?").arg( pickset_.name() ) );
		if ( res < 0 )
		    return false;
		else if ( res == 1 )
		    doSave();
	    }

	    this->prepareForShutdown();
	    parent_->removeChild( this );
	} break;
    }

    return true;
}


void uiODVw2DPickSetTreeItem::doSave()
{
    applMgr()->storePickSet( pickset_ );
}


void uiODVw2DPickSetTreeItem::doSaveAs()
{
    applMgr()->storePickSetAs( pickset_ );
}


void uiODVw2DPickSetTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DPickSetTreeItem::handleItemCheck( bool triggervwreq )
{
    if ( vw2dpickset_ )
	vw2dpickset_->enablePainting( isChecked() );

    if ( triggervwreq )
	emitPrRequest( isChecked() ? Presentation::Show : Presentation::Hide );
}


void uiODVw2DPickSetTreeItem::checkCB( CallBacker* )
{
    handleItemCheck( true );
}


uiTreeItem* uiODVw2DPickSetTreeItemFactory::createForVis(
				    const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet( const VW2DPickSet*, vw2dps,
			vwr2d.dataMgr()->getObject(id) );
    if ( !vw2dps )
	return 0;

    RefMan<Pick::Set> ps = Pick::SetMGR().fetchForEdit( vw2dps->pickSetID() );
    if ( !ps )
	{ pErrMsg("Refcount should prevent this"); return 0; }

    return new uiODVw2DPickSetTreeItem( *ps );
}


const Vw2DDataObject* uiODVw2DPickSetTreeItem::vw2DObject() const
{ return vw2dpickset_; }
