/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vismpeseedcatcher.h"

#include "attribdataholder.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfacetr.h"
#include "emhorizon2d.h"
#include "survinfo.h"
#include "visdataman.h"
#include "visemobjdisplay.h"
#include "visevent.h"
#include "vishorizon2ddisplay.h"
#include "vismpeeditor.h"
#include "visrandomtrackdisplay.h"
#include "visrgbatexturechannel2rgba.h"
#include "vistexturechannels.h"
#include "visseis2ddisplay.h"
#include "visselman.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "visplanedatadisplay.h"

mCreateFactoryEntry( visSurvey::MPEClickCatcher )


namespace visSurvey
{

MPEClickCatcher::MPEClickCatcher()
    : visBase::VisualObjectImpl( false )
    , click( this )
    , endSowing( this )
    , sowing(this)
{
}


MPEClickCatcher::~MPEClickCatcher()
{
    setSceneEventCatcher( nullptr );
    setDisplayTransformation( nullptr );
    setEditor( nullptr );
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


void MPEClickCatcher::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();
}


const mVisTrans* MPEClickCatcher::getDisplayTransformation() const
{ return transformation_; }


#define mCheckTracker( typestr, typekey, legalclick, condition ) \
    if ( EM##typekey##TranslatorGroup::sGroupName()==typestr ) \
    { \
	const bool condval = (condition); \
	legalclick = legalclick || condval; \
    }


#define mCheckPlaneDataDisplay( typ, dataobj, plane, legalclick ) \
    mDynamicCastGet( PlaneDataDisplay*, plane, dataobj ); \
    if ( !plane || !plane->isOn() ) \
	plane = 0; \
    bool legalclick = !plane; \
    mCheckTracker( typ, Horizon3D, legalclick, \
		   plane !=0 ? plane->getOrientation()!=OD::ZSlice : false ); \
    mCheckTracker( typ, Fault3D, legalclick, true ); \
    mCheckTracker( typ, FaultStickSet, legalclick, true );

#define mCheckSeis2DDisplay( typ, dataobj, seis2ddisp, legalclick ) \
    mDynamicCastGet( Seis2DDisplay*, seis2ddisp, dataobj ); \
    if ( !seis2ddisp || !seis2ddisp->isOn() ) \
	seis2ddisp =  0; \
    bool legalclick = !seis2ddisp; \
    mCheckTracker( typ, Horizon2D, legalclick, true ); \
    mCheckTracker( typ, FaultStickSet, legalclick, true );

#define mCheckRdlDisplay( typ, dataobj, rdldisp, legalclick ) \
    mDynamicCastGet(RandomTrackDisplay*,rdldisp,dataobj); \
    if ( !rdldisp || !rdldisp->isOn() ) \
	rdldisp = 0; \
    bool legalclick = !rdldisp; \
    mCheckTracker( typ, Horizon3D, legalclick, true );


bool MPEClickCatcher::isClickable( const char* trackertype, VisID visid )
{
    visBase::DataObject* dataobj = visBase::DM().getObject( visid );
    if ( !dataobj )
	return false;

    mCheckPlaneDataDisplay( trackertype, dataobj, plane, legalclick1 );
    if ( plane && legalclick1 )
	return true;

    mCheckSeis2DDisplay( trackertype, dataobj, seis2ddisp, legalclick3 );
    if ( seis2ddisp && legalclick3 )
	return true;

    mCheckRdlDisplay( trackertype, dataobj, rdldisp, legalclick4 );
    if ( rdldisp && legalclick4 )
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
    if ( eventcatcher_->isHandled() || !isOn() || !editor_ )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    if ( editor_->sower().accept(eventinfo) )
	return;

    if ( eventinfo.type!=visBase::MouseDoubleClick )
    {
	if ( info().getPickedNode().isUdf() &&
	     (eventinfo.type!=visBase::MouseClick || !eventinfo.pressed) )
	    return;

	if ( !info().getPickedNode().isUdf() &&
	     OD::leftMouseButton(eventinfo.buttonstate_) )
	{
	    //mouse released/end seed drag
	    info().setPickedNode( TrcKey::udf() );
	    return;
	}

	if ( info().getPickedNode().isUdf() &&
	     !OD::leftMouseButton(eventinfo.buttonstate_) )
	    return;

	if ( OD::altKeyboardButton(eventinfo.buttonstate_) )
	    return;
    }

    info().setCtrlClicked( OD::ctrlKeyboardButton(eventinfo.buttonstate_) );
    info().setShiftClicked( OD::shiftKeyboardButton(eventinfo.buttonstate_) );
    info().setAltClicked( OD::altKeyboardButton(eventinfo.buttonstate_) );
    const bool doubleclick = eventinfo.type==visBase::MouseDoubleClick
						&& !eventinfo.pressed;
    info().setDoubleClicked( doubleclick );
    info().setPos( eventinfo.displaypickedpos );

    cureventinfo_ = &eventinfo;

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const VisID visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	if ( !dataobj )
	    continue;

	info().setObjID( visid );

	mDynamicCastGet( visSurvey::Horizon2DDisplay*, hor2ddisp, dataobj );
	if ( hor2ddisp )
	{
	    info().setEMObjID( hor2ddisp->getObjectID() );
	    info().setEMVisID( hor2ddisp->id() );
	    sendUnderlying2DSeis( hor2ddisp, eventinfo );
	    eventcatcher_->setHandled();
	    click.trigger();
	    break;
	}

	mDynamicCastGet( visSurvey::EMObjectDisplay*, emod, dataobj );
	if ( emod )
	{
	    info().setEMObjID( emod->getObjectID() );
	    info().setEMVisID( emod->id() );
	    sendUnderlyingPlanes( emod, eventinfo );
	    click.trigger();
	    eventcatcher_->setHandled();
	    break;
	}

	mCheckRdlDisplay( trackertype_, dataobj, rtd, legalclick0 )
	if ( rtd )
	{
	    info().setLegalClick( legalclick0 );

	    const int attrib =
			handleAttribute( *rtd, eventinfo.worldpickedpos );
	    if ( attrib >= 0 )
		info().setObjCS( rtd->getTrcKeyZSampling(attrib) );

	    info().setObjTKPath( rtd->getTrcKeyPath() );
	    info().setObjRandomLineID( rtd->getRandomLineID() );

	    click.trigger();
	    eventcatcher_->setHandled();
	    break;
	}

	mCheckPlaneDataDisplay( trackertype_, dataobj, pdd, legalclick1 );
	if ( pdd )
	{
	    info().setLegalClick( legalclick1 );

	    const int attrib =
			handleAttribute( *pdd, eventinfo.worldpickedpos );
	    if ( attrib >= 0 )
		info().setObjCS( pdd->getDataPackSampling(attrib) );

	    allowPickBasedReselection();
	    click.trigger();
	    eventcatcher_->setHandled();
	    break;
	}

	mCheckSeis2DDisplay( trackertype_, dataobj, seis2ddisp, legalclick3 );
	if ( seis2ddisp )
	{
	    handleObjectOnSeis2DDisplay( seis2ddisp, eventinfo.worldpickedpos );
	    info().setLegalClick( legalclick3 );
	    click.trigger();
	    eventcatcher_->setHandled();
	    break;
	}
    }
    info().clear();
}


void MPEClickCatcher::handleObjectOnSeis2DDisplay( Seis2DDisplay* seis2ddisp,
    const Coord3 worldpickedpos )
{
    handleAttribute( *seis2ddisp, worldpickedpos );
    info().setGeomID( seis2ddisp->getGeomID() );
    info().setObjLineName( seis2ddisp->name() );

    const TrcKey tk( seis2ddisp->getGeomID(),
	seis2ddisp->getNearestTraceNr(worldpickedpos) );
    info().setNode( tk );
}


#define mTranspCutOff 198

int MPEClickCatcher::handleAttribute( const MultiTextureSurveyObject& survobj,
				      const Coord3& pos )
{
    const visBase::TextureChannels* channels = survobj.getChannels();
    mDynamicCastGet(const visBase::ColTabTextureChannel2RGBA*,ctab,
		    survobj.SurveyObject::getChannels2RGBA())
    if ( !channels || !ctab )
	return -1;

    int attrib = -1;
    for ( int attridx=survobj.nrAttribs()-1; attridx>=0; attridx-- )
    {
	if ( !survobj.isAttribEnabled(attridx) ||
		ctab->getTransparency(attridx)>mTranspCutOff )
	    continue;

	const int version = survobj.selectedTexture( attridx );
	const Attrib::SelSpec* as = survobj.getSelSpec( attridx, version );
	if ( !as )
	    continue;

	float val = mUdf(float);
	if ( !survobj.getCacheValue(attridx,version,pos,val) )
	    continue;

	if ( !Math::IsNormalNumber(val) )
	    val = mUdf(float);

	bool islowest = true;
	for ( int loweridx=attridx-1; loweridx>=0; loweridx-- )
	{
	    if ( !survobj.hasCache(loweridx) ||
		    !survobj.isAttribEnabled(loweridx) ||
		    ctab->getTransparency(loweridx)>mTranspCutOff )
		continue;

	    islowest = false;
	    break;
	}

	if ( !islowest )
	{
	    const ColTab::Sequence* seq = survobj.getColTabSequence( attridx );
	    const ColTab::Mapper& map =
			channels->getColTabMapper( attridx, version );
	    const OD::Color col = mIsUdf(val) ? seq->undefColor()
					   : seq->color( map.position(val) );
	    if ( col.t()>mTranspCutOff )
		continue;
	}

	attrib = attridx;
	info().setObjDataSelSpec( *as );
	break;
    }

    if ( attrib < 0 )
	return -1;

    const DataPackID datapackid = survobj.getDataPackID( attrib );
    info().setObjDataPackID( datapackid );
    return attrib;
}


bool MPEClickCatcher::forceAttribute( const Attrib::SelSpec& as )
{
    const VisID clickedobjectid = info().getObjID();
    mDynamicCastGet(const visSurvey::MultiTextureSurveyObject*,survobj,
		    visBase::DM().getObject(clickedobjectid))
    if ( !survobj )
	return false;

    int attrib, version;
    if ( !survobj->hasSelSpec(as,attrib,version) )
	return false;

    info().setObjDataSelSpec( as );
    const DataPackID datapackid = survobj->getDataPackID( attrib );
    info().setObjDataPackID( datapackid );
    return true;
}


void MPEClickCatcher::sendUnderlying2DSeis(
				const visSurvey::EMObjectDisplay* emod,
				const visBase::EventInfo& eventinfo )
{
    const EM::EMObject* emobj = EM::EMM().getObject( emod->getObjectID() );
    mDynamicCastGet(const EM::Horizon2D*,hor2d,emobj)
    if ( !hor2d ) return;

    const Coord3 clickedpos = eventinfo.displaypickedpos;

    EM::PosID nodepid = emod->getPosAttribPosID(EM::EMObject::sSeedNode(),
					eventinfo.pickedobjids,clickedpos );
    const int lineidx = nodepid.getRowCol().row();
    const Pos::GeomID geomid = hor2d->geometry().geomID( lineidx );
    const TrcKey tk( geomid, nodepid.getRowCol().col() );
    info().setNode( sequentSowing() ? TrcKey::udf() : tk );

    Seis2DDisplay* seis2dclosest = 0;
    bool legalclickclosest = false;
    float mindisttoseis2d = mUdf(float);

    TypeSet<VisID> seis2dinscene;
    visBase::DM().getIDs( typeid(visSurvey::Seis2DDisplay), seis2dinscene );

    for ( int idx=0; idx<seis2dinscene.size(); idx++ )
    {
	visBase::DataObject* dataobj =
				visBase::DM().getObject( seis2dinscene[idx] );
	if ( !dataobj )
	    continue;

	mCheckSeis2DDisplay( trackertype_, dataobj, seis2ddisp, legalclick );
	if ( !seis2ddisp )
	    continue;

	if ( geomid == Survey::GeometryManager::cUndefGeomID() )
	{
	    Coord3 pos = eventinfo.worldpickedpos;
	    if ( transformation_ )
		transformation_->transform( pos );
	    float disttoseis2d = seis2ddisp->calcDist( pos );

	    if ( !seis2dclosest || disttoseis2d<mindisttoseis2d )
	    {
		mindisttoseis2d = disttoseis2d;
		seis2dclosest = seis2ddisp;
		legalclickclosest = legalclick;
	    }

	    continue;
	}

	if ( geomid == seis2ddisp->getGeomID() )
	{
	    mindisttoseis2d = 0;
	    seis2dclosest = seis2ddisp;
	    legalclickclosest = legalclick;
	    break;
	}
    }

    const Scene* scene = seis2dclosest ? seis2dclosest->getScene() : 0;
    const double zscale = scene ?
	scene->getZScale()*scene->getFixedZStretch() : 0.0;
    const Coord3 onesteptranslation = SI().oneStepTranslation( Coord3(0,0,1) );
    const double onestepdist = Coord3( 1, 1, zscale ).dot( onesteptranslation );

    if ( seis2dclosest && mindisttoseis2d<=0.5*onestepdist )
    {
	handleObjectOnSeis2DDisplay( seis2dclosest, eventinfo.worldpickedpos );
	info().setLegalClick( legalclickclosest );
	click.trigger();
	eventcatcher_->setHandled();
    }
    else
    {
	const bool validgeomid =
		geomid != Survey::GeometryManager::cUndefGeomID();
	info().setLegalClick( validgeomid );
    }
}


void MPEClickCatcher::sendUnderlyingPlanes(
				const visSurvey::EMObjectDisplay* emod,
				const visBase::EventInfo& eventinfo )
{
    const EM::EMObject* emobj = EM::EMM().getObject( emod->getObjectID() );
    if ( !emobj )
	return;

    const Coord3 clickedpos = eventinfo.displaypickedpos;

    const EM::PosID nodepid = emod->getPosAttribPosID(EM::EMObject::sSeedNode(),
					    eventinfo.pickedobjids,clickedpos);
    Coord3 nodepos = emobj->getPos( nodepid );
    if ( !nodepos.isDefined() )
	 nodepos = eventinfo.worldpickedpos;

    const BinID nodebid = SI().transform( nodepos );
    info().setNode( sequentSowing() ? TrcKey::udf() : TrcKey(nodebid) );

    TypeSet<VisID> planesinscene;
    visBase::DM().getIDs( typeid(visSurvey::PlaneDataDisplay), planesinscene );

    for ( int idx=0; idx<planesinscene.size(); idx++ )
    {
	visBase::DataObject* dataobj =
				visBase::DM().getObject( planesinscene[idx] );
	if ( !dataobj )
	    continue;

	mCheckPlaneDataDisplay( trackertype_, dataobj, pdd, legalclick );
	if ( !pdd )
	    continue;

	const TrcKeyZSampling cs = pdd->getTrcKeyZSampling();

	if ( cs.hsamp_.includes(nodebid) && cs.zsamp_.includes(nodepos.z,false))
	{
	    info().setLegalClick( legalclick );
	    info().setObjID( pdd->id() );
	    info().setObjCS( cs );

	    DataPackID datapackid = DataPack::cNoID();
	    int attrib = pdd->nrAttribs();
	    while ( attrib )
	    {
		attrib--;
		unsigned char transpar = pdd->getAttribTransparency( attrib );
		datapackid = pdd->getDataPackID( attrib );
		if ( (datapackid.isValid() && datapackid!=DataPack::cNoID()) &&
		     pdd->isAttribEnabled(attrib) && (transpar<198) )
		    break;
	    }

	    info().setObjDataPackID( datapackid );
	    info().setObjCS( pdd->getDataPackSampling() );
	    info().setObjDataSelSpec( *pdd->getSelSpec(attrib) );
	    allowPickBasedReselection();
	}
    }

    TypeSet<VisID> rtdids;
    visBase::DM().getIDs( typeid(visSurvey::RandomTrackDisplay), rtdids );
    for ( int idx=0; idx<rtdids.size(); idx++ )
    {
	visBase::DataObject* dataobj = visBase::DM().getObject( rtdids[idx] );
	if ( !dataobj ) continue;

	mCheckRdlDisplay( trackertype_, dataobj, rtd, legalclick );
	if ( !rtd ) continue;

	info().setLegalClick( legalclick );
	info().setObjID( rtd->id() );

	DataPackID datapackid = DataPack::cNoID();
	int attrib = rtd->nrAttribs();
	while ( attrib )
	{
	    attrib--;
	    unsigned char transpar = rtd->getAttribTransparency( attrib );
	    datapackid = rtd->getDataPackID( attrib );
	    if ( (datapackid.isValid() && datapackid!=DataPack::cNoID()) &&
		 rtd->isAttribEnabled(attrib) && (transpar<198) )
		break;
	}

	info().setObjCS( rtd->getTrcKeyZSampling(attrib) );
	info().setObjTKPath( rtd->getTrcKeyPath() );
	info().setObjRandomLineID( rtd->getRandomLineID() );
	info().setObjDataPackID( datapackid );
	info().setObjDataSelSpec( *rtd->getSelSpec(attrib) );
    }
}


void MPEClickCatcher::setEditor( MPEEditor* mpeeditor )
{
    if ( editor_ )
    {
	editor_->unRef();
	editor_ = 0;
    }
    editor_ = mpeeditor;
    if ( editor_ )
    {
	mAttachCB(editor_->sower().sowing, MPEClickCatcher::sowingCB);
	mAttachCB( editor_->sower().sowingend, MPEClickCatcher::sowingEnd );
	editor_->ref();
    }
}


bool MPEClickCatcher::activateSower( const OD::Color& color,
				     const TrcKeySampling* workrange )
{
    if ( editor_ && cureventinfo_ )
    {
	return editor_->sower().activate( color, *cureventinfo_,
					  info_.getObjID(), workrange );
    }
    return false;
}


void MPEClickCatcher::sowingCB( CallBacker* )
{
    sowing.trigger();
}


void MPEClickCatcher::sowingEnd( CallBacker* )
{
    endSowing.trigger();
}


bool MPEClickCatcher::sequentSowing() const
{ return editor_ && editor_->sower().mode()==Sower::SequentSowing; }


bool MPEClickCatcher::moreToSow() const
{ return editor_ && editor_->sower().moreToSow(); }


void MPEClickCatcher::stopSowing()
{
    if ( editor_ )
	editor_->sower().stopSowing();
}


void MPEClickCatcher::allowPickBasedReselection()
{
    if ( editor_ && editor_->sower().mode()==Sower::Idle )
	visBase::DM().selMan().reselnotifier.trigger( info_.getObjID() );
}


// MPEClickInfo

MPEClickInfo::MPEClickInfo()
    : pickednode_(TrcKey::udf())
{
    clear();
}


MPEClickInfo::~MPEClickInfo()
{
}


bool MPEClickInfo::isLegalClick() const
{ return legalclick_; }


bool MPEClickInfo::isCtrlClicked() const
{ return ctrlclicked_; }


bool MPEClickInfo::isShiftClicked() const
{ return shiftclicked_; }


bool MPEClickInfo::isAltClicked() const
{ return altclicked_; }


bool MPEClickInfo::isDoubleClicked() const
{ return doubleclicked_; }


const TrcKey& MPEClickInfo::getPickedNode() const
{ return pickednode_; }


const TrcKey& MPEClickInfo::getNode() const
{ return clickednode_; }


const Coord3& MPEClickInfo::getPos() const
{ return clickedpos_; }


VisID MPEClickInfo::getObjID() const
{ return clickedobjid_; }


EM::ObjectID MPEClickInfo::getEMObjID() const
{ return clickedemobjid_; }


const TrcKeyZSampling& MPEClickInfo::getObjCS() const
{ return clickedcs_; }


DataPackID MPEClickInfo::getObjDataPackID() const
{ return datapackid_; }


const RegularSeisDataPack* MPEClickInfo::getObjData() const
{ return attrdata_; }


const Attrib::SelSpec* MPEClickInfo::getObjDataSelSpec() const
{
   if ( attrsel_.id().asInt() == Attrib::SelSpec::cAttribNotSel().asInt() )
       return 0;

   return &attrsel_;
}


Pos::GeomID MPEClickInfo::getGeomID() const
{ return geomid_; }


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
    clickednode_.setUdf();
    clickedpos_ = Coord3::udf();
    clickedobjid_.setUdf();
    clickedemobjid_.setUdf();
    clickedcs_.init( false);
    attrsel_ = nullptr;
    attrdata_ = nullptr;
    linedata_ = nullptr;
    linename_ = "";
    geomid_ = Survey::GM().cUndefGeomID();
    doubleclicked_ = false;
    rdltkpath_ = 0;
    rdlid_.setUdf();
    emvisids_.setUdf();
}


