/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimpepartserv.h"

#include "attribdataholder.h"
#include "attribdatacubes.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "attribdescsetsholder.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emsurfacetr.h"
#include "emtracker.h"
#include "emobject.h"
#include "executor.h"
#include "file.h"
#include "geomelement.h"
#include "ioman.h"
#include "iopar.h"
#include "mpeengine.h"
#include "randcolor.h"
#include "survinfo.h"
#include "sectiontracker.h"
#include "sectionadjuster.h"
#include "mousecursor.h"
#include "undo.h"

#include "uitaskrunner.h"
#include "uihorizontracksetup.h"
#include "uimsg.h"

int uiMPEPartServer::evGetAttribData()		    { return 0; }
int uiMPEPartServer::evStartSeedPick()		    { return 1; }
int uiMPEPartServer::evEndSeedPick()	   	    { return 2; }
int uiMPEPartServer::evAddTreeObject()	      	    { return 3; }
int uiMPEPartServer::evShowToolbar()	   	    { return 4; }
int uiMPEPartServer::evInitFromSession()	    { return 5; }
int uiMPEPartServer::evRemoveTreeObject()	    { return 6; }
int uiMPEPartServer::evSetupLaunched()		    { return 7; }
int uiMPEPartServer::evSetupClosed()	   	    { return 8; }
int uiMPEPartServer::evCreate2DSelSpec()	    { return 9; }
int uiMPEPartServer::evMPEDispIntro()		    { return 10; }
int uiMPEPartServer::evUpdateTrees()	   	    { return 11; }
int uiMPEPartServer::evUpdateSeedConMode()   	    { return 12; }
int uiMPEPartServer::evMPEStoreEMObject()	    { return 13; }
int uiMPEPartServer::evHideToolBar()		    { return 14; }
int uiMPEPartServer::evSaveUnsavedEMObject() 	    { return 15; }
int uiMPEPartServer::evRemoveUnsavedEMObject()	    { return 16; }
int uiMPEPartServer::evRetrackInVolume()	    { return 17; }


uiMPEPartServer::uiMPEPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , attrset3d_( 0 )
    , attrset2d_( 0 )
    , activetrackerid_(-1)
    , eventattrselspec_( 0 )
    , blockdataloading_( false )
    , postponedcs_( false )
    , temptrackerid_(-1)
    , cursceneid_(-1)
    , trackercurrentobject_(-1)
    , initialundoid_(mUdf(int))
    , seedhasbeenpicked_(false)
    , setupbeingupdated_(false)
    , seedswithoutattribsel_(false)
    , setupgrp_(0)
{
    MPE::engine().setActiveVolume( MPE::engine().getDefaultActiveVolume() );
    MPE::engine().activevolumechange.notify(
	    mCB(this, uiMPEPartServer, activeVolumeChange) );
    MPE::engine().loadEMObject.notify(
	    mCB(this, uiMPEPartServer, loadEMObjectCB) );
    MPE::engine().trackeraddremove.notify(
	    mCB(this, uiMPEPartServer, loadTrackSetupCB) );
    EM::EMM().addRemove.notify( mCB(this,uiMPEPartServer,nrHorChangeCB) );
    trackerseedbox_.setEmpty();
}


uiMPEPartServer::~uiMPEPartServer()
{
    MPE::engine().activevolumechange.remove(
	    mCB(this, uiMPEPartServer, activeVolumeChange) );
    MPE::engine().loadEMObject.remove(
	    mCB(this, uiMPEPartServer, loadEMObjectCB) );
    MPE::engine().trackeraddremove.remove(
	    mCB(this, uiMPEPartServer, loadTrackSetupCB) );
    EM::EMM().addRemove.remove( mCB(this,uiMPEPartServer,nrHorChangeCB) );

    trackercurrentobject_ = -1;
    initialundoid_ = mUdf(int);
    seedhasbeenpicked_ = false;
    setupbeingupdated_ = false;

    sendEvent( uiMPEPartServer::evEndSeedPick() );
    sendEvent( uiMPEPartServer::evShowToolbar() );
    sendEvent( ::uiMPEPartServer::evSetupClosed() );
    if ( setupgrp_ && setupgrp_->mainwin() ) 
	setupgrp_->mainwin()->close();
}


void uiMPEPartServer::setCurrentAttribDescSet( const Attrib::DescSet* ads )
{ 
    if ( !ads )
	return;

    ads->is2D() ? attrset2d_ = ads : attrset3d_ = ads;
}


const Attrib::DescSet* uiMPEPartServer::getCurAttrDescSet( bool is2d ) const
{ return is2d ? attrset2d_ : attrset3d_; }


int uiMPEPartServer::getTrackerID( const EM::ObjectID& emid ) const
{
    for ( int idx=0; idx<=MPE::engine().highestTrackerID(); idx++ )
    {
	if ( MPE::engine().getTracker(idx) )
	{
	    EM::ObjectID objid = MPE::engine().getTracker(idx)->objectID();
	    if ( objid==emid )
		return idx;
	}
    }

    return -1;
}


