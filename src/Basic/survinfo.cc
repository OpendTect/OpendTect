/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "survinfo.h"

#include "ascstream.h"
#include "coordsystem.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "trckeyzsampling.h"
#include "latlong.h"
#include "undefval.h"
#include "safefileio.h"
#include "separstr.h"
#include "oddirs.h"
#include "odjson.h"
#include "iopar.h"
#include "zdomain.h"
#include "keystrs.h"
#include "posidxpair2coord.h"
#include "od_istream.h"
#include "oscommand.h"
#include "surveydisklocation.h"
#include "uistrings.h"
#include <stdio.h>


static const char* sKeySI = "Survey Info";
static const char* sKeyXTransf = "Coord-X-BinID";
static const char* sKeyYTransf = "Coord-Y-BinID";
static const char* sSurvFile = ".survey";
static const char* sKeyDefsFile = ".defs";
static const char* sKeySurvDefs = "Survey defaults";
static const char* sKeyLogFile = "survey.log";
static const char* sKeySurvLog = "Survey log";
static const char* sKeyLatLongAnchor = "Lat/Long anchor";

const char* SurveyInfo::sKeyInlRange()	    { return "In-line range"; }
const char* SurveyInfo::sKeyCrlRange()	    { return "Cross-line range"; }
const char* SurveyInfo::sKeyXRange()	    { return "X range"; }
const char* SurveyInfo::sKeyYRange()	    { return "Y range"; }
const char* SurveyInfo::sKeyZRange()	    { return "Z range"; }
const char* SurveyInfo::sKeyDpthInFt()	    { return "Show depth in feet"; }
const char* SurveyInfo::sKeyXYInFt()	    { return "XY in feet"; }
const char* SurveyInfo::sKeySurvDataType()  { return "Survey Data Type"; }
const char* SurveyInfo::sKeySeismicRefDatum(){return "Seismic Reference Datum";}

mDefineEnumUtils(SurveyInfo,Pol2D3D,"Survey Type")
{ "Only 3D", "Both 2D and 3D", "Only 2D", nullptr };

#define mXYInFeet() (coordsystem_ && coordsystem_->isFeet())
#define mXYUnit() (mXYInFeet() ? Feet : Meter)

Survey::Geometry3D& Survey::Geometry3D::current()
{
    return const_cast<Geometry3D&>(*Geometry::default3D().as3D());
}


Coord Survey::Geometry3D::toCoord( int linenr, int tracenr ) const
{
    return transform( BinID(linenr,tracenr) );
}


namespace Survey
{
    //Fill OD::JSON::Object functions
    void fillObjFromRanges( const TrcKeyZSampling& tkzs, bool zistime,
			    bool depthisfeet, OD::JSON::Object& jsonobj )
    {
	OD::JSON::Array inlobj( OD::JSON::Number );
	inlobj.add( tkzs.hsamp_.start_.inl() );
	inlobj.add( tkzs.hsamp_.stop_.inl() );
	inlobj.add( tkzs.hsamp_.step_.inl() );
	jsonobj.set( SurveyInfo::sKeyInlRange(), new OD::JSON::Array(inlobj) );

	OD::JSON::Array crlobj( OD::JSON::Number );
	crlobj.add( tkzs.hsamp_.start_.crl() );
	crlobj.add( tkzs.hsamp_.stop_.crl() );
	crlobj.add( tkzs.hsamp_.step_.crl() );
	jsonobj.set( SurveyInfo::sKeyCrlRange(), new OD::JSON::Array(crlobj) );

	OD::JSON::Array zobj( OD::JSON::Number );
	zobj.add( tkzs.zsamp_.start );
	zobj.add( tkzs.zsamp_.stop );
	zobj.add( tkzs.zsamp_.step );
	OD::JSON::Object zdef;
	zdef.set( sKey::Range(), new OD::JSON::Array(zobj) );
	zdef.set( SurveyInfo::sKeyDomain(), zistime ? sKey::Time() :
				    depthisfeet ? "Feet" : sKey::Depth() );
	jsonobj.set( SurveyInfo::sKeyZAxis(), new OD::JSON::Object(zdef) );
    }


    void fillObjWithDirTransform( const TypeSet<float>& dirx,
	const TypeSet<float>& diry, OD::JSON::Object& obj )
    {
	OD::JSON::Array dirxarr( OD::JSON::Number );
	dirxarr.add( dirx[0] ).add( dirx[1] ).add( dirx[2] );

	OD::JSON::Array diryarr( OD::JSON::Number );
	diryarr.add( diry[0] ).add( diry[1] ).add( diry[2] );

	OD::JSON::Object trans;
	trans.set( sKeyXTransf, new OD::JSON::Array(dirxarr) );
	OD::JSON::Object ytransf;
	trans.set( sKeyYTransf, new OD::JSON::Array(diryarr) );
	obj.set( SurveyInfo::sKeyTransformation(),
	    new OD::JSON::Object(trans) );
    }


    void fillObjWithDirTransform( const Pos::IdxPair2Coord& b2c,
	OD::JSON::Object& obj )
    {
	const Pos::IdxPair2Coord::DirTransform& dirx =
						    b2c.getTransform( true );
	TypeSet<float> dirxset;
	dirxset.add( dirx.a ).add( dirx.b ).add( dirx.c );

	const Pos::IdxPair2Coord::DirTransform& diry =
						    b2c.getTransform( false );
	TypeSet<float> diryset;
	diryset.add( diry.a ).add( diry.b ).add( diry.c );

	fillObjWithDirTransform( dirxset, diryset, obj );
    }


    void fillObjWithSetPts( const TypeSet<BinID>& bids,
	const TypeSet<Coord>& crds, OD::JSON::Object& obj )
    {
	if ( bids.size() < 3 || crds.size() < 3 )
	    return;

	OD::JSON::Array setarr( true );
	for ( int idx=0; idx<3; idx++ )
	{
	    const BinID bid = bids[idx];
	    OD::JSON::Array arrint( OD::JSON::Number );
	    arrint.add( bid.inl() ).add( bid.crl() );
	    OD::JSON::Object ptobj;
	    ptobj.set( SurveyInfo::sKeyIC(), new OD::JSON::Array(arrint) );

	    const Coord crd = crds[idx];
	    OD::JSON::Array arrdoub( OD::JSON::Number );
	    arrdoub.add( crd.x ).add( crd.y );
	    ptobj.set( SurveyInfo::sKeyXY(), new OD::JSON::Array(arrdoub) );

	    setarr.add( new OD::JSON::Object(ptobj) );
	}

	obj.set( SurveyInfo::sKeySetPt(), new OD::JSON::Array(setarr) );
    }


    void fillObjWithUnit( const char* xystr, const char* depthstr,
							OD::JSON::Object& obj )
    {
	OD::JSON::Object unitobj;
	unitobj.set( SurveyInfo::sKeyXY(), xystr );
	unitobj.set( sKey::Z(), depthstr );
	obj.set( sKey::Units(), new OD::JSON::Object(unitobj) );
    }

    //Use OD::JSON::Object functions
    void fillRangesFromObj( const OD::JSON::Object& obj, SurveyInfo& si )
    {
	TrcKeyZSampling tkzs( false );
	//Inline range setting
	const auto* inlarr = obj.getArray( SurveyInfo::sKeyInlRange() );
	if ( inlarr )
	{
	    tkzs.hsamp_.start_.inl() = inlarr->getIntValue( 0 );
	    tkzs.hsamp_.stop_.inl() = inlarr->getIntValue( 1 );
	    tkzs.hsamp_.step_.inl() = inlarr->getIntValue( 2 );
	}

	//Crossline range setting
	const auto* crlarr = obj.getArray( SurveyInfo::sKeyCrlRange() );
	if ( crlarr )
	{
	    tkzs.hsamp_.start_.crl() = crlarr->getIntValue( 0 );
	    tkzs.hsamp_.stop_.crl() = crlarr->getIntValue( 1 );
	    tkzs.hsamp_.step_.crl() = crlarr->getIntValue( 2 );
	}

	//ZRange setting
	const auto* zobj = obj.getObject( SurveyInfo::sKeyZAxis() );
	if ( zobj )
	{
	    const BufferString domain = zobj->getStringValue(
						SurveyInfo::sKeyDomain() );
	    const bool isztime = domain.isEqual( sKey::Time() );
	    const bool isdepthfeet = domain.isEqual( "Feet" );
	    const auto* zarr = zobj->getArray( sKey::Range() );
	    if ( zarr )
	    {
		tkzs.zsamp_.start = zarr->getDoubleValue( 0 );
		tkzs.zsamp_.stop = zarr->getDoubleValue( 1 );
		tkzs.zsamp_.step = zarr->getDoubleValue( 2 );
		if ( Values::isUdf(tkzs.zsamp_.step)
		    || mIsZero(tkzs.zsamp_.step,mDefEps) )
		    tkzs.zsamp_.step = 0.004;

	    }

	    si.setZUnit( isztime, isdepthfeet );
	}

	si.setRange( tkzs, false );
    }


