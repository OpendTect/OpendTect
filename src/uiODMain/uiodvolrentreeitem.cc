/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id: uiodvolrentreeitem.cc,v 1.70 2012-07-10 14:59:01 cvskris Exp $";


#include "uiodvolrentreeitem.h"

#include "uiamplspectrum.h"
#include "uiattribpartserv.h"
#include "mousecursor.h"
#include "settings.h"
#include "oddirs.h"
#include "uifiledlg.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodattribtreeitem.h"
#include "uiodscenemgr.h"
#include "uiodbodydisplaytreeitem.h"
#include "uislicesel.h"
#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"
#include "uiobjdisposer.h"
#include "vismarchingcubessurface.h"
#include "vismarchingcubessurfacedisplay.h"
#include "uiviscoltabed.h"
#include "uivisisosurface.h"
#include "uivisslicepos3d.h"
#include "uivispartserv.h"
#include "visvolorthoslice.h"
#include "visvolren.h"
#include "visvolumedisplay.h"
#include "filepath.h"
#include "ioobj.h"
#include "keystrs.h"
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
    mnu.insertItem( new uiMenuItem("&Add"), 0 );
    const int mnuid = mnu.exec();
    if ( mnuid==0 )
	addChild( new uiODVolrenTreeItem(-1), true );

    return true;
}


int uiODVolrenParentTreeItem::sceneID() const
{
    int sceneid;
    if ( !getProperty<int>( uiODTreeTop::sceneidkey(), sceneid ) )
	return -1;
    return sceneid;
}


bool uiODVolrenParentTreeItem::init()
{
    return uiTreeItem::init();
}


const char* uiODVolrenParentTreeItem::parentType() const
{ return typeid(uiODTreeTop).name(); }



uiTreeItem*
    uiODVolrenTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
		    ODMainWin()->applMgr().visServer()->getObject(visid) );
    return vd ? new uiODVolrenTreeItem( visid ) : 0;
}


const char* uiODVolrenTreeItemFactory::getName()
{ return typeid(uiODVolrenTreeItemFactory).name(); }



uiODVolrenTreeItem::uiODVolrenTreeItem( int displayid )
    : addmnuitem_("&Add")
    , statisticsmnuitem_("&Histogram ...") 
    , amplspectrummnuitem_( "&Amplitude Spectrum ...")		     
    , positionmnuitem_("&Position ...")
    , addlinlslicemnuitem_("&In-line slice")
    , addlcrlslicemnuitem_("&Cross-line slice")
    , addltimeslicemnuitem_("&Z slice")
    , addvolumemnuitem_("&Volume")
    , addisosurfacemnuitem_("Iso s&urface")
    , selattrmnuitem_( uiODAttribTreeItem::sKeySelAttribMenuTxt(), 10000 )
    , colsettingsmnuitem_( uiODAttribTreeItem::sKeyColSettingsMenuTxt() )
    , savevolumemnuitem_( "Save volume file" )
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
    return visserv_->showMenu( displayid_, uiMenuHandler::fromTree() );
}


const char* uiODVolrenTreeItem::parentType() const
{ return typeid(uiODVolrenParentTreeItem).name(); }


bool uiODVolrenTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::VolumeDisplay* display = visSurvey::VolumeDisplay::create();
	visserv_->addObject(display,sceneID(),true);
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
	addChild( new uiODVolrenSubTreeItem(ownchildren[idx]), true );

    return uiODDisplayTreeItem::init();
}


BufferString uiODVolrenTreeItem::createDisplayName() const
{
    return uiODAttribTreeItem::createDisplayName( displayid_, 0 );
}


uiODDataTreeItem* uiODVolrenTreeItem::createAttribItem(
						const Attrib::SelSpec* ) const
{ return 0; }


void uiODVolrenTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() || istb ) return;

    const bool islocked = visserv_->isLocked( displayID() );
    selattrmnuitem_.removeItems();
    uiODAttribTreeItem::createSelMenu( selattrmnuitem_, displayID(),
	    				0, sceneID() );
    if ( selattrmnuitem_.nrItems() )
	mAddMenuItem( menu, &selattrmnuitem_, !islocked, false );

    const uiAttribPartServer* attrserv = applMgr()->attrServer();
    const Attrib::SelSpec* as = visserv_->getSelSpec( displayID(), 0 );
    if ( attrserv->getIOObj(*as) )
	mAddMenuItem( menu, &colsettingsmnuitem_, true, false );

    mAddMenuItem( menu, &displaymnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &addmnuitem_, true, false );
    mAddMenuItem( &addmnuitem_, &addlinlslicemnuitem_, true, false );
    mAddMenuItem( &addmnuitem_, &addlcrlslicemnuitem_, true, false );
    mAddMenuItem( &addmnuitem_, &addltimeslicemnuitem_, true, false );
    mAddMenuItem( &addmnuitem_, &addvolumemnuitem_, !hasVolume(), false );
    mAddMenuItem( &addmnuitem_, &addisosurfacemnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &amplspectrummnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &statisticsmnuitem_, true, false );
    mAddMenuItem( &displaymnuitem_, &positionmnuitem_, !islocked, false );

    bool yn = false;
    Settings::common().getYN( IOPar::compKey("dTect","Dump OI Menu"), yn ); 
    if ( yn )
	mAddMenuItem( menu, &savevolumemnuitem_, true, false )
    else
	mResetMenuItem( &savevolumemnuitem_ );

}


void uiODVolrenTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    MenuHandler* menu = dynamic_cast<MenuHandler*>(caller);
    if ( !menu || mnuid==-1 || menu->isHandled() || 
	 menu->menuID() != displayID() )
	return;

    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
	    	     visserv_->getObject( displayid_ ) )
    if ( mnuid == colsettingsmnuitem_.id )
    {
	menu->setIsHandled(true);
	const uiAttribPartServer* attrserv = applMgr()->attrServer();
	const Attrib::SelSpec* as = visserv_->getSelSpec( displayID(), 0 );
	PtrMan<IOObj> ioobj = attrserv->getIOObj( *as );
	if ( !ioobj ) return;

	FilePath fp( ioobj->fullUserExpr(true) );
	fp.setExtension( "par" );
	BufferString fnm = fp.fullPath();
	IOPar iop;
	ODMainWin()->colTabEd().fillPar( iop );
	iop.write( fnm, sKey::Pars() );
    }
    else if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled( true );
	CubeSampling maxcs = SI().sampling( true );
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()));
	if ( scene && scene->getZAxisTransform() )
	    maxcs = scene->getCubeSampling();

	CallBack dummycb;
	uiSliceSelDlg dlg( getUiParent(), vd->getCubeSampling(true,true,-1),
			   maxcs, dummycb, uiSliceSel::Vol,
			   scene->zDomainInfo() );
	if ( !dlg.go() ) return;
	CubeSampling cs = dlg.getCubeSampling();
	vd->setCubeSampling( cs );
	visserv_->calculateAttrib( displayid_, 0, false );
	updateColumnText(0);
    }
    else if ( mnuid==statisticsmnuitem_.id )
    {
        const DataPack::ID dpid = visserv_->getDataPackID( displayID(), 0 );
        const DataPackMgr::ID dmid = visserv_->getDataPackMgrID( displayID() );
	uiStatsDisplay::Setup su; su.countinplot( false );
	uiStatsDisplayWin* dwin =
	    new uiStatsDisplayWin( applMgr()->applService().parent(),
		    		   su, 1, false );
	dwin->statsDisplay()->setDataPackID( dpid, dmid );
	dwin->setDataName( DPM(dmid).nameOf(dpid)  );
	dwin->windowClosed.notify( mCB(uiOBJDISP(),uiObjDisposer,go) );
	dwin->show();
        menu->setIsHandled( true );
    }	
    else if ( mnuid==amplspectrummnuitem_.id )
    {
	const DataPack::ID dpid = visserv_->getDataPackID( displayID(), 0 );
	const DataPackMgr::ID dmid = visserv_->getDataPackMgrID( displayID() );
	uiAmplSpectrum* asd = new uiAmplSpectrum(
					applMgr()->applService().parent() );
	asd->setDataPackID( dpid, dmid );
	asd->windowClosed.notify( mCB(uiOBJDISP(),uiObjDisposer,go) );
	asd->show();
	menu->setIsHandled( true );
    }

    else if ( mnuid==addlinlslicemnuitem_.id )
    {
	addChild( new uiODVolrenSubTreeItem(
		  vd->addSlice(visSurvey::VolumeDisplay::cInLine()) ), true );
	menu->setIsHandled( true );
    }
    else if ( mnuid==addlcrlslicemnuitem_.id )
    {
	addChild( new uiODVolrenSubTreeItem(
		  vd->addSlice(visSurvey::VolumeDisplay::cCrossLine())), true );
	menu->setIsHandled( true );
    }
    else if ( mnuid==addltimeslicemnuitem_.id )
    {
	addChild( new uiODVolrenSubTreeItem(
		  vd->addSlice(visSurvey::VolumeDisplay::cTimeSlice())), true );
	menu->setIsHandled( true );
    }
    else if ( mnuid==addvolumemnuitem_.id && !hasVolume() )
    {
	vd->showVolRen(true);
	int volrenid = vd->volRenID();
	addChild( new uiODVolrenSubTreeItem(volrenid), true );
	menu->setIsHandled( true );
    }
    else if ( mnuid==addisosurfacemnuitem_.id )
    {
	menu->setIsHandled( true );
	const int surfobjid = vd->addIsoSurface( 0, false );
	const int surfidx = vd->getNrIsoSurfaces()-1;
	visBase::MarchingCubesSurface* mcs = vd->getIsoSurface(surfidx);
	uiSingleGroupDlg dlg( applMgr()->applService().parent(),
		uiDialog::Setup( "Iso value selection", 0, "50.0.16" ) );
	dlg.setGroup( new uiVisIsoSurfaceThresholdDlg(&dlg,mcs,vd) );
	dlg.go();

	addChild( new uiODVolrenSubTreeItem(surfobjid), true );
    }
    else if ( mnuid==savevolumemnuitem_.id )
    {
	menu->setIsHandled( true );
	uiFileDialog filedlg( getUiParent(), false, GetPersonalDir(), "*.vol",
			      "Select volume filename" );
	if ( filedlg.go() )
	{
	    if ( !vd->writeVolume( filedlg.fileName() ) )
	    {
		uiMSG().error( vd->errMsg() );
	    }
	}
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

    if ( visserv_->interpolationEnabled( displayID() ) )
	 ODMainWin()->applMgr().updateColorTable( displayID(), 0 );

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
    : resetisosurfacemnuitem_("&Settings")
    , convertisotobodymnuitem_("&Convert to body")
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

    visserv_->getUiSlicePos()->setDisplay( -1 );
}


bool uiODVolrenSubTreeItem::isVolume() const
{
    mDynamicCastGet(visBase::VolrenDisplay*,vd,visserv_->getObject(displayid_));
    return vd;
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

    if ( slice )
    {
	slice->setSelectable( true );
	slice->deSelection()->notify( mCB(this,uiODVolrenSubTreeItem,selChgCB));
	
	mAttachCB( *slice->selection(), uiODVolrenSubTreeItem, selChgCB );
	mAttachCB( *slice->deSelection(), uiODVolrenSubTreeItem, selChgCB);
	mAttachCB( visserv_->getUiSlicePos()->positionChg,
		   uiODVolrenSubTreeItem, posChangeCB);
    }

    return uiODDisplayTreeItem::init();
}