int uiMPEPartServer::getTrackerID( const char* trackername ) const
{
    return MPE::engine().getTrackerByObject(trackername);
}


void uiMPEPartServer::getTrackerTypes( BufferStringSet& res ) const
{ MPE::engine().getAvailableTrackerTypes(res); }


int uiMPEPartServer::addTracker( const EM::ObjectID& emid,
				 const Coord3& pickedpos )
{
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return -1;

    const int res = MPE::engine().addTracker( emobj );
    if ( res == -1 )
    {
	uiMSG().error("Could not create tracker for this object");
	return -1;
    }

    blockDataLoading( true );

    if ( pickedpos.isDefined() ) 
    {
	CubeSampling poscs(false);
	const BinID bid = SI().transform(pickedpos);
	poscs.hrg.start = poscs.hrg.stop = bid;
	poscs.zrg.start = poscs.zrg.stop = (float) pickedpos.z;
	expandActiveVolume( poscs );
    }

    blockDataLoading( false );
    postponeLoadingCurVol();

    return res;
}


bool uiMPEPartServer::addTracker( const char* trackertype, int addedtosceneid )
{
    if ( trackercurrentobject_ != -1 && !seedswithoutattribsel_ )
	return false;

    seedswithoutattribsel_ = false;
    cursceneid_ = addedtosceneid;
    //NotifyStopper notifystopper( MPE::engine().trackeraddremove );

    EM::EMObject* emobj = EM::EMM().createTempObject( trackertype );
    if ( !emobj )
	return false;

    emobj->setNewName();
    emobj->setFullyLoaded( true );

    const int trackerid = MPE::engine().addTracker( emobj );

    if ( trackerid == -1 ) return false;

//    EM::ObjectID objid = emobj->id();
//    if ( !MPE::engine().getEditor(objid,false) )
//	MPE::engine().getEditor(objid,true);

    MPE::EMTracker* tracker = MPE::engine().getTracker(trackerid);
    if ( !tracker ) return false;

    activetrackerid_ = trackerid;
    if ( (addedtosceneid!=-1) && 
	 !sendEvent(::uiMPEPartServer::evAddTreeObject()) )
    {
	pErrMsg("Could not create tracker" );
	MPE::engine().removeTracker( trackerid );
	emobj->ref(); emobj->unRef();
    }

    const EM::SectionID sid = emobj->sectionID( emobj->nrSections()-1 );
    emobj->setPreferredColor( getRandomColor(false) );
    trackercurrentobject_ = emobj->id();
    if ( !initSetupDlg(emobj,tracker,sid,true) )
	return false;

    initialundoid_ = EM::EMM().undo().currentEventID();
    propertyChangedCB(0);

    return true;
}


void uiMPEPartServer::aboutToAddRemoveSeed( CallBacker* )
{
    const int trackerid = getTrackerID( trackercurrentobject_ );
    if ( trackerid == -1 )
	return;

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker )
	return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker( true );
    if ( !seedpicker )
	return;

    if ( !seedpicker->getSelSpec() )
	return;

    bool fieldchange = false;
    bool isvalidsetup = false;

    if ( setupgrp_ )
    {
	if ( seedpicker->nrSeeds() < 1 )
	    setupgrp_->setAttribSelSpec( seedpicker->getSelSpec() );
	else if ( !setupgrp_->isSameSelSpec(seedpicker->getSelSpec()) &&
		seedpicker->getSeedConnectMode()!=seedpicker->DrawBetweenSeeds )
	{
	    seedpicker->blockSeedPick( true );
	    uiMSG().error( "Tracking Setup has different attribute"
		    	   " than your seeds" );
	    return;
	}

	isvalidsetup = setupgrp_->commitToTracker( fieldchange );
    }

    seedpicker->blockSeedPick( !isvalidsetup );
    if ( !isvalidsetup )
	return;

    if ( !seedhasbeenpicked_ )
	sendEvent( uiMPEPartServer::evShowToolbar() );

    seedhasbeenpicked_ = true;
}


void uiMPEPartServer::modeChangedCB( CallBacker* )
{
    const int trackerid = getTrackerID( trackercurrentobject_ );
    if ( trackerid == -1 ) return;

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker( true );
    if ( !seedpicker) return;

    if ( setupgrp_ )
	seedpicker->setSeedConnectMode( setupgrp_->getMode() );

    sendEvent( uiMPEPartServer::evUpdateSeedConMode() );
}


void uiMPEPartServer::eventorsimimlartyChangedCB( CallBacker* )
{
    if ( setupgrp_ )
	setupgrp_->commitToTracker();
}


void uiMPEPartServer::propertyChangedCB( CallBacker* )
{
    if ( trackercurrentobject_ != -1 )
    {
	EM::EMObject* emobj = EM::EMM().getObject( trackercurrentobject_ );
	if ( !emobj ) return;

	if ( setupgrp_ )
	{
	    emobj->setPreferredColor( setupgrp_->getColor() );
	    sendEvent( uiMPEPartServer::evUpdateTrees() );
	    emobj->setPosAttrMarkerStyle( EM::EMObject::sSeedNode(),
		    			  setupgrp_->getMarkerStyle() );
	}	    
    }
}