    void fillDirTransformFromObj( const OD::JSON::Object& obj, SurveyInfo& si )
    {
	const auto* transformobj = obj.getObject(
					    SurveyInfo::sKeyTransformation() );
	if ( !transformobj )
	    return;

	const auto* xarr = transformobj->getArray( sKeyXTransf );
	if ( xarr )
	    si.setTr( xarr->getDoubleValue(0), xarr->getDoubleValue(1),
				    xarr->getDoubleValue(2), true );

	const auto* yarr = transformobj->getArray( sKeyYTransf );
	if ( yarr )
	    si.setTr( yarr->getDoubleValue(0), yarr->getDoubleValue(1),
		yarr->getDoubleValue(2), false );
    }


    void fillSetPtsFromObj( const OD::JSON::Object& obj, SurveyInfo& si )
    {
	const auto* setptsarr = obj.getArray( SurveyInfo::sKeySetPt() );
	if ( !setptsarr )
	    return;

	BinID bids[2];
	Coord crds[3];
	int xline = 0;
	for ( int idx=0; idx<setptsarr->size(); idx++ )
	{
	    auto& ptobj = setptsarr->object( idx );
	    const auto* icarr = ptobj.getArray( SurveyInfo::sKeyIC() );
	    if ( icarr )
	    {
		if ( idx == 2 )
		    xline = icarr->getIntValue( 1 );
		else
		{
		    bids[idx].inl() = icarr->getIntValue( 0 );
		    bids[idx].crl() = icarr->getIntValue( 1 );
		}
	    }

	    const auto* xyarr = ptobj.getArray( SurveyInfo::sKeyXY() );
	    if ( xyarr )
	    {
		crds[idx].x = xyarr->getDoubleValue( 0 );
		crds[idx].y = xyarr->getDoubleValue( 1 );
	    }

	}

	si.set3PtsWithMsg( crds, bids, xline );
    }

    void setUnitsFromObj( const OD::JSON::Object& obj, SurveyInfo& si )
    {
	const OD::JSON::Object* unitsobj = obj.getObject( sKey::Units() );
	if ( !unitsobj )
	    return;

	const BufferString xystr =
			    unitsobj->getStringValue( SurveyInfo::sKeyXY() );
	if ( !xystr.isEmpty() )
	    si.setXYInFeet( xystr == getDistUnitString(true,false) );
    }
}


TrcKey Survey::Geometry3D::nearestTrace( const Coord& crd, float* dist ) const
{
    TrcKey tk( transform(crd) );
    if ( dist )
    {
	if ( sampling_.hsamp_.includes(tk.position()) )
	{
	    const Coord projcoord( transform(tk.position()) );
	    *dist = (float)projcoord.distTo( crd );
	}
	else
	{
	    TrcKey nearbid( sampling_.hsamp_.getNearest(tk.position()) );
	    const Coord nearcoord( transform(nearbid.position()) );
	    *dist = (float)nearcoord.distTo( crd );
	}
    }
    return tk;
}


bool Survey::Geometry3D::includes( int line, int tracenr ) const
{
    return sampling_.hsamp_.includes( BinID(line,tracenr) );
}


bool Survey::Geometry3D::isRightHandSystem() const
{
    const double xinl = b2c_.getTransform(true).b;
    const double xcrl = b2c_.getTransform(true).c;
    const double yinl = b2c_.getTransform(false).b;
    const double ycrl = b2c_.getTransform(false).c;

    const double det = xinl*ycrl - xcrl*yinl;
    return det < 0;
}


float Survey::Geometry3D::averageTrcDist() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c10 = transform( BinID(sampling_.hsamp_.step_.inl(),0) );
    const Coord c01 = transform( BinID(0,sampling_.hsamp_.step_.crl()) );
    return (float) ( c00.distTo(c10) + c00.distTo(c01) )/2;
}


BinID Survey::Geometry3D::transform( const Coord& c ) const
{
    return b2c_.transformBack( c, sampling_.hsamp_.start_,
				  sampling_.hsamp_.step_ );
}


Coord Survey::Geometry3D::transform( const BinID& b ) const
{
    return b2c_.transform(b);
}


const Survey::Geometry3D* Survey::Geometry::as3D() const
{
    return const_cast<Geometry*>( this )->as3D();
}


Survey::Geometry3D::Geometry3D( const char* nm, const ZDomain::Def& zd )
    : name_( nm )
    , zdomain_( zd )
{ sampling_.hsamp_.survid_ = OD::Geom3D; }


StepInterval<int> Survey::Geometry3D::inlRange() const
{ return sampling_.hsamp_.inlRange(); }


StepInterval<int> Survey::Geometry3D::crlRange() const
{ return sampling_.hsamp_.crlRange(); }


StepInterval<float> Survey::Geometry3D::zRange() const
{ return sampling_.zsamp_; }


int Survey::Geometry3D::inlStep() const
{ return sampling_.hsamp_.step_.inl(); }


int Survey::Geometry3D::crlStep() const
{ return sampling_.hsamp_.step_.crl(); }


float Survey::Geometry3D::zStep() const
{ return sampling_.zsamp_.step; }


static void doSnap( int& idx, int start, int step, int dir )
{
    if ( step < 2 ) return;
    int rel = idx - start;
    int rest = rel % step;
    if ( !rest ) return;

    idx -= rest;

    if ( !dir ) dir = rest > step / 2 ? 1 : -1;
    if ( rel > 0 && dir > 0 )	   idx += step;
    else if ( rel < 0 && dir < 0 ) idx -= step;
}


void Survey::Geometry3D::snap( BinID& binid, const BinID& rounding ) const
{
    const BinID& stp = sampling_.hsamp_.step_;
    if ( stp.inl() == 1 && stp.crl() == 1 ) return;
    doSnap( binid.inl(), sampling_.hsamp_.start_.inl(), stp.inl(),
	    rounding.inl() );
    doSnap( binid.crl(), sampling_.hsamp_.start_.crl(), stp.crl(),
	    rounding.crl() );
}

#define mSnapStep(ic) \
rest = s.ic % stp.ic; \
if ( rest ) \
{ \
int hstep = stp.ic / 2; \
bool upw = rounding.ic > 0 || (rounding.ic == 0 && rest > hstep); \
s.ic -= rest; \
if ( upw ) s.ic += stp.ic; \
}

void Survey::Geometry3D::snapStep( BinID& s, const BinID& rounding ) const
{
    const BinID& stp = sampling_.hsamp_.step_;
    if ( s.inl() < 0 ) s.inl() = -s.inl();
    if ( s.crl() < 0 ) s.crl() = -s.crl();
    if ( s.inl() < stp.inl() ) s.inl() = stp.inl();
    if ( s.crl() < stp.crl() ) s.crl() = stp.crl();
    if ( s == stp || (stp.inl() == 1 && stp.crl() == 1) )
	return;

    int rest;


    mSnapStep(inl())
    mSnapStep(crl())
}


void Survey::Geometry3D::snapZ( float& z, int dir ) const
{
    const StepInterval<float>& zrg = sampling_.zsamp_;
    const float eps = 1e-8;

    if ( z < zrg.start + eps )
    { z = zrg.start; return; }
    if ( z > zrg.stop - eps )
    { z = zrg.stop; return; }

    const float relidx = zrg.getfIndex( z );
    int targetidx = mNINT32(relidx);
    const float zdiff = z - zrg.atIndex( targetidx );
    if ( !mIsZero(zdiff,eps) && dir )
	targetidx = (int)( dir < 0 ? Math::Floor(relidx) : Math::Ceil(relidx) );
    z = zrg.atIndex( targetidx );;
    if ( z > zrg.stop - eps )
	z = zrg.stop;
}


void Survey::Geometry3D::setGeomData( const Pos::IdxPair2Coord& b2c,
				const TrcKeyZSampling& cs, float zscl )
{
    b2c_ = b2c;
    sampling_ = cs;
    zscale_ = zscl;
}


Coord3 Survey::Geometry3D::oneStepTranslation( const Coord3& planenormal ) const
{
    Coord3 translation( 0, 0, 0 );

    if ( fabs(planenormal.z) > 0.5 )
    {
	translation.z = zStep();
    }
    else
    {
	Coord norm2d = planenormal.coord();
	norm2d.normalize();

	if ( fabs(norm2d.dot(b2c_.inlDir())) > 0.5 )
	    translation.x = inlDistance();
	else
	    translation.y = crlDistance();
    }

    return translation;
}


