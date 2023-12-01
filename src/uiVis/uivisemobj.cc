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
#include "keyboardevent.h"
#include "mousecursor.h"
#include "od_helpids.h"
#include "settings.h"
#include "survinfo.h"
#include "simpnumer.h"

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

#include "visdataman.h"
#include "vishorizondisplay.h"
#include "vishorizon2ddisplay.h"
#include "vishorizonsectiondef.h"
#include "vismarchingcubessurfacedisplay.h"
#include "vissurvobj.h"
#include "vishorizonsectiondef.h"


uiVisEMObject::uiVisEMObject( uiParent* uip, VisID newid, uiVisPartServer* vps )
    : displayid_(newid)
    , visserv_(vps)
    , uiparent_(uip)
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;

    const MultiID mid = emod->getMultiID();
    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    const EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet( const EM::Horizon3D*, hor3d, emobj );
    if ( hor3d )
	checkHorizonSize( hor3d );

    visSurvey::Scene* scene = emod->getScene();
    mDynamicCastGet(const visSurvey::HorizonDisplay*,hordisp,emod);

    uiTaskRunner dlg( uiparent_ );
    if ( !EM::EMM().getObject(emid) )
    {
	Executor* exec = 0;
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
		if ( rowrg.step!=-1 && colrg.step!=-1 )
		{
		    sel.rg.start_.inl() = rowrg.start;
		    sel.rg.start_.crl() = colrg.start;
		    sel.rg.stop_.inl() = rowrg.stop;
		    sel.rg.step_.crl() = colrg.step;
		    sel.rg.step_.inl() = rowrg.step;
		    sel.rg.stop_.crl() = colrg.stop;
		}
	    }

	    exec = EM::EMM().objectLoader( mid, &sel );
	}

	if ( exec )
	{
	    emid = EM::EMM().getObjectID( mid );
	    EM::EMObject* emobject = EM::EMM().getObject( emid );
	    emobject->ref();
	    if ( !TaskRunner::execute( &dlg, *exec ) )
	    {
		emid.setUdf();
		emobject->unRef();
		if ( scene ) visserv_->removeObject( emod, scene->id() );
		delete exec;
		return;
	    }

	    delete exec;
	    emobject->unRefNoDelete();
	}
    }

    if ( !emod->setEMObject(emid,&dlg) )
    {
	if ( scene ) visserv_->removeObject( emod, scene->id() );
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

    setUpConnections();
}


#define mRefUnrefRet { emod->ref(); emod->unRef(); return; }

uiVisEMObject::uiVisEMObject( uiParent* uip, const EM::ObjectID& emid,
			      SceneID sceneid, uiVisPartServer* vps )
    : visserv_( vps )
    , uiparent_( uip )
{
    const EM::EMObject* emobj = EM::EMM().getObject(emid);
    visSurvey::EMObjectDisplay* emod = nullptr;
    mDynamicCastGet(const EM::Horizon3D*,hor3d,emobj);
    if ( hor3d )
    {
	checkHorizonSize( hor3d );
	emod = new visSurvey::HorizonDisplay;
    }
    mDynamicCastGet(const EM::Horizon2D*,hor2d,emobj);
    if ( hor2d )
	emod = new visSurvey::Horizon2DDisplay;

    mDynamicCastGet(visSurvey::Scene*,scene,visBase::DM().getObject(sceneid))
    if ( scene && emod )
    {
	emod->setZDomain( emobj->zDomain() );
	emod->setDisplayTransformation( scene->getUTM2DisplayTransform() );
	ZAxisTransform* zt = const_cast<ZAxisTransform*>(
		scene->getZAxisTransform() );
	emod->setZAxisTransform( zt, 0 );

	uiTaskRunner dlg( uiparent_ );
	if ( !emod->setEMObject(emid, &dlg) )
	    mRefUnrefRet

	visserv_->addObject( emod, sceneid, true );
	displayid_ = emod->id();
    }

    setUpConnections();
}


uiVisEMObject::~uiVisEMObject()
{
    MenuHandler* menu = visserv_->getMenuHandler();
    if ( menu )
    {
	menu->createnotifier.remove( mCB(this,uiVisEMObject,createMenuCB) );
	menu->handlenotifier.remove( mCB(this,uiVisEMObject,handleMenuCB) );
    }

    MenuHandler* tb = visserv_->getToolBarHandler();
    if ( tb )
    {
	tb->createnotifier.remove( mCB(this,uiVisEMObject,addToToolBarCB) );
	tb->handlenotifier.remove( mCB(this,uiVisEMObject,handleMenuCB) );
    }

    visserv_->keyEvent.remove( mCB(this,uiVisEMObject,keyEventCB) );
}


