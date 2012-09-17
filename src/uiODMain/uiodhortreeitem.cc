/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodhortreeitem.cc,v 1.74 2011/10/07 21:53:43 cvsnanne Exp $";

#include "uiodhortreeitem.h"

#include "datapointset.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "mpeengine.h"
#include "selector.h"
#include "survinfo.h"

#include "uiattribpartserv.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uihor2dfrom3ddlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiposprovider.h"
#include "uitaskrunner.h"
#include "uivisemobj.h"
#include "uivispartserv.h"

#include "visemobjdisplay.h"
#include "vishorizondisplay.h"
#include "vishorizonsection.h"
#include "vissurvscene.h"
#include "visrgbatexturechannel2rgba.h"
#include "zaxistransform.h"


#define mAddIdx		0
#define mAddCBIdx	1
#define mNewIdx		2
#define mSectIdx	3
#define mFullIdx	4
#define mSectFullIdx	5

uiODHorizonParentTreeItem::uiODHorizonParentTreeItem()
    : uiODTreeItem( "Horizon" )
{}


bool uiODHorizonParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    ODMainWin()->applMgr().visServer()->getObject(sceneID()));

    const bool hastransform = scene && scene->getZAxisTransform();

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Add ..."), mAddIdx );
    mnu.insertItem( new uiMenuItem("Add &color blended..."), mAddCBIdx );

    uiMenuItem* newmenu = new uiMenuItem("&New ...");
    mnu.insertItem( newmenu, mNewIdx );
    newmenu->setEnabled( !hastransform );
    if ( children_.size() )
    {
	mnu.insertSeparator();
	uiPopupMenu* displaymnu =
		new uiPopupMenu( getUiParent(), "&Display all" );
	displaymnu->insertItem( new uiMenuItem("&Only at sections"),
				mSectIdx );
	displaymnu->insertItem( new uiMenuItem("&In full"), mFullIdx );
	displaymnu->insertItem( new uiMenuItem("&At sections and in full"),
				mSectFullIdx );
	mnu.insertItem( displaymnu );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid == mAddIdx || mnuid==mAddCBIdx )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectHorizons( objs, false ); 
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    if ( MPE::engine().getTrackerByObject(objs[idx]->id()) != -1 )
	    {
		 MPE::engine().addTracker( objs[idx] );
		 applMgr()->visServer()->turnSeedPickingOn( true );
	    }
	    uiODHorizonTreeItem* itm =
		new uiODHorizonTreeItem( objs[idx]->id(), mnuid==mAddCBIdx );
	    addChld( itm, false, false );
	}

	deepUnRef( objs );
    }
    else if ( mnuid == mNewIdx )
    {
	if ( !applMgr()->visServer()->
			 clickablesInScene(EM::Horizon3D::typeStr(),sceneID()) )
	    return true;
	
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet(
				applMgr()->attrServer()->curDescSet(false) );

	mps->addTracker( EM::Horizon3D::typeStr(), sceneID() );
	return true;
    }
    else if ( mnuid == mSectIdx || mnuid == mFullIdx || mnuid == mSectFullIdx )
    {
	const bool onlyatsection = mnuid == mSectIdx;
	const bool both = mnuid == mSectFullIdx;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODEarthModelSurfaceTreeItem*,itm,children_[idx])
	    if ( itm )
	    {
		const int displayid = itm->visEMObject()->id();
		mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
				applMgr()->visServer()->getObject(displayid))
		if ( !hd ) continue;

		hd->displayIntersectionLines( both );
		hd->setOnlyAtSectionsDisplay( onlyatsection );
		itm->updateColumnText( uiODSceneMgr::cColorColumn() );
	    }
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


static uiTreeItem* gtItm( const MultiID& mid, ObjectSet<uiTreeItem>& itms )
{
    for ( int idx=0; idx<itms.size(); idx++ )
    {
	mDynamicCastGet(const uiODEarthModelSurfaceTreeItem*,itm,itms[idx])
	const EM::ObjectID emid = itm && itm->visEMObject() ?
		     itm->visEMObject()->getObjectID() : -1;
	if ( mid == EM::EMM().getMultiID(emid) )
	    return itms[idx];
    }

    return 0;
}