float Survey::Geometry3D::inlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c10 = transform( BinID(1,0) );
    return (float) c00.distTo(c10);
}


float Survey::Geometry3D::crlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c01 = transform( BinID(0,1) );
    return (float) c00.distTo(c01);
}


Survey::Geometry::RelationType Survey::Geometry3D::compare(
				const Geometry& geom, bool usezrg ) const
{
    mDynamicCastGet( const Survey::Geometry3D*, geom3d, &geom );
    if ( !geom3d )
	return UnRelated;

    const bool havesametransform = b2c_ == geom3d->b2c_;
    if ( !havesametransform )
	return UnRelated;

    const StepInterval<int> myinlrg = inlRange();
    const StepInterval<int> mycrlrg = crlRange();
    const StepInterval<float> myzrg = zRange();
    const StepInterval<int> othinlrg = geom3d->inlRange();
    const StepInterval<int> othcrlrg = geom3d->crlRange();
    const StepInterval<float> othzrg = geom3d->zRange();
    if ( myinlrg == othinlrg && mycrlrg == othcrlrg &&
	    (!usezrg || myzrg.isEqual(othzrg,1e-3)) )
	return Identical;
    if ( myinlrg.includes(othinlrg) && mycrlrg.includes(othcrlrg) &&
	    (!usezrg || myzrg.includes(othzrg)) )
	return SuperSet;
    if ( othinlrg.includes(myinlrg) && othcrlrg.includes(mycrlrg) &&
	    (!usezrg || othzrg.includes(myzrg)) )
	return SubSet;

    return Related;
}


//==============================================================================


static ManagedObjectSet<SurveyInfo>* survinfostack = nullptr;

static void deleteSurveyStack()
{
    if ( survinfostack )
	survinfostack->setEmpty();
}

ManagedObjectSet<SurveyInfo>& survInfoStackMgr_()
{
    static PtrMan<ManagedObjectSet<SurveyInfo> > sistackmgr = nullptr;
    if ( !sistackmgr )
    {
	auto* newsistackmgr = new ManagedObjectSet<SurveyInfo>;
	if ( sistackmgr .setIfNull(newsistackmgr,true) )
	    NotifyExitProgram( &deleteSurveyStack );
    }
    return *sistackmgr.ptr();
}

ManagedObjectSet<SurveyInfo>& survInfoStack()
{
    if ( !survinfostack )
	survinfostack = &survInfoStackMgr_();
    return *survinfostack;
}


const SurveyInfo& SI()
{
    if ( survInfoStack().isEmpty() )
    {
	SurveyInfo* newsi = SurveyInfo::readDirectory( GetDataDir() );
	if ( !newsi )
	    newsi = new SurveyInfo;

	survInfoStack().add( newsi );
    }

    return *survInfoStack().last();
}


void SurveyInfo::pushSI( SurveyInfo* newsi )
{
    if ( newsi )
	survInfoStack().add( newsi );
    else
	pFreeFnErrMsg("Null survinfo pushed");
}


SurveyInfo* SurveyInfo::popSI()
{
    if ( !survInfoStack().isEmpty() )
	survInfoStack().pop();

    return nullptr;
}


mDefineInstanceCreatedNotifierAccess(SurveyInfo)

SurveyInfo::SurveyInfo()
    : tkzs_(*new TrcKeyZSampling(false))
    , wcs_(*new TrcKeyZSampling(false))
    , zdef_(*new ZDomain::Def(ZDomain::Time()) )
    , depthsinfeet_(false)
    , xyinfeet_(false)
    , pars_(*new IOPar(sKeySurvDefs))
    , ll2c_(*new LatLong2Coord)
    , workRangeChg(this)
    , survdatatype_(OD::Both2DAnd3D)
    , survdatatypeknown_(false)
    , seisrefdatum_(0.f)
    , coordsystem_(0)
{
    rdxtr_.b = rdytr_.c = 1;
    for ( int idx=0; idx<3; idx++ )
	set3binids_[idx].setUdf();

    // We need a 'reasonable' transform even when no proper SI is available
    // For example DataPointSets need to work
    Pos::IdxPair2Coord::DirTransform xtr, ytr;
    xtr.b = 1000; xtr.c = 0;
    ytr.b = 0; ytr.c = 1000;
    b2c_.setTransforms( xtr, ytr );
    tkzs_.hsamp_.survid_ = wcs_.hsamp_.survid_ = OD::Geom3D;

    mTriggerInstanceCreatedNotifier();
}



SurveyInfo::SurveyInfo( const SurveyInfo& si )
    : NamedCallBacker( si )
    , tkzs_(*new TrcKeyZSampling(false))
    , wcs_(*new TrcKeyZSampling(false))
    , pars_(*new IOPar(sKeySurvDefs))
    , zdef_(*new ZDomain::Def( si.zDomain() ) )
    , ll2c_(*new LatLong2Coord)
    , workRangeChg(this)
{
    *this = si;

    mTriggerInstanceCreatedNotifier();
}


SurveyInfo::~SurveyInfo()
{
    sendDelNotif();

    delete &pars_;
    delete &ll2c_;
    delete &tkzs_;
    delete &wcs_;
    delete &zdef_;

    Survey::Geometry3D* old = work_s3dgeom_.setToNull();
    if ( old ) old->unRef();

    old = s3dgeom_.setToNull();
    if ( old ) old->unRef();
}


SurveyInfo& SurveyInfo::operator =( const SurveyInfo& oth )
{
    if ( &oth == this )
	return *this;

    setName( oth.name() );
    zdef_ = oth.zdef_;
    disklocation_ = oth.disklocation_;
    coordsystem_ = oth.coordsystem_;
    xyinfeet_ = oth.xyinfeet_;
    depthsinfeet_ = oth.depthsinfeet_;
    b2c_ = oth.b2c_;
    survdatatype_ = oth.survdatatype_;
    survdatatypeknown_ = oth.survdatatypeknown_;
    for ( int idx=0; idx<3; idx++ )
    {
	set3binids_[idx] = oth.set3binids_[idx];
	set3coords_[idx] = oth.set3coords_[idx];
    }

    tkzs_ = oth.tkzs_;
    wcs_ = oth.wcs_;
    pars_ = oth.pars_;
    logpars_ = oth.logpars_;
    ll2c_ = oth.ll2c_;

    seisrefdatum_ = oth.seisrefdatum_;
    rdxtr_ = oth.rdxtr_;
    rdytr_ = oth.rdytr_;
    comment_ = oth.comment_;
    sipnm_ = oth.sipnm_;
    update3DGeometry();

    return *this;
}


bool SurveyInfo::operator==( const SurveyInfo& oth ) const
{
    if ( &oth == this )
	return true;

    if ( name() != oth.name() ||
	survdatatype_ != oth.survdatatype_ ||
	survdatatypeknown_ != oth.survdatatypeknown_ ||
	xyinfeet_ != oth.xyinfeet_ ||
	depthsinfeet_ != oth.depthsinfeet_ ||
	disklocation_ != oth.disklocation_ ||
	zdef_ != oth.zdef_ ||
	b2c_ != oth.b2c_ ||
	tkzs_ != oth.tkzs_ ||
	wcs_ != oth.wcs_ ||
	comment_ != oth.comment_ ||
	ll2c_ != oth.ll2c_ ||
	sipnm_ != oth.sipnm_ )
	return false;

    if ( (coordsystem_.ptr() && !oth.coordsystem_.ptr()) ||
	 (oth.coordsystem_.ptr() && !coordsystem_.ptr()) )
	return false;

    if ( coordsystem_.ptr() && *coordsystem_.ptr() !=
			       *oth.coordsystem_.ptr() )
	return false;

    if ( !mIsEqual(seisrefdatum_,oth.seisrefdatum_,1e-1) )
	return false;

    for ( int idx=0; idx<3; idx++ )
	if ( set3binids_[idx] != oth.set3binids_[idx] )
	    return false;
    for ( int idx=0; idx<3; idx++ )
	if ( set3coords_[idx] != oth.set3coords_[idx] )
	    return false;

    return mIsEqual(rdxtr_.a,oth.rdxtr_.a,mDefEps) &&
	   mIsEqual(rdxtr_.b,oth.rdxtr_.b,mDefEps) &&
	   mIsEqual(rdxtr_.c,oth.rdxtr_.c,mDefEps) &&
	   mIsEqual(rdytr_.a,oth.rdytr_.a,mDefEps) &&
	   mIsEqual(rdytr_.b,oth.rdytr_.b,mDefEps) &&
	   mIsEqual(rdytr_.c,oth.rdytr_.c,mDefEps) &&
	   pars_ == oth.pars_ &&
	   logpars_ == oth.logpars_;
}


