/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodpicksettreeitem.h"

#include "emmanager.h"
#include "emrandomposbody.h"
#include "ioobj.h"
#include "ioman.h"
#include "pickset.h"
#include "survinfo.h"

#include "uicalcpoly2horvol.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodbodydisplaytreeitem.h"
#include "uiodscenemgr.h"
#include "uinotsaveddlg.h"
#include "uipickpartserv.h"
#include "uipickpropdlg.h"
#include "uipolygonzchanger.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "threadwork.h"
#include "visseedpainter.h"
#include "visrandomposbodydisplay.h"
#include "visselman.h"
#include "vissurvscene.h"


static bool isPickSetOrPolygon( const MultiID& mid )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	return false;

    return ioobj->translator() == "dGB";
}


CNotifier<uiODPickSetParentTreeItem,uiMenu*>&
	uiODPickSetParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODPickSetParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODPickSetParentTreeItem::uiODPickSetParentTreeItem()
    : uiODParentTreeItem( uiStrings::sPointSet())
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
    if ( !ps )
	return;

    auto* item = new uiODPickSetTreeItem( VisID::udf(), *ps );
    addChild( item, false );
    item->setChecked( true );
}


void uiODPickSetParentTreeItem::setRemovedCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps )
	return;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiODPickSetTreeItem*,itm,children_[idx])
	if ( !itm )
	    continue;

	if ( itm->getSet().ptr() == ps)
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
#define mExpGISIdx	5
#define mDisplayIdx	6
#define mShowAllIdx	7
#define mMergeIdx	8