void uiMPEPartServer::retrackCB( CallBacker* )
{
    if ( trackercurrentobject_ != -1 )
	retrack( trackercurrentobject_ );
}


void uiMPEPartServer::nrHorChangeCB( CallBacker* cb )
{
    if ( trackercurrentobject_ == -1 ) return;

    for ( int idx=EM::EMM().nrLoadedObjects()-1; idx>=0; idx-- )
    {
	const EM::ObjectID oid = EM::EMM().objectID( idx );
	if ( oid == trackercurrentobject_ ) return;
    }

    trackercurrentobject_ = -1;
    initialundoid_ = mUdf(int);
    seedhasbeenpicked_ = false;
    setupbeingupdated_ = false;

    sendEvent( uiMPEPartServer::evEndSeedPick() );
    sendEvent( uiMPEPartServer::evShowToolbar() );
    sendEvent( ::uiMPEPartServer::evSetupClosed() );
    if ( setupgrp_ )
	setupgrp_->mainwin()->close();
}


void uiMPEPartServer::trackerWinClosedCB( CallBacker* cb )
{
    cleanSetupDependents();
    seedswithoutattribsel_ = false;

    //setupgrp_ = 0;

    if ( trackercurrentobject_ == -1 ) return;

    const int trackerid = getTrackerID( trackercurrentobject_ );
    if ( trackerid == -1 )
    {
	trackercurrentobject_ = -1;
	initialundoid_ = mUdf(int);
	seedhasbeenpicked_ = false;
	setupbeingupdated_ = false;

	return;
    }

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker( true );
    if ( !seedpicker ) return;

    if ( setupbeingupdated_ )
    {
	if ( seedpicker->doesModeUseSetup() )
	    saveSetup( EM::EMM().getMultiID( trackercurrentobject_) );
	
	trackercurrentobject_ = -1;
	setupbeingupdated_ = false;
	sendEvent( uiMPEPartServer::evShowToolbar() );
	sendEvent( ::uiMPEPartServer::evSetupClosed() );
	return;
    }
    
    if ( !seedhasbeenpicked_ )
    {
	BufferString str( "No seeds have been picked. "
			  "Do you want to continue with horizon tracking?" );
	const bool res = uiMSG().askContinue( str );
	if ( !res )
	{
	    noTrackingRemoval();
	    return;
	}
    }

    if ( setupgrp_ && !setupgrp_->commitToTracker() )
	if ( !seedhasbeenpicked_ )
	{
	    seedswithoutattribsel_ = true;
	    return;
	}

    NotifierAccess* addrmseednotifier = seedpicker->aboutToAddRmSeedNotifier();
    if ( addrmseednotifier )
	addrmseednotifier->remove(
			   mCB(this,uiMPEPartServer,aboutToAddRemoveSeed) );

    setupgrp_ = 0;

    if ( seedhasbeenpicked_ )
	adjustSeedBox();

    // finishing time
    blockDataLoading( true );
    //sendEvent( uiMPEPartServer::evEndSeedPick() );
    blockDataLoading( false );

    blockDataLoading( true );

    if ( seedpicker->doesModeUseVolume() && !trackerseedbox_.isEmpty() )
	expandActiveVolume( trackerseedbox_ );

    if ( !seedhasbeenpicked_ || !seedpicker->doesModeUseVolume() )
	sendEvent( uiMPEPartServer::evStartSeedPick() );

    blockDataLoading( false );

    postponeLoadingCurVol();
    //sendEvent( uiMPEPartServer::evMPEDispIntro() );
    sendEvent( uiMPEPartServer::evShowToolbar() );
    if ( seedpicker->doesModeUseSetup() )
	saveSetup( EM::EMM().getMultiID( trackercurrentobject_) );

    sendEvent( ::uiMPEPartServer::evSetupClosed() );

    trackercurrentobject_ = -1;
    initialundoid_ = mUdf(int);
    seedhasbeenpicked_ = false;
    setupbeingupdated_ = false;
}


void uiMPEPartServer::adjustSeedBox()
{
    const int trackerid = getTrackerID( trackercurrentobject_ );
    const MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return;

    const EM::ObjectID objid = tracker->objectID();
    EM::EMObject* emobj = EM::EMM().getObject( objid );
    if ( !emobj ) return;

    const EM::SectionID sid = emobj->sectionID( emobj->nrSections()-1 );
    PtrMan<EM::EMObjectIterator> iterator = emobj->createIterator( sid );
    while( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID() == -1 )
	    break;

	const Coord3 pos = emobj->getPos( pid );
	if ( !pos.isDefined() ||
	     !emobj->isPosAttrib(pid, EM::EMObject::sSeedNode()) )
	    continue;

	const BinID bid = SI().transform(pos);
	if ( trackerseedbox_.isEmpty() )
	{
	    trackerseedbox_.hrg.start = trackerseedbox_.hrg.stop = bid;
	    trackerseedbox_.zrg.start = trackerseedbox_.zrg.stop =(float)pos.z;
	}
	else
	{
	    trackerseedbox_.hrg.include( bid );
	    trackerseedbox_.zrg.include( (float)pos.z );
	}
    }
} 


