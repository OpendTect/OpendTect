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
#include "idxable.h"
#include "iopar.h"
#include "stratlevel.h"
#include "uistrings.h"
#include "wellman.h"
#include "wellreader.h"

#include "hiddenparam.h"

HiddenParam<Well::Data,BufferStringSet*> hp_lognms_(nullptr);

// Keys for IOPars
const char* Well::Info::sKeyDepthUnit() { return sKey::DepthUnit(); }
const char* Well::Info::sKeyUwid()	{ return "Unique Well ID"; }
const char* Well::Info::sKeyOper()	{ return "Operator"; }
const char* Well::Info::sKeyState()	{ return "State"; }
const char* Well::Info::sKeyCounty()	{ return "County"; }
const char* Well::Info::sKeyCoord()	{ return "Surface coordinate"; }
const char* Well::Info::sKeyWellType()	{ return "WellType"; }
const char* Well::Info::sKeyTD()	{ return "Total Depth [TD]"; }
const char* Well::Info::sKeyTVDSS()	{ return "Z [TVDSS]"; }
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
uiString Well::Info::sTVDSS()
	{ return tr("Z [TVDSS]"); }


mDefineEnumUtils( Well::Info, WellType, "Well Type" )
{ "none", "oilwell", "gaswell", "oilgaswell", "dryhole", "pluggedoilwell",
  "pluggedgaswell", "pluggedoilgaswell", "permittedlocation",
  "canceledlocation", "injectiondisposalwell", 0 };

mDefineEnumUtils(Well::Info, DepthType, "Depth type")
{ Well::Info::sKeyMD(), Well::Info::sKeyTVD(), Well::Info::sKeyTVDSS(),
  sKey::TWT(), nullptr };
 template <>
 void EnumDefImpl<Well::Info::DepthType>::init()
 {
     uistrings_ += ::toUiString( "MD" );
     uistrings_ += ::toUiString( "TVD" );
     uistrings_ += ::toUiString( "TVDSS" );
     uistrings_ += uiStrings::sTWT();
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


Notifier<Well::DahObj>& Well::DahObj::instanceCreated()
{
    mDefineStaticLocalObject( Notifier<Well::DahObj>, theNotif, (0));
    return theNotif;
}


Well::DahObj::DahObj( const char* nm )
  : NamedCallBacker(nm)
{
    dahrange_.setUdf();
    instanceCreated().trigger( this );
}


Well::DahObj::DahObj( const Well::DahObj& d )
  : NamedCallBacker(d.name())
  , dah_(d.dah_)
  , dahrange_(d.dahrange_)
{
    instanceCreated().trigger( this );
}


Well::DahObj::~DahObj()
{
    dah_.erase();
    sendDelNotif();
}


Well::DahObj& Well::DahObj::operator =( const Well::DahObj& oth )
{
    setName( oth.name() );
    dah_ = oth.dah_;
    dahrange_ = oth.dahrange_;
    return *this;
}


int Well::DahObj::indexOf( float dh ) const
{
    int idx1 = -1;
    IdxAble::findFPPos( dah_, dah_.size(), dh, -1, idx1 );
    return idx1;
}


Interval<float>& Well::DahObj::dahRange()
{
    return dahrange_;
}


Interval<float> Well::DahObj::dahRange() const
{
    return dahrange_;
}


float Well::DahObj::dahStep( bool ismin ) const
{
    const int sz = dah_.size();
    if ( sz < 2 ) return mUdf(float);

    float res = dah_[1] - dah_[0];
    if ( res <0 ) res = 0;
    int nrvals = 1;
    for ( int idx=2; idx<sz; idx++ )
    {
	float val = dah_[idx] - dah_[idx-1];
	if ( mIsZero(val,mDefEps) )
	    continue;

	if ( !ismin )
	    res += val;
	else
	{
	    if ( val < res && val >= 0 )
		res = val;
	}
	nrvals++;
    }

    if ( !ismin ) res /= nrvals; // average
    return mIsZero(res,mDefEps) ? mUdf(float) : res;
}


void Well::DahObj::deInterpolate()
{
    TypeSet<Coord> bpfinp;
    bpfinp.setCapacity( dah_.size(), false );
    for ( int idx=0; idx<dah_.size(); idx++ )
	bpfinp += Coord( dah_[idx]*0.1, value( idx ) );
	// for time we want a fac of 1000, but for track 1. Compromise.

    BendPointFinder2D finder( bpfinp, 1e-5 );
    if ( !finder.execute() || finder.bendPoints().size()<1 )
	return;

    const TypeSet<int>& bpidxs = finder.bendPoints();

    int bpidx = 0;
    TypeSet<int> torem;
    for ( int idx=0; idx<bpfinp.size(); idx++ )
    {
	if ( idx != bpidxs[bpidx] )
	    torem += idx;
	else
	    bpidx++;
    }

    for ( int idx=torem.size()-1; idx>-1; idx-- )
	remove( torem[idx] );
}


void Well::DahObj::addToDahFrom( int fromidx, float extradah )
{
    for ( int idx=fromidx; idx<dah_.size(); idx++ )
	dah_[idx] += extradah;

    updateDahRange();
}


void Well::DahObj::removeFromDahFrom( int fromidx, float extradah )
{
    for ( int idx=fromidx; idx<dah_.size(); idx++ )
	dah_[idx] -= extradah;

    updateDahRange();
}


void Well::DahObj::updateDahRange()
{
    if ( !dah_.isEmpty() )
	dahrange_.set( dah_.first(), dah_.last() );
}


Well::Data::Data( const char* nm )
    : info_(nm)
    , track_(*new Well::Track)
    , logs_(*new Well::LogSet)
    , disp2d_(*new Well::DisplayProperties(sKey2DDispProp()))
    , disp3d_(*new Well::DisplayProperties(sKey3DDispProp()))
    , d2tmodel_(0)
    , csmodel_(0)
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
    lvlset.levelToBeRemoved.notify( mCB(this, Well::Data, levelToBeRemoved ) );

    hp_lognms_.setParam( this, new BufferStringSet );
    mAttachCB(logschanged, Well::Data::reloadLogNames);
}


