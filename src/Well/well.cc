/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2003
-*/


#include "welldata.h"
#include "wellinfo.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welldisp.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "bendpointfinder.h"
#include "iopar.h"
#include "stratlevel.h"
#include "wellmanager.h"


int Well::nrSubObjTypes()
{
    return mWellNrSubObjTypes;
}


float Well::getDefaultVelocity()
{
    const float replvelm = 2000.f;
    const float replvelft = 8000.f;
    if ( SI().zInFeet() )
	return replvelft;
    else
	return SI().depthsInFeet() ? replvelft * mFromFeetFactorF : replvelm;
}


// Keys for IOPars
const char* Well::Info::sKeyWellName()	{ return "Well name"; }
const char* Well::Info::sKeyUwid()	{ return "Unique Well ID"; }
const char* Well::Info::sKeyOper()	{ return "Operator"; }
const char* Well::Info::sKeyState()	{ return "State"; }
const char* Well::Info::sKeyCounty()	{ return "County"; }
const char* Well::Info::sKeyCoord()	{ return "Surface coordinate"; }
const char* Well::Info::sKeyWellType()	{ return "WellType"; }
const char* Well::Info::sKeyTD()	{ return "Total Depth [TD]"; }
const char* Well::Info::sKeyTVD()	{ return "True Vertical Depth [TVD]"; }
const char* Well::Info::sKeyTVDSS()	{ return "Z [TVDSS]"; }
const char* Well::Info::sKeyMD()	{ return "Measured Depth [MD]"; }
const char* Well::Info::sKeyKBElev()
	{ return "Reference Datum Elevation [KB]"; }
const char* Well::Info::sKeyReplVel()
	{ return "Replacement velocity [from KB to SRD]"; }
const char* Well::Info::sKeyGroundElev()
	{ return "Ground Level elevation [GL]"; }

// Strings for GUI
uiString Well::Info::sUwid()	{ return tr("Unique Well ID"); }
uiString Well::Info::sOper()	{ return tr("Operator"); }
uiString Well::Info::sState()	{ return tr("State"); }
uiString Well::Info::sCounty()	{ return tr("County"); }
uiString Well::Info::sCoord()	{ return tr("Surface coordinate"); }
uiString Well::Info::sKBElev()
	{ return tr("Reference Datum Elevation [KB]"); }
uiString Well::Info::sReplVel()
	{ return tr("Replacement Velocity [From KB to SRD]"); }
uiString Well::Info::sGroundElev()
	{ return tr("Ground Level Elevation [GL]"); }
uiString Well::Info::sTD()
	{ return tr("Total Depth [TD]"); }


mDefineEnumUtils( Well::Info, WellType, "Well Type" )
{ "none", "oilwell", "gaswell", "oilgaswell", "dryhole", "pluggedoilwell",
  "pluggedgaswell", "pluggedoilgaswell", "permittedlocation",
  "canceledlocation", "injectiondisposalwell", 0 };

mDefineEnumUtils(Well::Info, DepthType, "Depth Type")
{ Well::Info::sKeyMD(), Well::Info::sKeyTVD(), Well::Info::sKeyTVDSS(), 0 };
template <>
void EnumDefImpl<Well::Info::DepthType>::init()
{
    uistrings_ += ::toUiString( "MD" );
    uistrings_ += ::toUiString( "TVD" );
    uistrings_ += ::toUiString( "TVDSS" );
}

template<>
void EnumDefImpl<Well::Info::WellType>::init()
{
    uistrings_ += uiStrings::sNone();
    uistrings_ += mEnumTr("Oil Well",0);
    uistrings_ += mEnumTr("Gas Well",0);
    uistrings_ += mEnumTr("Oil-Gas Well",0);
    uistrings_ += mEnumTr("Dry Hole",0);
    uistrings_ += mEnumTr("Plugged Oil Well",0);
    uistrings_ += mEnumTr("Plugged Gas Well",0);
    uistrings_ += mEnumTr("Plugged Oil-Gas Well",0);
    uistrings_ += mEnumTr("Permitted Location",0);
    uistrings_ += mEnumTr("Canceled Location",0);
    uistrings_ += mEnumTr("Injection Disposal Well",0);
}


