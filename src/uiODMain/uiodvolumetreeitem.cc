/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/



#include "uiodvolumetreeitem.h"

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
#include "visvolorthoslice.h"
#include "visvolumedisplay.h"

#include "attribprobelayer.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "probeimpl.h"
#include "probemanager.h"
#include "objdisposer.h"
#include "oddirs.h"
#include "settingsaccess.h"
#include "survinfo.h"
#include "zaxistransform.h"
#include "od_helpids.h"

#define mAddIdx		0
#define mAddCBIdx	1


/* OSG-TODO: Port VolumeDisplay and OrthogonalSlice occurences to OSG
   if these classes are prolongated. */


uiODVolumeParentTreeItem::uiODVolumeParentTreeItem()
    : uiODSceneProbeParentTreeItem( uiStrings::sVolume() )
{
}


uiODVolumeParentTreeItem::~uiODVolumeParentTreeItem()
{
}


bool uiODVolumeParentTreeItem::canAddVolumeToScene()
{
    if ( SettingsAccess().doesUserWantShading(true) &&
	 visSurvey::VolumeDisplay::canUseVolRenShading() )
	return true;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet( uiODDisplayTreeItem*, itm, getChild(idx) );
	const int displayid = itm ? itm->displayID() : -1;
	mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		    ODMainWin()->applMgr().visServer()->getObject(displayid) );

	if ( vd && !vd->usesShading() )
	{
	    mTIUiMsg().error(
		tr( "Can only display one fixed-function volume per scene.\n"
		    "If available, enabling OpenGL shading for volumes\n"
		    "in the 'Look and Feel' settings may help." ) );

	    return false;
	}
    }

    return true;
}


Probe* uiODVolumeParentTreeItem::createNewProbe() const
{
    return new VolumeProbe();
}


uiPresManagedTreeItem* uiODVolumeParentTreeItem::addChildItem(
	const Presentation::ObjInfo& prinfo )
{
    mDynamicCastGet(const ProbePresentationInfo*,probeprinfo,&prinfo)
    if ( !probeprinfo )
	return 0;

    RefMan<Probe> probe = ProbeMGR().fetchForEdit( probeprinfo->storedID() );
    mDynamicCastGet(VolumeProbe*,volprobe,probe.ptr())
    if ( !volprobe )
	return 0;

    uiODVolumeTreeItem* newitem = new uiODVolumeTreeItem( *probe );
    addChild( newitem, false );
    return newitem;
}


bool uiODVolumeParentTreeItem::setProbeToBeAddedParams( int mnuid )
{
    if ( !isSceneAddMnuId(mnuid) )
	return false;

    typetobeadded_ = getAddType( mnuid );
    SI().getSampling( cs_, OD::UsrWork );
    TrcKeyZSampling fullcs( true );
    const bool haveworkarea = !(cs_.hsamp_ == fullcs.hsamp_);
    if ( !haveworkarea )
    {
	const Interval<int> inlrg = cs_.hsamp_.lineRange();
	const Interval<int> crlrg = cs_.hsamp_.trcRange();
	const BinID center( inlrg.center(), crlrg.center() );
	const BinID width( inlrg.width(), crlrg.width() );

#	define mSetICBound(ic,strtstp,plusmin) \
	if ( width.ic() > 100 ) \
	{ \
	    if ( width.ic() < 200 ) \
		cs_.hsamp_.strtstp.ic() = center.ic() plusmin width.ic() / 4; \
	    else \
		cs_.hsamp_.strtstp.ic() = center.ic() plusmin width.ic() / 20; \
	}

	mSetICBound( inl, start_, - )
	mSetICBound( crl, start_, - )
	mSetICBound( inl, stop_, + )
	mSetICBound( crl, stop_, + )
    }

    const int nrsteps = cs_.zsamp_.nrSteps();
    const int fullnrsteps = fullcs.zsamp_.nrSteps();
    const bool haveworkzrg = nrsteps != fullnrsteps; //!< probably works
    getDefZRange( cs_.zsamp_ );
    if ( !haveworkzrg && nrsteps > 100 )
    {
	const float zcenter = cs_.zsamp_.center();
	const float zwidth = cs_.zsamp_.width() * .1f;
	const float scl = nrsteps < 200 ? 0.25f : 0.1f;
	cs_.zsamp_.start = zcenter - zwidth * scl;
	cs_.zsamp_.stop = zcenter + zwidth * scl;
	fullcs.zsamp_.snap( cs_.zsamp_.start );
	fullcs.zsamp_.snap( cs_.zsamp_.stop );
    }

    return true;
}


