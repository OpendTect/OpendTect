/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2004
 RCS:           $Id: uimpepartserv.cc,v 1.13 2005-07-28 10:53:51 cvshelene Exp $
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

const int uiMPEPartServer::evGetAttribData	= 0;
const int uiMPEPartServer::evStartSeedPick	= 1;
const int uiMPEPartServer::evEndSeedPick	= 2;
const int uiMPEPartServer::evAddTreeObject	= 3;
const int uiMPEPartServer::evShowToolbar	= 4;
const int uiMPEPartServer::evInitFromSession	= 5;


uiMPEPartServer::uiMPEPartServer( uiApplService& a, const Attrib::DescSet* ads )
    : uiApplPartServer(a)
    , attrset( ads )
    , wizard(0)
{
    MPE::initStandardClasses();
    csfromseeds.init( false );
}


uiMPEPartServer::~uiMPEPartServer()
{
    delete wizard;
}


void uiMPEPartServer::setCurrentAttribDescSet( const Attrib::DescSet* ads )
{ attrset = ads; }


int uiMPEPartServer::getTrackerID( const MultiID& mid ) const
{
    for ( int idx=0; idx<MPE::engine().highestTrackerID(); idx++ )
    {
	if ( MPE::engine().getTracker(idx) )
	{
	    EM::ObjectID objid = MPE::engine().getTracker(idx)->objectID();
	    if ( objid==-1 )
		continue;

	    EM::EMObject* emobj = EM::EMM().getObject(objid);
	    if ( emobj && emobj->multiID()==mid )
		return idx;
	}
    }

    return -1;
}



int uiMPEPartServer::getTrackerID( const char* name ) const
{
    return MPE::engine().getTrackerByObject(name);
}


void uiMPEPartServer::getTrackerTypes( BufferStringSet& res ) const
{ MPE::engine().getAvaliableTrackerTypes(res); }


int uiMPEPartServer::addTracker( const MultiID& mid, const Coord3& pickedpos )
{
    const EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject( objid );
    if ( !emobj ) return -1;

    activetrackerid = MPE::engine().addTracker( emobj );
    startWizard( emobj->getTypeStr(), 2 );
    wizard->setTrackerID( activetrackerid );

    csfromseeds.zrg.start = csfromseeds.zrg.stop = pickedpos.z;
    HorSampling& hrg = csfromseeds.hrg;
    const BinID bid = SI().transform( pickedpos );
    hrg.start.inl = bid.inl - 10 * hrg.step.inl;
    hrg.stop.inl = bid.inl + 10 * hrg.step.inl;
    hrg.start.crl = bid.crl - 10 * hrg.step.crl;
    hrg.stop.crl = bid.crl + 10 * hrg.step.crl;
    return activetrackerid;
}


int uiMPEPartServer::addTracker( const char* trackertype, const char* name )
{
    MPE::Engine& engine = MPE::engine();
    if ( !engine.highestTrackerID() )
	engine.setActiveVolume( engine.getDefaultActiveVolume() );

    activetrackerid = engine.addTracker( name, trackertype );
    if ( activetrackerid==-1 )
	uiMSG().error( MPE::engine().errMsg() );
    else
    {
	const EM::ObjectID objid =
			    engine.getTracker(activetrackerid)->objectID();
	if ( !engine.getEditor(objid,false) )
	    engine.getEditor(objid,true);

	if ( !sendEvent( evAddTreeObject ) )
	{
	    pErrMsg("Could not add treeitem");
	    engine.removeTracker( activetrackerid );
	    activetrackerid = -1;
	    //TODO? Remove new object?
	    //TODO? Remove new editor?
	}

	EM::EMObject* emobj = EM::EMM().getObject( objid );
	if ( emobj && emobj->isChanged() )
	{
	    PtrMan<Executor> saver = emobj->saver();
	    saver->execute();
	}
    }

    return activetrackerid;
}


MultiID uiMPEPartServer::getTrackerMultiID( int trackerid ) const
{
    const MPE::EMTracker* emt = MPE::engine().getTracker(trackerid);
    if ( emt )
    {
	const EM::EMObject* emo = EM::EMM().getObject(emt->objectID());
	if ( emo ) return emo->multiID();
    }

   return MultiID(-1);
}


