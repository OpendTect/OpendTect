/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismpeseedcatcher.cc,v 1.37 2009-07-22 16:01:45 cvsbert Exp $";

#include "vismpeseedcatcher.h"

#include "attribdataholder.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfacetr.h"
#include "emhorizon2d.h"
#include "survinfo.h"
#include "visdataman.h"
#include "visemobjdisplay.h"
#include "visevent.h"
#include "vishorizon2ddisplay.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
#include "vismpe.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "visplanedatadisplay.h"

mCreateFactoryEntry( visSurvey::MPEClickCatcher );


namespace visSurvey
{

MPEClickCatcher::MPEClickCatcher()
    : visBase::VisualObjectImpl( false )
    , click( this )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , trackertype_( 0 )
{ }


MPEClickCatcher::~MPEClickCatcher()
{
    setSceneEventCatcher( 0 );
    setDisplayTransformation( 0 );
}


void MPEClickCatcher::setSceneEventCatcher( visBase::EventCatcher* nev )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,MPEClickCatcher,clickCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = nev;

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,MPEClickCatcher,clickCB));
	eventcatcher_->ref();
    }
}


void MPEClickCatcher::setDisplayTransformation( visBase::Transformation* nt )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();
}


visBase::Transformation* MPEClickCatcher::getDisplayTransformation()
{ return transformation_; }


