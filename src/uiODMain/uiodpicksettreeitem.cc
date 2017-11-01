/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodpicksettreeitem.h"

#include "emmanager.h"
#include "emrandomposbody.h"
#include "pickset.h"
#include "randcolor.h"
#include "selector.h"
#include "survinfo.h"

#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodbodydisplaytreeitem.h"
#include "uiodscenemgr.h"
#include "uipickpartserv.h"
#include "uipickpropdlg.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "vispicksetdisplay.h"
#include "vispolylinedisplay.h"
#include "visrandomposbodydisplay.h"
#include "visselman.h"
#include "vissurvscene.h"

uiODPickSetParentTreeItem::uiODPickSetParentTreeItem()
    : uiODTreeItem( uiStrings::sPointSet())
{
    mAttachCB( Pick::Mgr().setToBeRemoved,
		uiODPickSetParentTreeItem::setRemovedCB );

}


uiODPickSetParentTreeItem::~uiODPickSetParentTreeItem()
{
    detachAllNotifiers();
}


const char* uiODPickSetParentTreeItem::iconName() const
{ return "tree-pickset"; }


void uiODPickSetParentTreeItem::addPickSet( Pick::Set* ps )
{
    if ( !ps ) return;

    uiODDisplayTreeItem* item = new uiODPickSetTreeItem( -1, *ps );
    addChild( item, false );
    item->setChecked( true );
}


void uiODPickSetParentTreeItem::setRemovedCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps ) return;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiODPickSetTreeItem*,itm,children_[idx])
	if ( !itm ) continue;
	if ( &itm->getSet() == ps )
	{
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    uiTreeItem::removeChild( itm );
	    return;
	}
    }
}


#define mLoadIdx	0
#define mEmptyIdx	1
#define mGen3DIdx	2
#define mRandom2DIdx	3
#define mSaveIdx	4
#define mDisplayIdx	5
#define mShowAllIdx	6
#define mMergeIdx	7


bool uiODPickSetParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    applMgr()->visServer()->getObject(sceneID()));
    const bool hastransform = scene && scene->getZAxisTransform();

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(m3Dots(tr("Add"))), mLoadIdx );
    uiMenu* newmnu = new uiMenu( getUiParent(), tr("New") );
    newmnu->insertItem( new uiAction(m3Dots(tr("Empty"))), mEmptyIdx );
    newmnu->insertItem( new uiAction(m3Dots(tr("Generate 3D"))), mGen3DIdx );
    if ( SI().has2D() )
	newmnu->insertItem( new uiAction(m3Dots(tr("Generate 2D"))),
                            mRandom2DIdx);
    mnu.insertItem( newmnu );

    if ( children_.size() > 0 )
    {
	mnu.insertSeparator();
	uiAction* filteritem =
	    new uiAction( tr("Display Only at Sections") );
	mnu.insertItem( filteritem, mDisplayIdx );
	filteritem->setEnabled( !hastransform );
	uiAction* shwallitem = new uiAction( tr("Display in Full") );
	mnu.insertItem( shwallitem, mShowAllIdx );
	shwallitem->setEnabled( !hastransform );
	mnu.insertSeparator();
	mnu.insertItem( new uiAction(m3Dots(tr("Merge Sets"))), mMergeIdx );
	mnu.insertItem( new uiAction(tr("Save Changes")), mSaveIdx );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid<0 )
	return false;

    Pick::SetMgr& psm = Pick::Mgr();

    if ( mnuid==mLoadIdx )
    {
	TypeSet<MultiID> mids;
	if ( !applMgr()->pickServer()->loadSets(mids,false) )
	    return false;

	for ( int idx=0; idx<mids.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<mids.size()-1 );
	    Pick::Set& ps = psm.get( mids[idx] );
	    addPickSet( &ps );
	}
    }
    else if ( mnuid==mGen3DIdx )
    {
	if ( !applMgr()->pickServer()->create3DGenSet() )
	    return false;
	Pick::Set& ps = psm.get( psm.size()-1 );
	addPickSet( &ps );
    }
    else if ( mnuid==mRandom2DIdx )
    {
	if ( !applMgr()->pickServer()->createRandom2DSet() )
	    return false;
	Pick::Set& ps = psm.get( psm.size()-1 );
	addPickSet( &ps );
    }
    else if ( mnuid==mEmptyIdx )
    {
	if ( !applMgr()->pickServer()->createEmptySet(false) )
	    return false;
	Pick::Set& ps = psm.get( psm.size()-1 );
	addPickSet( &ps );
    }
    else if ( mnuid==mSaveIdx )
    {
	if ( !applMgr()->pickServer()->storePickSets() )
	    uiMSG().error( tr("Problem saving changes. "
			      "Check write protection.") );
    }
    else if ( mnuid==mDisplayIdx || mnuid==mShowAllIdx )
    {
	const bool showall = mnuid==mShowAllIdx;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODPickSetTreeItem*,itm,children_[idx])
	    if ( !itm ) continue;

	    itm->setOnlyAtSectionsDisplay( !showall );
	    itm->updateColumnText( uiODSceneMgr::cColorColumn() );
	}
    }
    else if ( mnuid==mMergeIdx )
	{ MultiID mid; applMgr()->pickServer()->mergePickSets( mid ); }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem*
    uiODPickSetTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    if ( !psd ) return 0;

    Pick::Set* pickset = psd->getSet();
    return pickset->isPolygon() ? 0 : new uiODPickSetTreeItem(visid,*pickset);
}