void uiMPEPartServer::noTrackingRemoval()
{
    sendEvent( uiMPEPartServer::evEndSeedPick() );
    if( !mIsUdf(initialundoid_) )
    {
	EM::EMObject* emobj = EM::EMM().getObject( trackercurrentobject_ );
	if ( !emobj ) return;

	emobj->setBurstAlert( true );
	EM::EMM().undo().unDo(
			EM::EMM().undo().currentEventID()-initialundoid_);
	EM::EMM().undo().removeAllAfterCurrentEvent();
	emobj->setBurstAlert( false );
    }

    const MultiID mid = EM::EMM().getMultiID( trackercurrentobject_ );
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( ioobj )
    {
	if ( !fullImplRemove(*ioobj) || !IOM().permRemove(mid) )
	    pErrMsg( "Could not remove object" );
    }

    MPE::engine().trackeraddremove.disable();

    if ( (trackercurrentobject_!= -1) && (cursceneid_!=-1) )
	sendEvent( ::uiMPEPartServer::evRemoveTreeObject() );

    const int trackerid = getTrackerID( trackercurrentobject_ );
    if ( trackerid != -1 )
	MPE::engine().removeTracker( trackerid );

    trackercurrentobject_ = -1;
    initialundoid_ = mUdf(int);
    seedhasbeenpicked_ = false;

    if ( cursceneid_ == -1 )
    {
	setupbeingupdated_ = false;
	setupgrp_ = 0;
    }

    MPE::engine().trackeraddremove.enable();
    MPE::engine().trackeraddremove.trigger();
    sendEvent( uiMPEPartServer::evShowToolbar() );
    sendEvent( ::uiMPEPartServer::evSetupClosed() );
}


void uiMPEPartServer::retrack( const EM::ObjectID& oid )
{
    bool fieldchange = false;
    if ( setupgrp_ && !setupgrp_->commitToTracker(fieldchange) )
	return;

    const int trackerid = getTrackerID( oid );
    if ( trackerid == -1 ) return;

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return;
    
    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker( true );
    if ( !seedpicker) return;

    Undo& undo = EM::EMM().undo();
    int cureventnr = undo.currentEventID();
    undo.setUserInteractionEnd( cureventnr, false );

    MouseCursorManager::setOverride( MouseCursor::Wait );
    EM::EMObject* emobj = EM::EMM().getObject( oid );
    if ( !emobj ) return;

    emobj->setBurstAlert( true );
    emobj->removeAllUnSeedPos();

    CubeSampling oldactivevol = MPE::engine().activeVolume();

    ObjectSet<CubeSampling>* trackedcubes = 
				MPE::engine().getTrackedFlatCubes( trackerid );
    if ( trackedcubes )
    {
	for ( int idx=0; idx<trackedcubes->size(); idx++ )
	{
	    NotifyStopper notifystopper( MPE::engine().activevolumechange );
	    MPE::engine().setActiveVolume( *(*trackedcubes)[idx] );
	    notifystopper.restore();

	    const CubeSampling curvol =  MPE::engine().activeVolume();
	    if ( curvol.nrInl()==1 || curvol.nrCrl()==1 )
		loadAttribData();

	    seedpicker->reTrack();
	}

	emobj->setBurstAlert( false );
	deepErase( *trackedcubes );
    }
    else
    {
	seedpicker->reTrack();
	emobj->setBurstAlert( false );
    }

    MPE::engine().setActiveVolume( oldactivevol );
    eventorsimimlartyChangedCB( 0 );

    if ( !(MPE::engine().activeVolume().nrInl()==1) &&
         !(MPE::engine().activeVolume().nrCrl()==1) )
    {
	sendEvent( uiMPEPartServer::evRetrackInVolume() );
    }

    MouseCursorManager::restoreOverride();

    undo.setUserInteractionEnd( undo.currentEventID() );
}


void uiMPEPartServer::cleanSetupDependents()
{
    if ( !setupgrp_ ) return;

    NotifierAccess* modechangenotifier = setupgrp_->modeChangeNotifier();
    if ( modechangenotifier )
	modechangenotifier->remove( mCB(this,uiMPEPartServer,modeChangedCB) );
    
    NotifierAccess* propertychangenotifier = 
				setupgrp_->propertyChangeNotifier();
    if ( propertychangenotifier )
	propertychangenotifier->remove(
				mCB(this,uiMPEPartServer,propertyChangedCB) );

    NotifierAccess* eventchangenotifier = setupgrp_->eventChangeNotifier();
    if ( eventchangenotifier )
	eventchangenotifier->remove(
			mCB(this,uiMPEPartServer,eventorsimimlartyChangedCB) );
    
    NotifierAccess* similartyChangeNotifier =
					  setupgrp_->similartyChangeNotifier();
    if ( similartyChangeNotifier )
	similartyChangeNotifier->remove(
			mCB(this,uiMPEPartServer,eventorsimimlartyChangedCB) );
}