void uiODHorizonParentTreeItem::sort()
{
    TypeSet<MultiID> mids, sortedmids;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(const uiODEarthModelSurfaceTreeItem*,itm,children_[idx])
	if ( !itm || !itm->visEMObject() )
	    continue;

	const EM::ObjectID emid = itm->visEMObject()->getObjectID();
	mids += EM::EMM().getMultiID( emid );
    }

    EM::EMM().sortHorizonsList( mids, sortedmids, false );
    uiTreeItem* previtm = 0;
    for ( int idx=sortedmids.size()-1; idx>=0; idx-- )
    {
	uiTreeItem* itm = gtItm( sortedmids[idx], children_ );
	if ( !itm ) continue;

	if ( !previtm )
	    itm->moveItemToTop();
	else
	    itm->moveItem( previtm );

	previtm = itm;
    }
}


bool uiODHorizonParentTreeItem::addChld( uiTreeItem* child, bool below,
					  bool downwards )
{
    bool res = uiTreeItem::addChld( child, below, downwards );
    if ( res ) sort();
    return res;
}


uiTreeItem*
    uiODHorizonTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    const FixedString objtype = uiVisEMObject::getObjectType(visid);
    if ( !objtype ) return 0;

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
          ODMainWin()->applMgr().visServer()->getObject(visid));
    if ( hd )
    {
	mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgba,
			 hd->getChannels2RGBA() );

	return new uiODHorizonTreeItem(visid, rgba, true);
    }


    return 0;
}


// uiODHorizonTreeItem

uiODHorizonTreeItem::uiODHorizonTreeItem( const EM::ObjectID& emid, bool rgba )
    : uiODEarthModelSurfaceTreeItem( emid )
    , rgba_( rgba )
{ initMenuItems(); }


uiODHorizonTreeItem::uiODHorizonTreeItem( int visid, bool rgba, bool )
    : uiODEarthModelSurfaceTreeItem( 0 )
    , rgba_( rgba )
{
    initMenuItems();
    displayid_ = visid;
}


void uiODHorizonTreeItem::initMenuItems()
{
    algomnuitem_.text = "&Tools";
    workflowsmnuitem_.text = "Workflows";
    positionmnuitem_.text = "&Position ...";
    shiftmnuitem_.text = "&Shift ...";
    fillholesmnuitem_.text = "&Gridding ...";
    filterhormnuitem_.text = "&Filtering ...";
    snapeventmnuitem_.text = "Snapping ...";
    geom2attrmnuitem_.text = "Store Z as &Attribute ...";
}


void uiODHorizonTreeItem::initNotify()
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,
	    	    emd,visserv_->getObject(displayid_));
    if ( emd )
	emd->changedisplay.notify( mCB(this,uiODHorizonTreeItem,dispChangeCB) );
}


BufferString uiODHorizonTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv =
	const_cast<uiODHorizonTreeItem*>(this)->visserv_;

    BufferString res = cvisserv->getObjectName( displayid_ );

    if (  uivisemobj_ && uivisemobj_->getShift() )
    {
	res += " (";
	res += uivisemobj_->getShift() * SI().zFactor();
	res += ")";
    }

    return res;
}


bool uiODHorizonTreeItem::init()
{
    if ( !createUiVisObj() )
	return false;

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
		    visserv_->getObject(displayid_));
    if ( rgba_ )
    {
	if ( !hd ) return false;

	mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgba,
			hd->getChannels2RGBA() );
	if ( !rgba )
	{
	    if ( !hd->setChannels2RGBA(
			visBase::RGBATextureChannel2RGBA::create()) )
		return false;

	    hd->addAttrib();
	    hd->addAttrib();
	    hd->addAttrib();
	}
    }

    visBase::HorizonSection* sect = hd ? hd->getHorizonSection(0) : 0;
    const bool geodf = !sect ? false    
	: (hd->geometryRowRange().width() && hd->geometryColRange().width());
    if ( geodf )
    {
	const StepInterval<int> rrg = hd->geometryRowRange();
	const StepInterval<int> crg = hd->geometryColRange();
	const HorSampling& rg = applMgr()->EMServer()->horizon3DDisplayRange();
	bool userchanged = rg.inlRange()!=rrg || rg.crlRange()!=crg;
	if ( rg.inlRange().start<rrg.start || rg.inlRange().stop>rrg.stop || 
	     rg.crlRange().start<crg.start || rg.crlRange().stop>crg.stop )
	    userchanged = false;		
	
	if ( rg.isDefined() && userchanged )
	{
	    sect->setDisplayRange( rg.inlRange(), rg.crlRange() );
	    for ( int idx=0; idx<hd->nrAttribs(); idx++ )
	    {
		if ( hd->hasDepth(idx) ) hd->setDepthAsAttrib( idx );
		else applMgr()->calcRandomPosAttrib( displayID(), idx );
	    }	    
	}
    }

    const bool res = uiODEarthModelSurfaceTreeItem::init();
    if ( !res ) return res;

    mDynamicCastGet(const EM::Horizon3D*,hor3d,EM::EMM().getObject(emid_))
    if ( hor3d )
    {
	const int nrauxdata = hor3d->auxdata.nrAuxData();
	for ( int idx=0; idx<nrauxdata; idx++ )
	{
	    DataPointSet vals( false, true );
	    float shift;
	    applMgr()->EMServer()->getAuxData( emid_, idx, vals, shift );
	    if ( vals.isEmpty() ) continue;

	    uiODDataTreeItem* itm = addAttribItem();
	    mDynamicCastGet(uiODEarthModelSurfaceDataTreeItem*,emitm,itm);
	    if ( emitm ) emitm->setDataPointSet( vals );
	}
    }

    return res;
}


