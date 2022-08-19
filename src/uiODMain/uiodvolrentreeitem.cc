/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodvolrentreeitem.h"

#include "uiattribpartserv.h"
#include "uifiledlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodattribtreeitem.h"
#include "uiodscenemgr.h"
#include "uiodbodydisplaytreeitem.h"
#include "uiposprovider.h"
#include "uiseisamplspectrum.h"
#include "uislicesel.h"
#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uiviscoltabed.h"
#include "uivisisosurface.h"
#include "uivispartserv.h"
#include "uivisslicepos3d.h"
#include "vismarchingcubessurface.h"
#include "vismarchingcubessurfacedisplay.h"
#include "visrgbatexturechannel2rgba.h"
#include "visvolorthoslice.h"
#include "visvolumedisplay.h"

#include "filepath.h"
#include "ioobj.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "objdisposer.h"
#include "oddirs.h"
#include "settingsaccess.h"
#include "survinfo.h"
#include "zaxistransform.h"
#include "od_helpids.h"

#define mAddIdx		0
#define mAddCBIdx	1


/* OSG-TODO: Port VolrenDisplay and OrthogonalSlice occurences to OSG
   if these classes are prolongated. */


CNotifier<uiODVolrenParentTreeItem,uiMenu*>&
	uiODVolrenParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODVolrenParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODVolrenParentTreeItem::uiODVolrenParentTreeItem()
    : uiODParentTreeItem( uiStrings::sVolume() )
{
    //Check if there are any volumes already in the scene
}


uiODVolrenParentTreeItem::~uiODVolrenParentTreeItem()
{}


bool uiODVolrenParentTreeItem::canAddVolumeToScene()
{
    if ( SettingsAccess().doesUserWantShading(true) &&
	 visSurvey::VolumeDisplay::canUseVolRenShading() )
	return true;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet( uiODDisplayTreeItem*, itm, getChild(idx) );
	const VisID displayid = itm ? itm->displayID() : VisID::udf();
	mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		    ODMainWin()->applMgr().visServer()->getObject(displayid) );

	if ( vd && !vd->usesShading() )
	{
	    uiMSG().message(
		tr( "Can only display one fixed-function volume per scene.\n"
		    "If available, enabling OpenGL shading for volumes\n"
		    "in the 'Look and Feel' settings may help." ) );

	    return false;
	}
    }
    return true;
}


bool uiODVolrenParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(uiStrings::sAdd()), mAddIdx );
    mnu.insertAction( new uiAction(uiStrings::sAddColBlend()), mAddCBIdx );
    showMenuNotifier().trigger( &mnu, this );

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==mAddIdx || mnuid==mAddCBIdx )
    {
	if ( canAddVolumeToScene() )
	    addChild( new uiODVolrenTreeItem(VisID::udf(),mnuid==mAddCBIdx),
		      false );
    }

    handleStandardItems( mnuid );
    return true;
}


const char* uiODVolrenParentTreeItem::iconName() const
{ return "tree-vol"; }



uiTreeItem*
    uiODVolrenTreeItemFactory::createForVis( VisID visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
		    ODMainWin()->applMgr().visServer()->getObject(visid) );
    if ( vd )
    {
	mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgba,
			 vd->getChannels2RGBA() );
	return new uiODVolrenTreeItem( visid, rgba );
    }

    return 0;
}


const char* uiODVolrenTreeItemFactory::getName()
{ return typeid(uiODVolrenTreeItemFactory).name(); }



uiODVolrenTreeItem::uiODVolrenTreeItem( VisID displayid, bool rgba )
    : uiODDisplayTreeItem()
    , positionmnuitem_(m3Dots(tr("Position")))
    , rgba_(rgba)
{
    positionmnuitem_.iconfnm = "orientation64";
    displayid_ = displayid;
}