#define mCheckTracker( typestr, typekey, legalclick, condition ) \
    if ( typestr && !strcmp(typestr,EM##typekey##TranslatorGroup::keyword()) ) \
	legalclick = legalclick || (condition); 

#define mCheckPlaneDataDisplay( typ, dataobj, plane, legalclick ) \
    mDynamicCastGet( PlaneDataDisplay*, plane, dataobj ); \
    if ( !plane || !plane->isOn() ) \
	plane = 0; \
    bool legalclick = !plane; \
    mCheckTracker( typ, Horizon3D, legalclick, \
		   plane->getOrientation()!=PlaneDataDisplay::Timeslice ); \
    mCheckTracker( typ, Fault3D, legalclick, true ); \
    mCheckTracker( typ, FaultStickSet, legalclick, true );

#define mCheckMPEDisplay( typ, dataobj, mped, cs, legalclick ) \
    mDynamicCastGet( MPEDisplay*, mped, dataobj ); \
    CubeSampling cs; \
    if ( !mped || !mped->isDraggerShown() || !mped->getPlanePosition(cs) ) \
	mped = 0; \
    bool legalclick = !mped; \
    mCheckTracker( typ, Horizon3D, legalclick, cs.nrZ()!=1 ); 

#define mCheckSeis2DDisplay( typ, dataobj, seis2ddisp, legalclick ) \
    mDynamicCastGet( Seis2DDisplay*, seis2ddisp, dataobj ); \
    if ( !seis2ddisp || !seis2ddisp->isOn() ) \
	seis2ddisp =  0; \
    bool legalclick = !seis2ddisp; \
    mCheckTracker( typ, Horizon2D, legalclick, true ); \
    mCheckTracker( typ, FaultStickSet, legalclick, true );


bool MPEClickCatcher::isClickable( const char* trackertype, int visid )
{
    visBase::DataObject* dataobj = visBase::DM().getObject( visid );
    if ( !dataobj )
	return false;

    mCheckPlaneDataDisplay( trackertype, dataobj, plane, legalclick1 );
    if ( plane && legalclick1 )
	return true;
    
    mCheckMPEDisplay( trackertype, dataobj, mpedisplay, cs, legalclick2 );
    if ( mpedisplay && legalclick2 )
	return true;

    mCheckSeis2DDisplay( trackertype, dataobj, seis2ddisp, legalclick3 );
    if ( seis2ddisp && legalclick3 )
	return true;

    return false;
}


void MPEClickCatcher::setTrackerType( const char* tt )
{ trackertype_ = tt; }


const MPEClickInfo& MPEClickCatcher::info() const
{ return info_; }


MPEClickInfo& MPEClickCatcher::info() 
{ return info_; }


void MPEClickCatcher::clickCB( CallBacker* cb )
{
    if ( eventcatcher_->isHandled() || !isOn() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type!=visBase::MouseClick || !eventinfo.pressed )
	return;

    if ( !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;

    if ( OD::altKeyboardButton(eventinfo.buttonstate_) )
	return;
    
    info().setCtrlClicked( OD::ctrlKeyboardButton(eventinfo.buttonstate_) );
    info().setShiftClicked( OD::shiftKeyboardButton(eventinfo.buttonstate_) );
    info().setAltClicked( OD::altKeyboardButton(eventinfo.buttonstate_) );
    info().setPos( eventinfo.displaypickedpos );

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	if ( !dataobj ) 
	    continue;
	info().setObjID( visid );

	mDynamicCastGet( visSurvey::Horizon2DDisplay*, hor2ddisp, dataobj );
	if ( hor2ddisp )
	{
	    sendUnderlying2DSeis( hor2ddisp, eventinfo );
	    eventcatcher_->setHandled();
	    break;
	}

	mDynamicCastGet( visSurvey::EMObjectDisplay*, emod, dataobj );
	if ( emod )
	{
	    sendUnderlyingPlanes( emod, eventinfo );
	    eventcatcher_->setHandled();
	    break;
	}

	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) !=
	     OD::shiftKeyboardButton(eventinfo.buttonstate_) )
	    continue;
	    
	mDynamicCastGet( visSurvey::RandomTrackDisplay*, rtdisp, dataobj );
	if ( rtdisp )
	{
	    info().setLegalClick( false );
	    click.trigger();
	    eventcatcher_->setHandled();
	    break;
	}

	mCheckPlaneDataDisplay( trackertype_, dataobj, plane, legalclick1 );
	if ( plane )
	{
	    info().setLegalClick( legalclick1 );
	    info().setObjCS( plane->getCubeSampling() );

	    DataPack::ID datapackid = DataPack::cNoID();
	    int attrib = plane->nrAttribs();
	    while ( attrib )
	    {
		attrib--;
		datapackid = plane->getDataPackID( attrib );
		if ( (datapackid > DataPack::cNoID()) && 
		     plane->isAttribEnabled(attrib) )
		    break;
	    }
	    
	    info().setObjDataPackID( datapackid );
	    info().setObjDataSelSpec( plane->getSelSpec(attrib) );
	    click.trigger();
	    eventcatcher_->setHandled();
	    break;
	}

	mCheckMPEDisplay( trackertype_, dataobj, mpedisplay, cs, legalclick2 );
	if ( mpedisplay )
	{
	    info().setLegalClick( legalclick2 );
	    info().setObjCS( cs );
	    click.trigger();
	    eventcatcher_->setHandled();
	    break;
	}

	mCheckSeis2DDisplay( trackertype_, dataobj, seis2ddisp, legalclick3 );
	if ( seis2ddisp )
	{
	    DataPack::ID datapackid = DataPack::cNoID();
	    int attrib = seis2ddisp->nrAttribs();
	    while ( attrib )
	    {
		attrib--;
		datapackid = seis2ddisp->getDataPackID( attrib );
		if ( (datapackid > DataPack::cNoID()) && 
		     seis2ddisp->isAttribEnabled(attrib) ) 
		    break;
	    }

	    info().setLegalClick( legalclick3 );
	    info().setObjDataPackID( datapackid );
	    info().setObjDataSelSpec( seis2ddisp->getSelSpec(attrib) );
	    info().setObjLineSet( seis2ddisp->lineSetID() );
	    info().setObjLineName( seis2ddisp->name() );
	    click.trigger();
	    eventcatcher_->setHandled();
	    break;
	}
    }
    info().clear();
}


