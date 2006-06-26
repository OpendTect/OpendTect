/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2004
 RCS:           $Id: uimpepartserv.cc,v 1.48 2006-06-26 07:57:50 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uimpepartserv.h"

#include "attribdataholder.h"
#include "attribdatacubes.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "emmanager.h"
#include "emtracker.h"
#include "emobject.h"
#include "executor.h"
#include "filegen.h"
#include "geomelement.h"
#include "iopar.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "sectiontracker.h"
#include "sectionadjuster.h"
#include "uicursor.h"
#include "uiexecutor.h"
#include "uihorizontracksetup.h"
#include "uimpewizard.h"
#include "uimsg.h"
#include "uisurfacerelationdlg.h"

const int uiMPEPartServer::evGetAttribData	= 0;
const int uiMPEPartServer::evStartSeedPick	= 1;
const int uiMPEPartServer::evEndSeedPick	= 2;
const int uiMPEPartServer::evAddTreeObject	= 3;
const int uiMPEPartServer::evShowToolbar	= 4;
const int uiMPEPartServer::evInitFromSession	= 5;
const int uiMPEPartServer::evRemoveTreeObject	= 6;
const int uiMPEPartServer::evWizardClosed	= 7;
const int uiMPEPartServer::evCreate2DSelSpec	= 8;


uiMPEPartServer::uiMPEPartServer( uiApplService& a, const Attrib::DescSet* ads )
    : uiApplPartServer(a)
    , attrset_( ads )
    , wizard_( 0 )
    , activetrackerid_(-1)
    , eventattrselspec_( 0 )
    , blockdataloading_( false )
    , postponedcs_( false )
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
    delete wizard_;
}


void uiMPEPartServer::setCurrentAttribDescSet( const Attrib::DescSet* ads )
{ attrset_ = ads; }


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
    if ( res==-1 )
    {
	uiMSG().error("Could not create tracker for this object");
	return -1;
    }

    blockDataLoading( true );
    readSetup( emobj->multiID() );

    if ( pickedpos.isDefined() ) 
    {
	CubeSampling poscs(false);
	const BinID bid = SI().transform(pickedpos);
	poscs.hrg.start = poscs.hrg.stop = bid;
	poscs.zrg.start = poscs.zrg.stop = pickedpos.z;
	expandActiveVolume( poscs );
    }

    blockDataLoading( false );
    postponeLoadingCurVol();

    return res;
}


bool uiMPEPartServer::addTracker( const char* trackertype )
{
    if ( !wizard_ ) wizard_ = new MPE::Wizard( appserv().parent(), this );
    else wizard_->reset();

    wizard_->setTrackingType( trackertype );
//  wizard_->setRotateMode(true);
    wizard_->go();

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
    if ( !wizard_ ) wizard_ = new MPE::Wizard( appserv().parent(), this );
    else wizard_->reset();

    const MPE::EMTracker* tracker = MPE::engine().getTracker(trackerid);
    if ( !tracker ) return;

    EM::EMObject* object = EM::EMM().getObject( tracker->objectID() );
    if ( !object ) return;

    //TODO Find a mechanism to get this work on multi-section objects
    wizard_->setObject( object->id(), object->sectionID(0) );

    wizard_->displayPage(MPE::Wizard::sNamePage, false );
    wizard_->displayPage(MPE::Wizard::sFinalizePage, false );
    wizard_->setRotateMode( false );
    
    wizard_->go();
}


bool uiMPEPartServer::isTrackingEnabled( int trackerid ) const
{ return MPE::engine().getTracker(trackerid)->isEnabled(); }


void uiMPEPartServer::enableTracking( int trackerid, bool yn )
{ return MPE::engine().getTracker(trackerid)->enable(yn); }


int uiMPEPartServer::activeTrackerID() const
{ return activetrackerid_; }


const Attrib::SelSpec* uiMPEPartServer::getAttribSelSpec() const
{ return eventattrselspec_; }



