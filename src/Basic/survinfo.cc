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
	Coord norm2d = planenormal;
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
    int cursurvinfoidx = survInfoStack().size() - 1;
    if ( cursurvinfoidx < 0 )
    {
	SurveyInfo* newsi = SurveyInfo::read( GetDataDir() );
	if ( !newsi )
	    newsi = new SurveyInfo;
	survInfoStack() += newsi;
	cursurvinfoidx = survInfoStack().size() - 1;
    }

    return *survInfoStack()[cursurvinfoidx];
}


void SurveyInfo::pushSI( SurveyInfo* newsi )
{
    if ( !newsi )
	pFreeFnErrMsg("Null survinfo pushed");
    else
	survInfoStack() += newsi;
}


SurveyInfo* SurveyInfo::popSI()
{
    return survInfoStack().isEmpty() ? 0
	 : survInfoStack().removeSingle( survInfoStack().size()-1 );
}


mDefineInstanceCreatedNotifierAccess(SurveyInfo)

SurveyInfo::SurveyInfo()
    : tkzs_(*new TrcKeyZSampling(false))
    , wcs_(*new TrcKeyZSampling(false))
    , zdef_(*new ZDomain::Def(ZDomain::Time()) )
    , pars_(*new IOPar(sKeySurvDefs))
    , ll2c_(*new LatLong2Coord)
    , workRangeChg(this)
    , survdatatype_(OD::Both2DAnd3D)
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


SurveyInfo& SurveyInfo::operator =( const SurveyInfo& si )
{
    if ( &si == this )
	return *this;

    setName( si.name() );
    zdef_ = si.zdef_;
    disklocation_ = si.disklocation_;
    coordsystem_ = si.coordsystem_;
    xyinfeet_ = si.xyinfeet_;
    depthsinfeet_ = si.depthsinfeet_;
    b2c_ = si.b2c_;
    survdatatype_ = si.survdatatype_;
    survdatatypeknown_ = si.survdatatypeknown_;
    for ( int idx=0; idx<3; idx++ )
    {
	set3binids_[idx] = si.set3binids_[idx];
	set3coords_[idx] = si.set3coords_[idx];
    }

    tkzs_ = si.tkzs_;
    wcs_ = si.wcs_;
    pars_ = si.pars_;
    ll2c_ = si.ll2c_;

    seisrefdatum_ = si.seisrefdatum_;
    rdxtr_ = si.rdxtr_; rdytr_ = si.rdytr_;
    sipnm_ = si.sipnm_;
    update3DGeometry();

    return *this;
}


SurveyInfo& SurveyInfo::empty()
{
    return *new SurveyInfo;
}


SurveyInfo* SurveyInfo::read( const char* survdir )
{
    return SurveyInfo::read( survdir, false );
}