bool uiODVolrenSubTreeItem::anyButtonClick( uiListViewItem* item )
{
    if ( item!=uilistviewitem_ )
	return uiODTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    const int displayid = parent_->selectionKey();
    if ( visserv_->interpolationEnabled( displayid ) )
	 ODMainWin()->applMgr().updateColorTable( displayid, 0 );

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
	float dispval = vd->slicePosition( slice );
	if ( slice->getDim() == 0 )
	{
	    mDynamicCastGet(visSurvey::Scene*,scene,
		ODMainWin()->applMgr().visServer()->getObject(sceneID()));
	    dispval *= scene->zDomainUserFactor();
	}

	uilistviewitem_->setText( toString(mNINT32(dispval)), col );
    }

    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
	    	    visserv_->getObject(displayid_));
    if ( isosurface && isosurface->getSurface() )
    {
	const float isoval = vd->isoValue(isosurface);
	BufferString coltext;
        if ( mIsUdf(isoval) )
	    coltext = "";
	else coltext = isoval;
	uilistviewitem_->setText( coltext.buf(), col );
    }
}


void uiODVolrenSubTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() || istb ) return;

    if ( !isIsoSurface() )
    {
	mResetMenuItem( &resetisosurfacemnuitem_ );
	mResetMenuItem( &convertisotobodymnuitem_ );
	return;
    }

    mAddMenuItem( menu, &resetisosurfacemnuitem_, true, false );
    mAddMenuItem( menu, &convertisotobodymnuitem_, true, false );
}


void uiODVolrenSubTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    MenuHandler* menu = dynamic_cast<MenuHandler*>(caller);
    if ( !menu || mnuid==-1 || menu->isHandled() || 
	 menu->menuID() != displayID() )
	return;

    if ( mnuid==resetisosurfacemnuitem_.id )
    {
	menu->setIsHandled( true );
	mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
			visserv_->getObject(displayid_));
	mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
			visserv_->getObject(parent_->selectionKey()));

	uiSingleGroupDlg dlg( getUiParent(),
		uiDialog::Setup( "Iso value selection", 0, mNoHelpID ) );
	dlg.setGroup( new uiVisIsoSurfaceThresholdDlg(&dlg, isosurface, vd) );
	dlg.go();
	updateColumnText(1);
    }
    else if ( mnuid==convertisotobodymnuitem_.id )
    {
	menu->setIsHandled( true );
	mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
			visserv_->getObject(displayid_));
	mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
			visserv_->getObject(parent_->selectionKey()));

	isosurface->ref();

	RefMan<visSurvey::MarchingCubesDisplay> mcdisplay =
	    visSurvey::MarchingCubesDisplay::create();

	BufferString newname = "Iso ";
	newname += vd->isoValue( isosurface );
	mcdisplay->setName( newname.buf() );

	if ( !mcdisplay->setVisSurface( isosurface ) )
	{
	    isosurface->unRef();
	    return; //TODO error msg.
	}

	visserv_->addObject( mcdisplay, sceneID(), true );
	addChild( new uiODBodyDisplayTreeItem(mcdisplay->id(),true), false );
	prepareForShutdown();
	vd->removeChild( isosurface->id() );
	isosurface->unRef();

	parent_->removeChild( this );
    }
}


void uiODVolrenSubTreeItem::posChangeCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
		    visserv_->getObject(parent_->selectionKey()));
    mDynamicCastGet(visBase::OrthogonalSlice*,slice,
	    	    visserv_->getObject(displayid_));
    if ( !slice || !vd || !vd->getSelectedSlice() ) return;

    uiSlicePos3DDisp* slicepos = visserv_->getUiSlicePos();
    if ( slicepos->getDisplayID() != parent_->selectionKey() ||
	    vd->getSelectedSlice()->id() != displayid_ )
	return;

    vd->setSlicePosition( slice, slicepos->getCubeSampling() );
}


void uiODVolrenSubTreeItem::selChgCB( CallBacker* cb )
{
    visserv_->getUiSlicePos()->setDisplay( parent_->selectionKey() );
}