bool SurveyInfo::operator!=( const SurveyInfo& oth ) const
{
    return !(*this == oth);
}


SurveyInfo& SurveyInfo::empty()
{
    return *new SurveyInfo;
}


SurveyInfo* SurveyInfo::read( const char* survdir )
{
    return readDirectory( survdir );
}


SurveyInfo* SurveyInfo::read( const char* path, bool pathisfile )
{
    return pathisfile ? readFile( path ) : readDirectory( path );
}


SurveyInfo* SurveyInfo::readDirectory( const char* loc )
{
    FilePath fp( loc );
    fp.add( sKeySetupFileName() );

    return readFile(fp.fullPath() );
}


SurveyInfo* SurveyInfo::readFile( const char* loc )
{
    FilePath fp( loc );
    const BufferString extension( fp.extension() );
    const bool isjson = extension.isEqual( "json", OD::CaseInsensitive );

    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(true) )
	return nullptr;

    PtrMan<SurveyInfo> si;
    uiRetVal ret;

    if ( isjson )
    {
	od_istream strm( fp.fullPath() );
	OD::JSON::Object obj;
	obj.read( strm );
	si = readJSON( obj, ret );
    }
    else
	si = readStrm( sfio.istrm(), ret );

    if ( ret.isError() )
    {
	sfio.closeFail();
	return nullptr;
    }

    sfio.closeSuccess();
    if ( !si->wrapUpRead() )
	return nullptr;

    return si.release();
}


SurveyInfo* SurveyInfo::readJSON( const OD::JSON::Object& obj, uiRetVal& ret )
{
    FilePath diskloc = obj.getFilePath( sKeySurvDiskLoc() );
    PtrMan<SurveyInfo> si = new SurveyInfo;
    si->disklocation_ = SurveyDiskLocation( diskloc );
    const BufferString surveynm = obj.getStringValue( sKey::Name() );
    si->setName( surveynm );
    auto* defobj = obj.getObject( sKeyDef() );
    if ( defobj )
    {
	si->getPars().useJSON( *defobj );
	si->getPars().setName( sKeySurvDefs );
	const FilePath fplog( diskloc.fullPath(), sKeyLogFile );
	si->getPars().removeWithKey( "Depth in feet" );
	IOPar& logpars = si->getLogPars();
	logpars.read( fplog.fullPath(), sKeySurvLog, true );
	logpars.setName( sKeySurvLog );
	if ( logpars.isEmpty() )
	    logpars.set( sKeyProjVersion(),
				    obj.getStringValue(sKey::Version()) );

	logpars.set( sKey::ModAt(), obj.getStringValue(sKey::ModAt()) );
    }

    const BufferString datatype = obj.getStringValue( sKeySurvDataType() );
    OD::Pol2D3D var;
    if ( !parseEnumPol2D3D(datatype,var) )
	var = OD::Both2DAnd3D;

    si->setSurvDataType( var );
    Survey::fillRangesFromObj( obj, *si );
    si->tkzs_.normalize();
    si->wcs_ = si->tkzs_;
    si->seisrefdatum_ = obj.getDoubleValue( sKeySeismicRefDatum() );
    if ( mIsUdf(si->seisrefdatum_) )
	si->seisrefdatum_ = 0.f;

    Survey::fillSetPtsFromObj( obj, *si );
    Survey::fillDirTransformFromObj( obj, *si );
    Survey::setUnitsFromObj( obj, *si );
    auto* crsobj = obj.getObject( sKeyCRS() );
    if ( crsobj )
    {
	IOPar crspar;
	crspar.useJSON( *crsobj );
	si->coordsystem_ = Coords::CoordSystem::createSystem( crspar );
    }

    if ( si->coordsystem_ )
	si->setXYInFeet( si->coordsystem_->isFeet() );
    else
    {
	const BufferString ll2cstr = obj.getStringValue( sKeyLatLongAnchor );
	si->ll2c_.fromString( ll2cstr );
	if ( si->ll2c_.isOK() )
	{
	    RefMan<Coords::AnchorBasedXY> anchoredsystem =
		new Coords::AnchorBasedXY( si->ll2c_.refLatLong(),
		    si->ll2c_.refCoord() );
	    anchoredsystem->setIsFeet( si->xyInFeet() );
	    si->coordsystem_ = anchoredsystem;
	}
	else
	{
	    RefMan<Coords::UnlocatedXY> undefsystem = new Coords::UnlocatedXY;
	    undefsystem->setIsFeet( si->xyInFeet() );
	    si->coordsystem_ = undefsystem;
	}
    }

    if ( si->zIsTime() )
    {
	bool depthinft = false;
	if ( si->xyInFeet() )
	    depthinft = true;
	else
	    si->getPars().getYN( sKeyDpthInFt(), depthinft );

	si->setDepthInFeet( depthinft );
    }

    const BufferString comments = obj.getStringValue( sKeyComments() );
    if ( !comments.isEmpty() )
	si->setComment( comments.str() );

    return si.release();
}


SurveyInfo* SurveyInfo::readStrm( od_istream& strm, uiRetVal& ret )
{
    ascistream astream( strm );
    FilePath fp( strm.fileName() );
    if ( !astream.isOfFileType(sKeySI) )
    {
	ret = tr("Survey definition file cannot be read.\n"
	    "Survey file '%1' has file type '%2'.\n"
	    "The file may be corrupt or not accessible.").
				arg(fp.fullPath()).arg(astream.fileType());
	ErrMsg( ret.getText() );
	return nullptr;
    }

    OD::JSON::Object obj;
    astream.next();
    FilePath fpsurvdir( fp.dirUpTo(fp.nrLevels()-2) );
    if ( fpsurvdir.isEmpty() )
    {
	ret = tr("Survey directory path is not valid or not specified");
	return nullptr;
    }

    obj.set( sKeySurvDiskLoc(), fpsurvdir );
    const FilePath fpdef( fpsurvdir.fullPath(), sKeyDefsFile );

    IOPar defpars;
    defpars.read( fpdef.fullPath(), sKeySurvDefs, true );
    defpars.removeWithKey( "Depth in feet" );
    OD::JSON::Object defobj;
    defpars.fillJSON( defobj );
    obj.set( sKeyDef(), new OD::JSON::Object(defobj) );
    obj.set( sKeyProjVersion(), astream.version() );
    obj.set( sKey::ModAt(), astream.timeStamp() );

    BufferString keyw = astream.keyWord();
    IOPar coordsystempar;
    TrcKeyZSampling samp( false );
    bool zistime = true;
    bool zisfeet = false;
    bool xyinfeet = false;
    TypeSet<float> dirxset;
    TypeSet<float> diryset;
    TypeSet<BinID> bids;
    TypeSet<Coord> crds;
    while ( !atEndOfSection(astream) )
    {
	keyw = astream.keyWord();
	if ( keyw == sKey::Name() )
	    obj.set( sKey::Name(), astream.value() );
	else if ( keyw == sKeyInlRange() )
	{
	    FileMultiString fms( astream.value() );
	    samp.hsamp_.start_.inl() = fms.getIValue( 0 );
	    samp.hsamp_.stop_.inl() = fms.getIValue( 1 );
	    samp.hsamp_.step_.inl() = fms.getIValue( 2 );
	}
	else if ( keyw == sKeyCrlRange() )
	{
	    FileMultiString fms( astream.value() );
	    samp.hsamp_.start_.crl() = fms.getIValue( 0 );
	    samp.hsamp_.stop_.crl() = fms.getIValue( 1 );
	    samp.hsamp_.step_.crl() = fms.getIValue( 2 );
	}
	else if ( keyw == sKeyZRange() )
	{
	    FileMultiString fms( astream.value() );
	    samp.zsamp_.start = fms.getFValue( 0 );
	    samp.zsamp_.stop = fms.getFValue( 1 );
	    samp.zsamp_.step = fms.getFValue( 2 );
	    if ( Values::isUdf(samp.zsamp_.step)
	      || mIsZero(samp.zsamp_.step,mDefEps) )
		samp.zsamp_.step = 0.004;
	    if ( fms.size() > 3 )
	    {
		if ( *fms[3] == 'T' )
		    zistime = true;
		else
		{
		    zistime = false;
		    zisfeet = *fms[3] == 'F';
		}
	    }
	}
	else if ( keyw == sKeySurvDataType() )
	    obj.set( sKeySurvDataType(), astream.value() );
	else if ( keyw == sKeyXYInFt() )
	    xyinfeet = astream.getYN();
	else if ( keyw == sKeySeismicRefDatum() )
	{
	    float refval = astream.getFValue();
	    /*if ( zisfeet )
		refval *= mToFeetFactorF;*/

	    obj.set( sKeySeismicRefDatum(), refval );
	}
	else if ( keyw == sKeyXTransf )
	{
	    FileMultiString fms( astream.value() );
	    dirxset.add( fms.getDValue(0) ).add( fms.getDValue(1) )
						    .add( fms.getDValue(2) );
	}
	else if ( keyw == sKeyYTransf )
	{
	    FileMultiString fms( astream.value() );
	    diryset.add( fms.getDValue(0) ).add( fms.getDValue(1) )
		.add( fms.getDValue(2) );
	}
	else if ( keyw.startsWith(sKey::CoordSys()) )
	    coordsystempar.add( keyw, astream.value() );
	else if ( keyw == sKeyLatLongAnchor )
	    obj.set( sKeyLatLongAnchor, astream.value() );
	else if ( keyw.startsWith(sKeySetPt()) )
	{
	    const char* ptr = firstOcc( (const char*)keyw, '.' );
	    if ( !ptr )
		return nullptr;

	    int ptidx = toInt( ptr + 1 ) - 1;
	    if ( ptidx < 0 )
		ptidx = 0;

	    if ( ptidx > 3 )
		ptidx = 2;

	    FileMultiString fms( astream.value() );
	    if ( fms.size() < 2 )
		return nullptr;

	    BinID bid;
	    bid.fromString( fms[0] );
	    bids.add( bid );
	    Coord crd;
	    crd.fromString( fms[1] );
	    crds.add( crd );
	}

	astream.next();
    }

    Survey::fillObjFromRanges( samp, zistime, zisfeet, obj );
    Survey::fillObjWithDirTransform( dirxset, diryset, obj );
    if ( !bids.isEmpty() && !crds.isEmpty() )
	Survey::fillObjWithSetPts( bids, crds, obj );

    PtrMan<IOPar> coordsyssubpar =
	coordsystempar.subselect( sKey::CoordSys() );
    if ( coordsyssubpar )
    {
	OD::JSON::Object crsobj;
	coordsyssubpar->fillJSON( crsobj );
	obj.set( sKeyCRS(), new OD::JSON::Object(crsobj) );
    }

    const BufferString zstr = zistime ? getTimeUnitString( true, false ) :
					getDistUnitString( zisfeet, false );
    Survey::fillObjWithUnit( getDistUnitString(xyinfeet,false), zstr, obj );
    BufferString line;
    BufferString comment;
    while ( astream.stream().getLine(line) )
    {
	if ( !comment.isEmpty() )
	    comment += "\n";

	comment += line;
    }

    if ( !comment.isEmpty() )
	obj.set( sKeyComments(), comment );

    return readJSON( obj, ret );
}