const char* uiODVolumeParentTreeItem::iconName() const
{
    return "tree-vol";
}


const char* uiODVolumeTreeItemFactory::getName()
{
    return typeid(uiODVolumeTreeItemFactory).name();
}



uiODVolumeTreeItem::uiODVolumeTreeItem( Probe& probe, int displayid )
    : uiODSceneProbeTreeItem( probe )
    , positionmnuitem_(m3Dots(uiStrings::sPosition()))
{
    positionmnuitem_.iconfnm = "orientation64";
    displayid_ = displayid;
}


uiODVolumeTreeItem::~uiODVolumeTreeItem()
{
    uitreeviewitem_->stateChanged.remove(
				mCB(this,uiODVolumeTreeItem,checkCB) );
    while ( children_.size() )
	removeChild(children_[0]);

    visserv_->removeObject( displayid_, sceneID() );
}


bool uiODVolumeTreeItem::showSubMenu()
{
    return visserv_->showMenu( displayid_, uiMenuHandler::fromTree() );
}


const char* uiODVolumeTreeItem::parentType() const
{
    return typeid(uiODVolumeParentTreeItem).name();
}


bool uiODVolumeTreeItem::init()
{
    visSurvey::VolumeDisplay* voldisp;
    if ( displayid_==-1 )
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

    return uiODSceneProbeTreeItem::init();
}


uiString uiODVolumeTreeItem::createDisplayName() const
{
    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		     visserv_->getObject( displayid_ ) )
    uiString info;
    if ( vd )
	vd->getTreeObjectInfo( info );

    return info;
}


void uiODVolumeTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    const bool islocked = visserv_->isLocked( displayID() );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &positionmnuitem_,
		      !islocked, false );
}


void uiODVolumeTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    MenuHandler* menu = dynamic_cast<MenuHandler*>(caller);
    if ( !menu || mnuid==-1 || menu->isHandled() ||
	 menu->menuID() != displayID() )
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


uiODVolumeAttribTreeItem::uiODVolumeAttribTreeItem( const char* ptype )
    : uiODAttribTreeItem( ptype )
    , statisticsmnuitem_(m3Dots(uiStrings::sHistogram()))
    , amplspectrummnuitem_(m3Dots(tr("Amplitude Spectrum")))
    , addisosurfacemnuitem_(m3Dots(tr("Create Iso Surface Body")))
{
    statisticsmnuitem_.iconfnm = "histogram";
    amplspectrummnuitem_.iconfnm = "amplspectrum";
}


uiODDataTreeItem* uiODVolumeAttribTreeItem::create( ProbeLayer& prblay )
{
    const char* parenttype = typeid(uiODVolumeTreeItem).name();
    uiODVolumeAttribTreeItem* attribtreeitem =
	new uiODVolumeAttribTreeItem( parenttype );
    attribtreeitem->setProbeLayer( &prblay );
    return attribtreeitem;

}


void uiODVolumeAttribTreeItem::initClass()
{
    uiODDataTreeItem::fac().addCreateFunc(
	    create, AttribProbeLayer::sFactoryKey(),VolumeProbe::sFactoryKey());

}


void uiODVolumeAttribTreeItem::createMenu( MenuHandler* menu, bool istb )
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
		    as->id() != Attrib::SelSpec::cAttribNotSelID();
}


void uiODVolumeAttribTreeItem::handleMenuCB( CallBacker* cb )
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
	const DataPack::ID dpid = visserv->getDataPackID(
					displayID(), attribNr() );
	const DataPackMgr::ID dmid =
		visserv->getDataPackMgrID( displayID() );
	const int version = visserv->selectedTexture(
					displayID(), attribNr() );
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
	const DataPack::ID dpid =
			    visserv->getDataPackID( displayID(), attribNr() );
	const DataPackMgr::ID dmid = visserv->getDataPackMgrID( displayID() );
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
	const int surfobjid = vd->addIsoSurface( 0, false );
	const int surfidx = vd->getNrIsoSurfaces()-1;
	visBase::MarchingCubesSurface* mcs = vd->getIsoSurface(surfidx);

        uiSingleGroupDlg<> dlg( applMgr()->applService().parent(),
                     new uiVisIsoSurfaceThresholdDlg( 0,mcs,vd,attribNr()));
        dlg.setHelpKey( mODHelpKey(mVolrenTreeItemHelpID) );

	if ( !dlg.go() )
	{
	    vd->removeChild( surfobjid );
	    return;
	}

	RefMan<visSurvey::MarchingCubesDisplay> mcdisplay =
	    new visSurvey::MarchingCubesDisplay;

	uiString newname = tr("Iso %1").arg( vd->isoValue( mcs ) );
	mcdisplay->setUiName( newname );

	if ( !mcdisplay->setVisSurface(mcs) )
	    { vd->removeChild( surfobjid ); return; }

	visserv->addObject( mcdisplay, sceneID(), true );
	addChild(new uiODBodyDisplayTreeItem(mcdisplay->id(),true), false);
	vd->removeChild( surfobjid );
    }
}