EM::ObjectID uiMPEPartServer::getEMObjectID( int trackerid ) const
{
    const MPE::EMTracker* emt = MPE::engine().getTracker(trackerid);
    return emt ? emt->objectID() : -1;
}


bool uiMPEPartServer::canAddSeed( int trackerid ) const
{
    pErrMsg("Not impl");
    return false;
}


bool uiMPEPartServer::isTrackingEnabled( int trackerid ) const
{ return MPE::engine().getTracker(trackerid)->isEnabled(); }


void uiMPEPartServer::enableTracking( int trackerid, bool yn )
{
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return;

    MPE::engine().setActiveTracker( tracker );
    tracker->enable( yn );
}


int uiMPEPartServer::activeTrackerID() const
{ return activetrackerid_; }


const Attrib::SelSpec* uiMPEPartServer::getAttribSelSpec() const
{ return eventattrselspec_; }


bool uiMPEPartServer::showSetupDlg( const EM::ObjectID& emid,
				    const EM::SectionID& sid )
{
    if ( emid<0 || sid<0 )
	return false;

    if ( trackercurrentobject_!=-1 && !seedswithoutattribsel_ )
	return false;
    
    const int trackerid = getTrackerID( emid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return false;

    trackercurrentobject_ = emid;

    setupbeingupdated_ = true;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return false;

    if ( !initSetupDlg(emobj,tracker,sid) )
	return false;

    if ( setupgrp_ ) setupgrp_->commitToTracker();

    tracker->applySetupAsDefault( sid );
    loadAttribData();

    return true;
}


#define mAskGoOnStr(setupavailable) \
    ( setupavailable ? \
	"This object has saved tracker settings.\n" \
	"Do you want to verify / change them?" : \
	"This object was created by manual drawing\n" \
        "only, or its tracker settings were not saved.\n" \
	"Do you want to specify them right now?" )

void uiMPEPartServer::useSavedSetupDlg( const EM::ObjectID& emid,
					const EM::SectionID& sid )
{
    const int trackerid = getTrackerID( emid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    MPE::SectionTracker* sectiontracker = 
			 tracker ? tracker->getSectionTracker( sid, true ) : 0;
    const bool setupavailable = sectiontracker &&
				sectiontracker->hasInitializedSetup();

    if ( uiMSG().askGoOn(mAskGoOnStr(setupavailable)) )
	showSetupDlg( emid, sid );
}


void uiMPEPartServer::setAttribData( const Attrib::SelSpec& spec,
				     DataPack::ID datapackid )
{
    MPE::engine().setAttribData( spec, datapackid );
}


void uiMPEPartServer::setAttribData( const Attrib::SelSpec& spec,
				     const Attrib::DataCubes* slcset )
{
    RefMan<MPE::DataHolder> dh = new MPE::DataHolder;
    dh->setCubeSampling( slcset->cubeSampling() );
    dh->set3DData( slcset );
    MPE::engine().setAttribData( spec, dh );
}


void uiMPEPartServer::setAttribData( const Attrib::SelSpec& as,
				     const Attrib::Data2DArray* newdata )
{
    RefMan<const Attrib::Data2DArray> reffer = newdata;

    if ( !newdata )
    {
	MPE::engine().setAttribData( as, 0 );
	return;
    }

    RefMan<MPE::DataHolder> dh = new MPE::DataHolder;
    dh->setCubeSampling( newdata->cubesampling_ );
    dh->set2DData( newdata );

    MPE::engine().setAttribData( as, dh );
}


const MultiID& uiMPEPartServer::get2DLineSet() const
{ return linesetid_; }


const char* uiMPEPartServer::get2DLineName() const
{ return linename_.size() ? (const char*) linename_ : 0; }


const char* uiMPEPartServer::get2DAttribName() const
{ return attribname_.size() ? (const char*) attribname_ : 0; }


void uiMPEPartServer::set2DSelSpec(const Attrib::SelSpec& as)
{ lineselspec_ = as; }


bool uiMPEPartServer::isDataLoadingBlocked() const
{ return blockdataloading_; }


void uiMPEPartServer::blockDataLoading( bool yn )
{ blockdataloading_ = yn; }


void uiMPEPartServer::postponeLoadingCurVol()
{ postponedcs_ = MPE::engine().activeVolume(); }


void uiMPEPartServer::loadPostponedVolume()
{
    if ( postponedcs_ == MPE::engine().activeVolume() )
    {
    	postponedcs_.setEmpty();
	loadAttribData();
    }
    else    
	postponedcs_.setEmpty();
}


void uiMPEPartServer::activeVolumeChange( CallBacker* )
{
    if ( MPE::engine().activeVolume()==MPE::engine().getDefaultActiveVolume() )
    {
	postponeLoadingCurVol();
	return;
    }

    loadAttribData();
}


void uiMPEPartServer::loadAttribData()
{
    if ( blockdataloading_ || postponedcs_==MPE::engine().activeVolume() )
	return;

    MouseCursorChanger changer( MouseCursor::Wait );

    ObjectSet<const Attrib::SelSpec> attribselspecs;
    MPE::engine().getNeededAttribs(attribselspecs);
    if ( attribselspecs.size() == 0 ) return;

    for ( int idx=0; idx<attribselspecs.size(); idx++ )
    {
	if ( attribselspecs[idx]->is2D() ) // TODO
	{
	    eventattrselspec_ = attribselspecs[idx];
	    const bool isloaded =
		getAttribCacheID(*eventattrselspec_) != DataPack::cNoID();
	    if ( !isloaded )
		sendEvent( evGetAttribData() );
	    continue;
	}

	eventattrselspec_ = attribselspecs[idx];
	const CubeSampling desiredcs = getAttribVolume( *eventattrselspec_ );

	CubeSampling possiblecs;
	if ( !desiredcs.getIntersection(SI().sampling(false),possiblecs) )
	    continue; 

	if ( MPE::engine().cacheIncludes(*eventattrselspec_,possiblecs) )
	    continue;

	const float marginfraction = 0.9;
	const CubeSampling mincs = MPE::engine().activeVolume();
	if ( MPE::engine().cacheIncludes(*eventattrselspec_,mincs) &&
	     marginfraction*desiredcs.nrZ() < mincs.nrZ() )
	    continue;
	    
	sendEvent( evGetAttribData() );
    }
}


DataPack::ID uiMPEPartServer::getAttribCacheID(
					const Attrib::SelSpec& spec ) const
{ return MPE::engine().getAttribCacheID( spec ); }


const MPE::DataHolder*
    uiMPEPartServer::getAttribCache( const Attrib::SelSpec& spec ) const
{ return MPE::engine().getAttribCache( spec ); }


CubeSampling uiMPEPartServer::getAttribVolume( const Attrib::SelSpec& as ) const
{ return MPE::engine().getAttribCube(as); }


bool uiMPEPartServer::activeVolumeIsDefault() const
{
    const CubeSampling activecs = MPE::engine().activeVolume();
    if ( activecs==MPE::engine().getDefaultActiveVolume() )
	return true;

    return false;
}


void uiMPEPartServer::expandActiveVolume(const CubeSampling& seedcs)
{
    if ( MPE::engine().isActiveVolShown() )
	return;

    const CubeSampling activecs = MPE::engine().activeVolume();
    const bool isdefault = activeVolumeIsDefault();

    CubeSampling newcube = isdefault || activecs.isFlat() ? seedcs : activecs;
    newcube.zrg.step = SI().zStep();
    if ( !isdefault )
    {
	newcube.hrg.include( seedcs.hrg.start );
	newcube.hrg.include( seedcs.hrg.stop );
	newcube.zrg.include( seedcs.zrg.start );
	newcube.zrg.include( seedcs.zrg.stop );
    }

    const int minnr = 20;
    if ( newcube.nrInl() < minnr )
    {
	newcube.hrg.start.inl -= minnr*newcube.hrg.step.inl;
	newcube.hrg.stop.inl += minnr*newcube.hrg.step.inl;
    }

    if ( newcube.nrCrl() < minnr )
    {
	newcube.hrg.start.crl -= minnr*newcube.hrg.step.crl;
	newcube.hrg.stop.crl += minnr*newcube.hrg.step.crl;
    }

    if ( isdefault )
	newcube.zrg.widen( 0.05 );

    newcube.snapToSurvey();
    newcube.limitTo( SI().sampling(true) );
    MPE::engine().setActiveVolume( newcube );
}


void uiMPEPartServer::loadEMObjectCB(CallBacker*)
{
    PtrMan<Executor> exec = EM::EMM().objectLoader( MPE::engine().midtoload );
    if ( !exec ) return;

    const EM::ObjectID emid = EM::EMM().getObjectID( MPE::engine().midtoload );
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    emobj->ref();
    uiTaskRunner uiexec( appserv().parent() );
    const bool keepobj = uiexec.execute( *exec );	
    exec.erase();
    if ( keepobj )
	emobj->unRefNoDelete();
    else
	emobj->unRef();
}


bool uiMPEPartServer::prepareSaveSetupAs( const MultiID& oldmid )
{
    const EM::ObjectID emid = EM::EMM().getObjectID( oldmid );
    if ( getTrackerID(emid) >= 0 )
	return true;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) 
	return false;

    temptrackerid_ = MPE::engine().addTracker( emobj );

    return temptrackerid_ >= 0; 
}


bool uiMPEPartServer::saveSetupAs( const MultiID& newmid )
{
    const int res = saveSetup( newmid );
    MPE::engine().removeTracker( temptrackerid_ );
    temptrackerid_ = -1;
    return res;
}


bool uiMPEPartServer::saveSetup( const MultiID& mid )
{
    const EM::ObjectID emid = EM::EMM().getObjectID( mid );
    const int trackerid = getTrackerID( emid );
    if ( trackerid<0 ) return false;

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker ) return false;

    IOPar iopar;
    iopar.set( "Seed Connection mode", seedpicker->getSeedConnectMode() );
    tracker->fillPar( iopar );
    ObjectSet<const Attrib::SelSpec> usedattribs;
    MPE::engine().getNeededAttribs( usedattribs );
    TypeSet<Attrib::DescID> usedattribids;
    const Attrib::DescSet* attrset = getCurAttrDescSet( tracker->is2D() );
    if ( !attrset )
	return false;
    for ( int idx=0; idx<usedattribs.size(); idx++ )
    {
	const Attrib::DescID descid = usedattribs[idx]->id();
	if ( attrset->getDesc(descid) )
	    usedattribids += descid;
    }

    PtrMan<Attrib::DescSet> ads = attrset->optimizeClone( usedattribids );
    IOPar attrpar;
    if ( ads.ptr() ) 
	ads->fillPar( attrpar );

    iopar.mergeComp( attrpar, "Attribs" );

    BufferString setupfilenm = MPE::engine().setupFileName( mid );
    iopar.write( setupfilenm, "Tracking Setup" );

    return true;
}