bool SurveyInfo::wrapUpRead()
{
    if ( set3binids_[2].isUdf() )
	get3Pts( set3coords_, set3binids_, set3binids_[2].crl() );

    b2c_.setTransforms( rdxtr_, rdytr_ );
    if ( !b2c_.isValid() )
    {
	BufferString errmsg( "Survey ", name() );
	errmsg.add( " has an invalid coordinate transformation" );
	ErrMsg( errmsg );
	return false;
    }

    return true;
}


void SurveyInfo::handleLineRead( const BufferString& keyw, const char* val )
{
    if ( keyw == sKeyXTransf )
	setTr( rdxtr_, val );
    else if ( keyw == sKeyYTransf )
	setTr( rdytr_, val );
    else if ( keyw == sKeyLatLongAnchor )
	ll2c_.fromString( val );
    else if ( keyw.startsWith(sKeySetPt()) )
    {
	const char* ptr = firstOcc( (const char*)keyw, '.' );
	if ( !ptr )
	    return;

	int ptidx = toInt( ptr + 1 ) - 1;
	if ( ptidx < 0 )
	    ptidx = 0;

	if ( ptidx > 3 )
	    ptidx = 2;

	FileMultiString fms( val );
	if ( fms.size() < 2 )
	    return;

	set3binids_[ptidx].fromString( fms[0] );
	set3coords_[ptidx].fromString( fms[1] );
    }
}


void SurveyInfo::updateDirName()
{
    if ( name_.isEmpty() )
	return;

    BufferString dirnm( name() );
    dirnm.clean( BufferString::AllowDots );
    disklocation_.setDirName( dirnm );
}


StepInterval<int> SurveyInfo::inlRange( bool work ) const
{
    StepInterval<int> ret; Interval<int> dum;
    sampling(work).hsamp_.get( ret, dum );
    return ret;
}


StepInterval<int> SurveyInfo::crlRange( bool work ) const
{
    StepInterval<int> ret; Interval<int> dum;
    sampling(work).hsamp_.get( dum, ret );
    return ret;
}


const StepInterval<float>& SurveyInfo::zRange( bool work ) const
{
    return sampling(work).zsamp_;
}

int SurveyInfo::maxNrTraces( bool work ) const
{
    return sampling(work).hsamp_.nrInl() * sampling(work).hsamp_.nrCrl();
}


int SurveyInfo::inlStep() const
{
    return tkzs_.hsamp_.step_.inl();
}
int SurveyInfo::crlStep() const
{
    return tkzs_.hsamp_.step_.crl();
}


float SurveyInfo::inlDistance() const
{
    return get3DGeometry(false)->inlDistance();
}


float SurveyInfo::crlDistance() const
{
    return get3DGeometry(false)->crlDistance();
}


float SurveyInfo::getArea( const Interval<int>& inlrg,
			       const Interval<int>& crlrg ) const
{
    const BinID step = sampling(false).hsamp_.step_;
    const Coord c00 = transform( BinID(inlrg.start,crlrg.start) );
    const Coord c01 = transform( BinID(inlrg.start,crlrg.stop+step.crl()) );
    const Coord c10 = transform( BinID(inlrg.stop+step.inl(),crlrg.start) );

    const double d01 = c00.distTo( c01 );
    const double d10 = c00.distTo( c10 );

    return sCast(float,d01*d10);
}


float SurveyInfo::getArea( bool work ) const
{
    return getArea( inlRange( work ), crlRange( work ) );
}


float SurveyInfo::zStep() const
{ return tkzs_.zsamp_.step; }


int SurveyInfo::nrZDecimals() const
{
    return zDomain().nrZDecimals( zStep() );
}


int SurveyInfo::nrXYDecimals() const
{
    int nrdec = 2;
    getPars().get( "Nr XY decimals", nrdec );
    return nrdec;
}


Coord3 SurveyInfo::oneStepTranslation( const Coord3& planenormal ) const
{
    return get3DGeometry(false)->oneStepTranslation( planenormal );
}


void SurveyInfo::setRange( const TrcKeyZSampling& cs, bool work )
{
    if ( work )
    {
	wcs_ = cs;
	return;
    }
    else
	tkzs_ = cs;

    tkzs_.hsamp_.survid_ = wcs_.hsamp_.survid_ = OD::Geom3D;
    if ( wcs_.isDefined() )
	wcs_.limitTo( tkzs_ );
    else
	wcs_ = tkzs_;

    wcs_.hsamp_.step_ = tkzs_.hsamp_.step_;
    wcs_.zsamp_.step = tkzs_.zsamp_.step;
    update3DGeometry();
}


bool SurveyInfo::isWorkRangeSet() const
{
    return wcs_.isDefined() && wcs_ != tkzs_;
}


void SurveyInfo::setWorkRange( const TrcKeyZSampling& cs )
{
    setRange( cs, true);
    workRangeChg.trigger();
}


Interval<int> SurveyInfo::reasonableRange( bool inl ) const
{
    const Interval<int> rg = inl
      ? Interval<int>( tkzs_.hsamp_.start_.inl(), tkzs_.hsamp_.stop_.inl() )
      : Interval<int>( tkzs_.hsamp_.start_.crl(), tkzs_.hsamp_.stop_.crl() );

    const int w = rg.stop - rg.start;

    return Interval<int>( rg.start - 3*w, rg.stop +3*w );
}


bool SurveyInfo::isReasonable( const BinID& b ) const
{
    return reasonableRange( true ).includes( b.inl(),false ) &&
	   reasonableRange( false ).includes( b.crl(),false );
}