void MPEClickCatcher::sendUnderlying2DSeis( 
				    const visSurvey::EMObjectDisplay* emod,
				    const visBase::EventInfo& eventinfo )
{
    const EM::EMObject* emobj = EM::EMM().getObject( emod->getObjectID() );
    if ( !emobj ) 
	return;
    
    const EM::PosID nodepid = emod->getPosAttribPosID( EM::EMObject::sSeedNode(),
						       eventinfo.pickedobjids );
    info().setNode( nodepid );

    mDynamicCastGet( const EM::Horizon2D*, hor2d, emobj );
    const int lineidx = RowCol( nodepid.subID() ).r();
    const int lineid = hor2d->geometry().lineID( lineidx );
    const BufferString linenm = hor2d->geometry().lineName( lineid );
    const MultiID& lineset = hor2d->geometry().lineSet( lineid );

    Seis2DDisplay* seis2dclosest = 0;
    bool legalclickclosest;
    float mindisttoseis2d;

    TypeSet<int> seis2dinscene;
    visBase::DM().getIds( typeid(visSurvey::Seis2DDisplay), seis2dinscene ); 

    for ( int idx=0; idx<seis2dinscene.size(); idx++ )
    {
	visBase::DataObject* dataobj = 
				visBase::DM().getObject( seis2dinscene[idx] );
	if ( !dataobj )
	    continue;

	mCheckSeis2DDisplay( trackertype_, dataobj, seis2ddisp, legalclick );
	if ( !seis2ddisp )
	    continue;
	
	if ( lineid < 0 )
	{
	    Coord3 pos = eventinfo.worldpickedpos;
	    if ( transformation_ )
		pos = transformation_->transform( pos );
	    float disttoseis2d = seis2ddisp->calcDist( pos );
	    
	    if ( !seis2dclosest || disttoseis2d<mindisttoseis2d )
	    {
		mindisttoseis2d = disttoseis2d;
		seis2dclosest = seis2ddisp;
		legalclickclosest = legalclick;
	    }

	    continue;
	}

	if ( lineset==seis2ddisp->lineSetID() && linenm==seis2ddisp->name() )
	{
	    seis2dclosest = seis2ddisp;
	    legalclickclosest = legalclick;
	    break;
	}
    }

    if ( seis2dclosest && mindisttoseis2d<=seis2dclosest->maxDist() )
    {
	DataPack::ID datapackid = DataPack::cNoID();
	int attrib = seis2dclosest->nrAttribs();
	while ( attrib )
	{
	    attrib--;
	    datapackid = seis2dclosest->getDataPackID( attrib );
	    if ( (datapackid > DataPack::cNoID()) && 
		 seis2dclosest->isAttribEnabled(attrib) ) 
		break;
	}

	info().setLegalClick( legalclickclosest );
	info().setObjDataPackID( datapackid );
	info().setObjDataSelSpec( seis2dclosest->getSelSpec(attrib) );
	info().setObjLineSet( seis2dclosest->lineSetID() );
	info().setObjLineName( seis2dclosest->name() );
	click.trigger();
    }
}


void MPEClickCatcher::sendUnderlyingPlanes( 
				    const visSurvey::EMObjectDisplay* emod,
				    const visBase::EventInfo& eventinfo )
{
    const EM::EMObject* emobj = EM::EMM().getObject( emod->getObjectID() );
    if ( !emobj ) 
	return;
    
    const EM::PosID nodepid = emod->getPosAttribPosID( EM::EMObject::sSeedNode(),
						       eventinfo.pickedobjids );
    Coord3 nodepos = emobj->getPos( nodepid );
    info().setNode( nodepid );
    
    if ( !nodepos.isDefined() )
    {
	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) !=
	     OD::shiftKeyboardButton(eventinfo.buttonstate_) )
	    return;
	 nodepos = eventinfo.worldpickedpos;
    }

    const BinID nodebid = SI().transform( nodepos );

    TypeSet<int> mpedisplays;
    visBase::DM().getIds( typeid(visSurvey::MPEDisplay), mpedisplays ); 
    
    CubeSampling trkplanecs(false);
    for ( int idx=0; idx<mpedisplays.size(); idx++ )
    {
	visBase::DataObject* dataobj = 
				visBase::DM().getObject( mpedisplays[idx] );
	if ( !dataobj )
	    continue;

	mCheckMPEDisplay( trackertype_, dataobj, mpedisplay, cs, legalclick );
	if ( mpedisplay && cs.hrg.includes(nodebid) && 
	     cs.zrg.includes(nodepos.z) )
	{
	    info().setLegalClick( legalclick );
	    info().setObjID( mpedisplay->id() );
	    info().setObjCS( cs );
	    click.trigger();
	    trkplanecs = cs;
	    break;
	}
    }

    TypeSet<int> planesinscene;
    visBase::DM().getIds( typeid(visSurvey::PlaneDataDisplay), planesinscene ); 
    
    for ( int idx=0; idx<planesinscene.size(); idx++ )
    {
	visBase::DataObject* dataobj = 
				visBase::DM().getObject( planesinscene[idx] );
	if ( !dataobj )
	    continue;

	mCheckPlaneDataDisplay( trackertype_, dataobj, plane, legalclick );
	if ( !plane )
	    continue;

	const CubeSampling cs = plane->getCubeSampling();
	if ( !trkplanecs.isEmpty() && trkplanecs.defaultDir()==cs.defaultDir() )
	    continue;

	if ( cs.hrg.includes(nodebid) && cs.zrg.includes(nodepos.z) )
	{
	    info().setLegalClick( legalclick );
	    info().setObjID( plane->id() );
	    info().setObjCS( cs );
	    
	    DataPack::ID datapackid = DataPack::cNoID();
	    int attrib = plane->nrAttribs();
	    while ( attrib )
	    {
		attrib--;
		datapackid = plane->getDataPackID( attrib );
		if ( (datapackid > DataPack::cNoID()) && 
		     plane->isAttribEnabled(attrib) )
		    break;
	    }
	    
	    info().setObjDataPackID( datapackid );
	    info().setObjDataSelSpec( plane->getSelSpec(attrib) );
	    click.trigger();
	}
    }
}