SurveyInfo* SurveyInfo::read( const char* survdir, bool isfile )
{
    FilePath fp( survdir );
    if ( !isfile )
	fp.add( sKeySetupFileName() );

    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(true) )
	return 0;

    ascistream astream( sfio.istrm() );
    if ( !astream.isOfFileType(sKeySI) )
    {
	BufferString errmsg( "Survey definition file cannot be read.\n"
			     "Survey file '" );
	errmsg += fp.fullPath(); errmsg += "' has file type '";
	errmsg += astream.fileType();
	errmsg += "'.\nThe file may be corrupt or not accessible.";
	ErrMsg( errmsg );
	sfio.closeFail();
	return 0;
    }

    astream.next();
    SurveyInfo* si = new SurveyInfo;

    if ( !isfile )
    {
	FilePath fpsurvdir( survdir );
	si->disklocation_ = SurveyDiskLocation( fpsurvdir );
	if ( !survdir || si->disklocation_.isEmpty() )
	    return si;

	si->setName( si->disklocation_.dirName() ); // good default

	//Read params here, so we can look at the pars below
	const FilePath fpdef( survdir, sKeyDefsFile );
	si->getPars().read( fpdef.fullPath(), sKeySurvDefs, true );
	si->getPars().setName( sKeySurvDefs );

	//Scrub away old settings (confusing to users)
	si->getPars().removeWithKey( "Depth in feet" );

	// Read log
	const FilePath fplog( survdir, sKeyLogFile );
	IOPar& logpars = si->getLogPars();
	logpars.read( fplog.fullPath(), sKeySurvLog, true );
	logpars.setName( sKeySurvLog );
	if ( logpars.isEmpty() )
	    logpars.set( sKey::Version(), astream.version() );
	logpars.set( sKey::ModAt(), astream.timeStamp() );
    }

    BufferString keyw = astream.keyWord();
    IOPar coordsystempar;
    while ( !atEndOfSection(astream) )
    {
	keyw = astream.keyWord();
	if ( keyw == sKey::Name() )
	    si->setName( astream.value() );
	else if ( keyw == sKeyInlRange() )
	{
	    FileMultiString fms( astream.value() );
	    si->tkzs_.hsamp_.start_.inl() = fms.getIValue( 0 );
	    si->tkzs_.hsamp_.stop_.inl() = fms.getIValue( 1 );
	    si->tkzs_.hsamp_.step_.inl() = fms.getIValue( 2 );
	}
	else if ( keyw == sKeyCrlRange() )
	{
	    FileMultiString fms( astream.value() );
	    si->tkzs_.hsamp_.start_.crl() = fms.getIValue( 0 );
	    si->tkzs_.hsamp_.stop_.crl() = fms.getIValue( 1 );
	    si->tkzs_.hsamp_.step_.crl() = fms.getIValue( 2 );
	}
	else if ( keyw == sKeyZRange() )
	{
	    FileMultiString fms( astream.value() );
	    si->tkzs_.zsamp_.start = fms.getFValue( 0 );
	    si->tkzs_.zsamp_.stop = fms.getFValue( 1 );
	    si->tkzs_.zsamp_.step = fms.getFValue( 2 );
	    if ( Values::isUdf(si->tkzs_.zsamp_.step)
	      || mIsZero(si->tkzs_.zsamp_.step,mDefEps) )
		si->tkzs_.zsamp_.step = 0.004;
	    if ( fms.size() > 3 )
	    {
		if ( *fms[3] == 'T' )
		    si->zdef_ = ZDomain::Time();
		else
		{
		    si->zdef_ = ZDomain::Depth();
		    si->depthsinfeet_ = *fms[3] == 'F';
		}
	    }
	}
	else if ( keyw == sKeySurvDataType() )
	{
	    OD::Pol2D3D var;
	    if ( !parseEnumPol2D3D( astream.value(), var ) )
		var = OD::Both2DAnd3D;

	    si->setSurvDataType( var );
	}
	else if ( keyw == sKeyXYInFt() )
	    si->xyinfeet_ = astream.getYN();
	else if ( keyw == sKeySeismicRefDatum() )
	    si->seisrefdatum_ = astream.getFValue();
	else if ( keyw.startsWith(sKey::CoordSys()) )
	    coordsystempar.add( keyw, astream.value() );
	else
	    si->handleLineRead( keyw, astream.value() );

	astream.next();
    }

    PtrMan<IOPar> coordsyssubpar =
	coordsystempar.subselect( sKey::CoordSys() );
    if ( !coordsyssubpar )
	coordsyssubpar = si->pars().subselect( sKey::CoordSys() );
    if ( coordsyssubpar )
	si->coordsystem_ =
		Coords::CoordSystem::createSystem( *coordsyssubpar );

    if ( si->coordsystem_ )
	si->xyinfeet_ = si->coordsystem_->isFeet();
    else
    {
	if ( si->ll2c_.isOK() )
	{
	    RefMan<Coords::AnchorBasedXY> anchoredsystem =
			new Coords::AnchorBasedXY( si->ll2c_.refLatLong(),
						   si->ll2c_.refCoord() );
	    anchoredsystem->setIsFeet( si->xyinfeet_ );
	    si->coordsystem_ = anchoredsystem;
	}
	else
	{
	    RefMan<Coords::UnlocatedXY> undefsystem = new Coords::UnlocatedXY;
	    undefsystem->setIsFeet( si->xyinfeet_ );
	    si->coordsystem_ = undefsystem;
	}
    }

    if ( si->zdef_ == ZDomain::Time() )
    {
	if ( si->xyinfeet_ )
	    si->depthsinfeet_ = true;
	else
	{
	    si->depthsinfeet_ = false;
	    si->getPars().getYN( sKeyDpthInFt(), si->depthsinfeet_ );
	}
    }

    si->tkzs_.normalize();
    si->wcs_ = si->tkzs_;

    BufferString line;
    while ( astream.stream().getLine(line) )
    {
	if ( !si->comment_.isEmpty() )
	    si->comment_ += "\n";
	si->comment_ += line;
    }
    sfio.closeSuccess();

    if ( !si->wrapUpRead() )
    { delete si; return nullptr; }

    return si;
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
    else if ( keyw.startsWith("Set Point") )
    {
	const char* ptr = firstOcc( (const char*)keyw, '.' );
	if ( !ptr ) return;
	int ptidx = toInt( ptr + 1 ) - 1;
	if ( ptidx < 0 ) ptidx = 0;
	if ( ptidx > 3 ) ptidx = 2;
	FileMultiString fms( val );
	if ( fms.size() < 2 ) return;
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
    const double zstep =
		sCast(double,zStep()*zDomain().userFactor());
    int nrdec = 0;
    double decval = zstep;
    while ( decval > Math::Floor(decval) &&
	    !mIsZero(decval,1e-4) && !mIsEqual(decval,1.,1e-4) )
    {
	nrdec++;
	decval = decval*10 - Math::Floor(decval*10);
    }

    return nrdec;
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
    snap( bid, BinID(1,1) ); intv.start = bid.inl();
    bid.inl() = intv.stop; snap( bid, BinID(-1,-1) ); intv.stop = bid.inl();
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
    snap( bid, BinID(1,1) ); intv.start = bid.crl();
    bid.crl() = intv.stop; snap( bid, BinID(-1,-1) ); intv.stop = bid.crl();
}



