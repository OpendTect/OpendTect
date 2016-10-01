/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          18-4-1996
________________________________________________________________________

-*/

#include "survinfo.h"
#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "coordsystem.h"
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
#include "uistrings.h"
#include <stdio.h>


static const char* sKeySI = "Survey Info";
static const char* sKeyXTransf = "Coord-X-BinID";
static const char* sKeyYTransf = "Coord-Y-BinID";
static const char* sKeyDefsFile = ".defs";
static const char* sKeySurvDefs = "Survey defaults";
static const char* sKeyLatLongAnchor = "Lat/Long anchor";
static const char* sKeySetPointPrefix = "Set Point";
const char* SurveyInfo::sKeyInlRange()	    { return "In-line range"; }
const char* SurveyInfo::sKeyCrlRange()	    { return "Cross-line range"; }
const char* SurveyInfo::sKeyXRange()	    { return "X range"; }
const char* SurveyInfo::sKeyYRange()	    { return "Y range"; }
const char* SurveyInfo::sKeyZRange()	    { return "Z range"; }
const char* SurveyInfo::sKeyDpthInFt()	    { return "Show depth in feet"; }
const char* SurveyInfo::sKeyXYInFt()	    { return "XY in feet"; }
const char* SurveyInfo::sKeySurvDataType()  { return "Survey Data Type"; }
const char* SurveyInfo::sKeySeismicRefDatum(){return "Seismic Reference Datum";}
static const char* sKeyCoordinateSystem = "Coordinate system";

mDefineInstanceCreatedNotifierAccess(SurveyInfo);


mDefineEnumUtils(SurveyInfo,Pol2D,"Survey Type")
{ "Only 3D", "Both 2D and 3D", "Only 2D", 0 };


Coord Survey::Geometry3D::toCoord( int linenr, int tracenr ) const
{
    return transform( BinID(linenr,tracenr) );
}