bool SurveyInfo::isReasonable( const Coord& crd ) const
{
    if ( Values::isUdf(crd.x) || Values::isUdf(crd.y) )
	return false;

    return isReasonable( transform(crd) );
}


#define mChkCoord(c) \
    if ( c.x < minc.x ) minc.x = c.x; if ( c.y < minc.y ) minc.y = c.y;

Coord SurveyInfo::minCoord( bool work ) const
{
    const TrcKeyZSampling& cs = sampling(work);
    Coord minc = transform( cs.hsamp_.start_ );
    Coord c = transform( cs.hsamp_.stop_ );
    mChkCoord(c)
    BinID bid( cs.hsamp_.start_.inl(), cs.hsamp_.stop_.crl() );
    c = transform( bid );
    mChkCoord(c)
    bid = BinID( cs.hsamp_.stop_.inl(), cs.hsamp_.start_.crl() );
    c = transform( bid );
    mChkCoord(c)
    return minc;
}


#undef mChkCoord
#define mChkCoord(c) \
    if ( c.x > maxc.x ) maxc.x = c.x; if ( c.y > maxc.y ) maxc.y = c.y;

Coord SurveyInfo::maxCoord( bool work ) const
{
    const TrcKeyZSampling& cs = sampling(work);
    Coord maxc = transform( cs.hsamp_.start_ );
    Coord c = transform( cs.hsamp_.stop_ );
    mChkCoord(c)
    BinID bid( cs.hsamp_.start_.inl(), cs.hsamp_.stop_.crl() );
    c = transform( bid );
    mChkCoord(c)
    bid = BinID( cs.hsamp_.stop_.inl(), cs.hsamp_.start_.crl() );
    c = transform( bid );
    mChkCoord(c)
    return maxc;
}


void SurveyInfo::checkInlRange( Interval<int>& intv, bool work ) const
{
    const TrcKeyZSampling& cs = sampling(work);
    intv.sort();
    if ( intv.start < cs.hsamp_.start_.inl() )
	intv.start = cs.hsamp_.start_.inl();
    if ( intv.start > cs.hsamp_.stop_.inl() )
	intv.start = cs.hsamp_.stop_.inl();
    if ( intv.stop > cs.hsamp_.stop_.inl() )
	intv.stop = cs.hsamp_.stop_.inl();
    if ( intv.stop < cs.hsamp_.start_.inl() )
	intv.stop = cs.hsamp_.start_.inl();

    BinID bid( intv.start, 0 );
    snap( bid, BinID(1,1) );
    intv.start = bid.inl();

    bid.inl() = intv.stop;
    snap( bid, BinID(-1,-1) );
    intv.stop = bid.inl();
}


void SurveyInfo::checkCrlRange( Interval<int>& intv, bool work ) const
{
    const TrcKeyZSampling& cs = sampling(work);
    intv.sort();
    if ( intv.start < cs.hsamp_.start_.crl() )
	intv.start = cs.hsamp_.start_.crl();
    if ( intv.start > cs.hsamp_.stop_.crl() )
	intv.start = cs.hsamp_.stop_.crl();
    if ( intv.stop > cs.hsamp_.stop_.crl() )
	intv.stop = cs.hsamp_.stop_.crl();
    if ( intv.stop < cs.hsamp_.start_.crl() )
	intv.stop = cs.hsamp_.start_.crl();

    BinID bid( 0, intv.start );
    snap( bid, BinID(1,1) );
    intv.start = bid.crl();

    bid.crl() = intv.stop;
    snap( bid, BinID(-1,-1) );
    intv.stop = bid.crl();
}


void SurveyInfo::checkZRange( Interval<float>& intv, bool work ) const
{
    const StepInterval<float>& rg = sampling(work).zsamp_;
    intv.sort();
    if ( intv.start < rg.start )
	intv.start = rg.start;
    if ( intv.start > rg.stop )
	intv.start = rg.stop;
    if ( intv.stop > rg.stop )
	intv.stop = rg.stop;
    if ( intv.stop < rg.start )
	intv.stop = rg.start;

    snapZ( intv.start, 1 );
    snapZ( intv.stop, -1 );
}


bool SurveyInfo::includes( const BinID& bid, const float z, bool work ) const
{
    const TrcKeyZSampling& cs = sampling(work);
    const float eps = 1e-8f;
    return cs.hsamp_.includes( bid )
	&& cs.zsamp_.start < z + eps && cs.zsamp_.stop > z - eps;
}


bool SurveyInfo::zIsTime() const
{
    return zdef_.isTime();
}


SurveyInfo::Unit SurveyInfo::xyUnit() const
{
    return xyinfeet_ ? Feet : Meter;
}


SurveyInfo::Unit SurveyInfo::zUnit() const
{
    if ( zIsTime() )
	return Second;

    return depthsinfeet_ ? Feet : Meter;
}


ZDomain::DepthType SurveyInfo::depthType() const
{
    return depthsInFeet() ? ZDomain::DepthType::Feet
			  : ZDomain::DepthType::Meter;
}


void SurveyInfo::putZDomain( IOPar& iop ) const
{
    zdef_.set( iop );
}


const ZDomain::Def& SurveyInfo::zDomain() const
{ return zdef_; }


const ZDomain::Info& SurveyInfo::zDomainInfo() const
{
    return zIsTime() ? ZDomain::TWT()
		     : (zInMeter() ? ZDomain::DepthMeter()
				   : ZDomain::DepthFeet() );
}


const char* SurveyInfo::getXYUnitString( bool wb ) const
{
    return getDistUnitString( xyInFeet(), wb );
}


uiString SurveyInfo::getUiXYUnitString( bool abbrvt, bool wb ) const
{
    return uiStrings::sDistUnitString( xyInFeet(), abbrvt, wb );
}


void SurveyInfo::setZUnit( bool istime, bool infeet )
{
    zdef_ = istime ? ZDomain::Time() : ZDomain::Depth();
    setDepthInFeet( infeet );
}


float SurveyInfo::defaultXYtoZScale( Unit zunit, Unit xyunit )
{
    if ( zunit==xyunit )
	return 1;

    if ( zunit==Second )
    {
	if ( xyunit==Meter )
	    return 1000;

	//xyunit==feet
	return 3048;
    }
    else if ( zunit==Feet && xyunit==Meter )
	return mFromFeetFactorF;

    //  zunit==Meter && xyunit==Feet
    return mToFeetFactorF;
}


float SurveyInfo::zScale() const
{
    return defaultXYtoZScale( zUnit(), xyUnit() );
}


Coord SurveyInfo::transform( const BinID& b ) const
{ return get3DGeometry(false)->transform( b ); }


BinID SurveyInfo::transform( const Coord& c ) const
{ return get3DGeometry(false)->transform( c ); }


void SurveyInfo::get3Pts( Coord c[3], BinID b[2], int& xline ) const
{
    const int firstinl = set3binids_[0].inl();
    if ( !mIsUdf(firstinl) )
    {
	b[0] = set3binids_[0]; c[0] = set3coords_[0];
	b[1] = set3binids_[1]; c[1] = set3coords_[1];
	c[2] = set3coords_[2]; xline = set3binids_[2].crl();
    }
    else
    {
	b[0] = tkzs_.hsamp_.start_; c[0] = transform( b[0] );
	b[1] = tkzs_.hsamp_.stop_; c[1] = transform( b[1] );
	BinID b2 = tkzs_.hsamp_.stop_; b2.inl() = b[0].inl();
	c[2] = transform( b2 ); xline = b2.crl();
    }
}


const char* SurveyInfo::set3Pts( const Coord c[3], const BinID b[2],
				 int xline )
{
    mDeclStaticString( ret );
    const uiString msg = set3PtsWithMsg( c, b, xline );
    ret.set( ::toString(msg) );
    return ret.buf();
}


uiString SurveyInfo::set3PtsWithMsg( const Coord c[3], const BinID b[2],
				     int xline )
{
    if ( b[1].inl() == b[0].inl() )
	return tr("Need two different in-lines");
    if ( b[0].crl() == xline )
	return tr("No Cross-line range present");

    if ( !b2c_.set3Pts( c[0], c[1], c[2], b[0], b[1], xline ) )
	return tr("Cannot construct a valid transformation matrix from this "
         "input.\nPlease check whether the data is on a single straight line.");

    set3binids_[0] = b[0];
    set3binids_[1] = b[1];
    set3binids_[2] = BinID( b[0].inl(), xline );
    set3coords_[0] = c[0];
    set3coords_[1] = c[1];
    set3coords_[2] = c[2];

    update3DGeometry();

    return uiString::empty();
}