uiODVolrenTreeItem::~uiODVolrenTreeItem()
{
    uitreeviewitem_->stateChanged.remove(
				mCB(this,uiODVolrenTreeItem,checkCB) );
    while ( children_.size() )
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
    visSurvey::VolumeDisplay* voldisp;
    if ( !displayid_.isValid() )
    {
	voldisp = new visSurvey::VolumeDisplay;
	visserv_->addObject( voldisp, sceneID(), true );
	displayid_ = voldisp->id();
    }
    else
    {
	mDynamicCast( visSurvey::VolumeDisplay*, voldisp,
		      visserv_->getObject(displayid_) );
	if ( !voldisp ) return false;
    }

    if ( rgba_ )
    {
	mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgba,
			 voldisp->getChannels2RGBA() );
	if ( !rgba )
	{
	    if ( voldisp->setChannels2RGBA(
				visBase::RGBATextureChannel2RGBA::create()) )
	    {
		voldisp->addAttrib();
		voldisp->addAttrib();
		voldisp->addAttrib();
	    }
	    else
		return false;
	}
    }

    return uiODDisplayTreeItem::init();
}


uiString uiODVolrenTreeItem::createDisplayName() const
{
    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		     visserv_->getObject( displayid_ ) )
    uiString info;
    if ( vd )
	vd->getTreeObjectInfo( info );

    return info;
}


uiODDataTreeItem*
	uiODVolrenTreeItem::createAttribItem( const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false) : 0;

    if ( !res )
	res = new uiODVolrenAttribTreeItem( parenttype );

    return res;
}


void uiODVolrenTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || !isDisplayID(menu->menuID()) )
	return;

    const bool islocked = visserv_->isLocked( displayID() );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &positionmnuitem_,
		      !islocked, false );
}


void uiODVolrenTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    MenuHandler* menu = dynamic_cast<MenuHandler*>(caller);
    if ( !menu || mnuid==-1 || menu->isHandled() ||
	 !isDisplayID(menu->menuID()) )
	return;

    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		     visserv_->getObject( displayid_ ) )

    if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled( true );
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()));
	if ( scene && scene->getZAxisTransform() )
	{
	    const TrcKeyZSampling maxcs = scene->getTrcKeyZSampling();
	    CallBack dummycb;
	    uiSliceSelDlg dlg( getUiParent(),
			vd->getTrcKeyZSampling(true,true,-1),
			maxcs, dummycb, uiSliceSel::Vol,
			scene->zDomainInfo() );
	    if ( !dlg.go() ) return;

	    TrcKeyZSampling cs = dlg.getTrcKeyZSampling();
	    vd->setTrcKeyZSampling( cs );
	}
	else
	{
	    uiPosProvider::Setup su( false, false, true );
	    uiPosProvDlg dlg( getUiParent(), su, tr("Set Volume Area") );
	    dlg.setSampling( vd->getTrcKeyZSampling(true,true,-1) );
	    if ( !dlg.go() ) return;

	    TrcKeyZSampling tkzs;
	    dlg.getSampling( tkzs );
	    vd->setTrcKeyZSampling( tkzs );
	}

	visserv_->calculateAttrib( displayid_, 0, false );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
}


uiODVolrenAttribTreeItem::uiODVolrenAttribTreeItem( const char* ptype )
    : uiODAttribTreeItem( ptype )
    , addmnuitem_(uiStrings::sAdd())
    , statisticsmnuitem_(m3Dots(uiStrings::sHistogram()))
    , amplspectrummnuitem_(m3Dots(tr("Amplitude Spectrum")))
    , addisosurfacemnuitem_(m3Dots(tr("Create Iso Surface Geobody")))
{
    statisticsmnuitem_.iconfnm = "histogram";
    amplspectrummnuitem_.iconfnm = "amplspectrum";
}


void uiODVolrenAttribTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );

    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &statisticsmnuitem_,
		      true, false );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &amplspectrummnuitem_,
		      true, false );
    mAddMenuOrTBItem( istb, 0, &displaymnuitem_, &addisosurfacemnuitem_,
		      true, false );

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    displaymnuitem_.enabled = as &&
		    as->id().asInt()!=Attrib::SelSpec::cAttribNotSel().asInt();
}


void uiODVolrenAttribTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);

    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( !menu || mnuid==-1 )
	return;

    if ( menu->isHandled() )
    {
	if ( parent_ )
	    parent_->updateColumnText( uiODSceneMgr::cNameColumn() );
	return;
    }

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		     visserv->getObject( displayID() ) )

    if ( mnuid==statisticsmnuitem_.id )
    {
	const DataPackID dpid = visserv->getDataPackID( displayID(),
							  attribNr() );
	const DataPackMgr::MgrID dmid = visserv->getDataPackMgrID(displayID());
	const int version = visserv->selectedTexture( displayID(), attribNr() );
	uiStatsDisplay::Setup su; su.countinplot( false );
	uiStatsDisplayWin* dwin =
	    new uiStatsDisplayWin( applMgr()->applService().parent(),
				   su, 1, false );
	dwin->statsDisplay()->setDataPackID( dpid, dmid, version );
	dwin->setDataName( DPM(dmid).nameOf(dpid)  );
	dwin->windowClosed.notify( mCB(OBJDISP(),ObjDisposer,go) );
	dwin->show();
        menu->setIsHandled( true );
    }
    else if ( mnuid==amplspectrummnuitem_.id )
    {
	const DataPackID dpid = visserv->getDataPackID(
					displayID(), attribNr() );
	const DataPackMgr::MgrID dmid =
		visserv->getDataPackMgrID( displayID() );
	const int version = visserv->selectedTexture(
					displayID(), attribNr() );
	uiSeisAmplSpectrum* asd = new uiSeisAmplSpectrum(
				  applMgr()->applService().parent() );
	asd->setDataPackID( dpid, dmid, version );
	asd->windowClosed.notify( mCB(OBJDISP(),ObjDisposer,go) );
	asd->show();
	menu->setIsHandled( true );
    }
    else if ( mnuid==addisosurfacemnuitem_.id )
    {
	menu->setIsHandled( true );
	const VisID surfobjid = vd->addIsoSurface( 0, false );
	const int surfidx = vd->getNrIsoSurfaces()-1;
	visBase::MarchingCubesSurface* mcs = vd->getIsoSurface(surfidx);
	uiSingleGroupDlg dlg( applMgr()->applService().parent(),
		uiDialog::Setup(tr("Iso value selection"),mNoDlgTitle,
				mODHelpKey(mVolrenTreeItemHelpID)) );
	dlg.setGroup( new uiVisIsoSurfaceThresholdDlg(&dlg,mcs,vd,attribNr()) );
	if ( !dlg.go() )
	{
	    vd->removeChild( surfobjid );
	    return;
	}

	RefMan<visSurvey::MarchingCubesDisplay> mcdisplay =
	    new visSurvey::MarchingCubesDisplay;

	uiString newname = tr( "Iso %1").arg( vd->isoValue( mcs ) );
	mcdisplay->setUiName( newname );
	if ( !mcdisplay->setVisSurface(mcs) )
	{
	    vd->removeChild( surfobjid );
	    return;
	}

	visserv->addObject( mcdisplay, sceneID(), true );
	addChild( new uiODBodyDisplayTreeItem(mcdisplay->id(),true), false );
	vd->removeChild( surfobjid );
    }
}


bool uiODVolrenAttribTreeItem::hasTransparencyMenu() const
{
    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		ODMainWin()->applMgr().visServer()->getObject(displayID()) );

    return vd && vd->usesShading();
}


uiODVolrenSubTreeItem::uiODVolrenSubTreeItem( VisID displayid )
    : resetisosurfacemnuitem_(uiStrings::sSettings())
    , convertisotobodymnuitem_(tr("Convert to Geobody"))
{ displayid_ = displayid; }


uiODVolrenSubTreeItem::~uiODVolrenSubTreeItem()
{
    detachAllNotifiers();
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
		    visserv_->getObject(getParentDisplayID()))

    if ( !vd )
	return;

    if ( displayid_==vd->volRenID() )
	vd->showVolRen(false);
    else
	vd->removeChild( displayid_ );

    visserv_->getUiSlicePos()->setDisplay( VisID::udf() );
}


VisID uiODVolrenSubTreeItem::getParentDisplayID() const
{
    mDynamicCastGet( uiODDataTreeItem*, datatreeitem, parent_ );
    return datatreeitem ? datatreeitem->displayID() : VisID::udf();
}


int uiODVolrenSubTreeItem::getParentAttribNr() const
{
    mDynamicCastGet( uiODDataTreeItem*, datatreeitem, parent_ );
    return datatreeitem ? datatreeitem->attribNr() : -1;
}


