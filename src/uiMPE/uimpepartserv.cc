/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2004
________________________________________________________________________

-*/

#include "uimpepartserv.h"

#include "attribdataholder.h"
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
#include "uimpe.h"
#include "uimsg.h"
#include "od_helpids.h"

int uiMPEPartServer::evGetAttribData()		{ return 0; }
int uiMPEPartServer::evStartSeedPick()		{ return 1; }
int uiMPEPartServer::evEndSeedPick()		{ return 2; }
int uiMPEPartServer::evAddTreeObject()		{ return 3; }
int uiMPEPartServer::evInitFromSession()	{ return 5; }
int uiMPEPartServer::evRemoveTreeObject()	{ return 6; }
int uiMPEPartServer::evSetupLaunched()		{ return 7; }
int uiMPEPartServer::evSetupClosed()		{ return 8; }
int uiMPEPartServer::evCreate2DSelSpec()	{ return 9; }
int uiMPEPartServer::evUpdateTrees()		{ return 11; }
int uiMPEPartServer::evUpdateSeedConMode()	{ return 12; }
int uiMPEPartServer::evStoreEMObject()		{ return 13; }


uiMPEPartServer::uiMPEPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , attrset3d_( 0 )
    , attrset2d_( 0 )
    , activetrackerid_(-1)
    , eventattrselspec_( 0 )
    , temptrackerid_(-1)
    , cursceneid_(-1)
    , trackercurrentobject_(-1)
    , initialundoid_(mUdf(int))
    , setupgrp_(0)
{
    MPE::engine().setValidator( new MPE::uiTrackSettingsValidator() );

    mAttachCB( MPE::engine().activevolumechange,
		uiMPEPartServer::activeVolumeChange );
    mAttachCB( MPE::engine().loadEMObject, uiMPEPartServer::loadEMObjectCB );
    mAttachCB( MPE::engine().trackeraddremove,
		uiMPEPartServer::trackerAddRemoveCB );
    mAttachCB( MPE::engine().trackertoberemoved,
		uiMPEPartServer::trackerToBeRemovedCB );
    mAttachCB( EM::EMM().addRemove, uiMPEPartServer::nrHorChangeCB );
}


