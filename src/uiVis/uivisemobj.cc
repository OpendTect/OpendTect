/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivisemobj.h"

#include "attribsel.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emioobjinfo.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "genc.h"
#include "keyboardevent.h"
#include "mousecursor.h"
#include "od_helpids.h"
#include "settings.h"
#include "survinfo.h"
#include "simpnumer.h"
#include "vishorizonsectiondef.h"

#include "uicolortable.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uimpe.h"
#include "uimsg.h"
#include "uimenuhandler.h"
#include "uiseedpropdlg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uivispartserv.h"


// uiVisEMObject

uiVisEMObject::uiVisEMObject( uiParent* uip, const EM::ObjectID& emid,
			      const SceneID& sceneid, uiVisPartServer* vps )
    : uiparent_( uip )
    , visserv_( vps )
    , sceneid_(sceneid)
{
    const EM::EMObject* emobj = EM::EMM().getObject( emid );
    RefMan<visSurvey::EMObjectDisplay> emod;
    mDynamicCastGet(const EM::Horizon3D*,hor3d,emobj);
    if ( hor3d )
    {
	checkHorizonSize( hor3d );
	emod = new visSurvey::HorizonDisplay;
    }

    mDynamicCastGet(const EM::Horizon2D*,hor2d,emobj);
    if ( hor2d )
	emod = new visSurvey::Horizon2DDisplay;

    if ( !emod )
    {
	pErrMsg("Incorrect data type for uiVisEMObject");
	if ( OD::InDebugMode() )
	    DBG::forceCrash( false );

	return;
    }

    displayid_ = emod->id();
    RefMan<visSurvey::Scene> scene = visserv_->getScene( sceneid_ );
    if ( !scene )
    {
	pErrMsg("Cannot add a uiVisEMObject without a scene");
	if ( OD::InDebugMode() )
	    DBG::forceCrash( false );

	return;
    }

    emod->setZDomain( emobj->zDomain() );
    emod->setDisplayTransformation( scene->getUTM2DisplayTransform() );
    auto* zt = const_cast<ZAxisTransform*>( scene->getZAxisTransform() );
    emod->setZAxisTransform( zt, nullptr );

    uiTaskRunner dlg( uiparent_ );
    if ( !emod->setEMObject(emid,&dlg) )
	return;

    visserv_->addObject( emod, sceneid_, true );

    emobjdisplay_ = emod;
    setUpConnections();
}


uiVisEMObject::uiVisEMObject( uiParent* uip, const VisID& displayid,
			      uiVisPartServer* vps )
    : uiparent_(uip)
    , visserv_(vps)
    , displayid_(displayid)
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    RefMan<visSurvey::EMObjectDisplay> emod;
    if ( visserv_ )
	emod = dCast( visSurvey::EMObjectDisplay*,
		      visserv_->getObject(displayid) );

    if ( !emod )
	return;

    const MultiID mid = emod->getMultiID();
    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    const EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet( const EM::Horizon3D*, hor3d, emobj );
    if ( hor3d )
	checkHorizonSize( hor3d );

    visSurvey::Scene* scene = emod->getScene();
    if ( scene )
	sceneid_ = scene->getID();

    mDynamicCastGet(const visSurvey::HorizonDisplay*,hordisp,emod.ptr());

    uiTaskRunner dlg( uiparent_ );
    if ( !EM::EMM().getObject(emid) )
    {
	PtrMan<Executor> exec;
	EM::IOObjInfo oi( mid ); EM::SurfaceIOData sd;
	uiString errmsg;
	if ( !oi.getSurfaceData(sd,errmsg) )
	    exec = EM::EMM().objectLoader( mid );
	else
	{
	    EM::SurfaceIODataSelection sel( sd );
	    sel.setDefault();
	    sel.selvalues.erase();

	    const BufferStringSet sections = emod->displayedSections();

	    TypeSet<int> sectionidx;
	    for ( int idx=sections.size()-1; idx>=0; idx-- )
	    {
		const int idy = sel.sd.sections.indexOf( *sections[idx] );
		if ( idy!=-1 )
		    sectionidx += idy;
	    }

	    if ( sectionidx.size() )
		sel.selsections = sectionidx;

	    if ( hordisp )
	    {
		const StepInterval<int> rowrg = hordisp->geometryRowRange();
		const StepInterval<int> colrg = hordisp->geometryColRange();
		if ( rowrg.step_!=-1 && colrg.step_!=-1 )
		{
		    sel.rg.start_.inl() = rowrg.start_;
		    sel.rg.start_.crl() = colrg.start_;
		    sel.rg.stop_.inl() = rowrg.stop_;
		    sel.rg.step_.crl() = colrg.step_;
		    sel.rg.step_.inl() = rowrg.step_;
		    sel.rg.stop_.crl() = colrg.stop_;
		}
	    }

	    exec = EM::EMM().objectLoader( mid, &sel );
	}

	if ( exec )
	{
	    emid = EM::EMM().getObjectID( mid );
	    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	    if ( !TaskRunner::execute(&dlg,*exec) )
	    {
		if ( scene )
		    visserv_->removeObject( displayid_, sceneid_ );

		return;
	    }

	    emobject.setNoDelete( true ); // TODO really ??
	}
    }

    if ( !emod->setEMObject(emid,&dlg) )
    {
	if ( scene )
	    visserv_->removeObject( displayid_, sceneid_ );

	return;
    }

    if ( hordisp && hordisp->usesTexture() )
    {
	for ( int idx=0; idx<emod->nrAttribs(); idx++ )
	{
	    if ( hordisp->getSelSpec(idx)->id().asInt()
		 ==Attrib::SelSpec::cNoAttrib().asInt() )
		setDepthAsAttrib( idx );
	}
    }

    emobjdisplay_ = emod;
    setUpConnections();
}


