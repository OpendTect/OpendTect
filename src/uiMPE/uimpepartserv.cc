/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2004
 RCS:           $Id: uimpepartserv.cc,v 1.32 2005-10-21 19:34:13 cvskris Exp $
________________________________________________________________________

-*/

#include "uimpepartserv.h"

#include "geomelement.h"
#include "mpeengine.h"
#include "emtracker.h"
#include "survinfo.h"
#include "uimpewizard.h"
#include "uimpesetup.h"
#include "uimsg.h"
#include "uicursor.h"
#include "uisurfacerelationdlg.h"
#include "emmanager.h"
#include "emobject.h"
#include "executor.h"
#include "uiexecutor.h"

const int uiMPEPartServer::evGetAttribData	= 0;
const int uiMPEPartServer::evStartSeedPick	= 1;
const int uiMPEPartServer::evEndSeedPick	= 2;
const int uiMPEPartServer::evAddTreeObject	= 3;
const int uiMPEPartServer::evShowToolbar	= 4;
const int uiMPEPartServer::evInitFromSession	= 5;
const int uiMPEPartServer::evRemoveTreeObject	= 6;
const int uiMPEPartServer::evWizardClosed	= 7;


uiMPEPartServer::uiMPEPartServer( uiApplService& a, const Attrib::DescSet* ads )
    : uiApplPartServer(a)
    , attrset( ads )
    , wizard( 0 )
    , activetrackerid(-1)
    , eventattrselspec( 0 )
    , blockdataloading( false )
{
    MPE::initStandardClasses();
    MPE::engine().setActiveVolume( MPE::engine().getDefaultActiveVolume() );
    MPE::engine().activevolumechange.notify(
	    mCB(this, uiMPEPartServer, activeVolumeChange) );
    MPE::engine().loadEMObject.notify(
	    mCB(this, uiMPEPartServer, loadEMObjectCB) );
}


uiMPEPartServer::~uiMPEPartServer()
{
    MPE::engine().activevolumechange.remove(
	    mCB(this, uiMPEPartServer, activeVolumeChange) );
    MPE::engine().loadEMObject.remove(
	    mCB(this, uiMPEPartServer, loadEMObjectCB) );
    delete wizard;
}


void uiMPEPartServer::setCurrentAttribDescSet( const Attrib::DescSet* ads )
{ attrset = ads; }


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
{ MPE::engine().getAvaliableTrackerTypes(res); }


int uiMPEPartServer::addTracker( const EM::ObjectID& emid,
				 const Coord3& pickedpos )
{
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return -1;

    const int res = MPE::engine().addTracker( emobj );
    if ( res==-1 )
    {
	uiMSG().error("Could not create tracker for this object");
	return -1;
    }

    //TODO: Fix for multi-section case
    const EM::SectionID sid =  emobj->sectionID(0);
    blockdataloading = true;
    if ( !showSetupDlg( emid, sid, true ) )
    {
	MPE::engine().removeTracker(res);
	return -1;
    }
    blockdataloading = false;

    CubeSampling poscs(false);
    const BinID bid = SI().transform(pickedpos);
    poscs.hrg.start = poscs.hrg.stop = bid;
    poscs.zrg.start = poscs.zrg.stop = pickedpos.z;
    expandActiveVolume( poscs );

    return res;
}


