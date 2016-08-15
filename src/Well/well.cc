/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "welldata.h"
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

int Well::Info::legacyLogWidthFactor()
{
   const int inlnr = SI().inlRange( true ).nrSteps() + 1;
   const int crlnr = SI().crlRange( true ).nrSteps() + 1;
   const float survfac = Math::Sqrt( (float)(crlnr*crlnr + inlnr*inlnr) );
   const int legacylogwidthfac = mNINT32(survfac*43/1000);
   //hack 43 best factor based on F3_Demo
   return legacylogwidthfac!=0 ? legacylogwidthfac : 1;
}


mDefineInstanceCreatedNotifierAccess(Well::Data)


Well::Data::Data( const char* nm )
    : SharedObject(nm)
    , info_(nm)
    , track_(*new Well::Track)
    , logs_(*new Well::LogSet)
    , disp2d_(*new Well::DisplayProperties(sKey2DDispProp()))
    , disp3d_(*new Well::DisplayProperties(sKey3DDispProp()))
    , d2tmodel_(*new D2TModel)
    , csmodel_(*new D2TModel)
    , markers_(*new MarkerSet)
    , d2tchanged(this)
    , csmdlchanged(this)
    , markerschanged(this)
    , trackchanged(this)
    , disp2dparschanged(this)
    , disp3dparschanged(this)
    , logschanged(this)
    , reloaded(this)
{
    Strat::LevelSet& lvlset = Strat::eLVLS();
    mAttachCB( lvlset.levelToBeRemoved, Data::levelToBeRemoved );
    mTriggerInstanceCreatedNotifier();
}


Well::Data::~Data()
{
    Well::MGR().removeObject( this );
    sendDelNotif();
    detachAllNotifiers();

    delete &track_;
    delete &logs_;
    delete &d2tmodel_;
    delete &csmodel_;
    delete &markers_;
    delete &disp2d_;
    delete &disp3d_;
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


void Well::Data::levelToBeRemoved( CallBacker* cb )
{
    mDynamicCastGet(Strat::LevelSet*,lvlset,cb)
    if ( !lvlset )
	{ pErrMsg( "cb null or not LevelSet" ); return; }
    const int lvlidx = lvlset->notifLvlIdx();
    if ( lvlset->levels().validIdx( lvlidx ) )
    {
	const Strat::Level& lvl = *lvlset->levels()[lvlidx];
	Well::Marker* mrk = markers().getByLvlID( lvl.id() );
	if ( mrk )
	    mrk->setLevelID( -1 );
    }
}


static const char* sWellName = "Well name";

void Well::Info::fillPar( IOPar& par ) const
{
    par.set( sWellName, name() );
    par.set( sKeyUwid(), uwid );
    par.set( sKeyOper(), oper );
    par.set( sKeyState(), state );
    par.set( sKeyCounty(), county );
    par.set( sKeyWellType(), welltype_ );

    par.set( sKeyCoord(), surfacecoord.toString() );
    par.set( sKeyReplVel(), replvel );
    par.set( sKeyGroundElev(), groundelev );
}


void Well::Info::usePar( const IOPar& par )
{
    setName( par.find(sWellName) );
    par.get( sKeyUwid(), uwid );
    par.get( sKeyOper(), oper );
    par.get( sKeyState(), state );
    par.get( sKeyCounty(), county );
    int welltype = 0;
    par.get( sKeyWellType(), welltype ); welltype_ = (WellType)welltype;

    surfacecoord.fromString( par.find(sKeyCoord()) );
    par.get( sKeyReplVel(), replvel );
    par.get( sKeyGroundElev(), groundelev );
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