uiVisEMObject::~uiVisEMObject()
{
    detachAllNotifiers();
    visserv_->removeObject( displayid_, sceneid_ );
}


bool uiVisEMObject::isOK() const
{
    return emobjdisplay_;
}


void uiVisEMObject::setUpConnections()
{
    singlecolmnuitem_.text = tr("Use single color");
    singlecolmnuitem_.checkable = true;

    seedsmenuitem_.text = tr("Seeds");
    seedsmenuitem_.checkable = false;
    showseedsmnuitem_.text = uiStrings::sShow();
    seedpropmnuitem_.text = m3Dots(uiStrings::sProperties());
    lockseedsmnuitem_.text = uiStrings::sLock();

    ctrlpointsmenuitem_.text = tr("Control Points");
    ctrlpointsmenuitem_.checkable = false;
    showctrlpointsmnuitem_.text = uiStrings::sShow();
    ctrlpointspropmnuitem_.text = m3Dots(uiStrings::sProperties());

    showonlyatsectionsmnuitem_.text = tr("Only at sections");
    showfullmnuitem_.text = tr("In full");
    showbothmnuitem_.text = tr("At sections and in full");
    showsurfacegridmnuitem_.text = tr("Surface Grid");

    showonlyatsectionsmnuitem_.checkable = true;
    showfullmnuitem_.checkable = true;
    showbothmnuitem_.checkable = true;
    showsurfacegridmnuitem_ .checkable = true;

    RefMan<MenuHandler> menu = visserv_->getMenuHandler();
    mAttachCB( menu->createnotifier, uiVisEMObject::createMenuCB );
    mAttachCB( menu->handlenotifier, uiVisEMObject::handleMenuCB );

    RefMan<MenuHandler> tbmenu = visserv_->getToolBarHandler();
    mAttachCB( tbmenu->createnotifier, uiVisEMObject::addToToolBarCB );
    mAttachCB( tbmenu->handlenotifier, uiVisEMObject::handleMenuCB );

    mAttachCB( visserv_->keyEvent, uiVisEMObject::keyEventCB );
}


void uiVisEMObject::setDepthAsAttrib( int attrib )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    RefMan<visSurvey::HorizonDisplay> hordisp = getHorizon3DDisplay();
    if ( hordisp )
	hordisp->setDepthAsAttrib( attrib );
}


int uiVisEMObject::nrSections() const
{
    return isOK() ? 1 : 0;
}


EM::SectionID uiVisEMObject::getSectionID( int idx ) const
{
    return EM::SectionID::def();
}


EM::SectionID uiVisEMObject::getSectionID( const TypeSet<VisID>* path ) const
{
    return path && isOK() ? EM::SectionID::def() : EM::SectionID::udf();
}


void uiVisEMObject::checkTrackingStatus()
{
    RefMan<visSurvey::EMObjectDisplay> emod = getDisplay();
    if ( emod )
	emod->updateFromMPE();
}