mDefineInstanceCreatedNotifierAccess(Well::Info);


Well::Info::Info( const char* nm )
    : NamedMonitorable(nm)
    , replvel_(getDefaultVelocity())
    , groundelev_(mUdf(float))
    , welltype_(None)
{
    mTriggerInstanceCreatedNotifier();
}


Well::Info::Info( const Info& oth )
    : NamedMonitorable(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::Info::~Info()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::Info, NamedMonitorable )


void Well::Info::copyClassData( const Info& oth )
{
    uwid_ = oth.uwid_;
    oper_ = oth.oper_;
    state_ = oth.state_;
    county_ = oth.county_;
    welltype_ = oth.welltype_;
    surfacecoord_ = oth.surfacecoord_;
    replvel_ = oth.replvel_;
    groundelev_ = oth.groundelev_;
    source_ = oth.source_;
}


Monitorable::ChangeType Well::Info::compareClassData( const Info& oth ) const
{
    mStartMonitorableCompare();
    mHandleMonitorableCompare( uwid_, cUWIDChange() );
    mHandleMonitorableCompare( oper_, cInfoChange() );
    mHandleMonitorableCompare( state_, cInfoChange() );
    mHandleMonitorableCompare( county_, cInfoChange() );
    mHandleMonitorableCompare( welltype_, cTypeChange() );
    mHandleMonitorableCompare( surfacecoord_, cPosChg() );
    mHandleMonitorableCompare( replvel_, cVelChg() );
    mHandleMonitorableCompare( groundelev_, cElevChg() );
    mHandleMonitorableCompare( source_, cSourceChg() );
    mDeliverMonitorableCompare();
}


void Well::Info::fillPar( IOPar& iop ) const
{
    if ( surfaceCoord() != Coord(0,0) )
	iop.set( sKeyCoord(), surfacecoord_ );
    else
	iop.removeWithKey( sKeyCoord() );

    iop.set( sKeyUwid(), uwid_ );
    iop.set( sKeyOper(), oper_ );
    iop.set( sKeyState(), state_ );
    iop.set( sKeyCounty(), county_ );
    iop.set( sKeyWellType(), (int)welltype_ );
    iop.set( sKeyReplVel(), replvel_ );
    iop.set( sKeyGroundElev(), groundelev_ );
    iop.set( sKeyWellName(), name() );
}


static const char* sKeyOldreplvel = "Replacement velocity";
static const char* sKeyOldgroundelev = "Ground Level elevation";


void Well::Info::usePar( const IOPar& par )
{
    const BufferString parnm( par.find(sKeyWellName()) );
    if ( !parnm.isEmpty() )
	setName( parnm );

    par.get( sKeyUwid(), uwid_ );
    par.get( sKeyOper(), oper_ );
    par.get( sKeyState(), state_ );
    par.get( sKeyCounty(), county_ );
    int welltype = (int)welltype_;
    par.get( sKeyWellType(), welltype );
    welltype_ = (WellType)welltype;
    const BufferString coordstr( par.find(sKeyCoord()) );
    if ( !coordstr.isEmpty() )
	surfacecoord_.fromString( coordstr );
    if ( !par.get(sKeyReplVel(),replvel_) )
	par.get( sKeyOldreplvel, replvel_ );
    if ( !par.get(sKeyGroundElev(),groundelev_) )
	par.get( sKeyOldgroundelev, groundelev_ );
}


mDefineInstanceCreatedNotifierAccess(Well::Data)

#define mDoAllSubObjs(preact,postact) \
    preact info_ postact; \
    preact track_ postact; \
    preact logs_ postact; \
    preact d2tmodel_ postact; \
    preact csmodel_ postact; \
    preact markers_ postact; \
    preact disp2d_ postact; \
    preact disp3d_ postact