bool uiODPickSetParentTreeItem::showSubMenu()
{
    RefMan<visSurvey::Scene> scene =
		    applMgr()->visServer()->getScene( sceneID() );
    const bool hastransform = scene && scene->getZAxisTransform();

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(tr("Add"))), mLoadIdx );
    auto* newmnu = new uiMenu( getUiParent(), tr("New") );
    newmnu->insertAction( new uiAction(m3Dots(tr("Empty"))), mEmptyIdx );
    newmnu->insertAction( new uiAction(m3Dots(tr("Generate 3D"))), mGen3DIdx );
    if ( SI().has2D() )
	newmnu->insertAction( new uiAction(m3Dots(tr("Generate 2D"))),
			      mRandom2DIdx);
    mnu.addMenu( newmnu );
    showMenuNotifier().trigger( &mnu, this );

    if ( !children_.isEmpty() )
    {
	mnu.insertSeparator();
	auto* filteritem = new uiAction( tr("Display Only at Sections") );
	mnu.insertAction( filteritem, mDisplayIdx );
	filteritem->setEnabled( !hastransform );
	auto* shwallitem = new uiAction( tr("Display in Full") );
	mnu.insertAction( shwallitem, mShowAllIdx );
	shwallitem->setEnabled( !hastransform );
	mnu.insertSeparator();
	mnu.insertAction( new uiAction(m3Dots(tr("Merge Sets"))), mMergeIdx );
	mnu.insertAction( new uiAction(uiStrings::sSaveChanges()), mSaveIdx );
	mnu.insertAction( new uiAction(m3Dots(uiGISExpStdFld::sToolTipTxt()),
			  uiGISExpStdFld::strIcon()), mExpGISIdx );

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
	    RefMan<Pick::Set> ps = psm.get( mids[idx] );
	    addPickSet( ps.ptr() );
	}
    }
    else if ( mnuid==mGen3DIdx )
	applMgr()->pickServer()->create3DGenSet();
    else if ( mnuid==mRandom2DIdx )
	applMgr()->pickServer()->createRandom2DSet();
    else if ( mnuid==mEmptyIdx )
	applMgr()->pickServer()->createEmptySet( false );
    else if ( mnuid==mSaveIdx )
    {
	if ( !applMgr()->pickServer()->storePickSets() )
	    uiMSG().error( tr("Problem saving changes. "
			      "Check write protection.") );
    }
    else if ( mnuid==mExpGISIdx )
    {
	uiPickPartServer& pickserver = *applMgr()->pickServer();
	RefObjectSet<const Pick::Set> gisdata;
	for ( const auto* child : children_ )
	{
	    mDynamicCastGet(const uiODPickSetTreeItem*,pstreeitem,child)
		if ( !pstreeitem )
		    continue;

	    ConstRefMan<visSurvey::PickSetDisplay> psd
						= pstreeitem->getDisplay();
	    if ( !psd )
		continue;

	    ConstRefMan<Pick::Set> ps = psd->getSet();
	    if ( !ps || ps->isEmpty() )
		continue;

	    RefMan<Pick::Set> pickset = new Pick::Set( *ps );
	    pickset->pars_.setYN( uiGISExportDlg::sKeyIsOn(),
		child->isSelected() );
	    gisdata.add( pickset.ptr() );
	}

	pickserver.exportPointSetsToGIS( getUiParent(), gisdata );
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
    {
	MultiID mid;
	applMgr()->pickServer()->mergePickSets( mid );
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODPickSetTreeItemFactory::createForVis( const VisID& visid,
						      uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    if ( !psd || !isPickSetOrPolygon(psd->getMultiID()) )
	return nullptr;

    RefMan<Pick::Set> pickset = psd->getSet();
    return pickset->isPolygon() ? nullptr
				: new uiODPickSetTreeItem(visid,*pickset);
}


// uiODPickSetTreeItem

uiODPickSetTreeItem::uiODPickSetTreeItem( const VisID& did, Pick::Set& ps )
    : uiODDisplayTreeItem()
    , set_(&ps)
    , storemnuitem_(uiStrings::sSave())
    , storeasmnuitem_(m3Dots(uiStrings::sSaveAs()))
    , dirmnuitem_(m3Dots(tr("Set Directions")))
    , onlyatsectmnuitem_(tr("Only at Sections"))
    , convertbodymnuitem_( tr("Convert to Geobody") )
    , propertymnuitem_(m3Dots(uiStrings::sProperties() ) )
    , paintingmnuitem_(m3Dots(tr("Start Painting")))
{
    displayid_ = did;
    onlyatsectmnuitem_.checkable = true;

    mAttachCB( Pick::Mgr().setChanged, uiODPickSetTreeItem::setChg );
    mAttachCB( visBase::DM().selMan().selnotifier,
	       uiODPickSetTreeItem::selChangedCB );

    mAttachCB( NotSavedPrompter::NSP().promptSaving,
	       uiODPickSetTreeItem::askSaveCB );

    propertymnuitem_.iconfnm = "disppars";
    paintingmnuitem_.iconfnm = "spraycan";
    storemnuitem_.iconfnm = "save";
    storeasmnuitem_.iconfnm = "saveas";
}


uiODPickSetTreeItem::~uiODPickSetTreeItem()
{
    detachAllNotifiers();
    delete paintdlg_;
    visserv_->removeObject( displayid_, sceneID() );
}


bool uiODPickSetTreeItem::actModeWhenSelected() const
{
    return set_ && set_->isEmpty();
}


void uiODPickSetTreeItem::selChangedCB( CallBacker* )
{
    enablePainting( isSelected() && paintingenabled_ );
    if ( !isSelected() )
	return;

    visserv_->setCurInterObjID( displayid_ );
}


bool uiODPickSetTreeItem::doubleClick( uiTreeViewItem* item )
{
    if ( item != uitreeviewitem_ )
	return uiTreeItem::doubleClick( item );

    RefMan<visSurvey::PickSetDisplay> pointsetdisplay = getDisplay();
    if ( !pointsetdisplay )
	return false;

    const OD::Color orgcolor( set_->disp3d().color() );
    uiPickPropDlg dlg( getUiParent(), *set_ , pointsetdisplay.ptr() );
    const bool ret = dlg.go();
    if ( set_->disp3d().color() != orgcolor )
	updateColumnText( uiODSceneMgr::cColorColumn() );

    return ret;
}


void uiODPickSetTreeItem::setChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps || set_.ptr()!=ps )
	return;

    RefMan<visSurvey::PickSetDisplay> pointsetdisplay = getDisplay();
    if ( pointsetdisplay )
	pointsetdisplay->setName( ps->name() );

    updateColumnText( uiODSceneMgr::cNameColumn() );
}