TrcKey Survey::Geometry3D::nearestTrace( const Coord& crd, float* dist ) const
{
    TrcKey tk( getSurvID(), transform(crd) );
    if ( dist )
    {
	if ( sampling_.hsamp_.includes(tk.binID()) )
	{
	    const Coord projcoord( transform(tk.binID()) );
	    *dist = projcoord.distTo<float>( crd );
	}
	else
	{
	    TrcKey nearbid( sampling_.hsamp_.getNearest(tk.binID()) );
	    const Coord nearcoord( transform(nearbid.binID()) );
	    *dist = (float)nearcoord.distTo<float>( crd );
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
    return ( c00.distTo<float>(c10) + c00.distTo<float>(c01) )/2;
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
{ sampling_.hsamp_.survid_ = getID(); }


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

    if ( fabs(planenormal.z_) > 0.5 )
    {
	translation.z_ = zStep();
    }
    else
    {
	Coord norm2d = Coord(planenormal.x_,planenormal.y_);
	norm2d.normalize();

	if ( fabs(norm2d.dot(b2c_.inlDir())) > 0.5 )
	    translation.x_ = inlDistance();
	else
	    translation.y_ = crlDistance();
    }

    return translation;
}


float Survey::Geometry3D::inlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c10 = transform( BinID(1,0) );
    return c00.distTo<float>(c10);
}


float Survey::Geometry3D::crlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c01 = transform( BinID(0,1) );
    return c00.distTo<float>(c01);
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


static ManagedObjectSet<SurveyInfo> survinfostack;

const SurveyInfo& SI()
{
    int cursurvinfoidx = survinfostack.size() - 1;
    if ( cursurvinfoidx < 0 )
    {
	uiString errmsg;
	SurveyInfo* newsi = SurveyInfo::read( GetDataDir(), errmsg );
	if ( !newsi )
	    newsi = new SurveyInfo;
	survinfostack += newsi;
	cursurvinfoidx = survinfostack.size() - 1;
    }

    return *survinfostack[cursurvinfoidx];
}


void SurveyInfo::pushSI( SurveyInfo* newsi )
{
    if ( !newsi )
	pFreeFnErrMsg("Null survinfo pushed");
    else
	survinfostack += newsi;
}


SurveyInfo* SurveyInfo::popSI()
{
    return survinfostack.isEmpty() ? 0
	 : survinfostack.removeSingle( survinfostack.size()-1 );
}


void SurveyInfo::deleteOriginal()
{
    if ( survinfostack.size() < 2 )
	return;

    delete survinfostack.removeSingle( 0 );
}


IOPar& SurveyInfo::getPars() const
{ return const_cast<SurveyInfo*>(this)->defaultPars(); }



SurveyInfo::SurveyInfo()
    : tkzs_(*new TrcKeyZSampling(false))
    , wcs_(*new TrcKeyZSampling(false))
    , zdef_(*new ZDomain::Def(ZDomain::Time()) )
    , depthsinfeet_(false)
    , surveydefaultpars_(sKeySurvDefs)
    , workRangeChg(this)
    , survdatatype_(Both2DAnd3D)
    , survdatatypeknown_(false)
    , seisrefdatum_(0.f)
{
    rdxtr_.b = rdytr_.c = 1;
    set3binids_[2].crl() = 0;

	// We need a 'reasonable' transform even when no proper SI is available
	// For example DataPointSets need to work
    Pos::IdxPair2Coord::DirTransform xtr, ytr;
    xtr.b = 1000; xtr.c = 0;
    ytr.b = 0; ytr.c = 1000;
    b2c_.setTransforms( xtr, ytr );
    tkzs_.hsamp_.survid_ = wcs_.hsamp_.survid_ = TrcKey::std3DSurvID();
    mTriggerInstanceCreatedNotifier();
}



SurveyInfo::SurveyInfo( const SurveyInfo& si )
    : NamedMonitorable( si )
    , tkzs_(*new TrcKeyZSampling(false))
    , wcs_(*new TrcKeyZSampling(false))
    , surveydefaultpars_(sKeySurvDefs)
    , zdef_(*new ZDomain::Def( si.zDomain() ) )
    , workRangeChg(this)
{
    *this = si;
    mTriggerInstanceCreatedNotifier();
}


SurveyInfo::~SurveyInfo()
{
    sendDelNotif();

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
    if ( &si == this ) return *this;

    setName( si.name() );
    zdef_ = si.zdef_;
    datadir_ = si.datadir_;
    dirname_ = si.dirname_;
    coordsystem_ = si.coordsystem_;
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
    surveydefaultpars_ = si.surveydefaultpars_;
    seisrefdatum_ = si.seisrefdatum_;
    rdxtr_ = si.rdxtr_; rdytr_ = si.rdytr_;
    sipnm_ = si.sipnm_;
    update3DGeometry();

    return *this;
}


SurveyInfo* SurveyInfo::read( const char* survdir, uiString& errmsg )
{
    FilePath fpsurvdir( survdir );
    FilePath fp( fpsurvdir, sKeySetupFileName() );
    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(true) )
    {
	errmsg = sfio.errMsg();
	return 0;
    }

    ascistream astream( sfio.istrm() );
    if ( !astream.isOfFileType(sKeySI) )
    {
	errmsg = tr("Survey definition file cannot be read.\n"
		    "Survey file '%1'  has file type '%2'.\n"
		    "The file may be corrupt or not accessible.")
		   .arg(fp.fullPath())
		   .arg(astream.fileType());
	sfio.closeFail();
	return 0;
    }

    astream.next();

    BufferString keyw = astream.keyWord();
    SurveyInfo* si = new SurveyInfo;
    si->setName( FilePath(survdir).fileName() ); // good default

    //Read params here, so we can look at the pars below
    fp = fpsurvdir; fp.add( sKeyDefsFile );
    si->defaultPars().read( fp.fullPath(), sKeySurvDefs, true );
    si->defaultPars().setName( sKeySurvDefs );

    //Scrub away old settings (confusing to users)
    si->defaultPars().removeWithKey( "Depth in feet" );

    si->dirname_ = fpsurvdir.fileName();
    si->datadir_ = fpsurvdir.pathOnly();
    if ( !survdir || si->dirname_.isEmpty() ) return si;

    const IOPar survpar( astream );
    si->usePar(survpar);

    BufferString line;
    while ( astream.stream().getLine(line) )
    {
	if ( !si->comment_.isEmpty() )
	    si->comment_ += "\n";
	si->comment_ += line;
    }
    sfio.closeSuccess();

    if ( !si->wrapUpRead() )
    { delete si; return 0; }

    return si;
}