uiMPEPartServer::~uiMPEPartServer()
{
    detachAllNotifiers();

    trackercurrentobject_ = -1;
    initialundoid_ = mUdf(int);

    sendEvent( ::uiMPEPartServer::evSetupClosed() );
    if ( setupgrp_ )
    {
	setupgrp_->setMPEPartServer( 0 );
	uiMainWin* mw = setupgrp_->mainwin();
	delete mw;
    }
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


int uiMPEPartServer::addTracker( const EM::ObjectID& emid )
{
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return -1;

    const int res = MPE::engine().addTracker( emobj );
    if ( res == -1 )
    {
	uiMSG().error(
		uiStrings::phrCannotCreate( tr("tracker for this object")) );
	return -1;
    }

    return res;
}


bool uiMPEPartServer::addTracker( const char* trackertype, int addedtosceneid )
{
    cursceneid_ = addedtosceneid;
    //NotifyStopper notifystopper( MPE::engine().trackeraddremove );

    EM::EMObject* emobj = EM::EMM().createTempObject( trackertype );
    if ( !emobj )
	return false;

    emobj->setNewName();
    emobj->setFullyLoaded( true );

    const int trackerid = MPE::engine().addTracker( emobj );
    if ( trackerid == -1 ) return false;

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return false;

    activetrackerid_ = trackerid;
    if ( (addedtosceneid!=-1) &&
	 !sendEvent(::uiMPEPartServer::evAddTreeObject()) )
    {
	pErrMsg("Could not create tracker" );
	MPE::engine().removeTracker( trackerid );
	emobj->ref(); emobj->unRef();
	return false;
    }

    const EM::SectionID sid = emobj->sectionID( emobj->nrSections()-1 );
    trackercurrentobject_ = emobj->id();
    if ( !initSetupDlg(emobj,tracker,sid,true) )
	return false;

    initialundoid_ = EM::EMM().undo().currentEventID();
    propertyChangedCB(0);

    MPE::engine().unRefTracker( emobj->id(), true );
    return true;
}


void uiMPEPartServer::seedAddedCB( CallBacker* cb )
{
    mDynamicCastGet(MPE::EMSeedPicker*,seedpicker,cb)
    if ( setupgrp_ && seedpicker )
	setupgrp_->setSeedPos( seedpicker->getAddedSeed() );
}


void uiMPEPartServer::aboutToAddRemoveSeed( CallBacker* cb )
{
    mDynamicCastGet(MPE::EMSeedPicker*,seedpicker,cb)

    bool fieldchange = false;
    bool isvalidsetup = false;
    if ( setupgrp_ )
	isvalidsetup = setupgrp_->commitToTracker( fieldchange );

    if ( seedpicker )
	seedpicker->blockSeedPick( !isvalidsetup );
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
    {
	seedpicker->setTrackMode( setupgrp_->getMode() );
	setupgrp_->commitToTracker();
    }
}


void uiMPEPartServer::eventChangedCB( CallBacker* )
{
    if ( trackercurrentobject_ == -1 )
	return;

    if ( setupgrp_ )
	setupgrp_->commitToTracker();
}


void uiMPEPartServer::correlationChangedCB( CallBacker* )
{
    if ( trackercurrentobject_ == -1 )
	return;

    if ( setupgrp_ )
	setupgrp_->commitToTracker();
}


void uiMPEPartServer::propertyChangedCB( CallBacker* )
{
    if ( trackercurrentobject_ == -1 )
	return;

    EM::EMObject* emobj = EM::EMM().getObject( trackercurrentobject_ );
    if ( !emobj ) return;

    if ( setupgrp_ )
    {
	emobj->setPreferredColor( setupgrp_->getColor() );
	sendEvent( uiMPEPartServer::evUpdateTrees() );
	emobj->setPosAttrMarkerStyle( EM::EMObject::sSeedNode(),
				      setupgrp_->getMarkerStyle() );

	OD::LineStyle ls = emobj->preferredLineStyle();
	ls.width_ = setupgrp_->getLineWidth();
	emobj->setPreferredLineStyle( ls );
    }
}


void uiMPEPartServer::nrHorChangeCB( CallBacker* )
{
    if ( trackercurrentobject_ == -1 ) return;

    for ( int idx=EM::EMM().nrLoadedObjects()-1; idx>=0; idx-- )
    {
	const EM::ObjectID oid = EM::EMM().objectID( idx );
	if ( oid == trackercurrentobject_ ) return;
    }

    trackercurrentobject_ = -1;
    initialundoid_ = mUdf(int);

    if ( MPE::engine().nrTrackersAlive() > 0 )
	return;

    sendEvent( ::uiMPEPartServer::evSetupClosed() );
    if ( setupgrp_ && setupgrp_->mainwin() )
	setupgrp_->mainwin()->close();
}


void uiMPEPartServer::trackerWinClosedCB( CallBacker* )
{
    cleanSetupDependents();

    if ( trackercurrentobject_ == -1 ) return;

    const int trackerid = getTrackerID( trackercurrentobject_ );
    if ( trackerid == -1 )
    {
	trackercurrentobject_ = -1;
	initialundoid_ = mUdf(int);
	return;
    }

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return;

    if ( setupgrp_ )
	setupgrp_->commitToTracker();


    saveSetup( EM::EMM().getMultiID( trackercurrentobject_) );

    sendEvent( ::uiMPEPartServer::evSetupClosed() );

    trackercurrentobject_ = -1;
    initialundoid_ = mUdf(int);
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
			mCB(this,uiMPEPartServer,eventChangedCB) );

    NotifierAccess* correlationChangeNotifier =
					setupgrp_->correlationChangeNotifier();
    if ( correlationChangeNotifier )
	correlationChangeNotifier->remove(
			mCB(this,uiMPEPartServer,correlationChangedCB) );
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
    if ( setupgrp_ ) setupgrp_->enableTracking( yn );

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker();
    if ( yn )
    {
	activetrackerid_ = trackerid;
	trackercurrentobject_ =
	    tracker->emObject() ? tracker->emObject()->id() : -1;
	fillTrackerSettings( trackerid );
	if ( seedpicker )
	{
	    seedpicker->seedToBeAddedRemoved.notify(
			mCB(this,uiMPEPartServer,aboutToAddRemoveSeed) );
	    seedpicker->seedAdded.notify(
			mCB(this,uiMPEPartServer,seedAddedCB) );
	}
    }
    else
    {
	if ( seedpicker )
	{
	    seedpicker->seedToBeAddedRemoved.remove(
			mCB(this,uiMPEPartServer,aboutToAddRemoveSeed) );
	    seedpicker->seedAdded.remove(
			mCB(this,uiMPEPartServer,seedAddedCB) );
	}
    }
}