float uiVisEMObject::getShift() const
{
    ConstRefMan<visSurvey::HorizonDisplay> hordisp = getHorizon3DDisplay();
    return hordisp ? (float) hordisp->getTranslation().z : 0.f;
}


void uiVisEMObject::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID() != displayid_.asInt() || !isOK() )
	return;

    ConstRefMan<visSurvey::HorizonDisplay> hordisp = getHorizon3DDisplay();
    ConstRefMan<visSurvey::Horizon2DDisplay> hor2ddisp = getHorizon2DDisplay();
    const visSurvey::EMObjectDisplay* emod = hordisp
		? (const visSurvey::EMObjectDisplay*)hordisp.ptr()
		: (const visSurvey::EMObjectDisplay*)hor2ddisp.ptr();
    const EM::ObjectID emid = emod->getObjectID();
    const EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj )
	return;

    MenuItemHolder* displaymnuitem = menu->findItem( "Display" );
    if ( !displaymnuitem )
	displaymnuitem = menu;

    seedsmenuitem_.removeItems();
    ctrlpointsmenuitem_.removeItems();
    ConstRefMan<visSurvey::Scene> scene = emod->getScene();
    const bool hastransform = scene && scene->getZAxisTransform();
    if ( displaymnuitem )
    {
	const bool hasseeds = emobj->hasPosAttrib( EM::EMObject::sSeedNode() );
	showseedsmnuitem_.text = emod->showsPosAttrib(EM::EMObject::sSeedNode())
			       ? uiStrings::sHide() : uiStrings::sShow();
	mAddMenuItem( &seedsmenuitem_, &showseedsmnuitem_,
		      !hastransform && hasseeds, false );
	mAddMenuItem( &seedsmenuitem_, &seedpropmnuitem_, true, false );
	mAddMenuItem( displaymnuitem, &seedsmenuitem_,
		      seedsmenuitem_.nrItems(), false );

	if ( emobj->hasPosAttrib(EM::EMObject::sTemporaryControlNode()) )
	{
	    showctrlpointsmnuitem_.text =
	       emod->showsPosAttrib(EM::EMObject::sTemporaryControlNode())
		    ? uiStrings::sHide() : uiStrings::sShow();
	    mAddMenuItem( &ctrlpointsmenuitem_, &showctrlpointsmnuitem_,
			  !hastransform, false );
	    mAddMenuItem( &ctrlpointsmenuitem_, &ctrlpointspropmnuitem_, true,
			  false );
	    mAddMenuItem( displaymnuitem, &ctrlpointsmenuitem_,
			  ctrlpointsmenuitem_.nrItems(), false );
	}
    }

    if ( hor2ddisp )
	mResetMenuItem( &singlecolmnuitem_ )
    else if ( displaymnuitem )
    {
	mAddMenuItem( displaymnuitem, &singlecolmnuitem_,
		      !emod->displayedOnlyAtSections() &&
				      (!hordisp || hordisp->canShowTexture()),
		      !hordisp || !hordisp->showsTexture() );
    }

    const bool atsect = emod->displayedOnlyAtSections();
    bool infull, both = false, showgrid = false;
    if ( hor2ddisp )
	infull = !emod->displayedOnlyAtSections();
    else
    {
	infull = !emod->displayedOnlyAtSections() &&
			!hordisp->displaysIntersectionLines();
	both = !emod->displayedOnlyAtSections() &&
			hordisp->displaysIntersectionLines();
	showgrid = !emod->displayedOnlyAtSections() &&
			hordisp->displaysSurfaceGrid();
    }

    if ( hordisp )
    {
	mAddMenuItem( displaymnuitem, &showsurfacegridmnuitem_, true, showgrid);
    }
    else
    {
	mResetMenuItem( &showsurfacegridmnuitem_ );
    }

    mAddMenuItem( displaymnuitem, &showonlyatsectionsmnuitem_, true,
		  atsect );
    mAddMenuItem( displaymnuitem, &showfullmnuitem_, true, infull );
    if ( hordisp )
    {
	mAddMenuItem( displaymnuitem, &showbothmnuitem_, true, both );
    }
    else
    {
	mResetMenuItem( &showbothmnuitem_ );
    }

    //Commented out as mAddMenu is commented out below
    //visSurvey::Scene* scene = hordisp ? hordisp->getScene() : 0;
    //const bool hastransform = scene && scene->getZAxisTransform();
    //const bool enabmenu =
	//getObjectType(displayid_)==EM::Horizon3D::typeStr()
	//&& !visserv_->isLocked(displayid_) && !hastransform;
}


