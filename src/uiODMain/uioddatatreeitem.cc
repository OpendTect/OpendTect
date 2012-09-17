/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uioddatatreeitem.cc,v 1.65 2012/07/10 14:08:29 cvsjaap Exp $";

#include "uioddatatreeitem.h"

#include "uilistview.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uioddisplaytreeitem.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uivispartserv.h"
#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"
#include "uiamplspectrum.h"
#include "attribsel.h"
#include "pixmap.h"

//TODO:remove when Flattened scene ok for 2D Viewer
#include "emhorizonztransform.h"
#include "vissurvscene.h"


mImplFactory2Param( uiODDataTreeItem, const Attrib::SelSpec&,
		     const char*, uiODDataTreeItem::factory );

uiODDataTreeItem::uiODDataTreeItem( const char* parenttype )
    : uiTreeItem("")
    , parenttype_(parenttype)
    , menu_(0)
    , movemnuitem_("&Move")
    , movetotopmnuitem_("to &top")
    , movetobottommnuitem_("to &bottom")
    , moveupmnuitem_("&up")
    , movedownmnuitem_("&down")
    , displaymnuitem_("&Display")
    , removemnuitem_("&Remove",-1000)
    , changetransparencyitem_("Change &transparency ...")
    , statisticsitem_("Show &Histogram ...")
    , amplspectrumitem_("Show &Amplitude Spectrum...")
    , addto2dvieweritem_("Display in a &2D Viewer as")
    , view2dwvaitem_("2D Viewer - &Wiggle")
    , view2dvditem_("2D Viewer - &VD")
{
    statisticsitem_.iconfnm = "histogram.png";
    removemnuitem_.iconfnm = "stop.png";
    view2dwvaitem_.iconfnm = "wva.png";
    view2dvditem_.iconfnm = "vd.png";
    amplspectrumitem_.iconfnm = "amplspectrum.png";
    movetotopmnuitem_.iconfnm = "totop.png";
    moveupmnuitem_.iconfnm = "uparrow.png";
    movedownmnuitem_.iconfnm = "downarrow.png";
    movetobottommnuitem_.iconfnm = "tobottom.png";
}


uiODDataTreeItem::~uiODDataTreeItem()
{
    if ( menu_ )
    {
	menu_->createnotifier.remove( mCB(this,uiODDataTreeItem,createMenuCB) );
	menu_->handlenotifier.remove( mCB(this,uiODDataTreeItem,handleMenuCB) );
	menu_->unRef();
    }

    uiVisPartServer* visserv = applMgr()->visServer();
    MenuHandler* tb = visserv->getToolBarHandler();
    tb->createnotifier.remove( mCB(this,uiODDataTreeItem,addToToolBarCB) );
    tb->handlenotifier.remove( mCB(this,uiODDataTreeItem,handleMenuCB) );
}

/*
uiODDataTreeItem* uiODDataTreeItem::create( const Attrib::SelSpec& as,
					    const char* pt )
{
    for ( int idx=0; idx<creators_.size(); idx++)
    {
        uiODDataTreeItem* res = creators_[idx]( as, pt );
	if ( res )
	    return res;
    }

    return 0;
}
*/


int uiODDataTreeItem::uiListViewItemType() const
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( visserv->canHaveMultipleAttribs( displayID() ) )
	return uiListViewItem::CheckBox;
    else
	return uiTreeItem::uiListViewItemType();
}


uiODApplMgr* uiODDataTreeItem::applMgr() const
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


bool uiODDataTreeItem::init()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( visserv->canHaveMultipleAttribs(displayID()) )
    {
	getItem()->stateChanged.notify( mCB(this,uiODDataTreeItem,checkCB) );
	uilistviewitem_->setChecked( visserv->isAttribEnabled(displayID(),
		    		     attribNr() ) );
    }

    MenuHandler* tb = visserv->getToolBarHandler();
    tb->createnotifier.notify( mCB(this,uiODDataTreeItem,addToToolBarCB) );
    tb->handlenotifier.notify( mCB(this,uiODDataTreeItem,handleMenuCB) );

    return uiTreeItem::init();
}