void uiMPEPartServer::fillTrackerSettings( int trackerid )
{
    if ( !setupgrp_ ) return;

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    EM::EMObject* emobj = tracker ? tracker->emObject() : 0;
    if ( !emobj || !seedpicker ) return;

    EM::SectionID sid = emobj->sectionID( 0 );
    MPE::SectionTracker* sectracker = tracker->getSectionTracker( sid, true );
    if ( !sectracker ) return;

    setupgrp_->setSectionTracker( sectracker );
    setupgrp_->setMode( seedpicker->getTrackMode() );
    setupgrp_->setColor( emobj->preferredColor() );
    setupgrp_->setLineWidth( emobj->preferredLineStyle().width_ );
    setupgrp_->setMarkerStyle( emobj->getPosAttrMarkerStyle(
						EM::EMObject::sSeedNode()) );

    TypeSet<TrcKey> seeds;
    seedpicker->getSeeds( seeds );
    if ( !seeds.isEmpty() )
    {
	TrcKeyValue lastseed( seeds.last() );
	mDynamicCastGet(EM::Horizon*,hor,emobj)
	lastseed.val_ = hor ? hor->getZ( lastseed.tk_ ) : mUdf(float);
	setupgrp_->setSeedPos( lastseed );
    }

    if ( setupgrp_->mainwin() )
    {
	const uiString caption =
		tr("Horizon Tracking Settings - %1").arg( emobj->name() );
	setupgrp_->mainwin()->setCaption( caption );
    }
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

    if ( trackercurrentobject_!=-1 && setupgrp_ )
    {
	if ( setupgrp_->mainwin() )
	{
	    setupgrp_->mainwin()->raise();
	    setupgrp_->mainwin()->show();
	}

	return true;
    }

    const int trackerid = getTrackerID( emid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return false;

    trackercurrentobject_ = emid;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return false;

    if ( !initSetupDlg(emobj,tracker,sid) )
	return false;

    if ( setupgrp_ ) setupgrp_->commitToTracker();

    tracker->applySetupAsDefault( sid );

    return true;
}


bool uiMPEPartServer::showSetupGroupOnTop( const EM::ObjectID& emid,
					   const char* grpnm )
{
    if ( emid<0 || emid!=trackercurrentobject_ || !setupgrp_ )
	return false;

    setupgrp_->showGroupOnTop( grpnm );
    return true;
}


uiString uiMPEPartServer::sYesAskGoOnStr()
{
    return tr("This object has saved tracker settings.\n\n"
				    "Do you want to verify / change them?");
}


uiString uiMPEPartServer::sNoAskGoOnStr()
{
    return tr( "This object was created by manual drawing"
			    " only, or its tracker settings were not saved."
			    "\n\nDo you want to specify them right now?" );
}


#define mAskGoOnStr(setupavailable) \
    ( setupavailable ? sYesAskGoOnStr() : sNoAskGoOnStr() )\


void uiMPEPartServer::useSavedSetupDlg( const EM::ObjectID& emid,
					const EM::SectionID& sid )
{
    const int trackerid = getTrackerID( emid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj )
	return;

    readSetup( emobj->multiID() );

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


const char* uiMPEPartServer::get2DLineName() const
{ return Survey::GM().getName(geomid_); }


void uiMPEPartServer::set2DSelSpec(const Attrib::SelSpec& as)
{ lineselspec_ = as; }


void uiMPEPartServer::activeVolumeChange( CallBacker* )
{
}


TrcKeyZSampling uiMPEPartServer::getAttribVolume(
					const Attrib::SelSpec& as )const
{ return MPE::engine().getAttribCube(as); }


void uiMPEPartServer::loadEMObjectCB(CallBacker*)
{
    PtrMan<Executor> exec = EM::EMM().objectLoader( MPE::engine().midtoload );
    if ( !exec ) return;

    const EM::ObjectID emid = EM::EMM().getObjectID( MPE::engine().midtoload );
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    emobj->ref();
    uiTaskRunner uiexec( appserv().parent() );
    const bool keepobj = TaskRunner::execute( &uiexec, *exec );
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

    mDynamicCastGet(EM::Horizon3D*,hor3d,EM::EMM().getObject(emid))
    if ( hor3d ) hor3d->saveParentArray();

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker ) return false;

    IOPar iopar;
    iopar.set( "Seed Connection mode", seedpicker->getTrackMode() );
    tracker->fillPar( iopar );

    const Attrib::DescSet* attrset = getCurAttrDescSet( tracker->is2D() );
    if ( !attrset )
	return false;

    TypeSet<Attrib::SelSpec> usedattribs;
    MPE::engine().getNeededAttribs( usedattribs );

    TypeSet<Attrib::DescID> usedattribids;
    for ( int idx=0; idx<usedattribs.size(); idx++ )
    {
	const Attrib::DescID descid = usedattribs[idx].id();
	if ( attrset->getDesc(descid) )
	    usedattribids += descid;
    }

    PtrMan<Attrib::DescSet> ads = attrset->optimizeClone( usedattribids );
    IOPar attrpar;
    if ( ads.ptr() )
	ads->fillPar( attrpar );

    iopar.mergeComp( attrpar, "Attribs" );

    BufferString setupfilenm = MPE::engine().setupFileName( mid );
    if ( !setupfilenm.isEmpty() && !iopar.write(setupfilenm,"Tracking Setup") )
    {
	uiString errmsg( tr("Unable to save tracking setup file \n"
			    " %1 .\nPlease check whether the file is writable")
			    .arg(setupfilenm) );
	uiMSG().error( errmsg );
	return false;
    }

    return true;
}


void uiMPEPartServer::trackerAddRemoveCB( CallBacker* )
{
    const int trackerid = MPE::engine().highestTrackerID();
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return;

    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj ) return;

// The rest should only be called when tracker is added ...
    const EM::SectionID sid = emobj->sectionID(0);
    const MPE::SectionTracker* sectracker =
			tracker->getSectionTracker( sid, false );
    if ( sectracker && !sectracker->hasInitializedSetup() )
	readSetup( emobj->multiID() );

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker();
    if ( seedpicker )
    {
	seedpicker->seedToBeAddedRemoved.notify(
			mCB(this,uiMPEPartServer,aboutToAddRemoveSeed) );
	seedpicker->seedAdded.notify(
			mCB(this,uiMPEPartServer,seedAddedCB) );
    }
}