bool uiODPickSetTreeItem::init()
{
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::PickSetDisplay> psd = new visSurvey::PickSetDisplay;
	displayid_ = psd->id();
	if ( set_->disp3d().size()>100)
	    set_->disp3d().markerstyle_.size_ = 3;

	psd->setSet( set_.ptr() );
	visserv_->addObject( psd.ptr(), sceneID(), true);
	psd->fullRedraw();
    }

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));
    if ( !psd )
	return false;

    const MultiID setid = psd->getMultiID();
    NotifyStopper ntfstop( Pick::Mgr().setAdded );
    Pick::Mgr().set( setid, psd->getSet().ptr() );

    updateColumnText( uiODSceneMgr::cColorColumn() );

    pointsetdisplay_ = psd;
    return uiODDisplayTreeItem::init();
}


ConstRefMan<visSurvey::PickSetDisplay> uiODPickSetTreeItem::getDisplay() const
{
    return pointsetdisplay_.get();
}


RefMan<visSurvey::PickSetDisplay> uiODPickSetTreeItem::getDisplay()
{
    return pointsetdisplay_.get();
}


void uiODPickSetTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || !isDisplayID(menu->menuID()) )
	return;

    ConstRefMan<visSurvey::PickSetDisplay> pointsetdisplay = getDisplay();
    if ( !pointsetdisplay )
	return;

    const int setidx = Pick::Mgr().indexOf( *set_ );
    const bool changed = setidx < 0 || Pick::Mgr().isChanged(setidx);
    const bool isreadonly = set_->isReadOnly();
    if ( istb )
    {
	mAddMenuItem( menu, &propertymnuitem_, true, false );
	mAddMenuItem( menu, &paintingmnuitem_, true, paintingenabled_ )
	mAddMenuItemCond( menu, &storemnuitem_, changed, false, !isreadonly );
	return;
    }

    mAddMenuItem( menu, &paintingmnuitem_, true, paintingenabled_ )
    mAddMenuItem( menu, &convertbodymnuitem_, true, false )

    mAddMenuItem( menu, &displaymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &onlyatsectmnuitem_,
		  !isreadonly, !pointsetdisplay->allShown());
    mAddMenuItem( &displaymnuitem_, &propertymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &dirmnuitem_, true, false );

    mAddMenuItemCond( menu, &storemnuitem_, changed, false, !isreadonly );
    mAddMenuItemCond( menu, &storeasmnuitem_, true, false, !isreadonly );
}


void uiODPickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( menu->isHandled() || mnuid==-1 )
	return;

    RefMan<visSurvey::PickSetDisplay> pointsetdisplay = getDisplay();
    if ( !pointsetdisplay )
	return;

    if ( !isDisplayID(menu->menuID()) )
	return;

    if ( mnuid==storemnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->storePickSet( *set_ );
    }
    else if ( mnuid==storeasmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->storePickSetAs( *set_ );
    }
    else if ( mnuid==dirmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->setPickSetDirs( *set_ );
    }
    else if ( mnuid==onlyatsectmnuitem_.id )
    {
	menu->setIsHandled( true );
	if ( pointsetdisplay )
	    setOnlyAtSectionsDisplay( !displayedOnlyAtSections() );
    }
    else if ( mnuid==propertymnuitem_.id )
    {
	menu->setIsHandled( true );
	uiPickPropDlg dlg( getUiParent(), *set_, pointsetdisplay.ptr() );
	dlg.go();
    }
    else if ( mnuid==paintingmnuitem_.id )
    {
	paintingenabled_ = !paintingenabled_;
	enablePainting( paintingenabled_ );
    }
    else if ( mnuid==convertbodymnuitem_.id )
    {
	menu->setIsHandled( true );

	RefMan<EM::EMObject> emobj =
	    EM::EMM().createTempObject( EM::RandomPosBody::typeStr() );
	mDynamicCastGet( EM::RandomPosBody*, emps, emobj.ptr() );
	if ( !emps )
	    return;

	emps->copyFrom( *set_ );
	emps->setPreferredColor( set_->disp3d().color() );
	emps->setName( BufferString("Geobody from ",set_->name()) );
	emps->setChangedFlag();

	RefMan<visSurvey::RandomPosBodyDisplay> npsd =
					    new visSurvey::RandomPosBodyDisplay;
	npsd->setSelectable( false );
	npsd->setEMID( emps->id() );
	npsd->setDisplayTransformation(
				pointsetdisplay->getDisplayTransformation() );
	addChild( new uiODBodyDisplayTreeItem(npsd->id(),true), false );

	visserv_->addObject( npsd.ptr(), sceneID(), true);
    }

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPickSetTreeItem::paintDlgClosedCB( CallBacker* )
{}