bool uiMPEPartServer::addTracker( const char* trackertype )
{
    if ( !wizard ) wizard = new MPE::Wizard( appserv().parent(), this );
    else wizard->reset();

    wizard->setTrackingType( trackertype );
    wizard->setRotateMode(true);
    wizard->go();

    return true;
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


void uiMPEPartServer::addSeed( int trackerid )
{
    if ( !wizard ) wizard = new MPE::Wizard( appserv().parent(), this );
    else wizard->reset();

    const MPE::EMTracker* tracker = MPE::engine().getTracker(trackerid);
    if ( !tracker ) return;

    EM::EMObject* object = EM::EMM().getObject( tracker->objectID() );
    if ( !object ) return;

    //TODO Find a mechanism to get this work on multi-section objects
    wizard->setObject( object->id(), object->sectionID(0) );

    wizard->displayPage(MPE::Wizard::sNamePage, false );
    wizard->displayPage(MPE::Wizard::sFinalizePage, false );
    wizard->setRotateMode( false );
    
    wizard->go();
}


bool uiMPEPartServer::isTrackingEnabled( int trackerid ) const
{ return MPE::engine().getTracker(trackerid)->isEnabled(); }


void uiMPEPartServer::enableTracking( int trackerid, bool yn )
{ return MPE::engine().getTracker(trackerid)->enable(yn); }


int uiMPEPartServer::activeTrackerID() const
{ return activetrackerid; }


const Attrib::SelSpec* uiMPEPartServer::getAttribSelSpec() const
{ return eventattrselspec; }



bool uiMPEPartServer::showSetupDlg( const EM::ObjectID& emid,
				    const EM::SectionID& sid,
				    bool showcancelbutton )
{
    uiDialog dlg( appserv().parent(), uiDialog::Setup("Tracking Setup") );
    if ( !showcancelbutton ) 
	dlg.setCtrlStyle( uiDialog::LeaveOnly );

    dlg.setHelpID( "108.0.1" );
    MPE::uiSetupSel* grp = new MPE::uiSetupSel( &dlg, attrset );
    grp->setType( emid, sid );
    if ( dlg.go() )
    {
	loadAttribData();
	sendEvent( evShowToolbar );
	return true;
    }

    return false;
}


void uiMPEPartServer::showRelationsDlg( const EM::ObjectID& objid,
					EM::SectionID sid )
{
    bool allowhorsel = false;
    bool allowfltsel = true;
    uiSurfaceRelationDlg dlg( appserv().parent(), objid,
			      allowhorsel, allowfltsel );
    dlg.go();
}


void uiMPEPartServer::setAttribData( const Attrib::SelSpec& spec,
					Attrib::SliceSet* slcset )
{
    MPE::engine().setAttribData( spec, slcset );
}


void uiMPEPartServer::activeVolumeChange(CallBacker*)
{ loadAttribData(); }


void uiMPEPartServer::loadAttribData()
{
    if ( blockdataloading )
	return;

    uiCursorChanger changer( uiCursor::Wait );

    ObjectSet<const Attrib::SelSpec> attribselspecs;
    MPE::engine().getNeededAttribs(attribselspecs);
    for ( int idx=0; idx<attribselspecs.size(); idx++ )
    {
	eventattrselspec = attribselspecs[idx];
	sendEvent( evGetAttribData );
    }

    sendEvent( evShowToolbar );
}


const Attrib::SliceSet*
    uiMPEPartServer::getAttribCache( const Attrib::SelSpec& spec ) const
{ return MPE::engine().getAttribCache( spec ); }


CubeSampling uiMPEPartServer::getActiveVolume() const
{ return MPE::engine().activeVolume(); }


bool uiMPEPartServer::activeVolumeIsDefault() const
{
    const CubeSampling activecs = MPE::engine().activeVolume();
    if ( activecs==MPE::engine().getDefaultActiveVolume() )
	return true;

    return false;
}


void uiMPEPartServer::expandActiveVolume(const CubeSampling& seedcs)
{
    const CubeSampling activecs = MPE::engine().activeVolume();
    const bool isdefault = activeVolumeIsDefault();

    CubeSampling newcube = isdefault ? seedcs : activecs;
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

    //sendEvent( evShowToolbar );
}


void uiMPEPartServer::loadEMObjectCB(CallBacker*)
{
    PtrMan<Executor> exec = EM::EMM().objectLoader( MPE::engine().midtoload );
    if ( !exec ) return;

    uiExecutor uiexec( appserv().parent(), *exec );
    uiexec.go();
}


void uiMPEPartServer::fillPar( IOPar& par ) const
{
    MPE::engine().fillPar( par );
}


bool uiMPEPartServer::usePar( const IOPar& par )
{
    delete wizard; wizard = 0;
    bool res = MPE::engine().usePar( par );
    if ( res )
    {
	if ( !sendEvent(evInitFromSession) )
	    return false;

	loadAttribData();
	sendEvent( evShowToolbar );
    }
    return res;
}