void uiMPEPartServer::trackerToBeRemovedCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,trackeridx,cb);
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackeridx );
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker() : 0;
    if ( seedpicker )
    {
	seedpicker->seedToBeAddedRemoved.remove(
			mCB(this,uiMPEPartServer,aboutToAddRemoveSeed) );
	seedpicker->seedAdded.remove(
			mCB(this,uiMPEPartServer,seedAddedCB) );
    }
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
    seedpicker->setTrackMode( (MPE::EMSeedPicker::TrackMode)connectmode );
    tracker->usePar( iopar );

    PtrMan<IOPar> attrpar = iopar.subselect( "Attribs" );
    if ( !attrpar ) return true;

    Attrib::DescSet newads( tracker->is2D() );
    newads.usePar( *attrpar );
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
		storedads->getStoredID( MultiID(idstr.buf()) );
			// will try to add if fail

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


bool uiMPEPartServer::initSetupDlg( EM::EMObject*& emobj,
				    MPE::EMTracker*& tracker,
				    const EM::SectionID& sid,
				    bool freshdlg )
{
    if ( !emobj || !tracker || sid<0 ) return false;

    if ( setupgrp_ )
    {
	uiMainWin* mw = setupgrp_->mainwin();
	delete mw;
    }

    uiDialog* setupdlg = new uiDialog( 0,
		uiDialog::Setup(uiStrings::sEmptyString(),mNoDlgTitle,
				mODHelpKey(mTrackingSetupGroupHelpID) )
				.modal(false) );
    setupdlg->showAlwaysOnTop();
    setupdlg->setCtrlStyle( uiDialog::CloseOnly );
    setupgrp_ = MPE::uiMPE().setupgrpfact.create( tracker->getTypeStr(),
						  setupdlg,
						  emobj->getTypeStr() );
    if ( !setupgrp_ )
    {
	delete setupdlg;
	return false;
    }

    setupgrp_->setMPEPartServer( this );
    MPE::SectionTracker* sectracker = tracker->getSectionTracker( sid, true );
    if ( !sectracker ) return false;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker( true );
    if ( !seedpicker ) return false;

    if ( freshdlg )
    {
	seedpicker->setTrackMode( MPE::EMSeedPicker::TrackFromSeeds );
    }
    else
    {
	const bool setupavailable = sectracker &&
				    sectracker->hasInitializedSetup();
	if ( !setupavailable )
	    seedpicker->setTrackMode( MPE::EMSeedPicker::TrackFromSeeds );
    }

    const int trackerid = MPE::engine().getTrackerByObject( emobj->id() );
    enableTracking( trackerid, true );

    NotifierAccess* modechangenotifier = setupgrp_->modeChangeNotifier();
    if ( modechangenotifier )
	modechangenotifier->notify( mCB(this,uiMPEPartServer,modeChangedCB) );

    NotifierAccess* propchangenotifier = setupgrp_->propertyChangeNotifier();
    if ( propchangenotifier )
	propchangenotifier->notify(
			mCB(this,uiMPEPartServer,propertyChangedCB) );

    NotifierAccess* evchangenotifier = setupgrp_->eventChangeNotifier();
    if ( evchangenotifier )
	evchangenotifier->notify(
			mCB(this,uiMPEPartServer,eventChangedCB) );

    NotifierAccess* simichangenotifier = setupgrp_->correlationChangeNotifier();
    if ( simichangenotifier )
	simichangenotifier->notify(
			mCB(this,uiMPEPartServer,correlationChangedCB));

    if ( cursceneid_ != -1 )
	sendEvent( uiMPEPartServer::evStartSeedPick() );

    setupdlg->windowClosed.notify(
			   mCB(this,uiMPEPartServer,trackerWinClosedCB) );
    setupdlg->go();
    sendEvent( uiMPEPartServer::evSetupLaunched() );

    return true;
}


bool uiMPEPartServer::sendMPEEvent( int evid )
{
    return sendEvent( evid );
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
    }

    return res;
}
