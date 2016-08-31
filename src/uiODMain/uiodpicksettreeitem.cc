/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/

#include "uiodpicksettreeitem.h"

#include "emmanager.h"
#include "emrandomposbody.h"
#include "picksetmanager.h"
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
#include "uishortcutsmgr.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "odpresentationmgr.h"
#include "vispicksetdisplay.h"
#include "vispolylinedisplay.h"
#include "visrandomposbodydisplay.h"
#include "visselman.h"
#include "vissurvscene.h"

uiODPickSetParentTreeItem::uiODPickSetParentTreeItem()
    : uiODSceneParentTreeItem( uiStrings::sPickSet())
{
}


uiODPickSetParentTreeItem::~uiODPickSetParentTreeItem()
{
    detachAllNotifiers();
}


const char* uiODPickSetParentTreeItem::iconName() const
{
    return "tree-pickset";
}


const char* uiODPickSetParentTreeItem::childObjTypeKey() const
{
    return Pick::SetPresentationInfo::sFactoryKey();
}


void uiODPickSetParentTreeItem::addChildItem( const MultiID& mid )
{
    RefMan<Pick::Set> ps = Pick::SetMGR().fetchForEdit( mid );
    ps.setNoDelete( true );
    if ( !ps || ps->isPolygon() )
	return;

    uiODDisplayTreeItem* item = new uiODPickSetTreeItem( -1, *ps );
	addChild( item, false );
    item->setChecked( true );
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

    if ( mnuid==mLoadIdx )
    {
	TypeSet<MultiID> mids;
	if ( !applMgr()->pickServer()->loadSets(mids,false) )
	    return false;

	for ( int chidx=0; chidx<mids.size(); chidx++ )
	{
	    addChildItem( mids[chidx] );
	    emitChildPRRequest( mids[chidx], OD::Add );
	}
    }
    else if ( mnuid==mGen3DIdx )
    {
	RefMan<Pick::Set> ps = applMgr()->pickServer()->create3DGenSet();
	if ( !ps )
	    return false;

	const MultiID storedid = Pick::SetMGR().getID( *ps );
	addChildItem( storedid );
	emitChildPRRequest( storedid, OD::Add );
    }
    else if ( mnuid==mRandom2DIdx )
    {
	RefMan<Pick::Set> ps = applMgr()->pickServer()->createRandom2DSet();
	if ( !ps )
	    return false;

	const MultiID storedid = Pick::SetMGR().getID( *ps );
	addChildItem( storedid );
	emitChildPRRequest( storedid, OD::Add );
    }
    else if ( mnuid==mEmptyIdx )
    {
	RefMan<Pick::Set> ps = applMgr()->pickServer()->createEmptySet(false);
	if ( !ps )
	    return false;

	const MultiID storedid = Pick::SetMGR().getID( *ps );
	addChildItem( storedid );
	emitChildPRRequest( storedid, OD::Add );
    }
    else if ( mnuid==mSaveIdx )
    {
	applMgr()->storePickSets( -1 );
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
    , convertbodymnuitem_(tr("Convert to Body"))
    , undomnuitem_(uiString::emptyString())
    , redomnuitem_(uiString::emptyString())
{
    set_.ref();
    storedid_ = Pick::SetMGR().getID( set_ );
    displayid_ = did;
    onlyatsectmnuitem_.checkable = true;

    mAttachCB( visBase::DM().selMan().selnotifier,
	       uiODPickSetTreeItem::selChangedCB );
    propertymnuitem_.iconfnm = "disppars";
    storemnuitem_.iconfnm = "save";
    storeasmnuitem_.iconfnm = "saveas";
    undomnuitem_.iconfnm = "undo";
    redomnuitem_.iconfnm = "redo";

    mAttachCB( set_.objectChanged(), uiODPickSetTreeItem::setChgCB );
}


uiODPickSetTreeItem::~uiODPickSetTreeItem()
{
    detachAllNotifiers();
    emitPRRequest( OD::Vanish );
    set_.unRef();
}


const char* uiODPickSetTreeItem::objectTypeKey() const
{
    return Pick::SetPresentationInfo::sFactoryKey();
}


OD::ObjPresentationInfo* uiODPickSetTreeItem::getObjPRInfo()
{
    Pick::SetPresentationInfo* psprinfo = new Pick::SetPresentationInfo();
    psprinfo->setStoredID( storedid_ );
    return psprinfo;
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


void uiODPickSetTreeItem::setChgCB( CallBacker* inpcb )
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));
    if ( psd )
	psd->setName( toUiString(set_.name()) );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


bool uiODPickSetTreeItem::init()
{
    if ( displayid_ == -1 )
    {
	visSurvey::PickSetDisplay* psd = new visSurvey::PickSetDisplay;
	displayid_ = psd->id();
	if ( set_.dispSize() > 100 )
	    set_.setDispSize( 3 );
	psd->setSet( &set_ );
	visserv_->addObject( psd, sceneID(), true );
	psd->fullRedraw();
    }

    updateColumnText( uiODSceneMgr::cColorColumn() );
    return uiODDisplayTreeItem::init();
}


void uiODPickSetTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    const bool needssave = Pick::SetMGR().needsSave( set_ );
    if ( istb )
    {
	mAddMenuItem( menu, &propertymnuitem_, true, false );
	mAddMenuItemCond( menu, &storemnuitem_, needssave, false, needssave );
	return;
    }

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));

    mAddMenuItem( menu, &convertbodymnuitem_, true, false )

    mAddMenuItem( menu, &displaymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &onlyatsectmnuitem_, true,!psd->allShown());
    mAddMenuItem( &displaymnuitem_, &propertymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &dirmnuitem_, true, false );

    mAddMenuItem( menu, &storemnuitem_, needssave, false );
    mAddMenuItem( menu, &storeasmnuitem_, true, false );

    Pick::SetMGR().getChangeInfo( storedid_, undomnuitem_.text,
				  redomnuitem_.text );
    if ( !undomnuitem_.text.isEmpty() )
	mAddMenuItem( menu, &undomnuitem_, true, false );
    if ( !redomnuitem_.text.isEmpty() )
	mAddMenuItem( menu, &redomnuitem_, true, false );
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
	emps->setPreferredColor( set_.dispColor() );
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
    else if ( mnuid==undomnuitem_.id || mnuid==redomnuitem_.id )
    {
	menu->setIsHandled( true );
	const bool isundo = mnuid==undomnuitem_.id;
	Pick::SetMGR().useChangeRecord( storedid_, isundo );
    }

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPickSetTreeItem::keyPressCB( CallBacker* cb )
{
    mCBCapsuleUnpack(uiKeyDesc,kd,cb);

    if ( kd.state()==OD::ControlButton && kd.key()==OD::KB_Z )
	Pick::SetMGR().useChangeRecord( storedid_, true );
    else if ( kd.state()==OD::ControlButton && kd.key()==OD::KB_Y )
	Pick::SetMGR().useChangeRecord( storedid_, false );
    else
	uiODDisplayTreeItem::keyPressCB( cb );
}


