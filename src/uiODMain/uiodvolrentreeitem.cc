/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: uiodvolrentreeitem.cc,v 1.12 2007-09-07 18:37:56 cvskris Exp $";


#include "uiodvolrentreeitem.h"

#include "uicursor.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodattribtreeitem.h"
#include "uiodscenemgr.h"
#include "uiodmarchingcubessurfacetreeitem.h"
#include "uislicesel.h"
#include "uistatisticsdlg.h"
#include "vismarchingcubessurface.h"
#include "vismarchingcubessurfacedisplay.h"
#include "uivisisosurface.h"
#include "uivispartserv.h"
#include "visvolorthoslice.h"
#include "visvolren.h"
#include "visvolumedisplay.h"
#include "visisosurface.h"
#include "survinfo.h"
#include "zaxistransform.h"


uiODVolrenParentTreeItem::uiODVolrenParentTreeItem()
    : uiTreeItem("Volume")
{
    //Check if there are any volumes already in the scene
}


uiODVolrenParentTreeItem::~uiODVolrenParentTreeItem()
{}


bool uiODVolrenParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
	ODMainWin()->applMgr().visServer()->getObject(sceneID()));

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Add"), 0 );
    const int mnuid = mnu.exec();
    if ( mnuid==0 )
	addChild( new uiODVolrenTreeItem(-1), true );

    return true;
}


int uiODVolrenParentTreeItem::sceneID() const
{
    int sceneid;
    if ( !getProperty<int>( uiODTreeTop::sceneidkey, sceneid ) )
	return -1;
    return sceneid;
}


bool uiODVolrenParentTreeItem::init()
{
    return uiTreeItem::init();
}


const char* uiODVolrenParentTreeItem::parentType() const
{ return typeid(uiODTreeTop).name(); }



uiTreeItem* uiODVolrenTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
		    ODMainWin()->applMgr().visServer()->getObject(visid) );
    return vd ? new uiODVolrenTreeItem( visid ) : 0;
}


const char* uiODVolrenTreeItemFactory::getName()
{ return typeid(uiODVolrenTreeItemFactory).name(); }



uiODVolrenTreeItem::uiODVolrenTreeItem( int displayid )
    : positionmnuid_("Position ...")
    , statisticsmnuid_("Show Histogram ...") 
    , addmnuid_("Add")
    , addlinlslicemnuid_("In-line slice")
    , addlcrlslicemnuid_("Cross-line slice")
    , addltimeslicemnuid_("Time slice")
    , addvolumemnuid_("Volume")
    , addisosurfacemnuid_("Iso surface")
    , selattrmnuitem_( uiODAttribTreeItem::sKeySelAttribMenuTxt(), 10000 )
{ displayid_ = displayid; }


uiODVolrenTreeItem::~uiODVolrenTreeItem()
{
    uilistviewitem_->stateChanged.remove(
				mCB(this,uiODVolrenTreeItem,checkCB) );
    while( children_.size() )
	removeChild(children_[0]);

    visserv_->removeObject( displayid_, sceneID() );
}


bool uiODVolrenTreeItem::showSubMenu()
{
    return visserv_->showMenu( displayid_ );
}


const char* uiODVolrenTreeItem::parentType() const
{ return typeid(uiODVolrenParentTreeItem).name(); }


bool uiODVolrenTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::VolumeDisplay* display = visSurvey::VolumeDisplay::create();
	visserv_->addObject(display,sceneID(),false);
	displayid_ = display->id();
    }
    else
    {
	mDynamicCastGet(visSurvey::VolumeDisplay*,display,
			visserv_->getObject(displayid_) );
	if ( !display ) return false;
    }

    TypeSet<int> ownchildren;
    visserv_->getChildIds( displayid_, ownchildren );
    for ( int idx=0; idx<ownchildren.size(); idx++ )
	addChild( new uiODVolrenSubTreeItem(ownchildren[idx]), true);

    return uiODDisplayTreeItem::init();
}


BufferString uiODVolrenTreeItem::createDisplayName() const
{
    return uiODAttribTreeItem::createDisplayName( displayid_, 0 );
}


uiODDataTreeItem* uiODVolrenTreeItem::createAttribItem(
						const Attrib::SelSpec* ) const
{ return 0; }


void uiODVolrenTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu || menu->menuID() != displayID() ) return;

    selattrmnuitem_.removeItems();
    uiODAttribTreeItem::createSelMenu( selattrmnuitem_, displayID(),
	    				0, sceneID() );

    if ( selattrmnuitem_.nrItems() )
	mAddMenuItem( menu, &selattrmnuitem_,
			!visserv_->isLocked(displayID()), false );

    mAddMenuItem( menu, &positionmnuid_, true, false );
    mAddMenuItem( menu, &statisticsmnuid_, true, false );
    mAddMenuItem( menu, &addmnuid_, true, false );
    mAddMenuItem( &addmnuid_, &addlinlslicemnuid_, true, false );
    mAddMenuItem( &addmnuid_, &addlcrlslicemnuid_, true, false );
    mAddMenuItem( &addmnuid_, &addltimeslicemnuid_, true, false );
    mAddMenuItem( &addmnuid_, &addvolumemnuid_, !hasVolume(), false );
    mAddMenuItem( &addmnuid_, &addisosurfacemnuid_, true, false );
}


void uiODVolrenTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    uiMenuHandler* menu = dynamic_cast<uiMenuHandler*>(caller);
    if ( !menu || mnuid==-1 || menu->isHandled() || 
	 menu->menuID() != displayID() )
	return;

    mDynamicCastGet(visSurvey::VolumeDisplay*,voldisp,
	    	    visserv_->getObject(displayid_))
    if ( mnuid==positionmnuid_.id )
    {
	menu->setIsHandled( true );
	CubeSampling maxcs = SI().sampling( true );
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()));
	if ( scene && scene->getDataTransform() )
	{
	    const Interval<float> zintv =
		scene->getDataTransform()->getZInterval( false );
	    maxcs.zrg.start = zintv.start;
	    maxcs.zrg.stop = zintv.stop;
	}

	CallBack dummycb;
	uiSliceSel dlg( getUiParent(), voldisp->getCubeSampling(0), maxcs,
			dummycb, uiSliceSel::Vol );
	if ( !dlg.go() ) return;
	CubeSampling cs = dlg.getCubeSampling();
	voldisp->setCubeSampling( cs );
	visserv_->calculateAttrib( displayid_, 0, false );
	updateColumnText(0);
    }
    else if ( mnuid==statisticsmnuid_.id )
    {
        const DataPack::ID dpid = visserv_->getDataPackID( displayID(), 0 );
        const DataPackMgr::ID dmid = visserv_->getDataPackMgrID( displayID() );
        uiStatisticsDlg dlg( applMgr()->applService().parent() );
        dlg.setDataPackID( dpid, dmid );
        dlg.go();
        menu->setIsHandled( true );
    }	
    else if ( mnuid==addlinlslicemnuid_.id )
    {
	addChild( new uiODVolrenSubTreeItem(voldisp->addSlice(
			visSurvey::VolumeDisplay::cInLine())), true );
	menu->setIsHandled( true );
    }
    else if ( mnuid==addlcrlslicemnuid_.id )
    {
	addChild( new uiODVolrenSubTreeItem(voldisp->addSlice(
			visSurvey::VolumeDisplay::cCrossLine())), true );
	menu->setIsHandled( true );
    }
    else if ( mnuid==addltimeslicemnuid_.id )
    {
	addChild( new uiODVolrenSubTreeItem(voldisp->addSlice(
			visSurvey::VolumeDisplay::cTimeSlice())), true );
	menu->setIsHandled( true );
    }
    else if ( mnuid==addvolumemnuid_.id && !hasVolume() )
    {
	voldisp->showVolRen(true);
	int volrenid = voldisp->volRenID();
	addChild( new uiODVolrenSubTreeItem(volrenid), true );
	menu->setIsHandled( true );
    }
    else if ( mnuid==addisosurfacemnuid_.id )
    {
	menu->setIsHandled( true );
	static bool isshown = false;
	if ( !isshown )
	{
	    if ( !uiMSG().askGoOn("The iso surface generation is experimental\n"
		    "and may generate large amounts of display data\n"
		    "that could destabilize OpendTect.\n"
		    "Do you want to continue?" ) )
	    return;
	}

	isshown = true;
	uiCursorChanger cursorchanger( uiCursor::Wait );
	addChild( new uiODVolrenSubTreeItem(voldisp->addIsoSurface()), true );
    }
    else if ( uiODAttribTreeItem::handleSelMenu( mnuid, displayID(), 0 ) )
    {
	menu->setIsHandled( true );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
}


bool uiODVolrenTreeItem::anyButtonClick( uiListViewItem* item )
{
    if ( item!=uilistviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    if ( !visserv_->isClassification( displayID(), 0) )
	 ODMainWin()->applMgr().modifyColorTable( displayID(), 0 );

    return true;
}


bool uiODVolrenTreeItem::hasVolume() const
{
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(const uiODVolrenSubTreeItem*,sti,children_[idx]);
	if ( sti && sti->isVolume() )
	    return true;
    }

    return false;
}



uiODVolrenSubTreeItem::uiODVolrenSubTreeItem( int displayid )
    : setisovaluemnuid_("Set isovalue")
    , convertisotobodymnuid_("Convert to body")
{ displayid_ = displayid; }


