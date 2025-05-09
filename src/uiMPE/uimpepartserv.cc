/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpepartserv.h"

#include "attribdataholder.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "attribdescsetsholder.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emseedpicker.h"
#include "emsurfacetr.h"
#include "emtracker.h"
#include "executor.h"
#include "file.h"
#include "geomelement.h"
#include "ioman.h"
#include "iopar.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "od_helpids.h"
#include "randcolor.h"
#include "survinfo.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "undo.h"

#include "uihorizontracksetup.h"
#include "uimain.h"
#include "uimpe.h"
#include "uimsg.h"
#include "uitaskrunner.h"

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
int uiMPEPartServer::evSelectAttribForTracking(){ return 15; }


uiMPEPartServer::uiMPEPartServer( uiApplService& a )
    : uiApplPartServer(a)
{
    MPE::engine().setValidator( new MPE::uiTrackSettingsValidator() );

    mAttachCB( MPE::engine().activevolumechange,
	       uiMPEPartServer::activeVolumeChange );
    mAttachCB( MPE::engine().trackeradded,
	       uiMPEPartServer::loadTrackSetupCB );
    mAttachCB( MPE::engine().settingsChanged,
	       uiMPEPartServer::settingsChangedCB );
    mAttachCB( EM::EMM().addRemove,
	       uiMPEPartServer::nrHorChangeCB );
}