MPEClickInfo::MPEClickInfo()
{ clear(); }


bool MPEClickInfo::isLegalClick() const
{ return legalclick_; }


bool MPEClickInfo::isCtrlClicked() const
{ return ctrlclicked_; }


bool MPEClickInfo::isShiftClicked() const
{ return shiftclicked_; }


bool MPEClickInfo::isAltClicked() const
{ return altclicked_; }


const EM::PosID& MPEClickInfo::getNode() const
{ return clickednode_; }


const Coord3& MPEClickInfo::getPos() const
{ return clickedpos_; }


int MPEClickInfo::getObjID() const
{ return clickedobjid_; }


const CubeSampling& MPEClickInfo::getObjCS() const
{ return clickedcs_; }


DataPack::ID MPEClickInfo::getObjDataPackID() const
{ return datapackid_; }


const Attrib::DataCubes* MPEClickInfo::getObjData() const
{ return attrdata_; }


const Attrib::SelSpec* MPEClickInfo::getObjDataSelSpec() const
{ return attrsel_; }


const MultiID& MPEClickInfo::getObjLineSet() const
{ return lineset_; }


const char* MPEClickInfo::getObjLineName() const
{ return linename_[0] ? (const char*) linename_ : 0; }


const Attrib::Data2DHolder* MPEClickInfo::getObjLineData() const
{ return linedata_; }


void MPEClickInfo::clear()
{
    legalclick_ = false; 
    ctrlclicked_ = false;
    shiftclicked_ = false;
    altclicked_ = false;
    clickednode_ = EM::PosID( -1, -1, -1 );
    clickedpos_ = Coord3::udf();
    clickedobjid_ = -1;
    clickedcs_.init( false);
    attrsel_ = 0;
    attrdata_ = 0;
    linedata_ = 0;
    lineset_ = MultiID( -1 );
    linename_ = "";
}


void MPEClickInfo::setLegalClick( bool yn )
{ legalclick_ = yn; }


void MPEClickInfo::setCtrlClicked( bool yn )
{ ctrlclicked_ = yn; }


void MPEClickInfo::setShiftClicked( bool yn )
{ shiftclicked_ = yn; }


void MPEClickInfo::setAltClicked( bool yn )
{ altclicked_ = yn; }


void MPEClickInfo::setNode( const EM::PosID& pid )
{ clickednode_ = pid; }


void MPEClickInfo::setPos(const Coord3& pos )
{ clickedpos_ = pos; }


void MPEClickInfo::setObjID( int visid )
{ clickedobjid_ = visid; }


void MPEClickInfo::setObjCS( const CubeSampling& cs )
{ clickedcs_ = cs; }


void MPEClickInfo::setObjDataPackID( DataPack::ID datapackid )
{ datapackid_ = datapackid; }


void MPEClickInfo::setObjData( const Attrib::DataCubes* ad )
{ attrdata_ = ad; }


void MPEClickInfo::setObjDataSelSpec( const Attrib::SelSpec* as )
{ attrsel_ = as; }


void MPEClickInfo::setObjLineSet( const MultiID& mid )
{ lineset_ = mid; }


void MPEClickInfo::setObjLineName( const char* str )
{ linename_ = str; }


void MPEClickInfo::setObjLineData( const Attrib::Data2DHolder* ad2dh )
{ linedata_ = ad2dh; }



}; //namespce