void uiODPickSetTreeItem::prepareForShutdown()
{
    uiODDisplayTreeItem::prepareForShutdown();
}


bool uiODPickSetTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    if ( !Pick::SetMGR().needsSave(set_) )
	return true;

    uiString warnstr = tr("'%1' has changed since the last save."
			    "\n\nDo you want to save it?").arg( set_.name() );
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
    : uiODSceneParentTreeItem( uiStrings::sPolygon() )
{
}


uiODPolygonParentTreeItem::~uiODPolygonParentTreeItem()
{
    detachAllNotifiers();
}


const char* uiODPolygonParentTreeItem::iconName() const
{ return "tree-polygon"; }


const char* uiODPolygonParentTreeItem::childObjTypeKey() const
{
    return Pick::SetPresentationInfo::sFactoryKey();
}


void uiODPolygonParentTreeItem::addChildItem( const MultiID& mid )
{
    RefMan<Pick::Set> ps = Pick::SetMGR().fetchForEdit( mid );
    ps.setNoDelete( true );
    if ( !ps || !ps->isPolygon() )
	return;

    uiODDisplayTreeItem* item = new uiODPolygonTreeItem( -1, *ps );
    addChild( item, false );
    item->setChecked( true );
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
	TypeSet<MultiID> setids;
	if ( !applMgr()->pickServer()->loadSets(setids,true) )
	    return false;
	for ( int idx=0; idx<setids.size(); idx++ )
	{
	    addChildItem( setids[idx] );
	    emitChildPRRequest( setids[idx], OD::Add );
	}
    }
    else if ( mnuid==mNewPolyIdx )
    {
	RefMan<Pick::Set> ps = applMgr()->pickServer()->createEmptySet(true);
	if ( !ps )
	    return false;

	const MultiID& storedid = Pick::SetMGR().getID( *ps );
	addChildItem( storedid );
	emitChildPRRequest( storedid, OD::Add );
    }
    else if ( mnuid==mSavePolyIdx )
    {
	applMgr()->pickServer()->storePickSets( 1 );
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
    set_.ref();
    storedid_ = Pick::SetMGR().getID( set_ );
    displayid_ = did;
    onlyatsectmnuitem_.checkable = true;

    propertymnuitem_.iconfnm = "disppars";
    storemnuitem_.iconfnm = "save";
    storeasmnuitem_.iconfnm = "saveas";

    mAttachCB( set_.objectChanged(), uiODPolygonTreeItem::setChgCB );
}