void MPEClickInfo::setLegalClick( bool yn )
{ legalclick_ = yn; }


void MPEClickInfo::setCtrlClicked( bool yn )
{ ctrlclicked_ = yn; }


void MPEClickInfo::setShiftClicked( bool yn )
{ shiftclicked_ = yn; }


void MPEClickInfo::setAltClicked( bool yn )
{ altclicked_ = yn; }


void MPEClickInfo::setDoubleClicked(bool yn)
{ doubleclicked_ = yn; }


void MPEClickInfo::setPickedNode( const TrcKey& pid )
{ pickednode_ = pid; }


void MPEClickInfo::setNode( const TrcKey& pid )
{ clickednode_ = pid; }


void MPEClickInfo::setPos(const Coord3& pos )
{ clickedpos_ = pos; }


void MPEClickInfo::setObjID( VisID visid )
{ clickedobjid_ = visid; }


void MPEClickInfo::setEMObjID( EM::ObjectID emobjid )
{ clickedemobjid_ = emobjid; }


void MPEClickInfo::setEMVisID( VisID visid )
{ emvisids_ = visid; }


void MPEClickInfo::setObjCS( const TrcKeyZSampling& cs )
{ clickedcs_ = cs; }


void MPEClickInfo::setObjDataPackID( DataPackID datapackid )
{ datapackid_ = datapackid; }


void MPEClickInfo::setObjData( const RegularSeisDataPack* ad )
{ attrdata_ = ad; }


void MPEClickInfo::setObjDataSelSpec( const Attrib::SelSpec& as )
{ attrsel_ = as; }


void MPEClickInfo::setGeomID( Pos::GeomID geomid )
{ geomid_ = geomid; }


void MPEClickInfo::setObjLineName( const char* str )
{ linename_ = str; }


void MPEClickInfo::setObjLineData( const Attrib::Data2DHolder* ad2dh )
{ linedata_ = ad2dh; }

void MPEClickInfo::setObjTKPath( const TrcKeyPath* tkp )
{ rdltkpath_ = tkp; }

const TrcKeyPath* MPEClickInfo::getObjTKPath() const
{ return rdltkpath_; }

void MPEClickInfo::setObjRandomLineID( RandomLineID rdlid )
{ rdlid_ = rdlid; }

RandomLineID MPEClickInfo::getObjRandomLineID() const
{ return rdlid_; }

} // namespace visSurvey
