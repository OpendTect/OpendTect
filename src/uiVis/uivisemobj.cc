/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2005
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
#include "od_helpids.h"
#include "odviscommon.h"
#include "settings.h"
#include "survinfo.h"
#include "simpnumer.h"

#include "uicolseqsel.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
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


uiVisEMObject::uiVisEMObject( uiParent* uip, int newid, uiVisPartServer* vps )
    : displayid_(newid)
    , visserv_(vps)
    , uiparent_(uip)
{
    uiUserShowWait usw( uiparent_, uiStrings::sUpdatingDisplay() );

    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;

    DBKey mid = emod->getDBKey();

    uiTaskRunner dlg( uiparent_ );
    ExistingTaskRunnerProvider trprov( &dlg );
    ConstRefMan<EM::Object> emobj = EM::MGR().fetch( mid, trprov );
    mDynamicCastGet( const EM::Horizon3D*, hor3d, emobj.ptr() );
    if ( hor3d )
	checkHorizonSize( hor3d );

    visSurvey::Scene* scene = emod->getScene();
    if ( !emobj )
    {
	mid = DBKey::getInvalid();
	if ( scene ) visserv_->removeObject( emod, scene->id() );
	return;
    }

    if ( !emod->setEMObject(mid,&dlg) )
    {
	if ( scene ) visserv_->removeObject( emod, scene->id() );
	return;
    }

    mDynamicCastGet(const visSurvey::HorizonDisplay*,hordisp,emod);
    if ( hordisp && hordisp->usesTexture() )
    {
	for ( int idx=0; idx<emod->nrAttribs(); idx++ )
	{
	    if ( hordisp->getSelSpec(idx)->id()
		     == Attrib::SelSpec::cNoAttribID() )
		setDepthAsAttrib( idx );
	}
    }

    setUpConnections();
}


#define mRefUnrefRet { emod->ref(); emod->unRef(); return; }

uiVisEMObject::uiVisEMObject( uiParent* uip, const DBKey& emid,
			      int sceneid, uiVisPartServer* vps )
    : displayid_(-1)
    , visserv_( vps )
    , uiparent_( uip )
{
    const EM::Object* emobj = EM::MGR().getObject(emid);
    visSurvey::EMObjectDisplay* emod = 0;
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
    if ( emod )
    {
	emod->setDisplayTransformation( scene->getUTM2DisplayTransform() );
	ZAxisTransform* zt = const_cast<ZAxisTransform*>(
		scene->getZAxisTransform() );
	emod->setZAxisTransform( zt, 0 );

	uiTaskRunner dlg( uiparent_ );
	if ( !emod->setEMObject(emid, &dlg) )
	    mRefUnrefRet

	    visserv_->addObject( emod, sceneid, true );
	displayid_ = emod->id();
	if ( !emobj->isEmpty() )
	    setDepthAsAttrib( 0 );
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
    seedsmenuitem_.text = uiStrings::sSeed(mPlural);
    seedsmenuitem_.checkable = false;
    showseedsmnuitem_.text = uiStrings::sShow();
    seedpropmnuitem_.text = m3Dots(uiStrings::sProperties());
    lockseedsmnuitem_.text = uiStrings::sLock();
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


BufferString uiVisEMObject::getObjectType( int id )
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,obj,visBase::DM().getObject(id))
    if ( !obj ) return 0;

    RefMan<EM::Object> emobj = EM::MGR().getObject( obj->getObjectID() );
    return emobj
	? BufferString(emobj->getTypeStr())
	: EM::MGR().objectType( obj->getDBKey() );
}


void uiVisEMObject::setDepthAsAttrib( int attrib )
{
    uiUserShowWait usw( uiparent_, uiStrings::sUpdatingDisplay() );
    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    if ( hordisp ) hordisp->setDepthAsAttrib( attrib );
    mDynamicCastGet( visSurvey::MarchingCubesDisplay*, mcdisp, getDisplay() );
    if ( mcdisp ) mcdisp->setDepthAsAttrib( attrib );
}


int uiVisEMObject::nrSections() const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return 0;

    DBKey emid = emod->getObjectID();
    const EM::Object* emobj = EM::MGR().getObject(emid);
    return emobj ? 1 : 0;
}


EM::SectionID uiVisEMObject::getSectionID( int idx ) const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return -1;

    DBKey emid = emod->getObjectID();
    const EM::Object* emobj = EM::MGR().getObject(emid);
    return emobj ? 0 : -1;
}


EM::SectionID uiVisEMObject::getSectionID( const TypeSet<int>* path ) const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    return path && emod ? 0 : -1;
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
    return hordisp ? (float) hordisp->getTranslation().z_ : 0;
}


