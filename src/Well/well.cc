/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
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
#include "wellman.h"


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
const char* Well::Info::sKeyDepthUnit() { return sKey::DepthUnit(); }
const char* Well::Info::sKeyUwid()	{ return "Unique Well ID"; }
const char* Well::Info::sKeyOper()	{ return "Operator"; }
const char* Well::Info::sKeyState()	{ return "State"; }
const char* Well::Info::sKeyCounty()	{ return "County"; }
const char* Well::Info::sKeyCoord()	{ return "Surface coordinate"; }
const char* Well::Info::sKeyWellType()	{ return "WellType"; }
const char* Well::Info::sKeyTD()	{ return "Total Depth [TD]"; }
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


static const char* sWellName = "Well name";


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
    copyAll( oth );
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


int Well::Info::legacyLogWidthFactor()
{
   const int inlnr = SI().inlRange( true ).nrSteps() + 1;
   const int crlnr = SI().crlRange( true ).nrSteps() + 1;
   const float survfac = Math::Sqrt( (float)(crlnr*crlnr + inlnr*inlnr) );
   const int legacylogwidthfac = mNINT32(survfac*43/1000);
   //hack 43 best factor based on F3_Demo
   return legacylogwidthfac!=0 ? legacylogwidthfac : 1;
}


void Well::Info::fillPar( IOPar& par ) const
{
    par.set( sWellName, name() );
    par.set( sKeyUwid(), uwid_ );
    par.set( sKeyOper(), oper_ );
    par.set( sKeyState(), state_ );
    par.set( sKeyCounty(), county_ );
    par.set( sKeyWellType(), (int)welltype_ );
    par.set( sKeyCoord(), surfacecoord_.toString() );
    par.set( sKeyReplVel(), replvel_ );
    par.set( sKeyGroundElev(), groundelev_ );
}


void Well::Info::usePar( const IOPar& par )
{
    setName( par.find(sWellName) );
    par.get( sKeyUwid(), uwid_ );
    par.get( sKeyOper(), oper_ );
    par.get( sKeyState(), state_ );
    par.get( sKeyCounty(), county_ );
    int welltype = (int)welltype_;
    par.get( sKeyWellType(), welltype );
    welltype_ = (WellType)welltype;

    surfacecoord_.fromString( par.find(sKeyCoord()) );
    par.get( sKeyReplVel(), replvel_ );
    par.get( sKeyGroundElev(), groundelev_ );
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
    , disp2d_(*new Well::DisplayProperties(sKey2DDispProp())) \
    , disp3d_(*new Well::DisplayProperties(sKey3DDispProp())) \
    , d2tmodel_(*new D2TModel) \
    , csmodel_(*new D2TModel) \
    , markers_(*new MarkerSet) \
    , d2tchanged(this) \
    , csmdlchanged(this) \
    , markerschanged(this) \
    , trackchanged(this) \
    , disp2dparschanged(this) \
    , disp3dparschanged(this) \
    , logschanged(this) \
    , reloaded(this)


Well::Data::Data( const char* nm )
    : mInitList(nm,nm)
{
    mTriggerInstanceCreatedNotifier();
}


Well::Data::Data( const Data& oth )
    : mInitList(oth,"")
{
    copyAll( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::Data::~Data()
{
    Well::MGR().removeObject( this );
    sendDelNotif();
    detachAllNotifiers();

    mDoAllSubObjs( delete &, );
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

    mid_ = oth.mid_;
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


Well::Data::DirtyCountType Well::Data::dirtyCount() const
{
    DirtyCountType ret = 0;
    mDoAllSubObjs( ret +=, .dirtyCount() );
    return ret;
}