bool uiODVolrenSubTreeItem::isIsoSurface() const
{
    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
		    visserv_->getObject(displayid_));
    return isosurface;
}


const char* uiODVolrenSubTreeItem::parentType() const
{ return typeid(uiODVolrenAttribTreeItem).name(); }


bool uiODVolrenSubTreeItem::init()
{
    if ( !displayid_.isValid() )
	return false;

//    mDynamicCastGet(visBase::VolrenDisplay*,volren,
//		    visserv_->getObject(displayid_));
    mDynamicCastGet(visBase::OrthogonalSlice*,slice,
		    visserv_->getObject(displayid_));
    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
		    visserv_->getObject(displayid_));
    if ( /*!volren && */ !slice && !isosurface )
	return false;

    if ( slice )
    {
	slice->setSelectable( true );
	slice->deSelection()->notify( mCB(this,uiODVolrenSubTreeItem,selChgCB));

	mAttachCB( *slice->selection(), uiODVolrenSubTreeItem::selChgCB );
	mAttachCB( *slice->deSelection(), uiODVolrenSubTreeItem::selChgCB);
	mAttachCB( visserv_->getUiSlicePos()->positionChg,
		  uiODVolrenSubTreeItem::posChangeCB);
    }

    return uiODDisplayTreeItem::init();
}


void uiODVolrenSubTreeItem::updateColumnText(int col)
{
    if ( col!=1 )
    {
	uiODDisplayTreeItem::updateColumnText(col);
	return;
    }

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
		    visserv_->getObject(getParentDisplayID()))
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

	uitreeviewitem_->setText( toUiString(mNINT32(dispval)), col );
    }

    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
		    visserv_->getObject(displayid_));
    if ( isosurface && isosurface->getSurface() )
    {
	const float isoval = vd->isoValue(isosurface);
	uiString coltext;
	if ( !mIsUdf(isoval) )
	    coltext = toUiString(isoval);
	uitreeviewitem_->setText( coltext, col );
    }
}


void uiODVolrenSubTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || !isDisplayID(menu->menuID()) || istb )
	return;

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
	 !isDisplayID(menu->menuID()) )
	return;

    if ( mnuid==resetisosurfacemnuitem_.id )
    {
	menu->setIsHandled( true );
	mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
			visserv_->getObject(displayid_));
	mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
			visserv_->getObject(getParentDisplayID()));

	uiSingleGroupDlg dlg( getUiParent(),
	uiDialog::Setup( tr("Iso Value Selection"), mNoDlgTitle,
			     mNoHelpKey ) );
	dlg.setGroup( new uiVisIsoSurfaceThresholdDlg(&dlg, isosurface, vd,
						      getParentAttribNr()) );
	if ( dlg.go() )
	    updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( mnuid==convertisotobodymnuitem_.id )
    {
	menu->setIsHandled( true );
	mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
			visserv_->getObject(displayid_));
	mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
			visserv_->getObject(getParentDisplayID()));

	isosurface->ref();

	RefMan<visSurvey::MarchingCubesDisplay> mcdisplay =
	    new visSurvey::MarchingCubesDisplay;

	uiString newname = tr( "Iso %1").arg( vd->isoValue( isosurface ) );
	mcdisplay->setUiName( newname );

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


void uiODVolrenSubTreeItem::posChangeCB( CallBacker* )
{
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
		    visserv_->getObject(getParentDisplayID()));
    mDynamicCastGet(visBase::OrthogonalSlice*,slice,
		    visserv_->getObject(displayid_));
    if ( !slice || !vd || !vd->getSelectedSlice() ) return;

    uiSlicePos3DDisp* slicepos = visserv_->getUiSlicePos();
    if ( slicepos->getDisplayID() != getParentDisplayID() ||
	 vd->getSelectedSlice()->id() != displayid_ )
	return;

    vd->setSlicePosition( slice, slicepos->getTrcKeyZSampling() );
}


void uiODVolrenSubTreeItem::selChgCB( CallBacker* )
{
    visserv_->getUiSlicePos()->setDisplay( getParentDisplayID() );
}
