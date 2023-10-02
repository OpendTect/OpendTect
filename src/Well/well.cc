/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
#include "mnemonics.h"
#include "stratlevel.h"
#include "uistrings.h"
#include "wellman.h"
#include "wellreader.h"

// Keys for IOPars
const char* Well::Info::sKeyDepthUnit() { return sKey::DepthUnit(); }
const char* Well::Info::sKeyDepthType() { return "Depth type"; }
const char* Well::Info::sKeyUwid()	{ return "Unique Well ID"; }
const char* Well::Info::sKeyOper()	{ return "Operator"; }
const char* Well::Info::sKeyField()	{ return "Field"; }
const char* Well::Info::sKeyCounty()	{ return "County"; }
const char* Well::Info::sKeyState()	{ return "State"; }
const char* Well::Info::sKeyProvince()	{ return "Province"; }
const char* Well::Info::sKeyCountry()	{ return "Country"; }
const char* Well::Info::sKeyCoord()	{ return "Surface coordinate"; }
const char* Well::Info::sKeyWellType()	{ return "WellType"; }
const char* Well::Info::sKeyTD()	{ return "Total Depth [TD]"; }
const char* Well::Info::sKeyTVDSS()	{ return "Z [TVDSS]"; }
const char* Well::Info::sKeyTVDSD()	{ return "TVDSD"; }
const char* Well::Info::sKeyTVDGL()	{ return "TVDGL"; }
const char* Well::Info::sKeyKBElev()
	{ return "Reference Datum Elevation [KB]"; }
const char* Well::Info::sKeyReplVel()
	{ return "Replacement velocity [from KB to SRD]"; }
const char* Well::Info::sKeyGroundElev()
	{ return "Ground Level elevation [GL]"; }
const char* Well::Info::sKeyMarkerDepth()
	{ return "Marker depth"; }

// Strings for GUI
uiString Well::Info::sUwid()		{ return tr("Unique Well ID"); }
uiString Well::Info::sOper()		{ return tr("Operator"); }
uiString Well::Info::sField()		{ return tr("Field"); }
uiString Well::Info::sCounty()		{ return tr("County"); }
uiString Well::Info::sState()		{ return tr("State"); }
uiString Well::Info::sProvince()	{ return tr("Province"); }
uiString Well::Info::sCountry()		{ return tr("Country"); }
uiString Well::Info::sCoord()		{ return tr("Surface coordinate"); }
uiString Well::Info::sKBElev()
	{ return tr("Reference Datum Elevation [KB]"); }
uiString Well::Info::sReplVel()
	{ return tr("Replacement Velocity [From KB to SRD]"); }
uiString Well::Info::sGroundElev()
	{ return tr("Ground Level Elevation [GL]"); }
uiString Well::Info::sTD()
	{ return tr("Total Depth [TD]"); }

mDefineEnumUtils(Well::Info, DepthType, "Depth type")
{ Well::Info::sKeyMD(), Well::Info::sKeyTVD(), Well::Info::sKeyTVDSS(),
  Well::Info::sKeyTVDSD(), sKey::TWT(), Well::Info::sKeyTVDGL(), nullptr
};

 template <>
 void EnumDefImpl<Well::Info::DepthType>::init()
 {
     uistrings_ += uiStrings::sMD();
     uistrings_ += uiStrings::sTVD();
     uistrings_ += uiStrings::sTVDSS();
     uistrings_ += uiStrings::sTVDSD();
     uistrings_ += uiStrings::sTWT();
     uistrings_ += uiStrings::sTVDRelGL();
 }


mDefineEnumUtils(Well::Info, InfoType, "Info type")
{
    sKey::None(), sKey::Name(), Well::Info::sKeyUwid(),
    Well::Info::sKeyWellType(), Well::Info::sKeyTD(), Well::Info::sKeyKBElev(),
    Well::Info::sKeyGroundElev(), Well::Info::sKeyMarkerDepth(),
    Well::Info::sKeyCoord(), "Surface Inl/Crl",
    Well::Info::sKeyOper(), Well::Info::sKeyField(),
    Well::Info::sKeyCounty(), Well::Info::sKeyState(),
    Well::Info::sKeyProvince(), Well::Info::sKeyCountry(), 0
};