void uiODPickSetTreeItem::enablePainting( bool yn )
{
    RefMan<visSurvey::PickSetDisplay> pointsetdisplay = getDisplay();
    if ( !yn )
    {
	if ( paintdlg_ )
	    paintdlg_->close();

	if ( pointsetdisplay )
	    pointsetdisplay->getPainter()->deActivate();

	return;
    }

    if ( pointsetdisplay )
	pointsetdisplay->getPainter()->activate();

    if ( paintdlg_ )
	paintdlg_->refresh();
    else
    {
	paintdlg_ = new uiSeedPainterDlg( getUiParent(),
					  pointsetdisplay.ptr() );
	mAttachCB( paintdlg_->windowClosed,
		   uiODPickSetTreeItem::paintDlgClosedCB );
    }

    paintdlg_->show();
}


void uiODPickSetTreeItem::showAllPicks( bool yn )
{
    RefMan<visSurvey::PickSetDisplay> pointsetdisplay = getDisplay();
    if ( pointsetdisplay )
	pointsetdisplay->showAll( yn );
}


bool uiODPickSetTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    const int setidx = Pick::Mgr().indexOf( *set_ );
    if ( set_->isReadOnly() || setidx<0 || !Pick::Mgr().isChanged(setidx) )
	return true;

    uiString warnstr = tr("This pickset has changed since the last save.\n\n"
			  "Do you want to save it?");
    const int retval = uiMSG().askSave( warnstr, withcancel );
    if (retval == 0 )
    {
	Pick::SetMgr& psm = Pick::Mgr();
	applMgr()->pickServer()->reLoadSet( psm.get(*set_) );
	return true;
    }
    else if ( retval == -1 )
	return false;
    else
	applMgr()->storePickSet( *set_ );

    return true;
}


void uiODPickSetTreeItem::askSaveCB( CallBacker* )
{
    const bool ischanged = Pick::Mgr().isChanged(
			Pick::Mgr().indexOf(set_.ptr()->name().buf()) );
    if ( !ischanged )
	return;

    const uiString obj = toUiString("%1 ").arg( set_->name().buf() );
    NotSavedPrompter::NSP().addObject( obj,
				       mCB(this,uiODPickSetTreeItem,saveCB),
				       false, 0 );
    Threads::WorkManager::twm().addWork(
	    Threads::Work( *new uiTreeItemRemover(parent_, this), true ), 0,
			   NotSavedPrompter::NSP().queueID(), false );
}


void uiODPickSetTreeItem::saveCB( CallBacker* cb )
{
    const bool issaved = applMgr()->storePickSet( *set_ );

    if ( issaved && cb )
	NotSavedPrompter::NSP().reportSuccessfullSave();
}


// uiODPolygonParentTreeItem

CNotifier<uiODPolygonParentTreeItem,uiMenu*>&
	uiODPolygonParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODPolygonParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODPolygonParentTreeItem::uiODPolygonParentTreeItem()
    : uiODParentTreeItem( uiStrings::sPolygon() )
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
    if ( !ps )
	return;

    auto* item = new uiODPolygonTreeItem( VisID::udf(), *ps );
    addChild( item, false );
}


void uiODPolygonParentTreeItem::setRemovedCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps )
	return;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiODPolygonTreeItem*,itm,children_[idx])
	if ( !itm )
	    continue;

	ConstRefMan<Pick::Set> pickset = itm->getSet();
	if ( pickset.ptr() == ps)
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
#define mExpGISPolyIdx	14
#define mOnlyAtPolyIdx	15
#define mAlwaysPolyIdx	16