uiODPickSetTreeItem::uiODPickSetTreeItem( int did, Pick::Set& ps )
    : set_(ps)
    , storemnuitem_(uiStrings::sSave())
    , storeasmnuitem_(m3Dots(uiStrings::sSaveAs()))
    , dirmnuitem_(m3Dots(tr("Set Directions")))
    , onlyatsectmnuitem_(tr("Only at Sections"))
    , propertymnuitem_(m3Dots(uiStrings::sProperties() ) )
    , convertbodymnuitem_( tr("Convert to Body") )
{
    displayid_ = did;
    Pick::Mgr().setChanged.notify( mCB(this,uiODPickSetTreeItem,setChg) );
    onlyatsectmnuitem_.checkable = true;

    mAttachCB( visBase::DM().selMan().selnotifier,
	       uiODPickSetTreeItem::selChangedCB );
    propertymnuitem_.iconfnm = "disppars";
    storemnuitem_.iconfnm = "save";
    storeasmnuitem_.iconfnm = "saveas";
}


uiODPickSetTreeItem::~uiODPickSetTreeItem()
{
    detachAllNotifiers();
    Pick::Mgr().removeCBs( this );
}


void uiODPickSetTreeItem::selChangedCB( CallBacker* )
{
    if ( !isSelected() )
	return;

    visserv_->setCurInterObjID( displayid_ );
}


bool uiODPickSetTreeItem::doubleClick( uiTreeViewItem* item )
{
    if ( item != uitreeviewitem_ )
	return uiTreeItem::doubleClick( item );

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));
    uiPickPropDlg dlg( getUiParent(), set_ , psd );
    return dlg.go();
}


void uiODPickSetTreeItem::setChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps || &set_!=ps ) return;

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));
    if ( psd ) psd->setName( mToUiStringTodo(ps->name()) );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


bool uiODPickSetTreeItem::init()
{
    if ( displayid_ == -1 )
    {
	visSurvey::PickSetDisplay* psd = new visSurvey::PickSetDisplay;
	displayid_ = psd->id();
	if ( set_.disp_.pixsize_>100 )
	    set_.disp_.pixsize_ = 3;
	psd->setSet( &set_ );
	visserv_->addObject( psd, sceneID(), true );
	psd->fullRedraw();
    }
    else
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv_->getObject(displayid_));
	if ( !psd ) return false;
	const MultiID setid = psd->getMultiID();
	NotifyStopper ntfstop( Pick::Mgr().setAdded );
	Pick::Mgr().set( setid, psd->getSet() );
    }

    updateColumnText( uiODSceneMgr::cColorColumn() );
    return uiODDisplayTreeItem::init();
}


void uiODPickSetTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    if ( istb )
    {
	mAddMenuItem( menu, &propertymnuitem_, true, false );
	const int setidx = Pick::Mgr().indexOf( set_ );
	const bool changed = setidx < 0 || Pick::Mgr().isChanged(setidx);
	mAddMenuItemCond( menu, &storemnuitem_, changed, false, changed );
	return;
    }

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));

    mAddMenuItem( menu, &convertbodymnuitem_, true, false )

    mAddMenuItem( menu, &displaymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &onlyatsectmnuitem_,
					   !set_.isReadOnly(),!psd->allShown());
    mAddMenuItem( &displaymnuitem_, &propertymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &dirmnuitem_, true, false );

    const int setidx = Pick::Mgr().indexOf( set_ );
    const bool changed = setidx < 0 || Pick::Mgr().isChanged(setidx);
    mAddMenuItem( menu, &storemnuitem_, changed, false );
    mAddMenuItem( menu, &storeasmnuitem_, true, false );
}


void uiODPickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( menu->isHandled() || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
	    visserv_->getObject(displayid_));

    if ( menu->menuID()!=displayID() )
	return;

    if ( mnuid==storemnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->storePickSet( set_ );
    }
    else if ( mnuid==storeasmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->storePickSetAs( set_ );
    }
    else if ( mnuid==dirmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->setPickSetDirs( set_ );
    }
    else if ( mnuid==onlyatsectmnuitem_.id )
    {
	menu->setIsHandled( true );
	if ( psd )
	    setOnlyAtSectionsDisplay( !displayedOnlyAtSections() );
    }
    else if ( mnuid==propertymnuitem_.id )
    {
	menu->setIsHandled( true );
	uiPickPropDlg dlg( getUiParent(), set_ , psd );
	dlg.go();
    }
    else if ( mnuid==convertbodymnuitem_.id )
    {
	menu->setIsHandled( true );

	RefMan<EM::EMObject> emobj =
	    EM::EMM().createTempObject( EM::RandomPosBody::typeStr() );
	mDynamicCastGet( EM::RandomPosBody*, emps, emobj.ptr() );
	if ( !emps )
	    return;

	emps->copyFrom( set_ );
	emps->setPreferredColor( set_.disp_.color_ );
	emps->setName( BufferString("Body from ",set_.name()) );
	emps->setChangedFlag();

	RefMan<visSurvey::RandomPosBodyDisplay> npsd =
	    new visSurvey::RandomPosBodyDisplay;

	npsd->setSelectable( false );
	npsd->setEMID( emps->id() );
	npsd->setDisplayTransformation( psd->getDisplayTransformation() );
	addChild( new uiODBodyDisplayTreeItem(npsd->id(),true), false );

        visserv_->addObject( npsd, sceneID(), true );
    }

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPickSetTreeItem::showAllPicks( bool yn )
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));
    psd->showAll( yn );
}


void uiODPickSetTreeItem::prepareForShutdown()
{
    uiODDisplayTreeItem::prepareForShutdown();
}


bool uiODPickSetTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    const int setidx = Pick::Mgr().indexOf( set_ );
    if ( set_.isReadOnly() || setidx<0 || !Pick::Mgr().isChanged(setidx) )
	return true;

    uiString warnstr = tr("This pickset has changed since the last save.\n\n"
			  "Do you want to save it?");
    const int retval = uiMSG().askSave( warnstr, withcancel );
    if ( retval == 0 )
	return true;
    else if ( retval == -1 )
	return false;
    else
	applMgr()->storePickSet( set_ );

    return true;
}



uiODPolygonParentTreeItem::uiODPolygonParentTreeItem()
    : uiODTreeItem( uiStrings::sPolygon() )
{

    mAttachCB( Pick::Mgr().setToBeRemoved,
		uiODPolygonParentTreeItem::setRemovedCB );
}


uiODPolygonParentTreeItem::~uiODPolygonParentTreeItem()
{
    detachAllNotifiers();
}


const char* uiODPolygonParentTreeItem::iconName() const
{ return "tree-polygon"; }


void uiODPolygonParentTreeItem::addPolygon( Pick::Set* ps )
{
    if ( !ps ) return;

    uiODDisplayTreeItem* item = new uiODPolygonTreeItem( -1, *ps );
    addChild( item, false );
}