void uiODHorizonTreeItem::dispChangeCB(CallBacker*)
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


bool uiODHorizonTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emid_, withcancel );
}


void uiODHorizonTreeItem::createMenuCB( CallBacker* cb )
{
    uiODEarthModelSurfaceTreeItem::createMenuCB( cb );
    mDynamicCastGet(MenuHandler*,menu,cb);

    mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()));

    const bool hastransform = scene && scene->getZAxisTransform();

    const Selector<Coord3>* selector = visserv_->getCoordSelector( sceneID() );

    if ( menu->menuID()!=displayID() || hastransform )
    {
	mResetMenuItem( &positionmnuitem_ );
	mResetMenuItem( &shiftmnuitem_ );
	mResetMenuItem( &fillholesmnuitem_ );
	mResetMenuItem( &filterhormnuitem_ );
	mResetMenuItem( &snapeventmnuitem_ );
	mResetMenuItem( &geom2attrmnuitem_ );
	mResetMenuItem( &createflatscenemnuitem_ );
    }
    else
    {
	mAddMenuItem( &displaymnuitem_, &positionmnuitem_, true, false );

	const bool islocked = visserv_->isLocked( displayID() );
	
	mAddMenuItem( menu, &algomnuitem_, true, false );
	mAddMenuItem( &algomnuitem_, &filterhormnuitem_, !islocked, false );
	mAddMenuItem( &algomnuitem_, &fillholesmnuitem_, !islocked, false );
	mAddMenuItem( &algomnuitem_, &shiftmnuitem_, !islocked, false )
	mAddMenuItem( &algomnuitem_, &snapeventmnuitem_, !islocked, false );
	mAddMenuItem( &algomnuitem_, &geom2attrmnuitem_, !islocked, false );

	mAddMenuItem( menu, &workflowsmnuitem_, true, false );
	mAddMenuItem( &workflowsmnuitem_, &createflatscenemnuitem_, true, false );
    }
}


#define mUpdateTexture() \
{ \
    mDynamicCastGet( visSurvey::HorizonDisplay*, hd, \
	    visserv_->getObject(displayid_) ); \
    if ( !hd ) return; \
    for ( int idx=0; idx<hd->nrAttribs(); idx++ ) \
    { \
	if ( hd->hasDepth(idx) ) hd->setDepthAsAttrib( idx ); \
	else applMgr()->calcRandomPosAttrib( visid, idx ); \
    } \
}

void uiODHorizonTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODEarthModelSurfaceTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller)
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    const int visid = displayID();
    uiEMPartServer* emserv = applMgr()->EMServer();
    uiEMAttribPartServer* emattrserv = applMgr()->EMAttribServer();
    uiAttribPartServer* attrserv = applMgr()->attrServer();
    bool handled = true;
    if ( mnuid==fillholesmnuitem_.id )
    {
	const bool isoverwrite = emserv->fillHoles( emid_, false );
	if ( isoverwrite ) { mUpdateTexture(); }
    }
    else if ( mnuid==filterhormnuitem_.id )
    {
	const bool isoverwrite = emserv->filterSurface( emid_ );
	if ( isoverwrite ) { mUpdateTexture(); }
    }
    else if ( mnuid==snapeventmnuitem_.id )
    {
	MultiID newmid;
	bool createnew = false;
	if ( emattrserv->snapHorizon(emid_,newmid,createnew,false) ) //Overwrite
	{
	    mUpdateTexture();
	}
	else if ( createnew )
	    emserv->displayEMObject( newmid );
    }
    else if ( mnuid==geom2attrmnuitem_.id )
    {
	if ( applMgr()->EMServer()->geom2Attr(emid_) )
	    mUpdateTexture();
    }
    else if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled(true);

	mDynamicCastGet( visSurvey::HorizonDisplay*, hd,
			 visserv_->getObject(displayid_) );
	if ( !hd ) return;

	visBase::HorizonSection* section = hd->getHorizonSection( 0 );
	if ( !section )
	    return;

	CubeSampling maxcs = SI().sampling(true);;
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	if ( scene && scene->getZAxisTransform() )
	{
	    const Interval<float> zintv =
		scene->getZAxisTransform()->getZInterval( false );
	    maxcs.zrg.start = zintv.start;
	    maxcs.zrg.stop = zintv.stop;
	}

	CubeSampling curcs;
	curcs.zrg.setFrom( SI().zRange(true) );
	curcs.hrg.set( section->displayedRowRange(),
		       section->displayedColRange() );

	uiPosProvider::Setup setup( false, true, false );
	setup.allownone_ = true;
	setup.seltxt( "Area subselection" );
	setup.cs_ = maxcs;
	
	uiDialog dlg( getUiParent(), 
		uiDialog::Setup("Positions","Specify positions", "103.1.6") );
	uiPosProvider pp( &dlg, setup );

	IOPar displaypar;
	pp.fillPar( displaypar );	//Get display type
	curcs.fillPar( displaypar );	//Get display ranges
	pp.usePar( displaypar );

	if ( !dlg.go() )
	    return;

	MouseCursorChanger cursorlock( MouseCursor::Wait );
	pp.fillPar( displaypar );
	
	CubeSampling newcs;
	if ( pp.isAll() )
	    newcs = maxcs;
	else
	    newcs.usePar( displaypar );

	section->setDisplayRange( newcs.hrg.inlRange(), newcs.hrg.crlRange() );
	emserv->setHorizon3DDisplayRange( newcs.hrg );
	
	for ( int idx=0; idx<hd->nrAttribs(); idx++ )
	{
	    if ( hd->hasDepth(idx) )
		hd->setDepthAsAttrib( idx );
	    else
		applMgr()->calcRandomPosAttrib( visid, idx );
	}
    }
    else if ( mnuid==shiftmnuitem_.id )
    {
	BoolTypeSet isenabled;
	const int nrattrib = visserv_->getNrAttribs( visid );
	for ( int idx=0; idx<nrattrib; idx++ )
	    isenabled += visserv_->isAttribEnabled( visid, idx ); 

	float curshift = visserv_->getTranslation( visid ).z;
	if ( mIsUdf( curshift ) ) curshift = 0;

	emattrserv->setDescSet( attrserv->curDescSet(false) );
	emattrserv->showHorShiftDlg( emid_, isenabled, curshift,
				     visserv_->canAddAttrib( visid, 1) );
    }
    else
	handled = false;

    menu->setIsHandled( handled );
}


uiODHorizon2DParentTreeItem::uiODHorizon2DParentTreeItem()
    : uiODTreeItem( "2D Horizon" )
{}


bool uiODHorizon2DParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    const bool hastransform = scene && scene->getZAxisTransform();
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Add ..."), 0 );
    uiMenuItem* newmenu = new uiMenuItem("&New ...");
    mnu.insertItem( newmenu, 1 );
    mnu.insertItem( new uiMenuItem("&Create from 3D ..."), 2 );
    newmenu->setEnabled( !hastransform );
    if ( children_.size() )
    {
	mnu.insertSeparator();
	mnu.insertItem( new uiMenuItem("&Display all only at sections"), 3 );
	mnu.insertItem( new uiMenuItem("&Show all in full"), 4 );
    }
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectHorizons( objs, true ); 
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    if ( MPE::engine().getTrackerByObject(objs[idx]->id()) != -1 )
	    {
		MPE::engine().addTracker( objs[idx] );
		applMgr()->visServer()->turnSeedPickingOn( true );
	    }
	    addChld( new uiODHorizon2DTreeItem(objs[idx]->id()), false, false);
	}

	deepUnRef( objs );
    }
    else if ( mnuid == 1 )
    {
	if ( !applMgr()->visServer()->
			clickablesInScene(EM::Horizon2D::typeStr(),sceneID()) )
	    return true; 

	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet(
			applMgr()->attrServer()->curDescSet(true) );
	
	mps->addTracker( EM::Horizon2D::typeStr(), sceneID() );
	return true;
    }
    else if ( mnuid == 2 )
    {
	uiHor2DFrom3DDlg dlg( getUiParent() );
	if( dlg.go() && dlg.doDisplay() )
	    addChld( new uiODHorizon2DTreeItem(dlg.getEMObjID()), true, false);
    }
    else if ( mnuid == 3 || mnuid == 4 )
    {
	const bool onlyatsection = mnuid == 3;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODHorizon2DTreeItem*,itm,children_[idx])
	    if ( itm )
	    {
		itm->visEMObject()->setOnlyAtSectionsDisplay( onlyatsection );
		itm->updateColumnText( uiODSceneMgr::cColorColumn() );
	    }
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