uiODVolrenSubTreeItem::~uiODVolrenSubTreeItem()
{
    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
	    	     visserv_->getObject(parent_->selectionKey() ));

    if ( !vd ) return; 

    if ( displayid_==vd->volRenID() )
	vd->showVolRen(false);
    else
	vd->removeChild( displayid_ );
}


bool uiODVolrenSubTreeItem::isVolume() const
{
    mDynamicCastGet(visBase::VolrenDisplay*,voldisp,
	    	    visserv_->getObject(displayid_));
    return voldisp;
}


bool uiODVolrenSubTreeItem::isIsoSurface() const
{
    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
	    	    visserv_->getObject(displayid_));
    return isosurface;
}


const char* uiODVolrenSubTreeItem::parentType() const
{ return typeid(uiODVolrenTreeItem).name(); }


bool uiODVolrenSubTreeItem::init()
{
    if ( displayid_==-1 ) return false;

    mDynamicCastGet(visBase::VolrenDisplay*,volren,
	    	    visserv_->getObject(displayid_));
    mDynamicCastGet(visBase::OrthogonalSlice*,slice,
	    	    visserv_->getObject(displayid_));
    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
	    	    visserv_->getObject(displayid_));
    if ( !volren && !slice && !isosurface )
	return false;

    return uiODDisplayTreeItem::init();
}


bool uiODVolrenSubTreeItem::anyButtonClick( uiListViewItem* item )
{
    if ( item!=uilistviewitem_ )
	return uiODTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    const int displayid = parent_->selectionKey();
    if ( !visserv_->isClassification( displayid, 0) )
	 ODMainWin()->applMgr().modifyColorTable( displayid, 0 );

    return true;
}


void uiODVolrenSubTreeItem::updateColumnText(int col)
{
    if ( col!=1 || isVolume() )
    {
	uiODDisplayTreeItem::updateColumnText(col);
	return;
    }

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
	            visserv_->getObject(parent_->selectionKey()))
    if ( !vd ) return;

    mDynamicCastGet(visBase::OrthogonalSlice*,slice,
	    	    visserv_->getObject(displayid_));
    if ( slice )
    {
	BufferString coltext = vd->slicePosition( slice );
	uilistviewitem_->setText( coltext, col );
    }

    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
	    	    visserv_->getObject(displayid_));
    if ( isosurface && isosurface->getSurface() )
    {
	BufferString coltext = vd->isoValue(isosurface);
	uilistviewitem_->setText( coltext, col );
    }
}


void uiODVolrenSubTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu || menu->menuID() != displayID() ) return;
    if ( !isIsoSurface() )
    {
	mResetMenuItem( &setisovaluemnuid_ );
	mResetMenuItem( &convertisotobodymnuid_ );
	return;
    }

    mAddMenuItem( menu, &setisovaluemnuid_, true, false );
#ifdef __debug__
    mAddMenuItem( menu, &convertisotobodymnuid_, true, false );
#endif
}


void uiODVolrenSubTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    uiMenuHandler* menu = dynamic_cast<uiMenuHandler*>(caller);
    if ( !menu || mnuid==-1 || menu->isHandled() || 
	 menu->menuID() != displayID() )
	return;

    if ( mnuid==setisovaluemnuid_.id )
    {
	menu->setIsHandled( true );
	mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
			visserv_->getObject(displayid_));
	mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
			visserv_->getObject(parent_->selectionKey()));
	TypeSet<float> histogram;
	if ( vd->getHistogram(0) ) histogram = *vd->getHistogram(0);

	uiSingleGroupDlg dlg(getUiParent(), uiDialog::Setup(0,0,0) );
	dlg.setGroup( new uiVisIsoSurfaceThresholdDlg( &dlg, isosurface, vd ) );
	dlg.go();
    }
    else if ( mnuid==convertisotobodymnuid_.id )
    {
	menu->setIsHandled( true );
	mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
			visserv_->getObject(displayid_));
	mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
			visserv_->getObject(parent_->selectionKey()));

	isosurface->ref();

	RefMan<visSurvey::MarchingCubesDisplay> mcdisplay =
	    visSurvey::MarchingCubesDisplay::create();
	if ( !mcdisplay->setVisSurface( isosurface ) )
	{
	    isosurface->unRef();
	    return; //TODO error msg.
	}

	BufferString newname = "Iso ";
	newname += vd->isoValue( isosurface );
	mcdisplay->setName( newname.buf() );

	visserv_->addObject( mcdisplay, sceneID(), true );
	addChild( new uiODMarchingCubesTreeItem( mcdisplay->id(), true), false);
	prepareForShutdown();
	vd->removeChild( isosurface->id() );
	isosurface->unRef();

	parent_->removeChild( this );
    }
}