uiODPolygonTreeItem::~uiODPolygonTreeItem()
{
    detachAllNotifiers();
    set_.unRef();
}


const char* uiODPolygonTreeItem::objectTypeKey() const
{
    return Pick::SetPresentationInfo::sFactoryKey();
}


OD::ObjPresentationInfo* uiODPolygonTreeItem::getObjPRInfo()
{
    Pick::SetPresentationInfo* psprinfo = new Pick::SetPresentationInfo();
    psprinfo->setStoredID( storedid_ );
    return psprinfo;
}


void uiODPolygonTreeItem::setChgCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));
    if ( psd )
	psd->setName(toUiString( set_.name() ) );
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
	if ( set_.dispSize() > 100 )
	    set_.setDispSize( 3 );
	psd->setSet( &set_ );
	visserv_->addObject( psd, sceneID(), true );
	psd->fullRedraw();
    }

    updateColumnText( uiODSceneMgr::cColorColumn() );
    return uiODDisplayTreeItem::init();
}


void uiODPolygonTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    const bool needssave = Pick::SetMGR().needsSave( set_ );
    if ( istb )
    {
	mAddMenuItem( menu, &propertymnuitem_, true, false );
	mAddMenuItemCond( menu, &storemnuitem_, needssave, false, needssave );
	return;
    }

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));

    if ( set_.connection() == Pick::Set::Disp::Open )
	mAddMenuItem( menu, &closepolyitem_, true, false )
    else
	mResetMenuItem( &closepolyitem_ );

    mAddMenuItem( menu, &displaymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &onlyatsectmnuitem_, true,!psd->allShown());
    mAddMenuItem( &displaymnuitem_, &propertymnuitem_, true, false );

    mAddMenuItem( menu, &storemnuitem_, needssave, false );
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

    if ( set_.connection()==Pick::Set::Disp::Open
	 && mnuid==closepolyitem_.id )
    {
	menu->setIsHandled( true );
	set_.setConnection( Pick::Set::Disp::Close );
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


void uiODPolygonTreeItem::prepareForShutdown()
{
    uiODDisplayTreeItem::prepareForShutdown();
}


bool uiODPolygonTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    if ( !Pick::SetMGR().needsSave(set_) )
	return true;

    uiString warnstr = tr("'%1' has changed since the last save."
			    "\n\nDo you want to save it?").arg( set_.name() );
    const int retval = uiMSG().askSave( warnstr, withcancel );
    if ( retval == 0 )
	return true;
    else if ( retval == -1 )
	return false;
    else
	applMgr()->storePickSet( set_ );

    return true;
}