void uiODHorizon2DParentTreeItem::sort()
{
    TypeSet<MultiID> mids, sortedmids;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(const uiODEarthModelSurfaceTreeItem*,itm,children_[idx])
	if ( !itm || !itm->visEMObject() )
	    continue;

	const EM::ObjectID emid = itm->visEMObject()->getObjectID();
	mids += EM::EMM().getMultiID( emid );
    }

    EM::EMM().sortHorizonsList( mids, sortedmids, true );
    uiTreeItem* previtm = 0;
    for ( int idx=sortedmids.size()-1; idx>=0; idx-- )
    {
	uiTreeItem* itm = gtItm( sortedmids[idx], children_ );
	if ( !itm ) continue;

	if ( !previtm )
	    itm->moveItemToTop();
	else
	    itm->moveItem( previtm );

	previtm = itm;
    }
}


bool uiODHorizon2DParentTreeItem::addChld( uiTreeItem* child, bool below,
					    bool downwards )
{
    bool res = uiTreeItem::addChld( child, below, downwards );
    if ( res ) sort();
    return res;
}


uiTreeItem*
    uiODHorizon2DTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    FixedString objtype = uiVisEMObject::getObjectType(visid);
    return objtype==EM::Horizon2D::typeStr()
	? new uiODHorizon2DTreeItem(visid,true) : 0;
}


// uiODHorizon2DTreeItem

uiODHorizon2DTreeItem::uiODHorizon2DTreeItem( const EM::ObjectID& objid )
    : uiODEarthModelSurfaceTreeItem( objid )
{ initMenuItems(); }


uiODHorizon2DTreeItem::uiODHorizon2DTreeItem( int id, bool )
    : uiODEarthModelSurfaceTreeItem( 0 )
{ 
    initMenuItems();
    displayid_=id; 
}


void uiODHorizon2DTreeItem::initMenuItems()
{
    derive3dhormnuitem_.text = "Derive &3D horizon ...";
    snapeventmnuitem_.text = "Snap to &event ...";
    interpolatemnuitem_.text = "&Interpolate ...";
}


void uiODHorizon2DTreeItem::initNotify()
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,
	    	    emd,visserv_->getObject(displayid_));
    if ( emd )
	emd->changedisplay.notify(mCB(this,uiODHorizon2DTreeItem,dispChangeCB));
}


void uiODHorizon2DTreeItem::dispChangeCB(CallBacker*)
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


bool uiODHorizon2DTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emid_, withcancel );
}


void uiODHorizon2DTreeItem::createMenuCB( CallBacker* cb )
{
    uiODEarthModelSurfaceTreeItem::createMenuCB( cb );
    mDynamicCastGet(MenuHandler*,menu,cb)

    if ( menu->menuID()!=displayID() )
    {
	mResetMenuItem( &derive3dhormnuitem_ );
	mResetMenuItem( &createflatscenemnuitem_ );
	mResetMenuItem( &snapeventmnuitem_ );
	mResetMenuItem( &interpolatemnuitem_ );
    }
    else
    {
	const bool isempty = applMgr()->EMServer()->isEmpty(emid_);
	mAddMenuItem( menu, &derive3dhormnuitem_, !isempty, false );
	mAddMenuItem( menu, &createflatscenemnuitem_, !isempty, false );
	mAddMenuItem( menu, &snapeventmnuitem_, !isempty, false );
	mAddMenuItem( menu, &interpolatemnuitem_, !isempty, false );
    }
	
}


void uiODHorizon2DTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODEarthModelSurfaceTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller)
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    bool handled = true;
    if ( mnuid==interpolatemnuitem_.id )
    {
	const int visid = displayID();
	const bool isoverwrite = applMgr()->EMServer()->fillHoles( emid_, true);
	mUpdateTexture();
    }
    else if ( mnuid==derive3dhormnuitem_.id )
	applMgr()->EMServer()->deriveHor3DFrom2D( emid_ );
    else if ( mnuid==snapeventmnuitem_.id )
    {
	const int visid = displayID();
	MultiID newmid;
	bool createnew = false;
	if ( applMgr()->EMAttribServer()->snapHorizon(
		    emid_,newmid,createnew,true) )
	{
	    mUpdateTexture();
	}
	else if ( createnew )
	    applMgr()->EMServer()->displayEMObject( newmid );
    }
    else
	handled = false;

    menu->setIsHandled( handled );
}