bool SurveyInfo::usePar( const IOPar& par )
{
    par.get( sKey::Name(), name_ );
    par.get( sKeyInlRange(),
	    tkzs_.hsamp_.start_.inl(),
	    tkzs_.hsamp_.stop_.inl(),
	    tkzs_.hsamp_.step_.inl() );

    par.get( sKeyCrlRange(),
	    tkzs_.hsamp_.start_.crl(),
	    tkzs_.hsamp_.stop_.crl(),
	    tkzs_.hsamp_.step_.crl() );

    FileMultiString fms;
    if ( par.get( sKeyZRange(), fms ) )
    {
	tkzs_.zsamp_.start = fms.getFValue( 0 );
	tkzs_.zsamp_.stop = fms.getFValue( 1 );
	tkzs_.zsamp_.step = fms.getFValue( 2 );
	if ( Values::isUdf(tkzs_.zsamp_.step)
	       || mIsZero(tkzs_.zsamp_.step,mDefEps) )
	{
	    tkzs_.zsamp_.step = 0.004f;
	}
	if ( fms.size() > 3 )
	{
	    if ( *fms[3] == 'T' )
	    {
		zdef_ = ZDomain::Time();
		depthsinfeet_ = false;
		defaultPars().getYN( sKeyDpthInFt(), depthsinfeet_ );
	    }
	    else
	    {
		zdef_ = ZDomain::Depth();
		depthsinfeet_ = *fms[3] == 'F';
	    }
	}
    }

    BufferString survdatatype;
    if ( par.get( sKeySurvDataType(), survdatatype) )
    {
	Pol2D var;
	if ( !Pol2DDef().parse( survdatatype, var ) )
	    var = Both2DAnd3D;

	setSurvDataType( var );
    }

    PtrMan<IOPar> coordsystempar = par.subselect( sKeyCoordinateSystem );
    if ( coordsystempar )
    {
	coordsystem_ = Coords::PositionSystem::createSystem( *coordsystempar );
    }

    if ( !coordsystem_ )
    {
	RefMan<Coords::UnlocatedXY> coordsystem = new Coords::UnlocatedXY;
	coordsystem_ = coordsystem;

	/*Try to read the parameters, there should be reference latlog and
	  Coordinates in there. */
	if ( !coordsystempar || !coordsystem->usePar( *coordsystempar ) )
	{
	    //Read from old format keys
	    bool xyinfeet = false;
	    par.getYN( sKeyXYInFt(), xyinfeet );
	    coordsystem->setIsFeet( xyinfeet );

	    BufferString anchor;
	    if ( par.get(sKeyLatLongAnchor,anchor) )
	    {
		char* ptr = anchor.find( '=' );
		if ( !ptr ) return false;
		*ptr++ = '\0';
		Coord c; LatLong l;
		if ( !c.fromString(anchor) || !l.fromString(ptr) )
		    return false;
		else if ( mIsZero(c.x_,1e-3) && mIsZero(c.y_,1e-3) )
		    return false;

		coordsystem->setLatLongEstimate( l, c );
	    }
	}
    }

    par.get( sKeySeismicRefDatum(), seisrefdatum_ );

    par.get( sKeyXTransf, rdxtr_.a, rdxtr_.b, rdxtr_.c );
    par.get( sKeyYTransf, rdytr_.a, rdytr_.b, rdytr_.c );

    PtrMan<IOPar> setpts = par.subselect( sKeySetPointPrefix );
    if ( setpts )
    {
	for ( int idx=0; idx<3; idx++ )
	{
	    const int keyidx = idx+1;
	    const BufferString key( ::toString(keyidx) );
	    setpts->get( key, fms );
	    if ( fms.size() >= 2 )
	    {
		set3binids_[idx].fromString( fms[0] );
		set3coords_[idx].fromString( fms[1] );
	    }
	}
    }

    tkzs_.normalise();
    wcs_ = tkzs_;
    return true;
}