bool uiODPolygonParentTreeItem::showSubMenu()
{
    RefMan<visSurvey::Scene> scene =
		    applMgr()->visServer()->getScene( sceneID() );
    const bool hastransform = scene && scene->getZAxisTransform();

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(tr("Add"))), mLoadPolyIdx );
    mnu.insertAction( new uiAction(m3Dots(tr("New"))), mNewPolyIdx );
    showMenuNotifier().trigger( &mnu, this );

    if ( !children_.isEmpty() )
    {
	mnu.insertSeparator();
	auto* filteritem = new uiAction( tr("Display Only at Sections") );
	mnu.insertAction( filteritem, mOnlyAtPolyIdx );
	filteritem->setEnabled( !hastransform );
	auto* shwallitem = new uiAction( tr("Display in Full") );
	mnu.insertAction( shwallitem, mAlwaysPolyIdx );
	shwallitem->setEnabled( !hastransform );
	mnu.insertSeparator();
	mnu.insertAction( new uiAction(uiStrings::sSaveChanges()),
			  mSavePolyIdx );
	mnu.insertAction( new uiAction(m3Dots(uiGISExpStdFld::sToolTipTxt()),
			  uiGISExpStdFld::strIcon()), mExpGISPolyIdx );
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
	    RefMan<Pick::Set> ps = Pick::Mgr().get( mids[idx] );
	    addPolygon( ps.ptr() );
	}
    }
    else if ( mnuid==mNewPolyIdx )
    {
	applMgr()->pickServer()->createEmptySet( true );
    }
    else if ( mnuid==mSavePolyIdx )
    {
	if ( !applMgr()->pickServer()->storePickSets() )
	    uiMSG().error( tr("Problem saving changes. "
			      "Check write protection.") );
    }
    else if ( mnuid==mExpGISPolyIdx )
    {
	uiPickPartServer& pickserver = *applMgr()->pickServer();
	RefObjectSet<const Pick::Set> gisdata;
	for ( const auto* child : children_ )
	{
	    mDynamicCastGet(const uiODPolygonTreeItem*,pgtreeitem,child)
	    if ( !pgtreeitem )
		continue;

	    ConstRefMan<visSurvey::PickSetDisplay> pgd
						    = pgtreeitem->getDisplay();
	    if ( !pgd )
		continue;

	    ConstRefMan<Pick::Set> pg = pgd->getSet();
	    if ( !pg || pg->isEmpty() )
		continue;

	    RefMan<Pick::Set> pickset = new Pick::Set( *pg );
	    pickset->pars_.setYN( uiGISExportDlg::sKeyIsOn(),
				  child->isSelected() );
	    gisdata.add( pickset.ptr() );
	}

	pickserver.exportPolygonsToGIS( getUiParent(), gisdata );
    }
    else if ( mnuid==mAlwaysPolyIdx || mnuid==mOnlyAtPolyIdx )
    {
	const bool showall = mnuid==mAlwaysPolyIdx;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODPolygonTreeItem*,itm,children_[idx])
	    if ( !itm )
		continue;

	    itm->setOnlyAtSectionsDisplay( !showall );
	    itm->updateColumnText( uiODSceneMgr::cColorColumn() );
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODPolygonTreeItemFactory::createForVis( const VisID& visid,
						      uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    if ( !psd || !isPickSetOrPolygon(psd->getMultiID()) )
	return nullptr;

    RefMan<Pick::Set> pickset = psd->getSet();
    return !pickset->isPolygon() ? nullptr
				 : new uiODPolygonTreeItem(visid,*pickset);
}


// uiODPolygonTreeItem