void SurveyInfo::gen3Pts()
{
    set3binids_[0] = tkzs_.hsamp_.start_;
    set3binids_[1] = tkzs_.hsamp_.stop_;
    set3binids_[2] = BinID( tkzs_.hsamp_.start_.inl(),
			    tkzs_.hsamp_.stop_.crl() );
    set3coords_[0] = transform( set3binids_[0] );
    set3coords_[1] = transform( set3binids_[1] );
    set3coords_[2] = transform( set3binids_[2] );
}


void SurveyInfo::snap( BinID& binid, const BinID& rounding ) const
{
    RefMan<Survey::Geometry3D> geom = get3DGeometry( false );
    geom->snap( binid, rounding );
}


void SurveyInfo::snapStep( BinID& step, const BinID& rounding ) const
{
    RefMan<Survey::Geometry3D> geom = get3DGeometry( true );
    geom->snapStep( step, rounding );
}




void SurveyInfo::snapZ( float& z, int dir ) const
{
    RefMan<Survey::Geometry3D> geom = get3DGeometry( true );
    geom->snapZ( z, dir );
}


void SurveyInfo::setTr( Pos::IdxPair2Coord::DirTransform& tr, const char* str )
{
    FileMultiString fms( str );
    tr.a = fms.getDValue(0); tr.b = fms.getDValue(1); tr.c = fms.getDValue(2);
}


void SurveyInfo::setTr( double a, double b, double c, bool isx )
{
    Pos::IdxPair2Coord::DirTransform& dirtr = isx ? rdxtr_ : rdytr_;
    dirtr.a = a;
    dirtr.b = b;
    dirtr.c = c;
}


void SurveyInfo::putTr( const Pos::IdxPair2Coord::DirTransform& dirtr,
			  ascostream& astream, const char* key ) const
{
    char buf[1024];
    od_sprintf( buf, 1024, "%.10lg`%.10lg`%.10lg", dirtr.a, dirtr.b, dirtr.c );
    astream.put( key, buf );
}


bool SurveyInfo::isRightHandSystem() const
{
    return get3DGeometry(false)->isRightHandSystem();
}


bool SurveyInfo::write( const char* basedir ) const
{
    return writeSurveyFile( basedir, false );
}


bool SurveyInfo::writeJSON( const char* basedir ) const
{
    return writeSurveyFile( basedir, true );
}


bool SurveyInfo::writeSurveyFile( const char* basedir, bool isjson ) const
{
    if ( !basedir || !*basedir )
	basedir = GetBaseDataDir();

    FilePath fp( basedir, disklocation_.dirName(), sKeySetupFileName() );
    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot open survey info file for write!" );
	if ( *sfio.errMsg() )
	{
	    msg += "\n\t";
	    msg += sfio.errMsg();
	}

	ErrMsg( msg );
	return false;
    }

    od_ostream& strm = sfio.ostrm();
    OD::JSON::Object* jsonobj( nullptr );
    if ( isjson )
    {
	jsonobj = new OD::JSON::Object();
	jsonobj->set( sKey::Name(), name() );
	jsonobj->set( sKeySurvDataType(), getPol2D3DString(survDataType()) );

	Survey::fillObjFromRanges( tkzs_, zIsTime(), zInFeet(), *jsonobj );

	if ( !comment_.isEmpty() )
	    jsonobj->set( sKeyComments(), comment_ );

	writeSpecLines( nullptr, jsonobj );
    }
    else
    {
	ascostream astream( strm );
	if ( !astream.putHeader(sKeySI) )
	{
	    ErrMsg( "Cannot write to survey info file!" );
	    return false;
	}

	astream.put( sKey::Name(), name() );
	astream.put( sKeySurvDataType(), getPol2D3DString(survDataType()) );
	FileMultiString fms;
	fms += tkzs_.hsamp_.start_.inl(); fms += tkzs_.hsamp_.stop_.inl();
				    fms += tkzs_.hsamp_.step_.inl();
	astream.put( sKeyInlRange(), fms );
	fms = "";

	fms += tkzs_.hsamp_.start_.crl(); fms += tkzs_.hsamp_.stop_.crl();
				    fms += tkzs_.hsamp_.step_.crl();
	astream.put( sKeyCrlRange(), fms );

	fms = ""; fms += tkzs_.zsamp_.start; fms += tkzs_.zsamp_.stop;
	fms += tkzs_.zsamp_.step;
	fms += zIsTime() ? "T" : ( depthsInFeet() ? "F" : "D" );
	astream.put( sKeyZRange(), fms );

	writeSpecLines( &astream );
	astream.newParagraph();
	const char* ptr = (const char*)comment_;
	if ( *ptr )
	{
	    while ( 1 )
	    {
		char* nlptr = const_cast<char*>( firstOcc( ptr, '\n' ) );
		if ( !nlptr )
		{
		    if ( *ptr )
			strm << ptr;
		    break;
		}
		*nlptr = '\0';
		strm << ptr << '\n';
		*nlptr = '\n';
		ptr = nlptr + 1;
	    }
	    strm << '\n';
	}

	if ( !strm.isOK() )
	{
	    sfio.closeFail();
	    BufferString msg( "Error during write of survey info file!" );
	    msg += strm.errMsg().getFullString();
	    ErrMsg( msg );
	    return false;
	}
	else if ( !sfio.closeSuccess() )
	{
	    BufferString msg( "Error closing survey info file:\n" );
	    msg += sfio.errMsg();
	    ErrMsg( msg );
	    return false;
	}
    }

    fp.set( basedir );
    fp.add( disklocation_.dirName() );
    if ( isjson )
	saveJSON( fp.fullPath(), jsonobj );
    else
	savePars( fp.fullPath() );

    saveLog( fp.fullPath() );
    if ( isjson )
    {
	const uiRetVal ret = jsonobj->write( strm, true );
	if ( !sfio.closeSuccess() || ret.isError() )
	{
	    BufferString msg( "Error closing survey info file:\n" );
	    msg += ret.isEmpty() ? sfio.errMsg() : ret.getText().buf();
	    ErrMsg( msg );
	}
    }

    return true;
}


void SurveyInfo::writeSpecLines( ascostream* astream,
						OD::JSON::Object* obj ) const
{
    if ( astream )
    {
	putTr( b2c_.getTransform(true), *astream, sKeyXTransf );
	putTr( b2c_.getTransform(false), *astream, sKeyYTransf );
    }
    else if ( obj )
	Survey::fillObjWithDirTransform( b2c_, *obj );

    FileMultiString fms;
    OD::JSON::Array setarr( true );
    TypeSet<BinID> bids;
    TypeSet<Coord> crds;
    for ( int idx=0; idx<3; idx++ )
    {
	BinID bid = set3binids_[idx];
	Coord crd = set3coords_[idx];
	if ( obj )
	{
	    bids.add( bid );
	    crds.add( crd );
	}
	else if ( astream )
	{
	    SeparString ky( sKeySetPt(), '.' );
	    ky += idx + 1;
	    fms = set3binids_[idx].toString();
	    fms += set3coords_[idx].toString();
	    astream->put( ky.buf(), fms.buf() );
	}

    }

    if ( coordsystem_ )
    {
	IOPar par;
	coordsystem_->fillPar( par );
	IOParIterator iter( par );
	if ( obj )
	{
	    par.removeWithKey( "XY in Feet" );
	    OD::JSON::Object crsobj;
	    par.fillJSON( crsobj );
	    obj->set( sKeyCRS(), new OD::JSON::Object(crsobj) );
	}
	else if ( astream )
	{
	    BufferString key, val, buf;
	    while ( iter.next(key,val) )
		astream->put( IOPar::compKey(sKey::CoordSys(),key), val );
	}
    }
    else if ( !obj )
	astream->putYN( sKeyXYInFt(), xyInFeet() );

    if ( obj )
    {
	Survey::fillObjWithSetPts( bids, crds, *obj );
	float seisrefval = seisrefdatum_;
	/*if ( zInFeet() )
	    seisrefval *= mToFeetFactorF;*/

	obj->set( sKeySeismicRefDatum(), seisrefval );
	Survey::fillObjWithUnit( getXYUnitString(false),
			    getUiZUnitString(false).getFullString(), *obj );
	if ( ll2c_.isOK() )
	    obj->set( sKeyLatLongAnchor, ll2c_.toString() );

    }
    else if ( astream )
    {
	astream->put( sKeySeismicRefDatum(), seisrefdatum_ );
	if ( ll2c_.isOK() )
	    astream->put( sKeyLatLongAnchor, ll2c_.toString() );
    }
}


#define uiErrMsg(s) { \
    BufferString msg( "Could not write to " ); \
    msg += s; \
    if ( File::isHidden(s) ) \
	msg += ". This is a hidden file"; \
    else \
	msg += " Please check the file permission"; \
    OD::DisplayErrorMessage( msg ); \
    return; \
}