void uiODDataTreeItem::checkCB( CallBacker* cb )
{
    uiVisPartServer* visserv = applMgr()->visServer();
    visserv->enableAttrib( displayID(), attribNr(), isChecked() );
}


bool uiODDataTreeItem::shouldSelect( int selid ) const
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    return selid!=-1 && selid==displayID() &&
	   visserv->getSelAttribNr()==attribNr();
}


int uiODDataTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODTreeTop::sceneidkey(), sceneid );
    return sceneid;
}



int uiODDataTreeItem::displayID() const
{
    mDynamicCastGet( uiODDisplayTreeItem*, odti, parent_ );
    return odti ? odti->displayID() : -1;
}


int uiODDataTreeItem::attribNr() const
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    const int nrattribs = visserv->getNrAttribs( displayID() );
    const int attribnr = nrattribs-siblingIndex()-1;
    return attribnr<0 || attribnr>=nrattribs ? 0 : attribnr;
}


void uiODDataTreeItem::addToToolBarCB( CallBacker* cb )
{
    mDynamicCastGet(uiTreeItemTBHandler*,tb,cb);
    if ( !tb || tb->menuID() != displayID() || !isSelected() )
	return;

    uiVisPartServer* visserv = applMgr()->visServer();
    const DataPack::ID dpid = visserv->getDataPackID( displayID(), attribNr() );
    const bool hasdatapack = dpid>DataPack::cNoID();
    const bool isvert = visserv->isVerticalDisp( displayID() );
    
    if ( hasdatapack )
    {
	mAddMenuItem( tb, &statisticsitem_, true, false )
	if ( isvert )
	    mAddMenuItem( tb, &amplspectrumitem_, true, false )
    }
    else
    {
	mResetMenuItem( &statisticsitem_ )
	mResetMenuItem( &amplspectrumitem_ )
    }

    mDynamicCastGet(visSurvey::Scene*,scene,
	                applMgr()->visServer()->getObject(sceneID()));
    const bool hasztransform = scene && scene->getZAxisTransform();
//TODO:remove when Z-transformed scenes are ok for 2D Viewer

    if ( visserv->canBDispOn2DViewer(displayID()) && !hasztransform
	    && dpid>DataPack::cNoID() )
    {
	const Attrib::SelSpec* as =
	    visserv->getSelSpec( displayID(), attribNr() );
	const bool hasattrib =
	    as && as->id().asInt()!=Attrib::SelSpec::cAttribNotSel().asInt();

	mAddMenuItem( tb, &view2dvditem_, hasattrib, false )
	mAddMenuItem( tb, &view2dwvaitem_, hasattrib, false )
    }
    else
    {
	mResetMenuItem( &view2dwvaitem_ );
	mResetMenuItem( &view2dvditem_ );
    }

    const bool islocked = visserv->isLocked( displayID() );
    mAddMenuItem( tb, &removemnuitem_,
		  !islocked && visserv->canRemoveAttrib( displayID()), false );
}


bool uiODDataTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	menu_->createnotifier.notify( mCB(this,uiODDataTreeItem,createMenuCB) );
	menu_->handlenotifier.notify( mCB(this,uiODDataTreeItem,handleMenuCB) );
    }

    return menu_->executeMenu(uiMenuHandler::fromTree());
}


void uiODDataTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    createMenu( menu, false );
}


void uiODDataTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiVisPartServer* visserv = applMgr()->visServer();
    const bool isfirst = !siblingIndex();
    const bool islast = siblingIndex()==visserv->getNrAttribs( displayID())-1;

    const bool islocked = visserv->isLocked( displayID() );

    if ( !islocked && (!isfirst || !islast) )
    {
	mAddMenuOrTBItem( istb, &movemnuitem_, &movetotopmnuitem_,
		      !islocked && !isfirst, false );
	mAddMenuOrTBItem( istb, &movemnuitem_, &moveupmnuitem_,
		      !islocked && !isfirst, false );
	mAddMenuOrTBItem( istb, &movemnuitem_, &movedownmnuitem_,
		      !islocked && !islast, false );
	mAddMenuOrTBItem( istb, &movemnuitem_, &movetobottommnuitem_,
		      !islocked && !islast, false );

	mAddMenuOrTBItem( istb, menu, &movemnuitem_, true, false );
    }
    else
    {
	mResetMenuItem( &changetransparencyitem_ );
	mResetMenuItem( &movetotopmnuitem_ );
	mResetMenuItem( &moveupmnuitem_ );
	mResetMenuItem( &movedownmnuitem_ );
	mResetMenuItem( &movetobottommnuitem_ );

	mResetMenuItem( &movemnuitem_ );
    }

    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, true, false );
    const DataPack::ID dpid = visserv->getDataPackID( displayID(), attribNr() );
    const bool hasdatapack = dpid>DataPack::cNoID();
    const bool isvert = visserv->isVerticalDisp( displayID() );
    if ( hasdatapack )
	mAddMenuOrTBItem( istb, &displaymnuitem_, &statisticsitem_, true, false )
    else
	mResetMenuItem( &statisticsitem_ )

    if ( hasdatapack && isvert )
	mAddMenuOrTBItem( istb, &displaymnuitem_, &amplspectrumitem_,true,false)
    else
	mResetMenuItem( &amplspectrumitem_ )

    mAddMenuOrTBItem( istb, menu, &removemnuitem_,
		  !islocked && visserv->canRemoveAttrib( displayID()), false );
    if ( visserv->canHaveMultipleAttribs(displayID()) && hasTransparencyMenu() )
	mAddMenuOrTBItem( istb, &displaymnuitem_, &changetransparencyitem_,
				true, false )
    else
	mResetMenuItem( &changetransparencyitem_ );

    mDynamicCastGet(visSurvey::Scene*,scene,
	                applMgr()->visServer()->getObject(sceneID()));
    const bool hasztransform = scene && scene->getZAxisTransform();
//TODO:remove when Z-transformed scenes are ok for 2D Viewer

    if ( visserv->canBDispOn2DViewer(displayID()) && !hasztransform
	    && dpid>DataPack::cNoID() )
    {
	const Attrib::SelSpec* as =
	    visserv->getSelSpec( displayID(), attribNr() );
	const bool hasattrib =
	    as && as->id().asInt()!=Attrib::SelSpec::cAttribNotSel().asInt();
	if ( isvert )
	    mAddMenuOrTBItem(istb, &displaymnuitem_, &view2dwvaitem_,
		    		  hasattrib, false)
	else
	    mResetMenuItem( &view2dwvaitem_ );

	mAddMenuOrTBItem( istb, &displaymnuitem_, &view2dvditem_, hasattrib,
			       false )
    }
    else
    {
	mResetMenuItem( &view2dwvaitem_ );
	mResetMenuItem( &view2dvditem_ );
    }
    mResetMenuItem( &addto2dvieweritem_ );
}


bool uiODDataTreeItem::select()
{
    applMgr()->visServer()->setSelObjectId( displayID(), attribNr() );
    return true;
}


void uiODDataTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = applMgr()->visServer();

    if ( mnuid==movetotopmnuitem_.id )
    {
	const int nrattribs = visserv->getNrAttribs( displayID() );
	for ( int idx=attribNr(); idx<nrattribs-1; idx++ )
	    visserv->swapAttribs( displayID(), idx, idx+1 );

	moveItemToTop();
	select();
	menu->setIsHandled( true );
    }
    else if ( mnuid==movetobottommnuitem_.id )
    {
	const int nrattribs = visserv->getNrAttribs( displayID() );
	for ( int idx=attribNr(); idx; idx-- )
	    visserv->swapAttribs( displayID(), idx, idx-1 );

	moveItem( parent_->lastChild() );
	select();
	menu->setIsHandled( true );
    }
    else if ( mnuid==moveupmnuitem_.id )
    {
	const int attribnr = attribNr();
	if ( attribnr<visserv->getNrAttribs( displayID() )-1 )
	{
	    const int targetattribnr = attribnr+1;
	    visserv->swapAttribs( displayID(), attribnr, targetattribnr );
	}

	moveItem( siblingAbove() );
	select();
	menu->setIsHandled(true);
    }
    else if ( mnuid==movedownmnuitem_.id )
    {
	const int attribnr = attribNr();
	if ( attribnr )
	{
	    const int targetattribnr = attribnr-1;
	    visserv->swapAttribs( displayID(), attribnr, targetattribnr );
	}

	moveItem( siblingBelow() );
	select();
	menu->setIsHandled( true );
    }
    else if ( mnuid==changetransparencyitem_.id )
    {
	visserv->showAttribTransparencyDlg( displayID(), attribNr() );
	menu->setIsHandled( true );
    }
    else if ( mnuid==statisticsitem_.id )
    {
	const DataPack::ID dpid = visserv->getDataPackID( displayID(),
							  attribNr() );
	const DataPackMgr::ID dmid = visserv->getDataPackMgrID( displayID() );
	uiStatsDisplay::Setup su; su.countinplot( false );
	uiStatsDisplayWin* dwin =
	    new uiStatsDisplayWin( applMgr()->applService().parent(), su,
		    		   1, false );
	dwin->statsDisplay()->setDataPackID( dpid, dmid );
	dwin->setDataName( DPM(dmid).nameOf(dpid)  );
	dwin->setDeleteOnClose( true );
	dwin->show();
	menu->setIsHandled( true );
    }
    else if ( mnuid==amplspectrumitem_.id )
    {
	const DataPack::ID dpid = visserv->getDataPackID( displayID(),
							  attribNr() );
	const DataPackMgr::ID dmid = visserv->getDataPackMgrID( displayID() );
	const bool isselmodeon = visserv->isSelectionModeOn();

	if ( !isselmodeon )
	{
	    uiAmplSpectrum* asd = new uiAmplSpectrum(
					applMgr()->applService().parent() );
	    asd->setDeleteOnClose( true );
	    asd->setDataPackID( dpid, dmid );
	    asd->show();
	}
	else
	{

	}
	
	menu->setIsHandled( true );
    }
    else if ( mnuid==view2dwvaitem_.id || mnuid==view2dvditem_.id )
    {
	ODMainWin()->viewer2DMgr().displayIn2DViewer( displayID(), attribNr(),
						   mnuid==view2dwvaitem_.id );
	menu->setIsHandled( true );
    }
    else if ( mnuid==removemnuitem_.id )
    {
	const int attribnr = attribNr(); 
	visserv->removeAttrib( displayID(), attribnr );
	applMgr()->updateColorTable( displayID(), attribnr ? attribnr-1 : 0 );

	prepareForShutdown();
	parent_->removeChild( this );
	menu->setIsHandled( true );
    }
}


void uiODDataTreeItem::prepareForShutdown()
{
    uiTreeItem::prepareForShutdown();
    applMgr()->updateColorTable( -1, -1 );
}


void uiODDataTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = createDisplayName();

    uiTreeItem::updateColumnText( col );
}


void uiODDataTreeItem::displayMiniCtab( const ColTab::Sequence* seq )
{
    if ( !seq )
    {
	uiTreeItem::updateColumnText( uiODSceneMgr::cColorColumn() );
	return;
    }

    PtrMan<ioPixmap> pixmap = new ioPixmap( *seq, cPixmapWidth(),
					    cPixmapHeight(), true );
    uilistviewitem_->setPixmap( uiODSceneMgr::cColorColumn(), *pixmap );
}