bool SurveyInfo::wrapUpRead()
{
    if ( set3binids_[2].crl() == 0 )
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


void SurveyInfo::updateDirName()
{
    if ( name().isEmpty() )
	return;

    dirname_ = name();
    dirname_.clean( BufferString::AllowDots );
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

    const float scale = xyInFeet() ? mFromFeetFactorF : 1;
    const float d01 = c00.distTo<float>( c01 ) * scale;
    const float d10 = c00.distTo<float>( c10 ) * scale;

    return d01*d10;
}


float SurveyInfo::getArea( bool work ) const
{
    return getArea( inlRange( work ), crlRange( work ) );
}



float SurveyInfo::zStep() const { return tkzs_.zsamp_.step; }


Coord3 SurveyInfo::oneStepTranslation( const Coord3& planenormal ) const
{
    return get3DGeometry(false)->oneStepTranslation( planenormal );
}


void SurveyInfo::setRange( const TrcKeyZSampling& cs, bool work )
{
    if ( work )
	wcs_ = cs;
    else
	tkzs_ = cs;

    tkzs_.hsamp_.survid_ = wcs_.hsamp_.survid_ = TrcKey::std3DSurvID();
    wcs_.limitTo( tkzs_ );
    wcs_.hsamp_.step_ = tkzs_.hsamp_.step_;
    wcs_.zsamp_.step = tkzs_.zsamp_.step;
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
    return reasonableRange( true ).includes( b.inl(), false ) &&
	   reasonableRange( false ).includes( b.crl(), false );
}


bool SurveyInfo::isReasonable( const Coord& crd ) const
{
    if ( Values::isUdf(crd.x_) || Values::isUdf(crd.y_) )
	return false;

    return isReasonable( transform(crd) );
}


#define mChkCoord(c) \
    if ( c.x_ < minc.x_ ) minc.x_ = c.x_; if ( c.y_ < minc.y_ ) minc.y_ = c.y_;

Coord SurveyInfo::minCoord( bool work ) const
{
    const TrcKeyZSampling& cs = sampling(work);
    Coord minc = transform( cs.hsamp_.start_ );
    Coord c = transform( cs.hsamp_.stop_ );
    mChkCoord(c);
    BinID bid( cs.hsamp_.start_.inl(), cs.hsamp_.stop_.crl() );
    c = transform( bid );
    mChkCoord(c);
    bid = BinID( cs.hsamp_.stop_.inl(), cs.hsamp_.start_.crl() );
    c = transform( bid );
    mChkCoord(c);
    return minc;
}


#undef mChkCoord
#define mChkCoord(c) \
    if ( c.x_ > maxc.x_ ) maxc.x_ = c.x_; if ( c.y_ > maxc.y_ ) maxc.y_ = c.y_;

Coord SurveyInfo::maxCoord( bool work ) const
{
    const TrcKeyZSampling& cs = sampling(work);
    Coord maxc = transform( cs.hsamp_.start_ );
    Coord c = transform( cs.hsamp_.stop_ );
    mChkCoord(c);
    BinID bid( cs.hsamp_.start_.inl(), cs.hsamp_.stop_.crl() );
    c = transform( bid );
    mChkCoord(c);
    bid = BinID( cs.hsamp_.stop_.inl(), cs.hsamp_.start_.crl() );
    c = transform( bid );
    mChkCoord(c);
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
    const float eps = 1e-8;
    return cs.hsamp_.includes( bid )
	&& cs.zsamp_.start < z + eps && cs.zsamp_.stop > z - eps;
}


bool SurveyInfo::zIsTime() const
{ return zdef_.isTime(); }


SurveyInfo::Unit SurveyInfo::xyUnit() const
{ return xyInFeet() ? Feet : Meter; }


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
    return getDistUnitString( xyInFeet(), wb );
}