void uiMPEPartServer::loadTrackSetupCB( CallBacker* )
{
    const int trackerid = MPE::engine().highestTrackerID();
    MPE::EMTracker* emtracker = MPE::engine().getTracker( trackerid );
    if ( !emtracker ) return;

    const EM::EMObject* emobj = EM::EMM().getObject( emtracker->objectID() );
    if ( !emobj ) return;

    const EM::SectionID sid = emobj->sectionID(0);
    const MPE::SectionTracker* sectracker = 
			       emtracker->getSectionTracker( sid, true );
    
    if ( sectracker && !sectracker->hasInitializedSetup() ) 
	readSetup( emobj->multiID() );
}


bool uiMPEPartServer::readSetup( const MultiID& mid ) 
{
    BufferString setupfilenm = MPE::engine().setupFileName( mid );
    if ( !File::exists(setupfilenm) ) return false;

    const EM::ObjectID emid = EM::EMM().getObjectID( mid );
    const int trackerid = getTrackerID( emid );
    if ( trackerid<0 ) return false;

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker ) return false;

    IOPar iopar;
    iopar.read( setupfilenm, "Tracking Setup", true );
    int connectmode = 0;
    iopar.get( "Seed Connection mode", connectmode );
    seedpicker->setSeedConnectMode( connectmode );
    tracker->usePar( iopar );
    PtrMan<IOPar> attrpar = iopar.subselect( "Attribs" );
    if ( !attrpar ) return true;

    BufferString version;
    float versionnr = 0;
    if ( iopar.get( "dTect", version ) )
    {
	const char* ptr = version.buf();
	ptr += 1;
	versionnr = toFloat( ptr );
    }

    Attrib::DescSet newads( tracker->is2D() );
    newads.usePar( *attrpar, versionnr );
    mergeAttribSets( newads, *tracker ); 

    return true;
}