bool uiVisEMObject::isOK() const
{
    return getDisplay();
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

    MenuHandler* menu = visserv_->getMenuHandler();
    menu->createnotifier.notify( mCB(this,uiVisEMObject,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,uiVisEMObject,handleMenuCB) );

    MenuHandler* tbmenu = visserv_->getToolBarHandler();
    tbmenu->createnotifier.notify( mCB(this,uiVisEMObject,addToToolBarCB) );
    tbmenu->handlenotifier.notify( mCB(this,uiVisEMObject,handleMenuCB) );

    visserv_->keyEvent.notify( mCB(this,uiVisEMObject,keyEventCB) );
}


const char* uiVisEMObject::getObjectType( VisID id )
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,obj,visBase::DM().getObject(id))
    if ( !obj ) return 0;

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( obj->getObjectID() );
    return emobj
	? emobj->getTypeStr()
	: EM::EMM().objectType( obj->getMultiID() );
}


void uiVisEMObject::setDepthAsAttrib( int attrib )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    if ( hordisp ) hordisp->setDepthAsAttrib( attrib );
    mDynamicCastGet( visSurvey::MarchingCubesDisplay*, mcdisp, getDisplay() );
    if ( mcdisp ) mcdisp->setDepthAsAttrib( attrib );
}


int uiVisEMObject::nrSections() const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    return emod ? 1 : 0;
}


EM::SectionID uiVisEMObject::getSectionID( int idx ) const
{
    return EM::SectionID::def();
}


EM::SectionID uiVisEMObject::getSectionID( const TypeSet<VisID>* path ) const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    return path && emod ? EM::SectionID::def() : EM::SectionID::udf();
}


void uiVisEMObject::checkTrackingStatus()
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;
    emod->updateFromMPE();
}


float uiVisEMObject::getShift() const
{
    mDynamicCastGet( const visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    return hordisp ? (float) hordisp->getTranslation().z : 0;
}


void uiVisEMObject::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID() != displayid_.asInt() )
	return;

    visSurvey::EMObjectDisplay* emod = getDisplay();
    const EM::ObjectID emid = emod->getObjectID();
    const EM::EMObject* emobj = EM::EMM().getObject(emid);

    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    mDynamicCastGet( visSurvey::Horizon2DDisplay*, hor2ddisp, getDisplay() );

    MenuItemHolder* displaymnuitem = menu->findItem( "Display" );
    if ( !displaymnuitem ) displaymnuitem = menu;

    seedsmenuitem_.removeItems();
    ctrlpointsmenuitem_.removeItems();
    visSurvey::Scene* scene = emod->getScene();
    const bool hastransform = scene && scene->getZAxisTransform();
    if ( displaymnuitem )
    {
	const bool hasseeds = emobj->hasPosAttrib( EM::EMObject::sSeedNode() );
	showseedsmnuitem_.text =
	   emod->showsPosAttrib(EM::EMObject::sSeedNode())
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
    { mAddMenuItem( displaymnuitem, &showsurfacegridmnuitem_, true, showgrid); }
    else
    { mResetMenuItem( &showsurfacegridmnuitem_ ); }

    mAddMenuItem( displaymnuitem, &showonlyatsectionsmnuitem_, true,
		  atsect );
    mAddMenuItem( displaymnuitem, &showfullmnuitem_, true, infull );
    if ( hordisp )
    {
	mAddMenuItem( displaymnuitem, &showbothmnuitem_, true, both );
    }
    else
    { mResetMenuItem( &showbothmnuitem_ ); }

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
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod || mnuid==-1 || !menu ||
	 menu->isHandled() || menu->menuID()!=displayid_.asInt() )
	return;

    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, getDisplay() );
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
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( emod )
	emod->setOnlyAtSectionsDisplay( yn );
}


bool uiVisEMObject::isOnlyAtSections() const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    return emod ? emod->displayedOnlyAtSections() : false;
}


EM::ObjectID uiVisEMObject::getObjectID() const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    return emod ? emod->getObjectID() : EM::ObjectID::udf();
}


visSurvey::EMObjectDisplay* uiVisEMObject::getDisplay()
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
		    visserv_->getObject(displayid_))
    return emod;
}


const visSurvey::EMObjectDisplay* uiVisEMObject::getDisplay() const
{
    return const_cast<uiVisEMObject*>(this)->getDisplay();
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

    uiLabel* lbl = new uiLabel( this, tr("Default Colortable") );
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