uiString SurveyInfo::getUiXYUnitString( bool abbrvt, bool wb ) const
{
    return uiStrings::sDistUnitString( xyInFeet(), abbrvt, wb );
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
    if ( set3binids_[0].inl() )
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
    if ( b[1].inl() == b[0].inl() )
        return "Need two different in-lines";
    if ( b[0].crl() == xline )
        return "No Cross-line range present";

    if ( !b2c_.set3Pts( c[0], c[1], c[2], b[0], b[1], xline ) )
	return "Cannot construct a valid transformation matrix from this input."
	       "\nPlease check whether the data is on a single straight line.";

    set3binids_[0] = b[0];
    set3binids_[1] = b[1];
    set3binids_[2] = BinID( b[0].inl(), xline );
    set3coords_[0] = c[0];
    set3coords_[1] = c[1];
    set3coords_[2] = c[2];
    return 0;
}


const uiString SurveyInfo::set3PtsUiMsg( const Coord c[3], const BinID b[2],
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
    return uiStrings::sEmptyString();
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


static void putTr( const Pos::IdxPair2Coord::DirTransform& trans,
		   IOPar& par, const char* key )
{
    BufferString res;

    snprintf( res.getCStr(), res.bufSize(), "%.10lg`%.10lg`%.10lg",
	      trans.a, trans.b, trans.c );
    par.set( key, res );
}


bool SurveyInfo::isRightHandSystem() const
{ return get3DGeometry(false)->isRightHandSystem(); }


bool SurveyInfo::write( const char* basedir ) const
{
    if ( !basedir ) basedir = GetBaseDataDir();

    FilePath fp( basedir, dirname_, sKeySetupFileName() );
    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot open survey info file for write!" );
	if ( sfio.errMsg().isSet() )
	    { msg += "\n\t"; msg += sfio.errMsg().getFullString(); }
	ErrMsg( msg );
	return false;
    }

    od_ostream& strm = sfio.ostrm();
    IOPar par;
    fillPar( par );


    if ( !par.write( strm, sKeySI ) )
    {
	ErrMsg( "Cannot write to survey info file!" );
	return false;
    }

    ascostream astream( strm );
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
	msg += sfio.errMsg().getFullString();
	ErrMsg( msg );
	return false;
    }

    fp.set( basedir );
    fp.add( dirname_ );
    saveDefaultPars( fp.fullPath() );
    return true;
}


void SurveyInfo::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name() );
    par.set( sKeySurvDataType(), toString( survDataType()) );
    par.set( sKeyInlRange(), tkzs_.hsamp_.start_.inl(),
	     tkzs_.hsamp_.stop_.inl(), tkzs_.hsamp_.step_.inl() );
    par.set( sKeyCrlRange(), tkzs_.hsamp_.start_.crl(),
	     tkzs_.hsamp_.stop_.crl(), tkzs_.hsamp_.step_.crl() );


    FileMultiString fms = "";
    fms += tkzs_.zsamp_.start; fms += tkzs_.zsamp_.stop;
    fms += tkzs_.zsamp_.step;
    fms += zIsTime() ? "T" : ( depthsinfeet_ ? "F" : "D" );
    par.set( sKeyZRange(), fms );

    putTr( b2c_.getTransform(true), par, sKeyXTransf );
    putTr( b2c_.getTransform(false), par, sKeyYTransf );

    for ( int idx=0; idx<3; idx++ )
    {
	SeparString ky( sKeySetPointPrefix, '.' );
	ky += idx + 1;
	fms = set3binids_[idx].toString();
	fms += set3coords_[idx].toString();
	par.set( ky.buf(), fms.buf() );
    }

    IOPar coordsystempar;
    coordsystem_->fillPar( coordsystempar );
    par.mergeComp( coordsystempar, sKeyCoordinateSystem );
    par.set( sKeySeismicRefDatum(), seisrefdatum_ );
}