void uiVisEMObject::addToToolBarCB( CallBacker* )
{
}


void uiVisEMObject::keyEventCB( CallBacker* )
{
}


void uiVisEMObject::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    RefMan<visSurvey::EMObjectDisplay> emod = getDisplay();
    if ( !emod || mnuid==-1 || !menu ||
	 menu->isHandled() || menu->menuID()!=displayid_.asInt() )
	return;

    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, emod.ptr() );
    const EM::ObjectID emid = emod->getObjectID();
    EM::EMObject* emobj = EM::EMM().getObject(emid);

    if ( mnuid==singlecolmnuitem_.id )
    {
	if ( hordisp ) hordisp->useTexture( !hordisp->showsTexture(), true );
	visserv_->triggerTreeUpdate();
	menu->setIsHandled( true );
    }
    else if ( mnuid==showonlyatsectionsmnuitem_.id )
    {
	setOnlyAtSectionsDisplay( true );
	menu->setIsHandled( true );
    }
    else if ( mnuid==showfullmnuitem_.id )
    {
	setOnlyAtSectionsDisplay( false );
	if ( hordisp )
	    hordisp->displayIntersectionLines( false );
	menu->setIsHandled( true );
    }
    else if ( mnuid==showbothmnuitem_.id )
    {
	setOnlyAtSectionsDisplay( false );
	if ( hordisp )
	    hordisp->displayIntersectionLines( true );
	menu->setIsHandled( true );
    }
    else if ( mnuid == showsurfacegridmnuitem_.id )
    {
	if ( hordisp )
	{
	    bool showgrid = hordisp->displaysSurfaceGrid();
	    hordisp->displaysSurfaceGrid( !showgrid );
	}
	menu->setIsHandled( true );
    }
    else if ( mnuid==showseedsmnuitem_.id )
    {
	menu->setIsHandled( true );
	emod->showPosAttrib( EM::EMObject::sSeedNode(),
			     !emod->showsPosAttrib(EM::EMObject::sSeedNode()) );
    }
    else if ( mnuid==seedpropmnuitem_.id )
    {
	if ( emobj && !visserv_->showSetupGroupOnTop("Properties") )
	{
	    uiSeedPropDlg dlg( uiparent_, emobj );
	    dlg.go();
	}
	menu->setIsHandled( true );
    }
    else if ( mnuid==lockseedsmnuitem_.id )
    {
	if ( emobj )
	    emobj->lockPosAttrib( EM::EMObject::sSeedNode(),
			!emobj->isPosAttribLocked(EM::EMObject::sSeedNode()) );
	menu->setIsHandled( true );
    }
    else if ( mnuid==showctrlpointsmnuitem_.id )
    {
	menu->setIsHandled( true );
	emod->showPosAttrib( EM::EMObject::sTemporaryControlNode(),
		!emod->showsPosAttrib(EM::EMObject::sTemporaryControlNode()) );
    }
    else if ( mnuid==ctrlpointspropmnuitem_.id )
    {
	if ( emobj && !visserv_->showSetupGroupOnTop("Properties") )
	{
	    uiSeedPropDlg dlg( uiparent_, emobj,
				EM::EMObject::sTemporaryControlNode() );
	    dlg.go();
	}
	menu->setIsHandled( true );
    }
}


void uiVisEMObject::setOnlyAtSectionsDisplay( bool yn )
{
    RefMan<visSurvey::EMObjectDisplay> emod = getDisplay();
    if ( emod )
	emod->setOnlyAtSectionsDisplay( yn );
}


bool uiVisEMObject::isOnlyAtSections() const
{
    ConstRefMan<visSurvey::EMObjectDisplay> emod = getDisplay();
    return emod ? emod->displayedOnlyAtSections() : false;
}


EM::ObjectID uiVisEMObject::getObjectID() const
{
    ConstRefMan<visSurvey::EMObjectDisplay> emod = getDisplay();
    return emod ? emod->getObjectID() : EM::ObjectID::udf();
}


ConstRefMan<visSurvey::EMObjectDisplay> uiVisEMObject::getDisplay() const
{
    return emobjdisplay_.get();
}