template <>
void EnumDefImpl<Well::Info::InfoType>::init()
{
    fillUiStrings();
    setUiStringForIndex( 0, uiString::empty() );
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


bool Well::Info::isDepth( const Mnemonic& mn, Well::Info::DepthType& dt )
{
    bool isdepth = true;
    if ( &mn==&Mnemonic::defMD() )
	dt = Well::Info::MD;
    else if ( &mn==&Mnemonic::defTVD() )
	dt = Well::Info::TVD;
    else if ( &mn==&Mnemonic::defTVDSS() )
	dt = Well::Info::TVDSS;
    else if ( &mn==&Mnemonic::defTVDSD() )
	dt = Well::Info::TVDSD;
    else if ( &mn==&Mnemonic::defTVDGL() )
	dt = Well::Info::TVDGL;
    else
	isdepth = false;

    return isdepth;
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


Well::DahObj& Well::DahObj::operator =( const DahObj& oth )
{
    NamedObject::operator=( oth );
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
    : d2tchanged(this)
    , csmdlchanged(this)
    , markerschanged(this)
    , trackchanged(this)
    , disp3dparschanged(this)
    , disp2dparschanged(this)
    , logschanged(this)
    , reloaded(this)
    , info_(nm)
    , track_(*new Well::Track)
    , logs_(*new Well::LogSet)
    , d2tmodel_(nullptr)
    , csmodel_(nullptr)
    , markers_(*new MarkerSet)
    , disp2d_(*new Well::DisplayProperties(sKey2DDispProp()))
    , disp3d_(*new Well::DisplayProperties(sKey3DDispProp()))
{
    Strat::LevelSet& lvlset = Strat::eLVLS();
    mAttachCB( lvlset.levelToBeRemoved, Data::levelToBeRemoved );
    mAttachCB( logschanged, Data::reloadLogNames );
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
}


void Well::Data::prepareForDelete()
{
    Well::MGR().cleanupNullPtrs();
}


bool Well::Data::haveMarkers() const
{
    return !markers_.isEmpty();
}


bool Well::Data::haveLogs() const
{
    if ( lognms_.isEmpty() )
	reloadLogNames();
    return !lognms_.isEmpty();
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
    setD2TModel( nullptr );
    setCheckShotModel( nullptr );
    track_.setEmpty();
    logs_.setEmpty();
    markers_.setEmpty();
}


uiString Well::Data::getInfoString( Well::Info::InfoType it,
				    const IOPar* modifier ) const
{
    const UnitOfMeasure* zun = UnitOfMeasure::surveyDefDepthUnit();
    uiString ret = uiString::empty();
    BinID surfbid;
    switch ( it )
    {
	case Well::Info::None:
	    break;

	case Well::Info::Name:
	    ret = toUiString( name() ); break;

	case Well::Info::UWID:
	    ret = toUiString( info().uwid_ ); break;

	case Well::Info::WellType:
	    ret = toUiString( info().welltype_ ); break;

	case Well::Info::TD:
	    if ( !track().isEmpty() )
	    {
		const float td = track().dahRange().stop;
		if ( !mIsUdf(td) )
		    ret = toUiString( zun->userValue(td), 2 );
	    }
	    break;

	case Well::Info::KB:
	    if ( !mIsUdf(track().getKbElev()) )
	    {
		ret = toUiString( zun->userValue(track().getKbElev()), 2 );
	    }
	    break;

	case Well::Info::GroundElev:
	    if ( !mIsUdf(info().groundelev_) )
	    {
		ret = toUiString( zun->userValue(info().groundelev_), 2 );
	    }
	    break;

	case Well::Info::MarkerDepth:
	    if ( modifier )
	    {
		const BufferString markername = modifier->find( sKey::Name() );
		const Marker* marker = markers().getByName( markername );
		if ( !marker )
		    return uiString::empty();

		const float dah = marker->dah();
		if ( mIsUdf(dah) )
		    return uiString::empty();

		Well::Info::DepthType dt;
		if ( !Well::Info::parseEnum(
			    *modifier,Well::Info::sKeyDepthType(),dt) )
		    return uiString::empty();

		if ( dt == Well::Info::MD )
		    ret = toUiString( zun->userValue(dah), 2 );
		else if ( dt == Well::Info::TWT )
		{
		    const float twt =
				d2TModel() ? d2TModel()->getTime( dah, track() )
					   : mUdf(float);
		    if ( mIsUdf(twt) )
			return uiString::empty();

		    ret = toUiString( UnitOfMeasure::surveyDefTimeUnit()
							->userValue(twt), 2 );
		}
		else
		{
		    float zval = track().getPos(dah).z;
		    if ( mIsUdf(zval) )
			return uiString::empty();

		    if ( dt == Well::Info::TVD )
			zval += track().getKbElev();
		    else if ( dt == Well::Info::TVDSD )
			zval += SI().seismicReferenceDatum();

		    ret = toUiString( zun->userValue(zval), 2 );
		}
	    }

	    break;

	case Well::Info::SurfCoord:
	    ret = toUiString(
		    info().surfacecoord_.toPrettyString( SI().nrXYDecimals()) );
	    break;

	case Well::Info::SurfBinID:
	    surfbid = SI().transform( info().surfacecoord_ );
	    ret = toUiString( surfbid.toString(false));
	    break;

	case Well::Info::Operator:
	    ret = toUiString( info().oper_ );
	    break;

	case Well::Info::Field:
	    ret = toUiString( info().field_ );
	    break;

	case Well::Info::County:
	    ret = toUiString( info().county_ );
	    break;

	case Well::Info::State:
	    ret = toUiString( info().state_ );
	    break;

	case Well::Info::Province:
	    ret = toUiString( info().province_ );
	    break;

	case Well::Info::Country:
	    ret = toUiString( info().country_ );
	    break;

	default: break;
    }

    return ret;
}


void Well::Data::levelToBeRemoved( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(Strat::LevelID,lvlid,cb);
    Marker* marker = markers().getByLvlID( lvlid );
    if ( marker )
	marker->setNoLevelID();
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
	int nloaded = 0;
	for ( int idx=0; idx<logs_.size(); idx++ )
	{
	    if ( logs_.getLog( idx ).isLoaded() )
		nloaded++;
	}
	if ( nloaded == lognms_.size() )
	 {
	    lreqs.add( Logs );
	    lreqs.add( LogInfos );
	 }
	else if ( logs_.size() == lognms_.size() )
	    lreqs.add( LogInfos );

    }

    if ( !track_.isEmpty() )
	lreqs.add( Trck );

    return lreqs;
}


const Well::Log* Well::Data::getLog( const char* nm ) const
{
    if ( lognms_.isEmpty() )
	reloadLogNames();

    if ( lognms_.isPresent( nm ) && !logs().isLoaded( nm ) )
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
    if ( lognms_.isEmpty() )
	reloadLogNames();

    if ( lognms_.isPresent( nm ) && !logs().isLoaded( nm ) )
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


void Well::Data::reloadLogNames() const
{
    MGR().getLogNamesByID(mid_, lognms_, false);
}


void Well::Data::reloadLogNames( CallBacker* )
{
    reloadLogNames();
}


// Well::Info
Well::Info::Info( const char* nm )
    : NamedCallBacker(nm)
    , replvel_(Well::getDefaultVelocity())
{}


Well::Info::Info( const Well::Info& oth )
    : Info(oth.name())
{
    *this = oth;
}


Well::Info::~Info()
{}


Well::Info& Well::Info::operator=( const Info& oth )
{
    name_ = oth.name_;
    uwid_ = oth.uwid_;
    oper_ = oth.oper_;
    field_ = oth.field_;
    county_ = oth.county_;
    state_ = oth.state_;
    province_ = oth.province_;
    country_ = oth.country_;
    source_ = oth.source_;
    welltype_ = oth.welltype_;
    surfacecoord_ = oth.surfacecoord_;
    replvel_ = oth.replvel_;
    groundelev_ = oth.groundelev_;
    return *this;
}


bool Well::Info::isLoaded() const
{
    return surfacecoord_ != Coord() ||
	   !uwid_.isEmpty() || !oper_.isEmpty() || !field_.isEmpty() ||
	   !county_.isEmpty() || !state_.isEmpty() || !province_.isEmpty() ||
	   !country_.isEmpty() || !source_.isEmpty() ||
	   welltype_ != OD::UnknownWellType || !mIsUdf(groundelev_) ||
	   !mIsEqual(replvel_,getDefaultVelocity(),1e-1f);
}

#define mName "Well name"

void Well::Info::fillPar( IOPar& par ) const
{
    par.set( mName, name() );
    par.set( sKeyUwid(), uwid_ );
    par.set( sKeyOper(), oper_ );
    par.set( sKeyField(), field_ );
    par.set( sKeyCounty(), county_ );
    par.set( sKeyState(), state_ );
    par.set( sKeyProvince(), province_ );
    par.set( sKeyCountry(), country_ );
    par.set( sKeyWellType(), welltype_ );

    par.set( sKeyCoord(), surfacecoord_.toString() );
    par.set( sKeyReplVel(), replvel_ );
    par.set( sKeyGroundElev(), groundelev_ );
}


void Well::Info::usePar( const IOPar& par )
{
    setName( par.find(mName) );
    par.get( sKeyUwid(), uwid_ );
    par.get( sKeyOper(), oper_ );
    par.get( sKeyField(), field_ );
    par.get( sKeyCounty(), county_ );
    par.get( sKeyState(), state_ );
    par.get( sKeyProvince(), province_ );
    par.get( sKeyCountry(), country_ );

    int welltype = 0;
    par.get( sKeyWellType(), welltype ); welltype_ = (OD::WellType)welltype;

    surfacecoord_.fromString( par.find(sKeyCoord()) );
    par.get( sKeyReplVel(), replvel_ );
    par.get( sKeyGroundElev(), groundelev_ );
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