void uiODPolygonParentTreeItem::setRemovedCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps ) return;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiODPolygonTreeItem*,itm,children_[idx])
	if ( !itm ) continue;
	if ( itm->getSet() == ps )
	{
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    uiTreeItem::removeChild( itm );
	    return;
	}
    }
}


#define mLoadPolyIdx	11
#define mNewPolyIdx	12
#define mSavePolyIdx	13
#define mOnlyAtPolyIdx	14
#define mAlwaysPolyIdx	15


bool uiODPolygonParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    applMgr()->visServer()->getObject(sceneID()));
    const bool hastransform = scene && scene->getZAxisTransform();

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(m3Dots(tr("Add"))), mLoadPolyIdx );
    mnu.insertItem( new uiAction(m3Dots(tr("New"))), mNewPolyIdx );

    if ( children_.size() > 0 )
    {
	mnu.insertSeparator();
	uiAction* filteritem =
	    new uiAction( tr("Display Only at Sections") );
	mnu.insertItem( filteritem, mOnlyAtPolyIdx );
	filteritem->setEnabled( !hastransform );
	uiAction* shwallitem = new uiAction( tr("Display in Full") );
	mnu.insertItem( shwallitem, mAlwaysPolyIdx );
	shwallitem->setEnabled( !hastransform );
	mnu.insertSeparator();
	mnu.insertItem( new uiAction(tr("Save Changes")), mSavePolyIdx );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid<0 )
	return false;

    if ( mnuid==mLoadPolyIdx )
    {
	TypeSet<MultiID> mids;
	if ( !applMgr()->pickServer()->loadSets(mids,true) )
	    return false;

	for ( int idx=0; idx<mids.size(); idx++ )
	{
	    Pick::Set& ps = Pick::Mgr().get( mids[idx] );
	    addPolygon( &ps );
	}
    }
    else if ( mnuid==mNewPolyIdx )
    {
	if ( !applMgr()->pickServer()->createEmptySet(true) )
	    return false;

	Pick::Set& ps = Pick::Mgr().get( Pick::Mgr().size()-1 );
	addPolygon( &ps );
    }
    else if ( mnuid==mSavePolyIdx )
    {
	if ( !applMgr()->pickServer()->storePickSets() )
	    uiMSG().error( tr("Problem saving changes. "
			      "Check write protection.") );
    }
    else if ( mnuid==mAlwaysPolyIdx || mnuid==mOnlyAtPolyIdx )
    {
	const bool showall = mnuid==mAlwaysPolyIdx;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODPolygonTreeItem*,itm,children_[idx])
	    if ( !itm ) continue;

	    itm->setOnlyAtSectionsDisplay( !showall );
	    itm->updateColumnText( uiODSceneMgr::cColorColumn() );
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem*
    uiODPolygonTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    if ( !psd ) return 0;

    Pick::Set* pickset = psd->getSet();
    return !pickset->isPolygon() ? 0 : new uiODPolygonTreeItem(visid,*pickset);
}


uiODPolygonTreeItem::uiODPolygonTreeItem( int did, Pick::Set& ps )
    : set_(ps)
    , storemnuitem_(uiStrings::sSave())
    , storeasmnuitem_(uiStrings::sSaveAs())
    , onlyatsectmnuitem_(tr("Only at Sections"))
    , propertymnuitem_(m3Dots(uiStrings::sProperties()))
    , closepolyitem_(tr("Close Polygon"))
{
    displayid_ = did;
    Pick::Mgr().setChanged.notify( mCB(this,uiODPolygonTreeItem,setChg) );
    onlyatsectmnuitem_.checkable = true;

    propertymnuitem_.iconfnm = "disppars";
    storemnuitem_.iconfnm = "save";
    storeasmnuitem_.iconfnm = "saveas";
}


uiODPolygonTreeItem::~uiODPolygonTreeItem()
{
    Pick::Mgr().removeCBs( this );
}


void uiODPolygonTreeItem::setChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps || &set_!=ps ) return;

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));
    if ( psd ) psd->setName(mToUiStringTodo( ps->name() ) );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


void uiODPolygonTreeItem::selChangedCB( CallBacker* )
{
    if ( !isSelected() )
	return;

    visserv_->setCurInterObjID( displayid_ );
}