const IOPar& SurveyInfo::defaultPars() const
{ return const_cast<SurveyInfo*>(this)->defaultPars(); }


#define uiErrMsg(s) { \
    BufferString cmd( "\"", \
	    FilePath(GetExecPlfDir(),"od_DispMsg").fullPath() ); \
    cmd += "\" --err "; \
    cmd += " Could not write to "; \
    cmd += s; \
    if ( File::isHidden(s) ) \
	cmd += ". This is a hidden file"; \
    else \
	cmd += " Please check the file permission"; \
    OS::ExecCommand( cmd ); \
}

void SurveyInfo::saveDefaultPars( const char* basedir ) const
{
    BufferString surveypath;
    if ( !basedir || !*basedir )
    {
	const BufferString storepath( FilePath(datadir_,dirname_).fullPath() );
	surveypath = File::exists(storepath) ? storepath.buf() : GetDataDir();
    }
    else
	surveypath = basedir;

    const BufferString defsfnm( FilePath(surveypath,sKeyDefsFile).fullPath() );
    if ( surveydefaultpars_.isEmpty() )
    {
	if ( File::exists(defsfnm) )
	    File::remove( defsfnm );
    }
    else if ( !surveydefaultpars_.write( defsfnm, sKeySurvDefs ) )
	uiErrMsg( defsfnm );
}


bool SurveyInfo::has2D() const
{ return survdatatype_ == Only2D || survdatatype_ == Both2DAnd3D; }


bool SurveyInfo::has3D() const
{ return survdatatype_ == No2D || survdatatype_ == Both2DAnd3D; }


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
	newsgeom->ref();
	if ( work )
	    newsgeom->setID( Survey::GM().default3DSurvID() );
	newsgeom->setGeomData( b2c_, sampling(work), zScale() );
	if ( sgeom.setIfEqual( 0, newsgeom ) )
	    newsgeom->ref();
    }

    return RefMan<Survey::Geometry3D>( sgeom );
}


float SurveyInfo::angleXInl() const
{
    Coord xy1 = transform( BinID(inlRange(false).start, crlRange(false).start));
    Coord xy2 = transform( BinID(inlRange(false).start, crlRange(false).stop) );
    const double xdiff = xy2.x_ - xy1.x_;
    const double ydiff = xy2.y_ - xy1.y_;
    return mCast(float, Math::Atan2( ydiff, xdiff ) );
}


bool SurveyInfo::isInside( const BinID& bid, bool work ) const
{
    const Interval<int> inlrg( inlRange(work) );
    const Interval<int> crlrg( crlRange(work) );
    return inlrg.includes(bid.inl(),false) && crlrg.includes(bid.crl(),false);
}


RefMan<Coords::PositionSystem> SurveyInfo::getCoordSystem()
{ return coordsystem_; }


ConstRefMan<Coords::PositionSystem> SurveyInfo::getCoordSystem() const
{ return coordsystem_; }


bool SurveyInfo::xyInFeet() const
{
    return coordsystem_ && coordsystem_->isFeet();
}


bool SurveyInfo::setCoordSystem( Coords::PositionSystem* system )
{
    if ( system && !system->isOrthogonal() )
	return false;

    coordsystem_ = system;
    return false;
}