#define mInitList(constrarg,nm) \
      SharedObject(constrarg) \
    , info_(*new Info(nm)) \
    , track_(*new Well::Track) \
    , logs_(*new Well::LogSet) \
    , disp2d_(*new Well::DisplayProperties2D()) \
    , disp3d_(*new Well::DisplayProperties3D()) \
    , d2tmodel_(*new D2TModel) \
    , csmodel_(*new D2TModel) \
    , markers_(*new MarkerSet)


Well::Data::Data( const char* nm )
    : mInitList(nm,nm)
{
    mTriggerInstanceCreatedNotifier();
}


Well::Data::Data( const Data& oth )
    : mInitList(oth,"")
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::Data::~Data()
{
    sendDelNotif();
    detachAllNotifiers();

    mDoAllSubObjs( delete &, );
}


Well::DisplayProperties& Well::Data::displayProperties( bool for2d )
{
    return for2d ? (Well::DisplayProperties&)disp2d_
		 : (Well::DisplayProperties&)disp3d_;
}


const Well::DisplayProperties& Well::Data::displayProperties( bool for2d ) const
{
    return for2d ? (Well::DisplayProperties&)disp2d_
		 : (Well::DisplayProperties&)disp3d_;
}


mImplMonitorableAssignment( Well::Data, SharedObject )


void Well::Data::copyClassData( const Data& oth )
{
    info_ = oth.info_;
    track_ = oth.track_;
    d2tmodel_ = oth.d2tmodel_;
    markers_ = oth.markers_;
    logs_ = oth.logs_;
    csmodel_ = oth.csmodel_;
    disp2d_ = oth.disp2d_;
    disp3d_ = oth.disp3d_;
}


Monitorable::ChangeType Well::Data::compareClassData( const Data& oth ) const
{
    mDeliverYesNoMonitorableCompare(
    info_ == oth.info_ &&
    track_ == oth.track_ &&
    d2tmodel_ == oth.d2tmodel_ &&
    markers_ == oth.markers_ &&
    logs_ == oth.logs_ &&
    csmodel_ == oth.csmodel_ &&
    disp2d_ == oth.disp2d_ &&
    disp3d_ == oth.disp3d_ );
}


bool Well::Data::depthsInFeet()
{
    return SI().zInFeet();
}


BufferString Well::Data::getName() const
{
    return info_.getName();
}


void Well::Data::setName( const char* nm )
{
    info_.setName( nm );
}


const OD::String& Well::Data::name() const
{
    return info_.name();
}


bool Well::Data::haveTrack() const
{
    return !track_.isEmpty();
}


bool Well::Data::haveMarkers() const
{
    return !markers_.isEmpty();
}


bool Well::Data::haveLogs() const
{
    return !logs_.isEmpty();
}


bool Well::Data::haveD2TModel() const
{
    return !d2tmodel_.isEmpty();
}


bool Well::Data::haveCheckShotModel() const
{
    return !csmodel_.isEmpty();
}


bool Well::Data::displayPropertiesRead() const
{
    return !disp2d_.valsAreDefaults() || !disp3d_.valsAreDefaults();
}


Well::D2TModel& Well::Data::gtMdl( bool ckshot ) const
{
    return const_cast<D2TModel&>( ckshot ? csmodel_ : d2tmodel_ );
}


Well::D2TModel* Well::Data::gtMdlPtr( bool ckshot ) const
{
    D2TModel& d2t = gtMdl( ckshot );
    return d2t.isEmpty() ? 0 : &d2t;
}


void Well::Data::setEmpty()
{
    csmodel_.setEmpty();
    markers_.setEmpty();
    logs_.setEmpty();
    d2tmodel_.setEmpty();
    track_.setEmpty();
}


void Well::Data::touch() const
{
    mDoAllSubObjs( , .touch() );
}


DirtyCountType Well::Data::dirtyCount() const
{
    DirtyCountType ret = 0;
    mDoAllSubObjs( ret +=, .dirtyCount() );
    return ret;
}


DBKey Well::Data::dbKey() const
{
    return MGR().getID( *this );
}