uiODPolygonTreeItem::uiODPolygonTreeItem( const VisID& did, Pick::Set& ps )
    : uiODDisplayTreeItem()
    , set_(&ps)
    , storemnuitem_(uiStrings::sSave())
    , storeasmnuitem_(uiStrings::sSaveAs())
    , onlyatsectmnuitem_(tr("Only at Sections"))
    , propertymnuitem_(m3Dots(uiStrings::sProperties()))
    , closepolyitem_(tr("Close Polygon"))
    , changezmnuitem_(m3Dots(tr("Change Z values")))
    , workareaitem_(m3Dots(tr("Set as Work Area")))
    , calcvolmnuitem_(m3Dots(tr("Calculate Volume")))
{
    displayid_ = did;
    onlyatsectmnuitem_.checkable = true;

    mAttachCB( Pick::Mgr().setChanged, uiODPolygonTreeItem::setChg );
    mAttachCB( NotSavedPrompter::NSP().promptSaving,
	       uiODPolygonTreeItem::askSaveCB );

    propertymnuitem_.iconfnm = "disppars";
    storemnuitem_.iconfnm = "save";
    storeasmnuitem_.iconfnm = "saveas";
}


uiODPolygonTreeItem::~uiODPolygonTreeItem()
{
    detachAllNotifiers();
    visserv_->removeObject( displayid_, sceneID() );
}


void uiODPolygonTreeItem::askSaveCB( CallBacker* )
{
    const bool ischanged = Pick::Mgr().isChanged(
			    Pick::Mgr().indexOf(set_.ptr()->name().buf()) );
    if ( !ischanged )
	return;

    const uiString obj = toUiString("%1 ").arg( set_->name().buf() );
    NotSavedPrompter::NSP().addObject( obj,
				       mCB(this,uiODPolygonTreeItem,saveCB),
				       false, nullptr );
    Threads::WorkManager::twm().addWork(
	    Threads::Work(*new uiTreeItemRemover(parent_,this),true), nullptr,
			   NotSavedPrompter::NSP().queueID(), false );
}


void uiODPolygonTreeItem::saveCB( CallBacker* cb )
{
    const bool issaved = applMgr()->storePickSet( *set_ );

    if ( issaved && cb )
	NotSavedPrompter::NSP().reportSuccessfullSave();
}



bool uiODPolygonTreeItem::actModeWhenSelected() const
{
    return set_ && set_->isEmpty();
}


void uiODPolygonTreeItem::setChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps || set_.ptr()!=ps )
	return;

    RefMan<visSurvey::PickSetDisplay> polygondisplay = getDisplay();
    if ( polygondisplay )
	polygondisplay->setName( ps->name() );

    updateColumnText( uiODSceneMgr::cNameColumn() );
}


bool uiODPolygonTreeItem::doubleClick( uiTreeViewItem* item )
{
    if ( item != uitreeviewitem_ )
	return uiTreeItem::doubleClick( item );

    RefMan<visSurvey::PickSetDisplay> polygondisplay = getDisplay();
    if ( !polygondisplay )
	return false;

    // TODO: Make polygon specific properties dialog
    uiPickPropDlg dlg( getUiParent(), *set_ , polygondisplay.ptr() );
    return dlg.go();
}


bool uiODPolygonTreeItem::init()
{
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::PickSetDisplay> psd = new visSurvey::PickSetDisplay;
	displayid_ = psd->id();
	if ( set_->disp3d().size()>100)
	    set_->disp3d().markerstyle_.size_ = 3;

	psd->setSet( set_.ptr() );
	visserv_->addObject( psd.ptr(), sceneID(), true);
	psd->fullRedraw();
    }

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv_->getObject(displayid_));
    if ( !psd )
	return false;

    const MultiID setid = psd->getMultiID();
    NotifyStopper ntfstop( Pick::Mgr().setAdded );

    updateColumnText( uiODSceneMgr::cColorColumn() );

    polygondisplay_ = psd;
    return uiODDisplayTreeItem::init();
}


ConstRefMan<visSurvey::PickSetDisplay> uiODPolygonTreeItem::getDisplay() const
{
    return polygondisplay_.get();
}


RefMan<visSurvey::PickSetDisplay> uiODPolygonTreeItem::getDisplay()
{
    return polygondisplay_.get();
}


void uiODPolygonTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || !isDisplayID(menu->menuID()) )
	return;

    if ( istb )
    {
	mAddMenuItem( menu, &propertymnuitem_, true, false );
	const int setidx = Pick::Mgr().indexOf( *set_ );
	const bool changed = setidx < 0 || Pick::Mgr().isChanged(setidx);
	mAddMenuItemCond( menu, &storemnuitem_, changed, false, changed );
	return;
    }

    ConstRefMan<visSurvey::PickSetDisplay> polygondisplay = getDisplay();
    if ( !polygondisplay )
	return;

    if ( set_->disp3d().polyDisp()->connect_ == Pick::Set::Connection::Open )
	mAddMenuItem( menu, &closepolyitem_, true, false )
    else
	mResetMenuItem( &closepolyitem_ );

    mAddMenuItem( menu, &displaymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &onlyatsectmnuitem_,
		  !set_->isReadOnly(),!polygondisplay->allShown() );
    mAddMenuItem( &displaymnuitem_, &propertymnuitem_, true, false );

    const int setidx = Pick::Mgr().indexOf( *set_ );
    const bool changed = setidx < 0 || Pick::Mgr().isChanged(setidx);
    mAddMenuItem( menu, &storemnuitem_, changed, false );
    mAddMenuItem( menu, &storeasmnuitem_, true, false );
    mAddMenuItem( menu, &workareaitem_, true, false );

    const bool islocked = visserv_->isLocked( displayID() );
    mAddMenuItem( menu, &changezmnuitem_, !islocked, false );
    mAddMenuItem( menu, &calcvolmnuitem_, true, false );
}


void uiODPolygonTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( menu->isHandled() || mnuid==-1 )
	return;

    if ( !isDisplayID(menu->menuID()) )
	return;

    RefMan<visSurvey::PickSetDisplay> polygondisplay = getDisplay();
    if ( !polygondisplay )
	return;

    bool handled = true;
    if ( set_->disp3d().polyDisp()->connect_==Pick::Set::Connection::Open &&
	 mnuid==closepolyitem_.id )
    {
	set_->disp3d().polyDisp()->connect_ = Pick::Set::Connection::Close;
	Pick::Mgr().reportDispChange( this, *set_ );
    }
    else if ( mnuid==storemnuitem_.id )
    {
	applMgr()->storePickSet( *set_ );
    }
    else if ( mnuid==storeasmnuitem_.id )
    {
	applMgr()->storePickSetAs( *set_ );
    }
    else if ( mnuid==onlyatsectmnuitem_.id )
    {
	if ( polygondisplay )
	    setOnlyAtSectionsDisplay( !displayedOnlyAtSections() );
    }
    else if ( mnuid==propertymnuitem_.id )
    {
	uiPickPropDlg dlg( getUiParent(), *set_ , polygondisplay.ptr() );
	dlg.go();
    }
    else if ( mnuid == changezmnuitem_.id )
    {
	if ( set_->isEmpty() )
	{
	    uiMSG().message( uiStrings::sPolygon()
		       .append( tr("%1 is empty. Pick some points in the %2") )
		       .arg( uiStrings::sPolygon() )
		       .arg( uiStrings::sPolygon().toLower() ) );
	    return;
	}

	uiPolygonZChanger dlg( getUiParent(), *set_ );
	dlg.go();
    }
    else if ( mnuid == workareaitem_.id )
    {
	TrcKeyZSampling tkzs;
	set_->getBoundingBox( tkzs );
	if ( tkzs.zsamp_.width() < 2*SI().zStep() )
	    tkzs.zsamp_ = SI().zRange( false );
	visserv_->setWorkingArea( tkzs );
    }
    else if ( mnuid == calcvolmnuitem_.id )
    {
	uiCalcPolyHorVol dlg( getUiParent(), *set_ );
	dlg.go();
    }
    else
	handled = false;

    menu->setIsHandled( handled );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPolygonTreeItem::showAllPicks( bool yn )
{
    RefMan<visSurvey::PickSetDisplay> polygondisplay = getDisplay();
    if ( polygondisplay )
	polygondisplay->showAll( yn );
}


bool uiODPolygonTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    const int setidx = Pick::Mgr().indexOf( *set_ );
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
	applMgr()->storePickSet( *set_ );

    return true;
}