uiMPEPartServer::~uiMPEPartServer()
{
    detachAllNotifiers();
    trackercurrentobject_.setUdf();
    initialundoid_ = mUdf(int);
    seedhasbeenpicked_ = false;
    setupbeingupdated_ = false;

    sendEvent( ::uiMPEPartServer::evSetupClosed() );
    if ( setupgrp_ )
    {
	setupgrp_->setMPEPartServer( nullptr );
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
{
    return is2d ? attrset2d_ : attrset3d_;
}


bool uiMPEPartServer::hasTracker() const
{
    return MPE::engine().hasTracker();
}


bool uiMPEPartServer::hasTracker( const EM::ObjectID& emid ) const
{
    return MPE::engine().hasTracker( emid );
}


bool uiMPEPartServer::isActiveTracker( const EM::ObjectID& emid ) const
{
    return MPE::engine().isActiveTracker( emid );
}


void uiMPEPartServer::setActiveTracker( const EM::ObjectID& emid )
{
    RefMan<MPE::EMTracker> tracker = MPE::engine().getTrackerByID( emid );
    MPE::engine().setActiveTracker( tracker.ptr() );
}


ConstRefMan<MPE::EMTracker> uiMPEPartServer::getActiveTracker() const
{
    return MPE::engine().getActiveTracker();
}


ConstRefMan<MPE::EMTracker>
	uiMPEPartServer::getTrackerByID( const EM::ObjectID& emid ) const
{
    return MPE::engine().getTrackerByID( emid );
}


RefMan<MPE::EMTracker> uiMPEPartServer::getTrackerByID( const EM::ObjectID& id )
{
    return MPE::engine().getTrackerByID( id );
}


void uiMPEPartServer::getTrackerIDsByType( const char* typestr,
					   TypeSet<EM::ObjectID>& ids ) const
{
    MPE::engine().getTrackerIDsByType( typestr, ids );
}



bool uiMPEPartServer::addTracker( const char* trackertype,
				  const SceneID& addedtosceneid )
{
    seedswithoutattribsel_ = false;
    cursceneid_ = addedtosceneid;

    RefMan<EM::EMObject> emobj = EM::EMM().createTempObject( trackertype );
    if ( !emobj )
	return false;

    emobj->setNewName();
    emobj->setFullyLoaded( true );
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj.ptr());
    if ( hor3d )
    {
	hor3d->initTrackingArrays();
	hor3d->initTrackingAuxData();
    }

    trackercurrentobject_ = emobj->id();
    if ( addedtosceneid.isValid() &&
	 !sendEvent(::uiMPEPartServer::evAddTreeObject()) )
    {
	pErrMsg("Could not create new EMObject display" );
	trackercurrentobject_.setUdf();
	return false;
    }

    RefMan<MPE::EMTracker> tracker = getTrackerByID( emobj->id() );
    if ( tracker )
    {
	tracker->getSectionTracker( true );
    }
    else
    {
	trackercurrentobject_.setUdf();
	return false;
    }

    if ( !initSetupDlg(*emobj.ptr(),*tracker.ptr(),true) )
    {
	trackercurrentobject_.setUdf();
	return false;
    }

    initialundoid_ = EM::EMM().undo(emobj->id()).currentEventID();
    propertyChangedCB( nullptr );

    emobj.setNoDelete( true );

    return true;
}


void uiMPEPartServer::seedAddedCB( CallBacker* )
{
    RefMan<MPE::EMTracker> tracker = getTrackerByID( trackercurrentobject_ );
    if ( !tracker )
	return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker();
    if ( !seedpicker || !seedpicker->getSelSpec() )
	return;

    if ( setupgrp_ )
	setupgrp_->setSeedPos( seedpicker->getAddedSeed() );
}


void uiMPEPartServer::aboutToAddRemoveSeed( CallBacker* )
{
    RefMan<MPE::EMTracker> tracker = getTrackerByID( trackercurrentobject_ );
    if ( !tracker )
	return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker();
    if ( !seedpicker || !seedpicker->getSelSpec() )
	return;

    bool fieldchange = false;
    bool isvalidsetup = false;

    if ( setupgrp_ )
	isvalidsetup = setupgrp_->commitToTracker( fieldchange );

    seedpicker->blockSeedPick( !isvalidsetup );
    if ( !isvalidsetup )
	return;

    seedhasbeenpicked_ = true;
}


void uiMPEPartServer::modeChangedCB( CallBacker* )
{
    RefMan<MPE::EMTracker> tracker = getTrackerByID( trackercurrentobject_ );
    if ( !tracker )
	return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker();
    if ( !seedpicker)
	return;

    if ( setupgrp_ )
    {
	seedpicker->setTrackMode( setupgrp_->getMode() );
	setupgrp_->commitToTracker();
    }
}


void uiMPEPartServer::eventChangedCB( CallBacker* )
{
    if ( !trackercurrentobject_.isValid() )
	return;

    if ( setupgrp_ )
	setupgrp_->commitToTracker();
}


void uiMPEPartServer::correlationChangedCB( CallBacker* )
{
    if ( !trackercurrentobject_.isValid() )
	return;

    if ( setupgrp_ )
	setupgrp_->commitToTracker();
}


void uiMPEPartServer::propertyChangedCB( CallBacker* )
{
    if ( !trackercurrentobject_.isValid() )
	return;

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( trackercurrentobject_ );
    if ( !emobj )
	return;

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
    if ( !trackercurrentobject_.isValid() )
	return;

    for ( int idx=EM::EMM().nrLoadedObjects()-1; idx>=0; idx-- )
    {
	const EM::ObjectID oid = EM::EMM().objectID( idx );
	if ( oid == trackercurrentobject_ )
	    return;
    }

    trackercurrentobject_.setUdf();
    initialundoid_ = mUdf(int);
    seedhasbeenpicked_ = false;
    setupbeingupdated_ = false;

    if ( MPE::engine().hasTracker() )
	return;

    sendEvent( ::uiMPEPartServer::evSetupClosed() );
    if ( setupgrp_ && setupgrp_->mainwin() )
	setupgrp_->mainwin()->close();
}


void uiMPEPartServer::trackerWinClosedCB( CallBacker* )
{
    cleanSetupDependents();
    seedswithoutattribsel_ = false;

    if ( !trackercurrentobject_.isValid() )
	return;

    if ( !hasTracker(trackercurrentobject_) )
    {
	trackercurrentobject_.setUdf();
	initialundoid_ = mUdf(int);
	seedhasbeenpicked_ = false;
	setupbeingupdated_ = false;
	return;
    }

    RefMan<MPE::EMTracker> tracker = getTrackerByID( trackercurrentobject_ );
    if ( !tracker )
	return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker();
    if ( !seedpicker )
	return;

    saveSetup( EM::EMM().getMultiID(trackercurrentobject_) );

    mDetachCB( seedpicker->seedToBeAddedRemoved,
	       uiMPEPartServer::aboutToAddRemoveSeed );
    mDetachCB( seedpicker->seedAdded, uiMPEPartServer::seedAddedCB );

    if ( !seedhasbeenpicked_ || !seedpicker->doesModeUseVolume() )
	sendEvent( uiMPEPartServer::evStartSeedPick() );

    saveSetup( EM::EMM().getMultiID(trackercurrentobject_) );

    sendEvent( ::uiMPEPartServer::evSetupClosed() );

    trackercurrentobject_.setUdf();
    initialundoid_ = mUdf(int);
    seedhasbeenpicked_ = false;
    setupbeingupdated_ = false;
}


void uiMPEPartServer::cleanSetupDependents()
{
    if ( !setupgrp_ )
	return;

    NotifierAccess* modechangenotifier = setupgrp_->modeChangeNotifier();
    if ( modechangenotifier )
	mDetachCB( *modechangenotifier, uiMPEPartServer::modeChangedCB );

    NotifierAccess* propertychangenotifier =
				setupgrp_->propertyChangeNotifier();
    if ( propertychangenotifier )
	mDetachCB( *propertychangenotifier, uiMPEPartServer::propertyChangedCB);

    NotifierAccess* eventchangenotifier = setupgrp_->eventChangeNotifier();
    if ( eventchangenotifier )
	mDetachCB( *eventchangenotifier, uiMPEPartServer::eventChangedCB );

    NotifierAccess* correlationChangeNotifier =
					setupgrp_->correlationChangeNotifier();
    if ( correlationChangeNotifier )
	mDetachCB( *correlationChangeNotifier,
		   uiMPEPartServer::correlationChangedCB );
}


bool uiMPEPartServer::isTrackingEnabled( const EM::ObjectID& emid ) const
{
    ConstRefMan<MPE::EMTracker> tracker = getTrackerByID( emid );
    return tracker && tracker->isEnabled();
}


bool uiMPEPartServer::trackingInProgress() const
{
    return MPE::engine().trackingInProgress();
}


void uiMPEPartServer::enableTracking( const EM::ObjectID& emid, bool yn )
{
    RefMan<MPE::EMTracker> tracker = getTrackerByID( emid );
    if ( !tracker )
	return;

    MPE::engine().setActiveTracker( tracker.ptr() );
    tracker->enable( yn );

    mDynamicCastGet(MPE::uiHorizonSetupGroup*,horgrp,setupgrp_);
    if ( horgrp )
	horgrp->enableTracking( yn );

    if ( yn )
    {
	trackercurrentobject_ = tracker->objectID();
	fillTrackerSettings( trackercurrentobject_ );
    }
}


bool uiMPEPartServer::hasEditor( const EM::ObjectID& emid ) const
{
    return MPE::engine().hasEditor( emid );
}


void uiMPEPartServer::fillTrackerSettings( const EM::ObjectID& emid )
{
    if ( !setupgrp_ )
	return;

    RefMan<MPE::EMTracker> tracker = getTrackerByID( emid );
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker()
					    : nullptr;
    RefMan<EM::EMObject> emobj = tracker ? tracker->emObject() : nullptr;
    if ( !emobj || !seedpicker )
	return;

    MPE::SectionTracker* sectracker = tracker->getSectionTracker( true );
    if ( !sectracker )
	return;

    setupgrp_->setSectionTracker( sectracker );
    setupgrp_->setMode( seedpicker->getTrackMode() );
    setupgrp_->setColor( emobj->preferredColor() );
    setupgrp_->setLineWidth( emobj->preferredLineStyle().width_ );
    setupgrp_->setMarkerStyle(
		emobj->getPosAttrMarkerStyle(EM::EMObject::sSeedNode()) );

    TypeSet<TrcKey> seeds;
    seedpicker->getSeeds( seeds );
    if ( !seeds.isEmpty() )
    {
	TrcKeyValue lastseed( seeds.last() );
	mDynamicCastGet(EM::Horizon*,hor,emobj.ptr())
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


void uiMPEPartServer::settingsChangedCB( CallBacker* )
{
    fillTrackerSettings( trackercurrentobject_ );
}


EM::ObjectID uiMPEPartServer::activeTrackerEMID() const
{
    return trackercurrentobject_;
}


const Attrib::SelSpec* uiMPEPartServer::getAttribSelSpec() const
{
    return eventattrselspec_;
}


bool uiMPEPartServer::showSetupDlg( const EM::ObjectID& emid )
{
    if ( !emid.isValid() )
	return false;

    if ( trackercurrentobject_.isValid() && setupgrp_ )
    {
	if ( setupgrp_->mainwin() )
	{
	    setupgrp_->mainwin()->raise();
	    setupgrp_->mainwin()->show();
	}
    }

    RefMan<MPE::EMTracker> tracker = getTrackerByID( emid );
    if ( !tracker )
	return false;

    trackercurrentobject_ = emid;

    setupbeingupdated_ = true;

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid );
    if ( !emobj )
	return false;

    if ( !initSetupDlg(*emobj.ptr(),*tracker.ptr()) )
	return false;

    if ( setupgrp_ )
	setupgrp_->commitToTracker();

    tracker->applySetupAsDefault();

    return true;
}


bool uiMPEPartServer::showSetupGroupOnTop( const EM::ObjectID& emid,
					   const char* grpnm )
{
    if ( !emid.isValid() || emid!=trackercurrentobject_ || !setupgrp_ )
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


void uiMPEPartServer::useSavedSetupDlg( const EM::ObjectID& emid )
{
    RefMan<MPE::EMTracker> tracker = getTrackerByID( emid );
    ConstRefMan<EM::EMObject> emobj = tracker ? tracker->emObject() : nullptr;
    if ( !emobj )
	return;

    readSetup( emobj->multiID() );
    showSetupDlg( emid );
}


const char* uiMPEPartServer::get2DLineName() const
{
    return Survey::GM().getName( geomid_ );
}


void uiMPEPartServer::set2DSelSpec( const Attrib::SelSpec& as )
{
    lineselspec_ = as;
}


void uiMPEPartServer::activeVolumeChange( CallBacker* )
{
}


TrcKeyZSampling uiMPEPartServer::getAttribVolume(
					const Attrib::SelSpec& as )const
{
    return MPE::engine().getAttribCube(as);
}


bool uiMPEPartServer::prepareSaveSetupAs( const MultiID& oldmid )
{
    const EM::ObjectID emid = EM::EMM().getObjectID( oldmid );
    return hasTracker( emid );
}


bool uiMPEPartServer::saveSetupAs( const MultiID& newmid )
{
    return saveSetup( newmid );
}


bool uiMPEPartServer::saveSetup( const MultiID& mid )
{
    const EM::ObjectID emid = EM::EMM().getObjectID( mid );
    RefMan<MPE::EMTracker> tracker = getTrackerByID( emid );
    if ( !tracker )
	return false;

    RefMan<EM::EMObject> emobject = tracker->emObject();
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobject.ptr());
    if ( hor3d )
	hor3d->saveNodeArrays();

    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker()
					    : nullptr;
    if ( !seedpicker )
	return false;

    IOPar iopar;
    iopar.set( "Seed Connection mode", seedpicker->getTrackMode() );
    tracker->fillPar( iopar );

    const Attrib::DescSet* attrset = getCurAttrDescSet( tracker->is2D() );
    if ( !attrset )
	return false;

    TypeSet<Attrib::SelSpec> usedattribs;
    MPE::engine().getNeededAttribs( usedattribs );

    TypeSet<Attrib::DescID> usedattribids;
    for ( const auto& usedattrib : usedattribs )
    {
	const Attrib::DescID descid = usedattrib.id();
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


void uiMPEPartServer::loadTrackSetupCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack(const EM::ObjectID&,emid,cb);
    RefMan<MPE::EMTracker> emtracker = getTrackerByID( emid );
    if ( !emtracker )
	return;

    ConstRefMan<EM::EMObject> emobj = emtracker->emObject();
    if ( !emobj )
	return;

    const MPE::SectionTracker* sectracker = emtracker->getSectionTracker();
    if ( sectracker && !sectracker->hasInitializedSetup() )
	readSetup( emobj->multiID() );
}


bool uiMPEPartServer::readSetup( const MultiID& mid )
{
    const BufferString setupfilenm = MPE::engine().setupFileName( mid );
    if ( !File::exists(setupfilenm) )
	return false;

    const EM::ObjectID emid = EM::EMM().getObjectID( mid );
    if ( !hasTracker(emid) )
	return false;

    RefMan<MPE::EMTracker> tracker = getTrackerByID( emid );
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker()
					    : nullptr;
    if ( !seedpicker )
	return false;

    IOPar iopar;
    iopar.read( setupfilenm, "Tracking Setup", true );
    int connectmode = 0;
    iopar.get( "Seed Connection mode", connectmode );
    seedpicker->setTrackMode( (MPE::EMSeedPicker::TrackMode)connectmode );
    seedpicker->startSeedPick();
    tracker->usePar( iopar );

    PtrMan<IOPar> attrpar = iopar.subselect( "Attribs" );
    if ( !attrpar )
	return true;

    Attrib::DescSet newads( tracker->is2D() );
    newads.usePar( *attrpar );
    mergeAttribSets( newads, *tracker.ptr() );

    return true;
}


void uiMPEPartServer::attribSelectedForTracking()
{
    if ( setupgrp_ )
	setupgrp_->updateAttribute();
}


void uiMPEPartServer::mergeAttribSets( const Attrib::DescSet& newads,
				       MPE::EMTracker& tracker )
{
    const Attrib::DescSet* attrset = getCurAttrDescSet( tracker.is2D() );
    ConstRefMan<EM::EMObject> emobj = tracker.emObject();
    for ( int sidx=0; sidx<emobj->nrSections(); sidx++ )
    {
	MPE::SectionTracker* st = tracker.getSectionTracker();
	if ( !st || !st->adjuster() )
	    continue;

	for ( int asidx=0; asidx<st->adjuster()->getNrAttributes(); asidx++ )
	{
	    const Attrib::SelSpec* as =
			st->adjuster()->getAttributeSel( asidx );
	    if ( !as || !as->id().isValid() )
		continue;

	    if ( as->isStored() )
	    {
		BufferString idstr;
		Attrib::Desc::getParamString( as->defString(), "id", idstr );
		Attrib::DescSet* storedads =
		    Attrib::eDSHolder().getDescSet( tracker.is2D() , true );
		int compnr = 0; BufferString compstr;
		if ( Attrib::Desc::getParamString(as->defString(),
					Attrib::Desc::sKeyOutput(),compstr) )
		    getFromString( compnr, compstr, 0 );

		MultiID key;
		key.fromString( idstr.buf() );
		storedads->getStoredID( key, compnr, true );
			// will try to add if fail

		Attrib::SelSpec newas( *as );
		newas.setIDFromRef( *storedads );
		st->adjuster()->setAttributeSel( asidx, newas );
		continue;
	    }

	    Attrib::DescID newid = Attrib::DescID::undef();
	    ConstRefMan<Attrib::Desc> usedad = newads.getDesc( as->id() );
	    if ( !usedad )
		continue;

	    for ( int ida=0; ida<attrset->size(); ida++ )
	    {
		const Attrib::DescID descid = attrset->getID( ida );
		ConstRefMan<Attrib::Desc> ad = attrset->getDesc( descid );
		if ( !ad )
		    continue;

		if ( usedad->isIdenticalTo(*ad.ptr()) )
		{
		    newid = ad->id();
		    break;
		}
	    }

	    if ( !newid.isValid() )
	    {
		auto* set = const_cast<Attrib::DescSet*>( attrset );
		RefMan<Attrib::Desc> newdesc = new Attrib::Desc( *usedad );
		newdesc->setDescSet( set );
		newid = set->addDesc( newdesc.ptr() );
	    }

	    Attrib::SelSpec newas( *as );
	    newas.setIDFromRef( *attrset );
	    st->adjuster()->setAttributeSel( asidx, newas );
	}
    }
}


bool uiMPEPartServer::initSetupDlg( EM::EMObject& emobj,
				    MPE::EMTracker& tracker,
				    bool freshdlg )
{
    MPE::EMSeedPicker* seedpicker = tracker.getSeedPicker();
    if ( !seedpicker )
	return false;

    mDynamicCastGet(EM::Horizon3D*,hor3d,&emobj);
    mDynamicCastGet(EM::Horizon2D*,hor2d,&emobj);
    if ( !hor3d && !hor2d )
	return false;

    closeSetupDlg();
    auto* setupdlg = new uiDialog( nullptr,
				   uiDialog::Setup(uiString::empty(),
					  mODHelpKey(mTrackingSetupGroupHelpID))
					.modal(false));
    setupdlg->showAlwaysOnTop();
    setupdlg->setCtrlStyle( uiDialog::CloseOnly );
    setupdlg->setVideoKey( hor3d ? mODVideoKey("horint3d")
				 : mODVideoKey("horint2d") );
    mAttachCB( uiMain::instance().topLevel()->windowClosed,
	       uiMPEPartServer::applClosedCB );
    if ( hor3d )
	setupgrp_ = MPE::uiHorizon3DSetupGroupBase::createInstance( setupdlg );
    else
	setupgrp_ = MPE::uiHorizon2DSetupGroupBase::createInstance( setupdlg );

    if ( !setupgrp_ )
    {
	delete setupdlg;
	return false;
    }

    setupgrp_->setMPEPartServer( this );
    MPE::SectionTracker* sectracker = tracker.getSectionTracker( true );
    if ( !sectracker )
	return false;

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

    fillTrackerSettings( emobj.id() );
    MPE::engine().setActiveTracker( &tracker );

    NotifierAccess* modechangenotifier = setupgrp_->modeChangeNotifier();
    if ( modechangenotifier )
	mAttachCB( *modechangenotifier, uiMPEPartServer::modeChangedCB );

    NotifierAccess* propchangenotifier = setupgrp_->propertyChangeNotifier();
    if ( propchangenotifier )
	mAttachCB( *propchangenotifier, uiMPEPartServer::propertyChangedCB );

    NotifierAccess* evchangenotifier = setupgrp_->eventChangeNotifier();
    if ( evchangenotifier )
	mAttachCB( *evchangenotifier, uiMPEPartServer::eventChangedCB );

    NotifierAccess* simichangenotifier = setupgrp_->correlationChangeNotifier();
    if ( simichangenotifier )
	mAttachCB( *simichangenotifier, uiMPEPartServer::correlationChangedCB );

    if ( cursceneid_.isValid() )
	sendEvent( uiMPEPartServer::evStartSeedPick() );

    mAttachCB( seedpicker->seedToBeAddedRemoved,
	       uiMPEPartServer::aboutToAddRemoveSeed );
    mAttachCB( seedpicker->seedAdded, uiMPEPartServer::seedAddedCB );
    mAttachCB( setupdlg->windowClosed, uiMPEPartServer::trackerWinClosedCB );
    setupdlg->go();
    sendEvent( uiMPEPartServer::evSetupLaunched() );

    return true;
}


void uiMPEPartServer::applClosedCB( CallBacker* )
{
    closeSetupDlg();
}


void uiMPEPartServer::closeSetupDlg()
{
    if ( !setupgrp_ )
	return;

    uiMainWin* mw = setupgrp_->mainwin();
    delete mw;
    setupgrp_ = nullptr;
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
    if ( !MPE::engine().usePar(par) )
	return false;

    return sendEvent( evInitFromSession() );
}