void uiVisEMObject::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID()!=displayid_ )
	return;

    visSurvey::EMObjectDisplay* emod = getDisplay();
    const DBKey emid = emod->getObjectID();
    const EM::Object* emobj = EM::MGR().getObject(emid);

    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    mDynamicCastGet( visSurvey::Horizon2DDisplay*, hor2ddisp, getDisplay() );

    MenuItemHolder* displaymnuitem = menu->findItem( "Display" );
    if ( !displaymnuitem ) displaymnuitem = menu;

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

    visSurvey::Scene* scene = emod->getScene();
    const bool hastransform = scene && scene->getZAxisTransform();
    //Commented out as mAddMenu is commented out below
    //visSurvey::Scene* scene = hordisp ? hordisp->getScene() : 0;
    //const bool hastransform = scene && scene->getZAxisTransform();
    //const bool enabmenu =
	//getObjectType(displayid_)==EM::Horizon3D::typeStr()
	//&& !visserv_->isLocked(displayid_) && !hastransform;

    seedsmenuitem_.removeItems();

    mResetMenuItem( &lockseedsmnuitem_ );
    MenuItem* trackmnu = menu->findItem( uiStrings::sTracking() );
    if ( trackmnu )
    {
	const TypeSet<EM::PosID>* seeds =
	    emobj->getPosAttribList(EM::Object::sSeedNode());
	showseedsmnuitem_.text =
	   emod->showsPosAttrib(EM::Object::sSeedNode())
		? uiStrings::sHide() : uiStrings::sShow();
	mAddMenuItem( &seedsmenuitem_, &showseedsmnuitem_,
		      !hastransform && seeds && seeds->size(), false );
	mAddMenuItem( &seedsmenuitem_, &seedpropmnuitem_, true, false );
	mAddMenuItem( trackmnu, &seedsmenuitem_,
		      seedsmenuitem_.nrItems(), false );
    }
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
	 menu->isHandled() || menu->menuID()!=displayid_ )
	return;

    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    const DBKey emid = emod->getObjectID();
    EM::Object* emobj = EM::MGR().getObject(emid);

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
	{
	    hordisp->displayIntersectionLines( false );
	    hordisp->enableAttrib( (hordisp->nrAttribs()-1), true );
	}
	menu->setIsHandled( true );
    }
    else if ( mnuid==showbothmnuitem_.id )
    {
	setOnlyAtSectionsDisplay( false );
	if ( hordisp )
	{
	    hordisp->displayIntersectionLines( true );
	    hordisp->enableAttrib( (hordisp->nrAttribs()-1), true );
	}
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
	emod->showPosAttrib( EM::Object::sSeedNode(),
			     !emod->showsPosAttrib(EM::Object::sSeedNode()) );
    }
    else if ( mnuid==seedpropmnuitem_.id )
    {
	if ( emobj && !visserv_->showSetupGroupOnTop("Properties") )
	{
	    uiSeedPropDlg dlg( uiparent_, *emobj );
	    dlg.go();
	}
	menu->setIsHandled( true );
    }
    else if ( mnuid==lockseedsmnuitem_.id )
    {
	if ( emobj )
	    emobj->lockPosAttrib( EM::Object::sSeedNode(),
			!emobj->isPosAttribLocked(EM::Object::sSeedNode()) );
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


DBKey uiVisEMObject::getObjectID() const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    return emod ? emod->getObjectID() : DBKey::getInvalid();
}


visSurvey::EMObjectDisplay* uiVisEMObject::getDisplay()
{
    mDynamicCastGet( visSurvey::EMObjectDisplay*, emod,
		     visserv_->getObject(displayid_));
    return emod;
}


const visSurvey::EMObjectDisplay* uiVisEMObject::getDisplay() const
{
    return const_cast<uiVisEMObject*>(this)->getDisplay();
}


#define cMaxHorTiles 10000

void uiVisEMObject::checkHorizonSize( const EM::Horizon3D* hor3d )
{
    if ( !hor3d )
	return;

    int res = (int)OD::getDefaultSurfaceResolution();
    if ( res < (int)OD::SurfaceResolution::Full )
	res = (int)OD::SurfaceResolution::Full;

    const TrcKeySampling tck = hor3d->range();
    const int nrrowblocks = nrBlocks(
    tck.inlRange().nrSteps()+1, cNumberNodePerTileSide, 1 );
    const int nrcolblocks = nrBlocks(
	tck.crlRange().nrSteps()+1, cNumberNodePerTileSide, 1 );

    const int nrtiles = nrrowblocks * nrcolblocks;
    int maxtiles = cMaxHorTiles;
    const bool istoobig = nrtiles > maxtiles;
    while ( nrtiles > maxtiles && res <= (int)OD::cMinSurfaceResolution() )
    {
	res++;
	maxtiles *= 2;
    }

    // This is still a terrible hack
    if ( istoobig )
	Settings::common().set( OD::sSurfaceResolutionSettingsKey(), res );
}