bool uiMPEPartServer::showSetupDlg( const EM::ObjectID& emid,
				    const EM::SectionID& sid,
				    bool showcancelbutton )
{
    const int trackerid = getTrackerID( emid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    MPE::SectionTracker* sectracker = tracker->getSectionTracker( sid, true );
    if ( !sectracker ) return false;

    uiDialog dlg( appserv().parent(), uiDialog::Setup("Tracking Setup") );
    if ( !showcancelbutton ) 
	dlg.setCtrlStyle( uiDialog::LeaveOnly );
    dlg.setHelpID( "108.0.1" );


    EM::EMObject* emobj = EM::EMM().getObject( emid );
    MPE::uiSetupGroup* grp = 
	MPE::uiMPE().setupgrpfact.create( &dlg, emobj->getTypeStr(), attrset_ );
    grp->setSectionTracker( sectracker );
   
    do
    {
	if ( !dlg.go() && showcancelbutton )
	    return false;
    }
    while ( !grp->commitToTracker() );

    tracker->applySetupAsDefault( sid );
    saveSetup( EM::EMM().getMultiID(emid) );
    loadAttribData();
    return true;
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
				     const Attrib::DataCubes* slcset )
{
    MPE::engine().setAttribData( spec, slcset );
}


void uiMPEPartServer::setAttribData( const Attrib::SelSpec& as,
				     const Attrib::Data2DHolder* newdata )
{
    RefMan<const Attrib::Data2DHolder> reffer = newdata;

    if ( !newdata )
    {
	MPE::engine().setAttribData( as, 0 );
	return;
    }
	
    RefMan<Attrib::DataCubes> dc = new Attrib::DataCubes;

    if ( newdata->fillDataCube(*dc) )
	MPE::engine().setAttribData( as, dc );
}


const MultiID& uiMPEPartServer::get2DLineSet() const { return linesetid_; }


const char* uiMPEPartServer::get2DLineName() const
{ return linename_.size() ? (const char*) linename_ : 0; }


const char* uiMPEPartServer::get2DAttribName() const
{ return attribname_.size() ? (const char*) attribname_ : 0; }


void uiMPEPartServer::set2DSelSpec(const Attrib::SelSpec& as)
{ lineselspec_ = as; }


void uiMPEPartServer::blockDataLoading( bool yn )
{
    blockdataloading_ = yn;
}


void uiMPEPartServer::postponeLoadingCurVol()
{
    postponedcs_ = MPE::engine().activeVolume();
}


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


void uiMPEPartServer::activeVolumeChange(CallBacker*)
{ loadAttribData(); }


void uiMPEPartServer::loadAttribData()
{
    if ( blockdataloading_ || postponedcs_ == MPE::engine().activeVolume() )
	return;

    uiCursorChanger changer( uiCursor::Wait );

    ObjectSet<const Attrib::SelSpec> attribselspecs;
    MPE::engine().getNeededAttribs(attribselspecs);
    if ( attribselspecs.size() == 0 ) return;

    for ( int idx=0; idx<attribselspecs.size(); idx++ )
    {
	eventattrselspec_ = attribselspecs[idx];
	sendEvent( evGetAttribData );
    }
}


const Attrib::DataCubes*
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
}


void uiMPEPartServer::loadEMObjectCB(CallBacker*)
{
    PtrMan<Executor> exec = EM::EMM().objectLoader( MPE::engine().midtoload );
    if ( !exec ) return;

    uiExecutor uiexec( appserv().parent(), *exec );
    uiexec.go();
}


bool uiMPEPartServer::saveSetup( const MultiID& mid )
{
    const EM::ObjectID emid = EM::EMM().getObjectID( mid );
    const int trackerid = getTrackerID( emid );
    if ( trackerid<0 ) return false;

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    IOPar iopar;
    tracker->fillPar( iopar );
    IOPar attrpar;
    attrset_->fillPar( attrpar );
    iopar.mergeComp( attrpar, "Attribs" );

    BufferString setupfilenm = MPE::engine().setupFileName( mid );
    iopar.write( setupfilenm, "Tracking Setup" );
    return true;
}


bool uiMPEPartServer::readSetup( const MultiID& mid ) 
{
    BufferString setupfilenm = MPE::engine().setupFileName( mid );
    if ( !File_exists(setupfilenm) ) return false;

    const EM::ObjectID emid = EM::EMM().getObjectID( mid );
    const int trackerid = getTrackerID( emid );
    if ( trackerid<0 ) return false;

    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    IOPar iopar;
    iopar.read( setupfilenm, "Tracking Setup", true );
    tracker->usePar( iopar );
    PtrMan<IOPar> attrpar = iopar.subselect( "Attribs" );
    if ( !attrpar ) return true;

    Attrib::DescSet newads;
    newads.usePar( *attrpar );
    mergeAttribSets( newads, *tracker ); 
    return true;
}


void uiMPEPartServer::mergeAttribSets( const Attrib::DescSet& newads,
				       MPE::EMTracker& tracker )
{
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
	    if ( !as || as->id()<0 ) continue;
	    Attrib::DescID newid( -1, true );
	    const Attrib::Desc* usedad = newads.getDesc( as->id() );
	    for ( int ida=0; ida<attrset_->nrDescs(); ida++ )
	    {
		const Attrib::DescID descid = attrset_->getID( ida );
		const Attrib::Desc* ad = attrset_->getDesc( descid );
		if ( !ad ) continue;

		if ( usedad->isIdenticalTo( *ad ) )
		{
		    newid = ad->id();
		    break;
		}
	    }

	    if ( newid < 0 )
	    {
		Attrib::DescSet* set = 
		    const_cast<Attrib::DescSet*>(attrset_);
		Attrib::Desc* newdesc = usedad->clone();
		newdesc->setDescSet( set );
		newid = set->addDesc( newdesc );
	    }

	    Attrib::SelSpec newas( *as );
	    newas.setIDFromRef( *attrset_ );
	    st->adjuster()->setAttributeSel( asidx, newas );
	}
    }
}


void uiMPEPartServer::fillPar( IOPar& par ) const
{
    MPE::engine().fillPar( par );
}


bool uiMPEPartServer::usePar( const IOPar& par )
{
    delete wizard_; wizard_ = 0;
    bool res = MPE::engine().usePar( par );
    if ( res )
    {
	if ( !sendEvent(evInitFromSession) )
	    return false;

	loadAttribData();
    }
    return res;
}