RefMan<visSurvey::EMObjectDisplay> uiVisEMObject::getDisplay()
{
    return emobjdisplay_.get();
}


ConstRefMan<visSurvey::HorizonDisplay>
uiVisEMObject::getHorizon3DDisplay() const
{
    return mSelf().getHorizon3DDisplay();
}


RefMan<visSurvey::HorizonDisplay> uiVisEMObject::getHorizon3DDisplay()
{
    RefMan<visSurvey::EMObjectDisplay> emobjdisplay = getDisplay();
    RefMan<visSurvey::HorizonDisplay> ret =
	dCast(visSurvey::HorizonDisplay*,emobjdisplay.ptr());
    return ret;
}


ConstRefMan<visSurvey::Horizon2DDisplay>
uiVisEMObject::getHorizon2DDisplay() const
{
    return mSelf().getHorizon2DDisplay();
}


RefMan<visSurvey::Horizon2DDisplay> uiVisEMObject::getHorizon2DDisplay()
{
    RefMan<visSurvey::EMObjectDisplay> emobjdisplay = getDisplay();
    RefMan<visSurvey::Horizon2DDisplay> ret =
	dCast(visSurvey::Horizon2DDisplay*,emobjdisplay.ptr());
    return ret;
}


static const char* sKeyHorizonRes = "dTect.Horizon.Resolution";
static const char* sKeyHorizonColTab = "dTect.Horizon.Color table";
static uiStringSet sResolutionNames;

static void fillResolutionNames( uiStringSet& nms )
{
    const int nrres = cMaximumResolution+1; // from vishorizonsectiondef.h
    for ( int idx=0; idx<nrres; idx++ )
	nms.add( uiStrings::sResolutionValue(idx) );
}


// uiHorizonSettings

uiHorizonSettings::uiHorizonSettings( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,uiStrings::sHorizon(mPlural),setts)
{
    if ( sResolutionNames.isEmpty() )
	fillResolutionNames( sResolutionNames );

    resolution_ = 0;
    setts.get( sKeyHorizonRes, resolution_ );
    resolutionfld_ = new uiGenInput( this, tr("Default Resolution"),
				     StringListInpSpec(sResolutionNames) );
    resolutionfld_->setValue( resolution_ );

    coltabnm_ = ColTab::defSeqName();
    setts.get( sKeyHorizonColTab, coltabnm_ );
    coltabfld_ = new uiColorTableGroup( this, ColTab::Sequence(coltabnm_) );
    coltabfld_->attach( alignedBelow, resolutionfld_ );

    auto* lbl = new uiLabel( this, tr("Default Colortable") );
    lbl->attach( leftOf, coltabfld_ );
}


uiHorizonSettings::~uiHorizonSettings()
{}


HelpKey uiHorizonSettings::helpKey() const
{ return mODHelpKey(mHorizonSettingsHelpID); }


bool uiHorizonSettings::acceptOK()
{
    updateSettings( resolution_, resolutionfld_->getIntValue(),
		    sKeyHorizonRes );
    updateSettings( coltabnm_, coltabfld_->colTabSeq().name(),
		    sKeyHorizonColTab );
    return true;
}


#define cMaxHorTitles 10000
#define cFullResolution 1
#define cHalfResolution 2

void uiVisEMObject::checkHorizonSize( const EM::Horizon3D* hor3d )
{
    if ( !hor3d ) return;

    const TrcKeySampling tck = hor3d->range();
    int res = 0;
    Settings::common().get( sKeyHorizonRes, res );
    if ( res<=cFullResolution )
    {
	const int nrrows = nrBlocks(
	tck.inlRange().nrSteps()+1, cNumberNodePerTileSide, 1 );
	const int nrcols = nrBlocks(
	    tck.crlRange().nrSteps()+1, cNumberNodePerTileSide, 1 );

	const int maxlines = nrrows*nrcols;
	if ( maxlines >= cMaxHorTitles )
	{
	    uiString msg =
		tr( "The horizon is too big for display\n"
		"Yes - using half resolution to save a certain memory.\n"
		"No - continue using default resolution." );
	    const int ret = uiMSG().askGoOn( msg );
	    if ( ret==1 )
		Settings::common().set(
		"dTect.Horizon.Resolution",cHalfResolution );
	}
    }
}
