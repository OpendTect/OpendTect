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

#include "objdisposer.h"
#include "settingsaccess.h"
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



uiTreeItem* uiODVolrenTreeItemFactory::createForVis( const VisID& visid,
						     uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
		    ODMainWin()->applMgr().visServer()->getObject(visid) );
    if ( vd )
    {
	mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgba,
			 vd->getChannels2RGBA() );
	return new uiODVolrenTreeItem( visid, rgba );
    }

    return nullptr;
}


const char* uiODVolrenTreeItemFactory::getName()
{ return typeid(uiODVolrenTreeItemFactory).name(); }



uiODVolrenTreeItem::uiODVolrenTreeItem( const VisID& displayid, bool rgba )
    : uiODDisplayTreeItem()
    , positionmnuitem_(m3Dots(tr("Position")))
    , rgba_(rgba)
{
    displayid_ = displayid;
    positionmnuitem_.iconfnm = "orientation64";
}


uiODVolrenTreeItem::~uiODVolrenTreeItem()
{
    detachAllNotifiers();
    uitreeviewitem_->stateChanged.remove(
				mCB(this,uiODVolrenTreeItem,checkCB) );
    while ( !children_.isEmpty() )
	removeChild( children_.first() );

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
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::VolumeDisplay> voldisp = new visSurvey::VolumeDisplay;
	displayid_ = voldisp->id();
	if ( rgba_ )
	{
	    RefMan<visBase::RGBATextureChannel2RGBA> text2rgba =
				visBase::RGBATextureChannel2RGBA::create();
	    if ( voldisp->setChannels2RGBA(text2rgba.ptr()) )
	    {
		voldisp->addAttrib();
		voldisp->addAttrib();
		voldisp->addAttrib();
	    }
	    else
		return false;
	}

	visserv_->addObject( voldisp.ptr(), sceneID(), true);
    }

    mDynamicCastGet(visSurvey::VolumeDisplay*,voldisp,
		    visserv_->getObject(displayid_));
    if ( !voldisp )
	return false;

    volumedisplay_ = voldisp;
    return uiODDisplayTreeItem::init();
}


ConstRefMan<visSurvey::VolumeDisplay> uiODVolrenTreeItem::getDisplay() const
{
    return volumedisplay_.get();
}


RefMan<visSurvey::VolumeDisplay> uiODVolrenTreeItem::getDisplay()
{
    return volumedisplay_.get();
}


uiString uiODVolrenTreeItem::createDisplayName() const
{
    ConstRefMan<visSurvey::VolumeDisplay> volumedisplay = getDisplay();
    uiString info;
    if ( volumedisplay )
	volumedisplay->getTreeObjectInfo( info );

    return info;
}


uiODDataTreeItem*
	uiODVolrenTreeItem::createAttribItem( const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false)
	: nullptr;

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

    RefMan<visSurvey::VolumeDisplay> volumedisplay = getDisplay();
    if ( !volumedisplay )
	return;

    if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled( true );
	RefMan<visSurvey::Scene> scene = visserv_->getScene( sceneID() );
	if ( scene && scene->getZAxisTransform() )
	{
	    const TrcKeyZSampling maxcs = scene->getTrcKeyZSampling();
	    CallBack dummycb;
	    uiSliceSelDlg dlg( getUiParent(),
			volumedisplay->getTrcKeyZSampling(true,true,-1),
			maxcs, dummycb, uiSliceSel::Vol,
			scene->zDomainInfo() );
	    dlg.grp()->enableApplyButton( false );
	    dlg.grp()->enableScrollButton( false );
	    if ( dlg.go() != uiDialog::Accepted )
		return;

	    TrcKeyZSampling cs = dlg.getTrcKeyZSampling();
	    volumedisplay->setTrcKeyZSampling( cs );
	}
	else
	{
	    uiPosProvider::Setup su( false, false, true );
	    uiPosProvDlg dlg( getUiParent(), su, tr("Set Volume Area") );
	    dlg.setSampling( volumedisplay->getTrcKeyZSampling(true,true,-1) );
	    if ( !dlg.go() )
		return;

	    TrcKeyZSampling tkzs;
	    dlg.getSampling( tkzs );
	    volumedisplay->setTrcKeyZSampling( tkzs );
	}

	visserv_->calculateAttrib( displayid_, 0, false );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
}


// uiODVolrenAttribTreeItem

uiODVolrenAttribTreeItem::uiODVolrenAttribTreeItem( const char* ptype )
    : uiODAttribTreeItem( ptype )
    , addmnuitem_(uiStrings::sAdd())
    , addisosurfacemnuitem_(m3Dots(tr("Create Iso Surface Geobody")))
{
}


uiODVolrenAttribTreeItem::~uiODVolrenAttribTreeItem()
{}


void uiODVolrenAttribTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );

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

    if ( mnuid==addisosurfacemnuitem_.id )
    {
	menu->setIsHandled( true );
	const VisID surfobjid = vd->addIsoSurface( 0, false );
	const int surfidx = vd->getNrIsoSurfaces()-1;
	visBase::MarchingCubesSurface* mcs = vd->getIsoSurface(surfidx);
	uiSingleGroupDlg dlg( applMgr()->applService().parent(),
		uiDialog::Setup(tr("Iso value selection"),
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

	visserv->addObject( mcdisplay.ptr(), sceneID(), true);
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



// uiODVolrenSubTreeItem
uiODVolrenSubTreeItem::uiODVolrenSubTreeItem( const VisID& displayid )
    : resetisosurfacemnuitem_(uiStrings::sSettings())
    , convertisotobodymnuitem_(tr("Convert to Geobody"))
{
    displayid_ = displayid;
}


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
		   uiODVolrenSubTreeItem::posChangeCB );
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
	    RefMan<visSurvey::Scene> scene = visserv_->getScene( sceneID() );
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
			      uiDialog::Setup(tr("Iso Value Selection"),
					      mNoHelpKey) );
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

	visserv_->addObject( mcdisplay.ptr(), sceneID(), true);
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