void uiMPEPartServer::mergeAttribSets( const Attrib::DescSet& newads,
				       MPE::EMTracker& tracker )
{
    const Attrib::DescSet* attrset = getCurAttrDescSet( tracker.is2D() );
    const EM::EMObject* emobj = EM::EMM().getObject( tracker.objectID() );
    for ( int sidx=0; sidx<emobj->nrSections(); sidx++ )
    {
	const EM::SectionID sid = emobj->sectionID( sidx );
	MPE::SectionTracker* st = tracker.getSectionTracker( sid, false );
	if ( !st || !st->adjuster() ) continue;

	for ( int asidx=0; asidx<st->adjuster()->getNrAttributes(); asidx++ )
	{
	    const Attrib::SelSpec* as =
			st->adjuster()->getAttributeSel( asidx );
	    if ( !as || !as->id().isValid() ) continue;

	    if ( as->isStored() )
	    {
		BufferString idstr;
		Attrib::Desc::getParamString( as->defString(), "id", idstr );
		Attrib::DescSet* storedads =
		    Attrib::eDSHolder().getDescSet( tracker.is2D() , true );
		storedads->getStoredID( idstr ); // will try to add if fail
		Attrib::SelSpec newas( *as );
		newas.setIDFromRef( *storedads );
		st->adjuster()->setAttributeSel( asidx, newas );
		continue;
	    }

	    Attrib::DescID newid = Attrib::DescID::undef();
	    const Attrib::Desc* usedad = newads.getDesc( as->id() );
	    if ( !usedad ) continue;

	    for ( int ida=0; ida<attrset->size(); ida++ )
	    {
		const Attrib::DescID descid = attrset->getID( ida );
		const Attrib::Desc* ad = attrset->getDesc( descid );
		if ( !ad ) continue;

		if ( usedad->isIdenticalTo( *ad ) )
		{
		    newid = ad->id();
		    break;
		}
	    }

	    if ( !newid.isValid() )
	    {
		Attrib::DescSet* set = 
		    const_cast<Attrib::DescSet*>( attrset );
		Attrib::Desc* newdesc = new Attrib::Desc( *usedad );
		newdesc->setDescSet( set );
		newid = set->addDesc( newdesc );
	    }

	    Attrib::SelSpec newas( *as );
	    newas.setIDFromRef( *attrset );
	    st->adjuster()->setAttributeSel( asidx, newas );
	}
    }
}