Well::Data::~Data()
{
    detachAllNotifiers();
    delete &track_;
    delete &logs_;
    delete &disp2d_;
    delete &disp3d_;
    delete d2tmodel_;
    delete csmodel_;
    delete &markers_;

    Strat::eLVLS().levelToBeRemoved.remove(
				mCB(this, Well::Data, levelToBeRemoved ) );

    hp_lognms_.removeAndDeleteParam( this );
}


void Well::Data::prepareForDelete() const
{
    Well::MGR().removeObject( this );
}


bool Well::Data::haveMarkers() const
{
    return !markers_.isEmpty();
}


bool Well::Data::haveLogs() const
{
    const BufferStringSet& lognms = storedLogNames();
    if ( lognms.isEmpty() )
	reloadLogNames();
    return !lognms.isEmpty();
}


void Well::Data::setD2TModel( D2TModel* d )
{
    if ( d2tmodel_ == d )
	return;
    delete d2tmodel_;
    d2tmodel_ = d;
}


void Well::Data::setCheckShotModel( D2TModel* d )
{
    if ( csmodel_ == d )
	return;
    delete csmodel_;
    csmodel_ = d;
}


void Well::Data::setEmpty()
{
    setD2TModel( 0 ); setCheckShotModel( 0 );
    track_.setEmpty();
    logs_.setEmpty();
    markers_.setEmpty();
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
	Marker* mrk = markers().getByLvlID( lvl.id() );
	if ( mrk )
	    mrk->setLevelID( -1 );
    }
}


Well::LoadReqs Well::Data::loadState() const
{
    LoadReqs lreqs( false );
    if ( info_.isLoaded() )
	lreqs.add( Inf );
    if ( disp2d_.isValid() || disp2d_.isModified() )
	lreqs.add( DispProps2D );
    if ( disp3d_.isValid() || disp3d_.isModified() )
	lreqs.add( DispProps3D );
    if ( haveMarkers() )
	lreqs.add( Mrkrs );
    if ( haveD2TModel() )
	lreqs.add( D2T );
    if ( haveCheckShotModel() )
	lreqs.add( CSMdl );
    if ( haveLogs() )
    {
	const BufferStringSet& lognms = storedLogNames();
	int nloaded = 0;
	for ( int idx=0; idx<logs_.size(); idx++ )
	{
	    if ( logs_.getLog( idx ).isLoaded() )
		nloaded++;
	}
	if (nloaded == lognms.size())
	{
	    lreqs.add(Logs);
	    lreqs.add(LogInfos);
	}
	else if ( logs_.size() == lognms.size() )
	    lreqs.add( LogInfos );
    }

    if ( !track_.isEmpty() )
	lreqs.add( Trck );

    return lreqs;
}


const Well::Log* Well::Data::getLog( const char* nm ) const
{
    if ( !haveLogs() )
	return nullptr;

    const BufferStringSet& lognms = storedLogNames();
    if ( lognms.isPresent( nm ) && !logs().isLoaded( nm ) )
    {
	Well::Data& wd = const_cast<Well::Data&>(*this);
	Well::Reader rdr( mid_, wd );
	if ( !rdr.getLog( nm ) )
	{
	    ErrMsg( rdr.errMsg() );
	    return nullptr;
	}
    }
    return logs().getLog( nm );
}


Well::Log* Well::Data::getLogForEdit( const char* nm )
{
    if ( !haveLogs() )
	return nullptr;

    const BufferStringSet& lognms = storedLogNames();
    if ( lognms.isPresent( nm ) && !logs().isLoaded( nm ) )
    {
	Well::Reader rdr( mid_, *this );
	if ( !rdr.getLog( nm ) )
	{
	    ErrMsg( rdr.errMsg() );
	    return nullptr;
	}
    }
    return logs().getLog( nm );
}


const BufferStringSet& Well::Data::storedLogNames() const
{
    BufferStringSet* lognms = hp_lognms_.getParam( this );
    return *lognms;
}


void Well::Data::reloadLogNames() const
{
    BufferStringSet* lognms = hp_lognms_.getParam( this );
    MGR().getLogNamesByID(mid_, *lognms, false);
}


void Well::Data::reloadLogNames( CallBacker* )
{
    reloadLogNames();
}


bool Well::Info::isLoaded() const
{
    return surfacecoord != Coord() ||
	   !uwid.isEmpty() || !oper.isEmpty() || !state.isEmpty() ||
	   !county.isEmpty() || !source_.isEmpty() ||
	   welltype_ != None || !mIsUdf(groundelev);
}

#define mName "Well name"

void Well::Info::fillPar( IOPar& par ) const
{
    par.set( mName, name() );
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
    setName( par.find(mName) );
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
    {
	return SI().depthsInFeet() ? replvelft * mFromFeetFactorF : replvelm;
    }
}