bool uiMPEPartServer::canAddSeed( int trackerid ) const
{
    pErrMsg("Not impl");
    return false;
}


void uiMPEPartServer::addSeed( int trackerid )
{
    /*
    const EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
    mDynamicCastGet(EM::Surface*,surface,EM::EMM().getObject(objid))
    if ( !surface ) return false;

    curemid = mid;
    cursectionid = surface->geometry.addSection( "", true );
    setObjectType( mid );
    startWizard( 1 );
    wizard->setSurfaceColor( surface->preferredColor() );

    int trackidx = getTrackerID( curemid );
    if ( trackidx < 0 )
	trackidx = trackman.addTracker( curemid );

    wizard->setTracker( trackman.getTracker(trackidx) );

    return true;
    */
}


bool uiMPEPartServer::isTrackingEnabled( int trackerid ) const
{ return MPE::engine().getTracker(trackerid)->isEnabled(); }


void uiMPEPartServer::enableTracking( int trackerid, bool yn )
{ return MPE::engine().getTracker(trackerid)->enable(yn); }


int uiMPEPartServer::activeTrackerID() const
{ return activetrackerid; }


const Attrib::SelSpec* uiMPEPartServer::getAttribSelSpec() const
{ return eventattrselspec; }



bool uiMPEPartServer::startWizard( const char* typestr, int startidx )
{
    csfromseeds.init( false );

    delete wizard;
    wizard = new MPE::Wizard( appserv().parent(), this );
    wizard->startAt( startidx );
    wizard->setTrackingType( typestr );
    wizard->go();
    return true;
}


void uiMPEPartServer::showSetupDlg( const MultiID& mid, EM::SectionID sid )
{
    EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
    uiDialog dlg( appserv().parent(), uiDialog::Setup("Tracking Setup") );
    dlg.setCtrlStyle( uiDialog::LeaveOnly );
    dlg.setHelpID( "108.0.1" );
    MPE::uiSetupSel* grp = new MPE::uiSetupSel( &dlg, attrset );
    grp->setType( objid, sid );
    if ( dlg.go() )
    {
	loadAttribData();
	sendEvent( evShowToolbar );
    }
}


void uiMPEPartServer::showRelationsDlg( const MultiID& mid, EM::SectionID sid )
{
    EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
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


void uiMPEPartServer::loadAttribData()
{
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


void uiMPEPartServer::updateVolumeFromSeeds()
{
    ObjectSet<Geometry::Element> seeds = MPE::engine().interactionseeds;
    if ( !seeds.size() ) return;

    Geometry::Element* element = MPE::engine().interactionseeds[0];
    IntervalND<float> intv = element->boundingBox(true);
    Coord start( intv.getRange(0).start, intv.getRange(1).start );
    Coord stop( intv.getRange(0).stop, intv.getRange(1).stop );

    CubeSampling elementcs;
    elementcs.hrg.start = SI().transform( start );
    elementcs.hrg.stop = SI().transform( stop );
    assign( elementcs.zrg, intv.getRange(2) );
    elementcs.normalise();
    
    if ( csfromseeds.isEmpty() )
	csfromseeds = elementcs;
    else
	csfromseeds.include( elementcs );

    deepErase( MPE::engine().interactionseeds );
}


void uiMPEPartServer::createActiveVolume()
{
    if ( !MPE::engine().highestTrackerID() ) return;

    CubeSampling activecs = MPE::engine().activeVolume();
    const bool isdefault = activecs == MPE::engine().getDefaultActiveVolume();

    CubeSampling newcube = isdefault ? csfromseeds : activecs;
    if ( !isdefault )
    {
	newcube.hrg.include( csfromseeds.hrg.start );
	newcube.hrg.include( csfromseeds.hrg.stop );
	newcube.zrg.include( csfromseeds.zrg.start );
	newcube.zrg.include( csfromseeds.zrg.stop );
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
    csfromseeds.init( false );

    sendEvent( evShowToolbar );
}


void uiMPEPartServer::setSeed( ObjectSet<Geometry::Element>& newseeds )
{
    deepErase( MPE::engine().interactionseeds );
    MPE::engine().interactionseeds = newseeds;
    newseeds.erase();
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
	if ( !sendEvent(evInitFromSession) )
	    return false;

	loadAttribData();
	sendEvent( evShowToolbar );
    }
    return res;
}