void SurveyInfo::savePars( const char* basedir ) const
{
    return saveJSON( basedir, nullptr );
}


void SurveyInfo::saveJSON( const char* basedir, OD::JSON::Object* obj  ) const
{
    if ( obj )
    {
	OD::JSON::Object defobj;
	pars_.fillJSON( defobj );
	obj->set( sKeyDef(), new OD::JSON::Object(defobj) );
	return;
    }

    BufferString surveypath;
    if ( !basedir || !*basedir )
    {
	const BufferString storepath = disklocation_.fullPath();
	surveypath = File::exists(storepath) ? storepath.buf() : GetDataDir();
    }
    else
	surveypath = basedir;

    const BufferString defsfnm( FilePath(surveypath,sKeyDefsFile).fullPath() );
    if ( pars_.isEmpty() )
    {
	if ( File::exists(defsfnm) )
	    File::remove(defsfnm);
    }
    else
    {
	const bool iswritablesurvey = File::exists(surveypath.buf()) &&
				      File::isWritable(surveypath.buf());
	const bool res = pars_.write( defsfnm, sKeySurvDefs );
	if ( iswritablesurvey && !res )
	    uiErrMsg(defsfnm)
    }
}


void SurveyInfo::saveLog( const char* basedir ) const
{
    BufferString surveypath;
    if ( !basedir || !*basedir )
    {
	const BufferString storepath = disklocation_.fullPath();
	surveypath = File::exists(storepath) ? storepath.buf() : GetDataDir();
    }
    else
	surveypath = basedir;

    const BufferString logfnm( FilePath(surveypath,sKeyLogFile).fullPath() );
    if ( logpars_.isEmpty() )
	return;

    if ( !logpars_.write(logfnm,sKeySurvLog) )
	uiErrMsg( logfnm )
}


bool SurveyInfo::has2D() const
{
    return survdatatype_ == OD::Only2D || survdatatype_ == OD::Both2DAnd3D;
}


bool SurveyInfo::has3D() const
{
    return survdatatype_ == OD::Only3D || survdatatype_ == OD::Both2DAnd3D;
}


void SurveyInfo::update3DGeometry()
{
    if ( s3dgeom_ )
	s3dgeom_->setGeomData( b2c_, sampling(false), zScale() );

    if ( work_s3dgeom_ )
	work_s3dgeom_->setGeomData( b2c_, sampling(true), zScale() );
}


RefMan<Survey::Geometry3D> SurveyInfo::get3DGeometry( bool work ) const
{
    Threads::AtomicPointer<Survey::Geometry3D>& sgeom
			= work ? work_s3dgeom_ : s3dgeom_;

    if ( !sgeom )
    {
	RefMan<Survey::Geometry3D> newsgeom
			= new Survey::Geometry3D( name(), zdef_ );
	newsgeom->setID( Survey::default3DGeomID() );
	newsgeom->setGeomData( b2c_, sampling(work), zScale() );
	if ( sgeom.setIfEqual(0,newsgeom) )
	    newsgeom.release();
    }

    return RefMan<Survey::Geometry3D>( sgeom );
}


float SurveyInfo::angleXInl() const
{
    Coord xy1 = transform( BinID(inlRange(false).start, crlRange(false).start));
    Coord xy2 = transform( BinID(inlRange(false).start, crlRange(false).stop) );
    const double xdiff = xy2.x - xy1.x;
    const double ydiff = xy2.y - xy1.y;
    return sCast(float, Math::Atan2( ydiff, xdiff ) );
}


float SurveyInfo::angleXCrl() const
{
    Coord xy1 = transform( BinID(inlRange(false).start, crlRange(false).start));
    Coord xy2 = transform( BinID(inlRange(false).stop, crlRange(false).start) );
    const double xdiff = xy2.x - xy1.x;
    const double ydiff = xy2.y - xy1.y;
    return sCast(float, Math::Atan2( ydiff, xdiff ) );
}


bool SurveyInfo::isInside( const BinID& bid, bool work ) const
{
    const Interval<int> inlrg( inlRange(work) );
    const Interval<int> crlrg( crlRange(work) );
    return inlrg.includes(bid.inl(),false) && crlrg.includes(bid.crl(),false);
}


RefMan<Coords::CoordSystem> SurveyInfo::getCoordSystem()
{
    return coordsystem_;
}


ConstRefMan<Coords::CoordSystem> SurveyInfo::getCoordSystem() const
{
    return ConstRefMan<Coords::CoordSystem>( coordsystem_.ptr() );
}


bool SurveyInfo::hasProjection() const
{
    return coordsystem_ && coordsystem_->isProjection();
}


bool SurveyInfo::xyInFeet() const
{
    return mXYInFeet();
}


bool SurveyInfo::setCoordSystem( Coords::CoordSystem* system )
{
    if ( system && !system->isOrthogonal() )
	return false;

    coordsystem_ = system;
    return false;
}


void SurveyInfo::readSavedCoordSystem() const
{
    const FilePath fp( disklocation_.fullPath(), sKeySetupFileName() );
    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(true) )
	return;

    ascistream astream( sfio.istrm() );
    if ( !astream.isOfFileType(sKeySI) )
    { sfio.closeSuccess(); return; }

    astream.next();
    const IOPar survpar( astream );

    PtrMan<IOPar> coordsystempar = survpar.subselect( sKey::CoordSys() );
    if ( !coordsystempar )
	coordsystempar = pars_.subselect( sKey::CoordSys() );
    if ( coordsystempar )
	const_cast<SurveyInfo*>(this)->coordsystem_ =
		Coords::CoordSystem::createSystem( *coordsystempar );

    sfio.closeSuccess();
}

#define mErrRetDoesntExist(fnm) \
    { ret.add( uiStrings::phrFileDoesNotExist(fnm) ); return ret; }

uiRetVal SurveyInfo::isValidDataRoot( const char* inpdirnm )
{
    uiRetVal ret;

    FilePath fp( inpdirnm ? inpdirnm : GetBaseDataDir() );
    const BufferString dirnm( fp.fullPath() );
    if ( !File::isDirectory(dirnm) || !File::isWritable(dirnm) )
	mErrRetDoesntExist( dirnm );

    fp.add( ".omf" );
    const BufferString omffnm( fp.fullPath() );
    if ( !File::exists(omffnm) )
	mErrRetDoesntExist( omffnm );

    fp.setFileName( SurveyInfo::sKeySetupFileName() );
    if ( File::exists(fp.fullPath()) )
    {
	// probably we're in a survey. So let's check:
	fp.setFileName( "Misc" );
	if ( File::isDirectory(fp.fullPath()) )
	    ret.add( tr("'%1' has '%2' file").arg(dirnm)
		    .arg(SurveyInfo::sKeySetupFileName()) );
    }

    return ret;
}


uiRetVal SurveyInfo::isValidSurveyDir( const char* dirnm )
{
    uiRetVal ret;

    FilePath fp( dirnm, ".omf" );
    const BufferString omffnm( fp.fullPath() );
    if ( !File::exists(omffnm) )
	mErrRetDoesntExist( omffnm );

    fp.setFileName( sSurvFile );
    const BufferString survfnm( fp.fullPath() );
    if ( !File::exists(survfnm) )
	mErrRetDoesntExist( survfnm );

    fp.setFileName( sSeismicSubDir() );
    const BufferString seisdirnm( fp.fullPath() );
    if ( !File::isDirectory(seisdirnm) )
	mErrRetDoesntExist( seisdirnm );

    return ret;
}


BufferString SurveyInfo::getDirName() const
{
    return disklocation_.dirName();
}


BufferString SurveyInfo::getDataDirName() const
{
    return disklocation_.basePath();
}


const SurveyDiskLocation& SurveyInfo::diskLocation() const
{
    return disklocation_;
}


void Survey::getDirectoryNames( BufferStringSet& list, bool addfullpath,
				const char* dataroot, const char* excludenm )
{
    BufferString basedir( dataroot );
    if ( basedir.isEmpty() )
	basedir = GetBaseDataDir();

    const DirList dl( basedir, File::DirsInDir );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString& dirnm = dl.get( idx );
	if ( excludenm && dirnm == excludenm )
	    continue;

	const FilePath fp( basedir, dirnm, SurveyInfo::sKeySetupFileName() );
	if ( File::isReadable(fp.fullPath()) )
	{
	    if ( addfullpath )
		list.add( dl.fullPath(idx) );
	    else
		list.add( dirnm );
	}
    }

    list.sort();
}