void SurveyInfo::checkZRange( Interval<float>& intv, bool work ) const
{
    const StepInterval<float>& rg = sampling(work).zsamp_;
    intv.sort();
    if ( intv.start < rg.start ) intv.start = rg.start;
    if ( intv.start > rg.stop )  intv.start = rg.stop;
    if ( intv.stop > rg.stop )   intv.stop = rg.stop;
    if ( intv.stop < rg.start )  intv.stop = rg.start;
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
{ return zdef_.isTime(); }


SurveyInfo::Unit SurveyInfo::xyUnit() const
{ return xyinfeet_ ? Feet : Meter; }


SurveyInfo::Unit SurveyInfo::zUnit() const
{
    if ( zIsTime() ) return Second;
    return depthsinfeet_ ? Feet : Meter;
}


void SurveyInfo::putZDomain( IOPar& iop ) const
{
    zdef_.set( iop );
}


const ZDomain::Def& SurveyInfo::zDomain() const
{ return zdef_; }


const char* SurveyInfo::getXYUnitString( bool wb ) const
{
    return getDistUnitString( xyinfeet_, wb );
}


uiString SurveyInfo::getUiXYUnitString( bool abbrvt, bool wb ) const
{
    return uiStrings::sDistUnitString( xyinfeet_, abbrvt, wb );
}


void SurveyInfo::setZUnit( bool istime, bool infeet )
{
    zdef_ = istime ? ZDomain::Time() : ZDomain::Depth();
    depthsinfeet_ = infeet;
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


void SurveyInfo::putTr( const Pos::IdxPair2Coord::DirTransform& tr,
			  ascostream& astream, const char* key ) const
{
    char buf[1024];
    od_sprintf( buf, 1024, "%.10lg`%.10lg`%.10lg", tr.a, tr.b, tr.c );
    astream.put( key, buf );
}


bool SurveyInfo::isRightHandSystem() const
{ return get3DGeometry(false)->isRightHandSystem(); }


bool SurveyInfo::write( const char* basedir ) const
{
    if ( !basedir ) basedir = GetBaseDataDir();

    FilePath fp( basedir, disklocation_.dirName(), sKeySetupFileName() );
    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot open survey info file for write!" );
	if ( *sfio.errMsg() )
	    { msg += "\n\t"; msg += sfio.errMsg(); }
	ErrMsg( msg );
	return false;
    }

    od_ostream& strm = sfio.ostrm();
    ascostream astream( strm );
    if ( !astream.putHeader(sKeySI) )
    {
	ErrMsg( "Cannot write to survey info file!" );
	return false;
    }

    astream.put( sKey::Name(), name() );
    astream.put( sKeySurvDataType(), getPol2D3DString( survDataType()) );
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
    fms += zIsTime() ? "T" : ( depthsinfeet_ ? "F" : "D" );
    astream.put( sKeyZRange(), fms );

    writeSpecLines( astream );

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

    fp.set( basedir ); fp.add( disklocation_.dirName() );
    savePars( fp.fullPath() );
    saveLog( fp.fullPath() );
    return true;
}


void SurveyInfo::writeSpecLines( ascostream& astream ) const
{
    putTr( b2c_.getTransform(true), astream, sKeyXTransf );
    putTr( b2c_.getTransform(false), astream, sKeyYTransf );
    FileMultiString fms;
    for ( int idx=0; idx<3; idx++ )
    {
	SeparString ky( "Set Point", '.' );
	ky += idx + 1;
	fms = set3binids_[idx].toString();
	fms += set3coords_[idx].toString();
	astream.put( ky.buf(), fms.buf() );
    }

    if ( coordsystem_ )
    {
	IOPar par;
	coordsystem_->fillPar( par );
	IOParIterator iter( par );
	BufferString key, val, buf;
	while ( iter.next(key,val) )
	    astream.put( IOPar::compKey(sKey::CoordSys(),key), val );
    }
    else
	astream.putYN( sKeyXYInFt(), xyinfeet_ );

    if ( ll2c_.isOK() )
	astream.put( sKeyLatLongAnchor, ll2c_.toString() );
    astream.putYN( sKeyXYInFt(), xyInFeet() );
    astream.put( sKeySeismicRefDatum(), seisrefdatum_ );
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
	    File::remove( defsfnm );
    }
    else if ( !pars_.write(defsfnm,sKeySurvDefs) )
	uiErrMsg( defsfnm )
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