bool uiMPEPartServer::fireAddTreeObjectEvent()
{ return sendEvent( ::uiMPEPartServer::evAddTreeObject() ); }


bool uiMPEPartServer::initSetupDlg( EM::EMObject*& emobj,
				    MPE::EMTracker*& tracker,
				    const EM::SectionID& sid,
				    bool freshdlg )
{
    if ( !emobj || !tracker || sid<0 ) return false;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker( true );
    if ( !seedpicker ) return false;

    uiDialog* setupdlg  = new uiDialog( parent(),
	    			uiDialog::Setup("Tracking Setup",0,"108.0.1")
				.modal(false) );
    setupdlg->setCtrlStyle( uiDialog::LeaveOnly );
    setupgrp_ = MPE::uiMPE().setupgrpfact.create( tracker->getTypeStr(),
	    					  setupdlg, emobj->getTypeStr(),
						  0 );

    if ( !setupgrp_ ) return false;

    MPE::SectionTracker* sectracker = tracker->getSectionTracker( sid, true );
    if ( !sectracker ) return false;

    setupgrp_->setAttribSet( getCurAttrDescSet(tracker->is2D()) );

    if ( freshdlg )
    {
	seedpicker->setSeedConnectMode( 0 );

	if ( cursceneid_ != -1 )
	    sendEvent( uiMPEPartServer::evUpdateSeedConMode() );
    }
    else
    {
	const bool setupavailable = sectracker &&
	    			    sectracker->hasInitializedSetup();
	if ( !setupavailable )
	    seedpicker->setSeedConnectMode( 
		    			seedpicker->defaultSeedConMode(false) );

	setupgrp_->setAttribSelSpec(sectracker->adjuster()->getAttributeSel(0));
    }

    setupgrp_->setMode( (MPE::EMSeedPicker::SeedModeOrder)
	    			seedpicker->getSeedConnectMode() );
    setupgrp_->setColor( emobj->preferredColor() );
    setupgrp_->setMarkerStyle( emobj->getPosAttrMarkerStyle(
						EM::EMObject::sSeedNode()) );
    setupgrp_->setSectionTracker( sectracker );

    NotifierAccess* modechangenotifier = setupgrp_->modeChangeNotifier();
    if ( modechangenotifier )
	modechangenotifier->notify( mCB(this,uiMPEPartServer,modeChangedCB) );

    NotifierAccess* propchgenotifier = setupgrp_->propertyChangeNotifier();
    if ( propchgenotifier )
	propchgenotifier->notify( mCB(this,uiMPEPartServer,propertyChangedCB) );

    NotifierAccess* evchgenotifier = setupgrp_->eventChangeNotifier();
    if ( evchgenotifier )
	evchgenotifier->notify(
			mCB(this,uiMPEPartServer,eventorsimimlartyChangedCB) );

    NotifierAccess* simichgenotifier = setupgrp_->similartyChangeNotifier();
    if ( simichgenotifier )
	simichgenotifier->notify(
			  mCB(this,uiMPEPartServer,eventorsimimlartyChangedCB));

    if ( cursceneid_ != -1 )
	sendEvent( uiMPEPartServer::evStartSeedPick() );

    NotifierAccess* addrmseednotifier = seedpicker->aboutToAddRmSeedNotifier();
    if ( addrmseednotifier )
	addrmseednotifier->notify(
			   mCB(this,uiMPEPartServer,aboutToAddRemoveSeed) );

    setupdlg->windowClosed.notify(
	    		   mCB(this,uiMPEPartServer,trackerWinClosedCB) );
    setupdlg->go();
    sendEvent( uiMPEPartServer::evSetupLaunched() );

    return true;
}


void uiMPEPartServer::fillPar( IOPar& par ) const
{
    MPE::engine().fillPar( par );
}


bool uiMPEPartServer::usePar( const IOPar& par )
{
    bool res = MPE::engine().usePar( par );
    if ( res )
    {
	if ( !sendEvent(evInitFromSession()) )
	    return false;

	if ( MPE::engine().nrTrackersAlive() )
	    sendEvent( evShowToolbar() );

	loadAttribData();
    }

    return res;
}


void uiMPEPartServer::saveUnsaveEMObject()
{
    for ( int idx=EM::EMM().nrLoadedObjects()-1; idx>=0; idx-- )
    {
	const EM::ObjectID objid = EM::EMM().objectID( idx );

	if ( EM::EMM().getMultiID(objid).isEmpty() )
	{
	    BufferString msg = EM::EMM().getObject(objid)->getTypeStr();
	    msg.add( " " ).add( EM::EMM().getObject( objid )->name() );
	    msg += " is not saved.\n Click 'Save' to save ";
	    msg += " to disk or\n click 'Remove' to remove permanently. ";

	    if ( uiMSG().askGoOn(msg, "Save", "Remove") )
		sendEvent( uiMPEPartServer::evSaveUnsavedEMObject() );
	    else
		sendEvent( uiMPEPartServer::evRemoveUnsavedEMObject() );
	}
    }
}
