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
int uiMPEPartServer::evHorizonTracking()	{ return 14; }


uiMPEPartServer::uiMPEPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , activetrackerid_(-1)
    , eventattrselspec_( 0 )
    , temptrackerid_(-1)
    , cursceneid_(-1)
    , trackercurrentobject_(DBKey::getInvalid())
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
    mAttachCB( EM::MGR().addRemove, uiMPEPartServer::nrHorChangeCB );
}


uiMPEPartServer::~uiMPEPartServer()
{
    detachAllNotifiers();

    trackercurrentobject_ = DBKey::getInvalid();
    initialundoid_ = mUdf(int);

    sendEvent( ::uiMPEPartServer::evSetupClosed() );
    if ( setupgrp_ )
    {
	setupgrp_->setMPEPartServer( 0 );
	uiMainWin* mw = setupgrp_->mainwin();
	delete mw;
    }
}


int uiMPEPartServer::getTrackerID( const DBKey& emid ) const
{
    for ( int idx=0; idx<=MPE::engine().highestTrackerID(); idx++ )
    {
	if ( MPE::engine().getTracker(idx) )
	{
	    DBKey objid = MPE::engine().getTracker(idx)->objectID();
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


int uiMPEPartServer::addTracker( const DBKey& emid )
{
    EM::Object* emobj = EM::MGR().getObject( emid );
    if ( !emobj ) return -1;

    const int res = MPE::engine().addTracker( emobj );
    if ( res == -1 )
    {
	uimsg().error(
		uiStrings::phrCannotCreate( tr("tracker for this object")) );
	return -1;
    }

    return res;
}


bool uiMPEPartServer::addTracker( const DBKey& objid, int addedtosceneid )
{
    cursceneid_ = addedtosceneid;
    //NotifyStopper notifystopper( MPE::engine().trackeraddremove );
    EM::Object* emobj = EM::MGR().getObject( objid );
    if ( !emobj ) return -1;

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

    trackercurrentobject_ = objid;
    if ( !initSetupDlg(emobj,tracker,true) )
	return false;

    initialundoid_ = EM::MGR().undo(objid).currentEventID();
    propertyChangedCB(0);

    MPE::engine().unRefTracker( objid, true );
    return true;
}


bool uiMPEPartServer::addTracker( const char* trackertype, int addedtosceneid )
{
    cursceneid_ = addedtosceneid;
    //NotifyStopper notifystopper( MPE::engine().trackeraddremove );

    EM::Object* emobj = EM::MGR().createTempObject( trackertype );
    if ( !emobj )
	return false;

    emobj->setNameToJustCreated();
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

    trackercurrentobject_ = emobj->id();
    if ( !initSetupDlg(emobj,tracker,true) )
	return false;

    initialundoid_ = EM::MGR().undo(emobj->id()).currentEventID();
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
    if ( trackercurrentobject_.isInvalid() )
	return;

    if ( setupgrp_ )
	setupgrp_->commitToTracker();
}


void uiMPEPartServer::correlationChangedCB( CallBacker* )
{
    if ( trackercurrentobject_.isInvalid() )
	return;

    if ( setupgrp_ )
	setupgrp_->commitToTracker();
}


void uiMPEPartServer::propertyChangedCB( CallBacker* )
{
    if ( trackercurrentobject_.isInvalid() )
	return;

    EM::Object* emobj = EM::MGR().getObject( trackercurrentobject_ );
    if ( !emobj ) return;

    if ( setupgrp_ )
    {
	emobj->setPreferredColor( setupgrp_->getColor() );
	sendEvent( uiMPEPartServer::evUpdateTrees() );
	emobj->setPosAttrMarkerStyle( EM::Object::sSeedNode(),
				      setupgrp_->getMarkerStyle() );

	OD::LineStyle ls = emobj->preferredLineStyle();
	ls.width_ = setupgrp_->getLineWidth();
	emobj->setPreferredLineStyle( ls );
    }
}


void uiMPEPartServer::nrHorChangeCB( CallBacker* )
{
    if ( trackercurrentobject_.isInvalid() ||
	    EM::MGR().getObject(trackercurrentobject_) )
	return;

    trackercurrentobject_ = DBKey::getInvalid();
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

    if ( trackercurrentobject_.isInvalid() ) return;

    const int trackerid = getTrackerID( trackercurrentobject_ );
    if ( trackerid == -1 )
    {
	trackercurrentobject_ = DBKey::getInvalid();
	initialundoid_ = mUdf(int);
	return;
    }

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return;


    saveSetup( trackercurrentobject_ );

    sendEvent( ::uiMPEPartServer::evSetupClosed() );

    trackercurrentobject_ = DBKey::getInvalid();
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


DBKey uiMPEPartServer::getEMObjectID( int trackerid ) const
{
    const MPE::EMTracker* emt = MPE::engine().getTracker(trackerid);
    return emt ? emt->objectID() : DBKey::getInvalid();
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
	trackercurrentobject_ = tracker->emObject() ? tracker->emObject()->id()
						    : DBKey::getInvalid();
	fillTrackerSettings( trackerid );
	if ( seedpicker )
	{
	    seedpicker->seedToBeAddedRemoved.notify(
			mCB(this,uiMPEPartServer,aboutToAddRemoveSeed) );
	    seedpicker->seedAdded.notify(
			mCB(this,uiMPEPartServer,seedAddedCB) );
	    seedpicker->startSeedPick();
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
    EM::Object* emobj = tracker ? tracker->emObject() : 0;
    if ( !emobj || !seedpicker ) return;

    MPE::SectionTracker* sectracker = tracker->getSectionTracker( true );
    if ( !sectracker ) return;

    setupgrp_->setSectionTracker( sectracker );
    setupgrp_->setMode( seedpicker->getTrackMode() );
    setupgrp_->setColor( emobj->preferredColor() );
    setupgrp_->setLineWidth( emobj->preferredLineStyle().width_ );
    setupgrp_->setMarkerStyle( emobj->getPosAttrMarkerStyle(
						EM::Object::sSeedNode()) );

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


bool uiMPEPartServer::showSetupDlg( const DBKey& emid )
{
    if ( emid.isInvalid() )
	return false;

    if ( !trackercurrentobject_.isInvalid() && setupgrp_ )
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

    EM::Object* emobj = EM::MGR().getObject( emid );
    if ( !emobj ) return false;

    if ( !initSetupDlg(emobj,tracker) )
	return false;

    if ( setupgrp_ ) setupgrp_->commitToTracker();

    return true;
}


bool uiMPEPartServer::showSetupGroupOnTop( const DBKey& emid,
					   const char* grpnm )
{
    if ( emid.isInvalid() || emid!=trackercurrentobject_ || !setupgrp_ )
	return false;

    setupgrp_->showGroupOnTop( grpnm );
    return true;
}


void uiMPEPartServer::useSavedSetupDlg( const DBKey& emid )
{
    const int trackerid = getTrackerID( emid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    const EM::Object* emobj = EM::MGR().getObject( tracker->objectID() );
    if ( !emobj )
	return;

    readSetup( emobj->dbKey() );
    showSetupDlg( emid );
}


void uiMPEPartServer::setAttribData( const Attrib::SelSpec& spec,
				     DataPack::ID datapackid )
{
    MPE::engine().setAttribData( spec, datapackid );
}


const char* uiMPEPartServer::get2DLineName() const
{
    return geomid_.name();
}


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
    uiTaskRunnerProvider trprov( appserv().parent() );
    ConstRefMan<EM::Object> emobj = EM::MGR().fetch( MPE::engine().emidtoload_,
					        trprov );
    if ( !emobj ) return;

    emobj.setNoDelete( true );
}


bool uiMPEPartServer::prepareSaveSetupAs( const DBKey& oldmid )
{
    if ( getTrackerID(oldmid) >= 0 )
	return true;

    EM::Object* emobj = EM::MGR().getObject( oldmid );
    if ( !emobj )
	return false;

    temptrackerid_ = MPE::engine().addTracker( emobj );

    return temptrackerid_ >= 0;
}


bool uiMPEPartServer::saveSetupAs( const DBKey& newmid )
{
    const int res = saveSetup( newmid );
    MPE::engine().removeTracker( temptrackerid_ );
    temptrackerid_ = -1;
    return res;
}


bool uiMPEPartServer::saveSetup( const DBKey& emid )
{
    const int trackerid = getTrackerID( emid );
    if ( trackerid<0 ) return false;

    mDynamicCastGet(EM::Horizon3D*,hor3d,EM::MGR().getObject(emid))
    if ( hor3d ) hor3d->saveNodeArrays();

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker ) return false;

    IOPar iopar;
    iopar.set( "Seed Connection mode", seedpicker->getTrackMode() );
    tracker->fillPar( iopar );

    const Attrib::DescSet& attrset = Attrib::DescSet::global( tracker->is2D() );
    Attrib::SelSpecList usedattribs;
    MPE::engine().getNeededAttribs( usedattribs );

    TypeSet<Attrib::DescID> usedattribids;
    for ( int idx=0; idx<usedattribs.size(); idx++ )
    {
	const Attrib::DescID descid = usedattribs[idx].id();
	if ( attrset.getDesc(descid) )
	    usedattribids += descid;
    }

    PtrMan<Attrib::DescSet> ads = attrset.optimizeClone( usedattribids );
    IOPar attrpar;
    if ( ads.ptr() )
	ads->fillPar( attrpar );

    iopar.mergeComp( attrpar, "Attribs" );

    BufferString setupfilenm = MPE::engine().setupFileName( emid );
    if ( !setupfilenm.isEmpty() && !iopar.write(setupfilenm,"Tracking Setup") )
    {
	uiString errmsg( tr("Unable to save tracking setup file \n"
			    " %1 .\nPlease check whether the file is writable")
			    .arg(setupfilenm) );
	uimsg().error( errmsg );
	return false;
    }

    return true;
}


void uiMPEPartServer::trackerAddRemoveCB( CallBacker* )
{
    const int trackerid = MPE::engine().highestTrackerID();
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( !tracker ) return;

    const EM::Object* emobj = EM::MGR().getObject( tracker->objectID() );
    if ( !emobj ) return;

// The rest should only be called when tracker is added ...
    const MPE::SectionTracker* sectracker =
			tracker->getSectionTracker( false );
    if ( sectracker && !sectracker->hasInitializedSetup() )
	readSetup( emobj->dbKey() );

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


bool uiMPEPartServer::readSetup( const DBKey& emid )
{
    BufferString setupfilenm = MPE::engine().setupFileName( emid );
    if ( !File::exists(setupfilenm) ) return false;

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
    seedpicker->startSeedPick();
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
    const Attrib::DescSet& attrset = Attrib::DescSet::global( tracker.is2D() );
    MPE::SectionTracker* st = tracker.getSectionTracker( false );
    if ( !st || !st->adjuster() ) return;

    for ( int asidx=0; asidx<st->adjuster()->getNrAttributes(); asidx++ )
    {
	const Attrib::SelSpec* as =
		    st->adjuster()->getAttributeSel( asidx );
	if ( !as || !as->id().isValid() )
	    continue;

	if ( as->isStored(&attrset) )
	{
	    BufferString idstr;
	    Attrib::Desc::getParamString( as->defString(), "id", idstr );
	    attrset.getStoredID( DBKey(idstr) );

	    Attrib::SelSpec newas( *as );
	    newas.setIDFromRef( attrset );
	    st->adjuster()->setAttributeSel( asidx, newas );
	    continue;
	}

	Attrib::DescID newid;
	const Attrib::Desc* usedad = newads.getDesc( as->id() );
	if ( !usedad )
	    continue;

	for ( int ida=0; ida<attrset.size(); ida++ )
	{
	    const Attrib::DescID descid = attrset.getID( ida );
	    const Attrib::Desc* ad = attrset.getDesc( descid );
	    if ( !ad )
		continue;

	    if ( usedad->isIdenticalTo( *ad ) )
	    {
		newid = ad->id();
		break;
	    }
	}

	if ( !newid.isValid() )
	{
	    Attrib::DescSet* set =
		const_cast<Attrib::DescSet*>( &attrset );
	    Attrib::Desc* newdesc = new Attrib::Desc( *usedad );
	    newdesc->setDescSet( set );
	    newid = set->addDesc( newdesc );
	}

	Attrib::SelSpec newas( *as );
	newas.setIDFromRef( attrset );
	st->adjuster()->setAttributeSel( asidx, newas );
    }
}


bool uiMPEPartServer::initSetupDlg( EM::Object*& emobj,
				    MPE::EMTracker*& tracker,
				    bool freshdlg )
{
    if ( !emobj || !tracker ) return false;

    if ( setupgrp_ )
    {
	uiMainWin* mw = setupgrp_->mainwin();
	delete mw;
    }

    uiDialog* setupdlg = new uiDialog( 0,
		uiDialog::Setup(uiString::empty(),mNoDlgTitle,
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
    MPE::SectionTracker* sectracker = tracker->getSectionTracker( true );
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