bool uiODVolumeAttribTreeItem::hasTransparencyMenu() const
{
    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		ODMainWin()->applMgr().visServer()->getObject(displayID()) );

    return vd && vd->usesShading();
}


uiODVolumeSubTreeItem::uiODVolumeSubTreeItem( int displayid )
    : resetisosurfacemnuitem_(uiStrings::sSettings())
    , convertisotobodymnuitem_(tr("Convert to Body"))
{ displayid_ = displayid; }


uiODVolumeSubTreeItem::~uiODVolumeSubTreeItem()
{
    detachAllNotifiers();
    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		     visserv_->getObject(getParentDisplayID()));

    if ( !vd ) return;

    if ( displayid_ == vd->volRenID() )
	vd->showVolRen( false );
    else
	vd->removeChild( displayid_ );

    visserv_->getUiSlicePos()->setDisplay( -1 );
}


int uiODVolumeSubTreeItem::getParentDisplayID() const
{
    mDynamicCastGet( uiODDataTreeItem*, datatreeitem, parent_ );
    return datatreeitem ? datatreeitem->displayID() : -1;
}


int uiODVolumeSubTreeItem::getParentAttribNr() const
{
    mDynamicCastGet( uiODDataTreeItem*, datatreeitem, parent_ );
    return datatreeitem ? datatreeitem->attribNr() : -1;
}


bool uiODVolumeSubTreeItem::isIsoSurface() const
{
    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
		    visserv_->getObject(displayid_));
    return isosurface;
}


const char* uiODVolumeSubTreeItem::parentType() const
{ return typeid(uiODVolumeAttribTreeItem).name(); }


bool uiODVolumeSubTreeItem::init()
{
    if ( displayid_==-1 ) return false;

    mDynamicCastGet(visBase::OrthogonalSlice*,slice,
		    visserv_->getObject(displayid_));
    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
		    visserv_->getObject(displayid_));
    if ( !slice && !isosurface )
	return false;

    if ( slice )
    {
	slice->setSelectable( true );
	slice->deSelection()->notify( mCB(this,uiODVolumeSubTreeItem,selChgCB));

	mAttachCB( *slice->selection(), uiODVolumeSubTreeItem::selChgCB );
	mAttachCB( *slice->deSelection(), uiODVolumeSubTreeItem::selChgCB);
	mAttachCB( visserv_->getUiSlicePos()->positionChg,
		  uiODVolumeSubTreeItem::posChangeCB);
    }

    return uiODDisplayTreeItem::init();
}


void uiODVolumeSubTreeItem::updateColumnText(int col)
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


void uiODVolumeSubTreeItem::createMenu( MenuHandler* menu, bool istb )
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


void uiODVolumeSubTreeItem::handleMenuCB( CallBacker* cb )
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
			visserv_->getObject(getParentDisplayID()));

        uiSingleGroupDlg<> dlg( getUiParent(),
            new uiVisIsoSurfaceThresholdDlg( 0, isosurface, vd,
                                            getParentAttribNr()));

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

	uiString newname = tr("Iso %1").arg( vd->isoValue(isosurface) );
	mcdisplay->setUiName( newname );

	if ( !mcdisplay->setVisSurface( isosurface ) )
	    { isosurface->unRef(); return; } //TODO error msg.

	visserv_->addObject( mcdisplay, sceneID(), true );
	addChild( new uiODBodyDisplayTreeItem(mcdisplay->id(),true), false );
	prepareForShutdown();
	vd->removeChild( isosurface->id() );
	isosurface->unRef();

	parent_->removeChild( this );
    }
}


void uiODVolumeSubTreeItem::posChangeCB( CallBacker* cb )
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


void uiODVolumeSubTreeItem::selChgCB( CallBacker* cb )
{
    visserv_->getUiSlicePos()->setDisplay( getParentDisplayID() );
}