bool uiODPolygonTreeItem::doubleClick( uiTreeViewItem* item )
{
    if ( item != uitreeviewitem_ )
	return uiTreeItem::doubleClick( item );

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));
    // TODO: Make polygon specific properties dialog
    uiPickPropDlg dlg( getUiParent(), set_ , psd );
    return dlg.go();
}


bool uiODPolygonTreeItem::init()
{
    if ( displayid_ == -1 )
    {
	visSurvey::PickSetDisplay* psd = new visSurvey::PickSetDisplay;
	displayid_ = psd->id();
	if ( set_.disp_.pixsize_>100 )
	    set_.disp_.pixsize_ = 3;
	psd->setSet( &set_ );
	visserv_->addObject( psd, sceneID(), true );
	psd->fullRedraw();
    }
    else
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv_->getObject(displayid_));
	if ( !psd ) return false;
	const MultiID setid = psd->getMultiID();
	NotifyStopper ntfstop( Pick::Mgr().setAdded );
	Pick::Mgr().set( setid, psd->getSet() );
    }

    updateColumnText( uiODSceneMgr::cColorColumn() );
    return uiODDisplayTreeItem::init();
}


void uiODPolygonTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    if ( istb )
    {
	mAddMenuItem( menu, &propertymnuitem_, true, false );
	const int setidx = Pick::Mgr().indexOf( set_ );
	const bool changed = setidx < 0 || Pick::Mgr().isChanged(setidx);
	mAddMenuItemCond( menu, &storemnuitem_, changed, false, changed );
	return;
    }

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));

    if ( set_.disp_.connect_ == Pick::Set::Disp::Open )
	mAddMenuItem( menu, &closepolyitem_, true, false )
    else
	mResetMenuItem( &closepolyitem_ );

    mAddMenuItem( menu, &displaymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &onlyatsectmnuitem_,
					   !set_.isReadOnly(),!psd->allShown());
    mAddMenuItem( &displaymnuitem_, &propertymnuitem_, true, false );

    const int setidx = Pick::Mgr().indexOf( set_ );
    const bool changed = setidx < 0 || Pick::Mgr().isChanged(setidx);
    mAddMenuItem( menu, &storemnuitem_, changed, false );
    mAddMenuItem( menu, &storeasmnuitem_, true, false );
}


void uiODPolygonTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( menu->isHandled() || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
	    visserv_->getObject(displayid_));

    if ( menu->menuID()!=displayID() )
	return;

    if ( set_.disp_.connect_==Pick::Set::Disp::Open
	 && mnuid==closepolyitem_.id )
    {
	menu->setIsHandled( true );
	set_.disp_.connect_ = Pick::Set::Disp::Close;
	Pick::Mgr().reportDispChange( this, set_ );
    }
    else if ( mnuid==storemnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->storePickSet( set_ );
    }
    else if ( mnuid==storeasmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->storePickSetAs( set_ );
    }
    else if ( mnuid==onlyatsectmnuitem_.id )
    {
	menu->setIsHandled( true );
	if ( psd )
	    setOnlyAtSectionsDisplay( !displayedOnlyAtSections() );
    }
    else if ( mnuid==propertymnuitem_.id )
    {
	menu->setIsHandled( true );
	uiPickPropDlg dlg( getUiParent(), set_ , psd );
	dlg.go();
    }

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPolygonTreeItem::showAllPicks( bool yn )
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));
    psd->showAll( yn );
}


void uiODPolygonTreeItem::prepareForShutdown()
{
    uiODDisplayTreeItem::prepareForShutdown();
}


bool uiODPolygonTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    const int setidx = Pick::Mgr().indexOf( set_ );
    if ( setidx < 0 || !Pick::Mgr().isChanged(setidx) )
	return true;

    uiString warnstr = tr("This polygon has changed since the last save.\n\n"
			  "Do you want to save it?");
    const int retval = uiMSG().askSave( warnstr, withcancel );
    if ( retval == 0 )
	return true;
    else if ( retval == -1 )
	return false;
    else
	applMgr()->storePickSet( set_ );

    return true;
}
