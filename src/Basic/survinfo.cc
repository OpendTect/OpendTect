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
static const char* sSurvFile = ".survey";
static const char* sDefsFile = ".defs";
static const char* sCommentsFile = ".comments";
static const char* sFreshFile = ".fresh";
static const char* sCreationFile = ".creation";
static const char* sKeySurvDefs = "Survey defaults";
static const char* sKeyFreshFileType = "Survey Creation Info";
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
mDefineEnumUtils(SurveyInfo,Pol2D,"Survey Type")
{ "Only 3D", "Both 2D and 3D", "Only 2D", 0 };

#define mXYInFeet() (coordsystem_ && coordsystem_->isFeet())
#define mXYUnit() (mXYInFeet() ? Feet : Meter)
#define mZUnit() (zdef_.isTime() ? Second : (depthsinfeet_ ? Feet : Meter))
#define mZScale() defaultXYtoZScale( mZUnit(), mXYUnit() )
#define mSampling(work) (work ? workcs_ : fullcs_)


static SurveyInfo global_si_;
const SurveyInfo& SI() { return global_si_; }



SurveyInfo::SurveyInfo()
    : fullcs_(*new TrcKeyZSampling(false))
    , workcs_(*new TrcKeyZSampling(false))
    , zdef_(*new ZDomain::Def(ZDomain::Time()) )
    , depthsinfeet_(false)
    , defpars_(sKeySurvDefs)
    , pol2d_(Both2DAnd3D)
    , pol2dknown_(false)
    , seisrefdatum_(0.f)
    , s3dgeom_(0)
    , work_s3dgeom_(0)
{
    rdxtr_.b = rdytr_.c = 1;
    set3binids_[2].crl() = 0;

	// We need a 'reasonable' transform even when no proper SI is available
	// For example DataPointSets need to work
    Pos::IdxPair2Coord::DirTransform xtr, ytr;
    xtr.b = 1000; xtr.c = 0;
    ytr.b = 0; ytr.c = 1000;
    b2c_.setTransforms( xtr, ytr );
    fullcs_.hsamp_.survid_ = workcs_.hsamp_.survid_ = TrcKey::std3DSurvID();
}



SurveyInfo::SurveyInfo( const SurveyInfo& oth )
    : NamedMonitorable( oth )
    , fullcs_(*new TrcKeyZSampling(false))
    , workcs_(*new TrcKeyZSampling(false))
    , defpars_(sKeySurvDefs)
    , zdef_(*new ZDomain::Def( oth.zDomain() ) )
    , s3dgeom_(0)
    , work_s3dgeom_(0)
{
    copyClassData( oth );
}


SurveyInfo::~SurveyInfo()
{
    sendDelNotif();

    delete &fullcs_;
    delete &workcs_;
    delete &zdef_;

    if ( work_s3dgeom_ )
	work_s3dgeom_->unRef();
    if ( s3dgeom_ )
	s3dgeom_->unRef();
}


mImplMonitorableAssignment( SurveyInfo, NamedMonitorable )


void SurveyInfo::copyClassData( const SurveyInfo& oth )
{
    zdef_ = oth.zdef_;
    basepath_ = oth.basepath_;
    dirname_ = oth.dirname_;
    coordsystem_ = oth.coordsystem_;
    depthsinfeet_ = oth.depthsinfeet_;
    b2c_ = oth.b2c_;
    pol2d_ = oth.pol2d_;
    pol2dknown_ = oth.pol2dknown_;
    for ( int idx=0; idx<3; idx++ )
    {
	set3binids_[idx] = oth.set3binids_[idx];
	set3coords_[idx] = oth.set3coords_[idx];
    }
    fullcs_ = oth.fullcs_;
    workcs_ = oth.workcs_;
    defpars_ = oth.defpars_;
    seisrefdatum_ = oth.seisrefdatum_;
    rdxtr_ = oth.rdxtr_; rdytr_ = oth.rdytr_;
    sipnm_ = oth.sipnm_;
    update3DGeometry();
}


#define mCmpRet(memb,ct) \
    if ( !(memb==oth->memb) ) \
        return ChangeData( ct(), ChangeData::cUnspecChgID() )
#define mCmpRetDeRef(memb,ct) \
    if ( !((*memb)==(*oth->memb)) ) \
        return ChangeData( ct(), ChangeData::cUnspecChgID() )


SurveyInfo::ChangeData SurveyInfo::compareWith( const Monitorable& mon ) const
{
    if ( this == &mon )
	return ChangeData::NoChange();
    mDynamicCastGet( const SurveyInfo*, oth, &mon );
    if ( !oth )
	return ChangeData::AllChanged();

    mCmpRet( basepath_, cEntireObjectChangeType );
    mCmpRet( dirname_, cEntireObjectChangeType ); //TODO name change only?
    mCmpRet( zdef_, cSetupChange );
    mCmpRet( b2c_, cSetupChange );
    mCmpRet( pol2d_, cSetupChange );
    mCmpRet( seisrefdatum_, cSetupChange );
    mCmpRet( fullcs_, cRangeChange );
    mCmpRet( workcs_, cWorkRangeChange );
    mCmpRet( depthsinfeet_, cParsChange );
    mCmpRetDeRef( coordsystem_, cParsChange );
    mCmpRet( defpars_, cParsChange );

    for ( int idx=0; idx<3; idx++ )
    {
	mCmpRet( set3binids_[idx], cAuxDataChange );
    }
    if ( !sipnm_.isEqualTo(oth->sipnm_) )
        return ChangeData( cAuxDataChange(), ChangeData::cUnspecChgID() );

    mCmpRet( name_, cNameChange );
    mCmpRet( comments_, cCommentChange );

    return ChangeData::NoChange();
}


#define mErrRetDoesntExist(fnm) \
    { ret.add( uiStrings::phrDoesntExist(::toUiString(fnm)) ); return ret; }


uiRetVal SurveyInfo::setSurveyLocation( const char* dr, const char* sd )
{
    uiRetVal ret;
    const BufferString olddataroot = SI().basepath_;
    const BufferString olddirname = SI().dirname_;
    BufferString newdataroot( dr ); BufferString newdirname( sd );
    const bool useolddr = newdataroot.isEmpty() || newdataroot == olddataroot;
    const bool useoldsd = newdirname.isEmpty() || newdirname == olddirname;
    if ( useolddr && useoldsd )
	return ret;

    if ( useolddr )
	newdataroot = olddataroot;
    if ( useoldsd )
	newdirname = olddirname;

    if ( !File::isDirectory(newdataroot) )
	mErrRetDoesntExist(newdataroot)
    File::Path fp( newdataroot, newdirname );
    const BufferString survdir = fp.fullPath();
    if ( !File::isDirectory(survdir) )
	mErrRetDoesntExist(survdir)
    fp.add( ".omf" );
    const BufferString omffnm = fp.fullPath();
    if ( !File::exists(omffnm) )
	mErrRetDoesntExist(omffnm)

    SurveyInfo* newsi = read( survdir, ret );
    if ( !newsi )
	return ret;

    global_si_ = *newsi;
    delete newsi;
    return ret;
}


SurveyInfo* SurveyInfo::read( const char* survdir, uiRetVal& uirv )
{
    File::Path fpsurvdir( survdir );
    File::Path fp( fpsurvdir, sSetupFileName() );
    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(true) )
	{ uirv = sfio.errMsg(); return 0; }

    ascistream astream( sfio.istrm() );
    if ( !astream.isOfFileType(sKeySI) )
    {
	uirv = tr("Survey definition file cannot be read.\n"
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
    si->setName( File::Path(survdir).fileName() ); // good default

    //Read params here, so we can look at the pars below
    fp = fpsurvdir; fp.add( sDefsFile );
    si->defpars_.read( fp.fullPath(), sKeySurvDefs, true );
    si->defpars_.setName( sKeySurvDefs );

    //Scrub away old settings (confusing to users)
    si->defpars_.removeWithKey( "Depth in feet" );

    si->dirname_ = fpsurvdir.fileName();
    si->basepath_ = fpsurvdir.pathOnly();
    if ( !survdir || si->dirname_.isEmpty() ) return si;

    const IOPar survpar( astream );
    si->usePar(survpar);

    BufferString line;
    while ( astream.stream().getLine(line) )
    {
	if ( !si->comments_.isEmpty() )
	    si->comments_.addNewLine();
	si->comments_.add( line );
    }
    sfio.closeSuccess();

    if ( !si->wrapUpRead() )
	{ delete si; return 0; }

    return si;
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

    File::Path fp( basepath_, dirname_, sCommentsFile );
    od_istream strm( fp.fullPath() );
    if ( strm.isOK() )
	strm.getAll( comments_ );

    return true;
}


bool SurveyInfo::usePar( const IOPar& par )
{
    par.get( sKey::Name(), name_ );
    par.get( sKeyInlRange(),
	    fullcs_.hsamp_.start_.inl(),
	    fullcs_.hsamp_.stop_.inl(),
	    fullcs_.hsamp_.step_.inl() );

    par.get( sKeyCrlRange(),
	    fullcs_.hsamp_.start_.crl(),
	    fullcs_.hsamp_.stop_.crl(),
	    fullcs_.hsamp_.step_.crl() );

    FileMultiString fms;
    if ( par.get( sKeyZRange(), fms ) )
    {
	fullcs_.zsamp_.start = fms.getFValue( 0 );
	fullcs_.zsamp_.stop = fms.getFValue( 1 );
	fullcs_.zsamp_.step = fms.getFValue( 2 );
	if ( Values::isUdf(fullcs_.zsamp_.step)
	       || mIsZero(fullcs_.zsamp_.step,mDefEps) )
	{
	    fullcs_.zsamp_.step = 0.004f;
	}
	if ( fms.size() > 3 )
	{
	    if ( *fms[3] == 'T' )
	    {
		zdef_ = ZDomain::Time();
		depthsinfeet_ = false;
		defpars_.getYN( sKeyDpthInFt(), depthsinfeet_ );
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

    fullcs_.normalise();
    workcs_ = fullcs_;
    return true;
}


IOPar SurveyInfo::defaultPars() const
{
    mLock4Read();
    return defpars_;
}


void SurveyInfo::setDefaultPar( const char* ky, const char* val,
				bool dosave ) const
{
    if ( !ky || !*ky )
	return;

    mLock4Read();
    BufferString curval = defpars_.find( ky );
    if ( curval == val )
	return;

    if ( !mLock2Write() )
    {
	curval = defpars_.find( ky );
	if ( curval == val )
	    return;
    }
    const_cast<SurveyInfo*>(this)->defpars_.update( ky, val );
    mSendChgNotif( cParsChange(), 0 );
    if ( dosave )
	saveDefaultPars();
}


void SurveyInfo::removeKeyFromDefaultPars( const char* ky,
					    bool dosave ) const
{
    if ( !ky || !*ky )
	return;

    mLock4Read();
    if ( !defpars_.find(ky) )
	return;
    if ( !mLock2Write() && !defpars_.find(ky) )
	return;

    const_cast<SurveyInfo*>(this)->defpars_.removeWithKey( ky );
    mSendChgNotif( cParsChange(), 0 );
    if ( dosave )
	saveDefaultPars();
}


void SurveyInfo::setDefaultPars( const IOPar& iop, bool dosave ) const
{
    mLock4Read();
    if ( iop == defpars_ )
	return;
    if ( !mLock2Write() && iop == defpars_ )
	return;
    const_cast<SurveyInfo*>(this)->defpars_ = iop;
    mSendChgNotif( cParsChange(), 0 );
    if ( dosave )
	saveDefaultPars();
}


BufferString SurveyInfo::getFullDirPath() const
{
    mLock4Read();
    return File::Path( basepath_, dirname_ ).fullPath();
}


BufferString SurveyInfo::dirNameForName( const char* nm )
{
    BufferString dirnm = nm;
    dirnm.clean( BufferString::AllowDots );
    return dirnm;
}


StepInterval<int> SurveyInfo::inlRange( bool work ) const
{
    mLock4Read();
    StepInterval<int> ret; Interval<int> dum;
    gtSampling(work).hsamp_.get( ret, dum );
    return ret;
}


StepInterval<int> SurveyInfo::crlRange( bool work ) const
{
    mLock4Read();
    StepInterval<int> ret; Interval<int> dum;
    gtSampling(work).hsamp_.get( dum, ret );
    return ret;
}


TrcKeyZSampling SurveyInfo::sampling( bool work ) const
{
    mLock4Read();
    return mSampling( work );
}


StepInterval<float> SurveyInfo::zRange( bool work ) const
{
    mLock4Read();
    return mSampling(work).zsamp_;
}

int SurveyInfo::maxNrTraces( bool work ) const
{
    mLock4Read();
    return mSampling(work).hsamp_.nrInl() * mSampling(work).hsamp_.nrCrl();
}


int SurveyInfo::inlStep() const
{
    mLock4Read();
    return fullcs_.hsamp_.step_.inl();
}


int SurveyInfo::crlStep() const
{
    mLock4Read();
    return fullcs_.hsamp_.step_.crl();
}


float SurveyInfo::inlDistance() const
{
    return get3DGeometry(false)->inlDistance();
}


float SurveyInfo::crlDistance() const
{
    return get3DGeometry(false)->crlDistance();
}


float SurveyInfo::getArea( Interval<int> inlrg, Interval<int> crlrg ) const
{
    mLock4Read();
    const BinID step = mSampling(false).hsamp_.step_;
    const Coord c00 = transform( BinID(inlrg.start,crlrg.start) );
    const Coord c01 = transform( BinID(inlrg.start,crlrg.stop+step.crl()) );
    const Coord c10 = transform( BinID(inlrg.stop+step.inl(),crlrg.start) );

    const float scale = mXYInFeet() ? mFromFeetFactorF : 1;
    const float d01 = c00.distTo<float>( c01 ) * scale;
    const float d10 = c00.distTo<float>( c10 ) * scale;

    return d01*d10;
}


float SurveyInfo::getArea( bool work ) const
{
    return getArea( inlRange( work ), crlRange( work ) );
}



float SurveyInfo::zStep() const
{
    mLock4Read();
    return fullcs_.zsamp_.step;
}


Coord3 SurveyInfo::oneStepTranslation( const Coord3& planenormal ) const
{
    return get3DGeometry(false)->oneStepTranslation( planenormal );
}


void SurveyInfo::setSipName( const uiString& str )
{
    mLock4Read();
    if ( sipnm_.isEqualTo(str) )
	return;
    if ( mLock2Write() || !sipnm_.isEqualTo(str) )
    {
	sipnm_ = str;
	mSendChgNotif( cAuxDataChange(), ChangeData::cUnspecChgID() );
    }
}


void SurveyInfo::setRange( const TrcKeyZSampling& cs )
{
    fullcs_ = cs;
    fullcs_.hsamp_.survid_ = workcs_.hsamp_.survid_ = TrcKey::std3DSurvID();
    workcs_.limitTo( fullcs_ );
    workcs_.hsamp_.step_ = fullcs_.hsamp_.step_;
    workcs_.zsamp_.step = fullcs_.zsamp_.step;
}


void SurveyInfo::setWorkRange( const TrcKeyZSampling& cs ) const
{
    mLock4Read();
    if ( workcs_ == cs )
	return;
    if ( !mLock2Write() && workcs_ == cs )
	return;

    const_cast<SurveyInfo*>(this)->workcs_ = cs;
    mSendChgNotif( cWorkRangeChange(), 0 );
}


Interval<int> SurveyInfo::reasonableRange( bool inl ) const
{
    mLock4Read();
    const Interval<int> rg = inl
      ? Interval<int>( fullcs_.hsamp_.start_.inl(), fullcs_.hsamp_.stop_.inl() )
      : Interval<int>( fullcs_.hsamp_.start_.crl(), fullcs_.hsamp_.stop_.crl() );

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
    const TrcKeyZSampling cs = sampling( work );
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
    const TrcKeyZSampling cs = sampling( work );
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
    const TrcKeyZSampling cs = sampling( work );
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
    const TrcKeyZSampling cs = sampling( work );
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
    const StepInterval<float> rg = sampling( work ).zsamp_;
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
    const TrcKeyZSampling cs = sampling( work );
    const float eps = 1e-8;
    return cs.hsamp_.includes( bid )
	&& cs.zsamp_.start < z + eps && cs.zsamp_.stop > z - eps;
}


bool SurveyInfo::zIsTime() const
{
    mLock4Read();
    return zdef_.isTime();
}


bool SurveyInfo::zInMeter() const
{
    mLock4Read();
    return zDomain().isDepth() && !depthsinfeet_;
}


bool SurveyInfo::zInFeet() const
{
    mLock4Read();
    return zDomain().isDepth() && depthsinfeet_;
}


float SurveyInfo::showZ2UserFactor() const
{
    mLock4Read();
    return (float)zDomain().userFactor();
}


const char* SurveyInfo::fileZUnitString( bool wp ) const
{
    mLock4Read();
    return zDomain().fileUnitStr( wp );
}


uiString SurveyInfo::zUnitString( bool wp ) const
{
    mLock4Read();
    return zDomain().unitStr( wp );
}


SurveyInfo::Unit SurveyInfo::xyUnit() const
{
    mLock4Read();
    return mXYUnit();
}


SurveyInfo::Unit SurveyInfo::zUnit() const
{
    mLock4Read();
    return mZUnit();
}


void SurveyInfo::putZDomain( IOPar& iop ) const
{
    zdef_.set( iop );
}


const ZDomain::Def& SurveyInfo::zDomain() const
{
    mLock4Read();
    return zdef_;
}


uiString SurveyInfo::xyUnitString( bool abbrvt, bool wb ) const
{
    return uiStrings::sDistUnitString( xyInFeet(), abbrvt, wb );
}


const char* SurveyInfo::fileXYUnitString( bool wb ) const
{
    return getDistUnitString( xyInFeet(), wb );
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
    mLock4Read();
    return mZScale();
}


Coord SurveyInfo::transform( const BinID& b ) const
{
    return get3DGeometry(false)->transform( b );
}


BinID SurveyInfo::transform( const Coord& c ) const
{
    return get3DGeometry(false)->transform( c );
}


void SurveyInfo::get3Pts( Coord c[3], BinID b[2], int& xline ) const
{
    mLock4Read();
    if ( set3binids_[0].inl() )
    {
	b[0] = set3binids_[0]; c[0] = set3coords_[0];
	b[1] = set3binids_[1]; c[1] = set3coords_[1];
	c[2] = set3coords_[2]; xline = set3binids_[2].crl();
    }
    else
    {
	b[0] = fullcs_.hsamp_.start_; c[0] = transform( b[0] );
	b[1] = fullcs_.hsamp_.stop_; c[1] = transform( b[1] );
	BinID b2 = fullcs_.hsamp_.stop_; b2.inl() = b[0].inl();
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
    set3binids_[0] = fullcs_.hsamp_.start_;
    set3binids_[1] = fullcs_.hsamp_.stop_;
    set3binids_[2] = BinID( fullcs_.hsamp_.start_.inl(),
			    fullcs_.hsamp_.stop_.crl() );
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
    char buf[1024];
    snprintf( buf, 1024, "%.10lg`%.10lg`%.10lg", trans.a, trans.b, trans.c );
    par.set( key, buf );
}


bool SurveyInfo::isRightHandSystem() const
{
    return get3DGeometry(false)->isRightHandSystem();
}


bool SurveyInfo::write( const char* basedir ) const
{
    if ( !basedir )
	basedir = basepath_;

    mLock4Read();

    File::Path fp( basedir, dirname_, sSetupFileName() );
    const BufferString dotsurvfnm( fp.fullPath() );
    SafeFileIO sfio( dotsurvfnm, false );
    if ( !sfio.open(false) )
    {
	uiString msg( tr("Cannot open survey info file for write:\n%1\n\n%2")
		      .arg( dotsurvfnm ).arg( sfio.errMsg() ) );
	ErrMsg( msg.getFullString() );
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
    const char* ptr = (const char*)comments_;
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
	BufferString msg( "Error during write of survey info file" );
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
    saveComments( fp.fullPath() );
    return true;
}


void SurveyInfo::fillPar( IOPar& par ) const
{
    mLock4Read();

    par.set( sKey::Name(), name() );
    par.set( sKeySurvDataType(), toString( survDataType()) );
    par.set( sKeyInlRange(), fullcs_.hsamp_.start_.inl(),
	     fullcs_.hsamp_.stop_.inl(), fullcs_.hsamp_.step_.inl() );
    par.set( sKeyCrlRange(), fullcs_.hsamp_.start_.crl(),
	     fullcs_.hsamp_.stop_.crl(), fullcs_.hsamp_.step_.crl() );


    FileMultiString fms = "";
    fms += fullcs_.zsamp_.start; fms += fullcs_.zsamp_.stop;
    fms += fullcs_.zsamp_.step;
    fms += zdef_.isTime() ? "T" : ( depthsinfeet_ ? "F" : "D" );
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


#define uiErrMsg(s) { \
    BufferString cmd( "\"", \
	    File::Path(GetExecPlfDir(),"od_DispMsg").fullPath() ); \
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
    if ( basedir && *basedir )
	surveypath = basedir;
    else
	surveypath = getFullDirPath();

    const BufferString defsfnm( File::Path(surveypath,sDefsFile).fullPath());
    if ( defpars_.isEmpty() )
    {
	if ( File::exists(defsfnm) )
	    File::remove( defsfnm );
    }
    else if ( !defpars_.write( defsfnm, sKeySurvDefs ) )
	uiErrMsg( defsfnm );
}


void SurveyInfo::saveComments( const char* basedir ) const
{
    BufferString surveypath;
    if ( basedir && *basedir )
	surveypath = basedir;
    else
	surveypath = getFullDirPath();

    const BufferString commentsfnm( File::Path(surveypath,sCommentsFile)
				.fullPath());
    if ( comments_.isEmpty() )
    {
	if ( File::exists(commentsfnm) )
	    File::remove( commentsfnm );
    }
    else
    {
	od_ostream strm( commentsfnm );
	strm << comments_;
	if ( !strm.isOK() )
	    uiErrMsg( commentsfnm );
    }
}


bool SurveyInfo::has2D() const
{
    mLock4Read();
    return pol2d_ == Only2D || pol2d_ == Both2DAnd3D;
}


bool SurveyInfo::has3D() const
{
    mLock4Read();
    return pol2d_ == No2D || pol2d_ == Both2DAnd3D;
}


void SurveyInfo::update3DGeometry()
{
    if ( s3dgeom_ )
	s3dgeom_->setGeomData( b2c_, mSampling(false), mZScale() );

    if ( work_s3dgeom_ )
	work_s3dgeom_->setGeomData( b2c_, mSampling(true), mZScale() );
}


RefMan<Survey::Geometry3D> SurveyInfo::get3DGeometry( bool work ) const
{
    mLock4Read();
    const Survey::Geometry3D* sgeom = work ? work_s3dgeom_ : s3dgeom_;

    if ( !sgeom )
    {
	if ( !mLock2Write() )
	    sgeom = work ? work_s3dgeom_ : s3dgeom_;
	if ( !sgeom )
	{
	    RefMan<Survey::Geometry3D> newsgeom
			    = new Survey::Geometry3D( name(), zdef_ );
	    newsgeom->ref();
	    if ( work )
		newsgeom->setID( Survey::GM().default3DSurvID() );
	    newsgeom->setGeomData( b2c_, mSampling(work), mZScale() );
	    newsgeom->ref();
	    SurveyInfo& self = *const_cast<SurveyInfo*>( this );
	    if ( work )
		self.work_s3dgeom_ = newsgeom;
	    else
		self.s3dgeom_ = newsgeom;
	    sgeom = newsgeom;
	}
    }

    return RefMan<Survey::Geometry3D>( const_cast<Survey::Geometry3D*>(sgeom) );
}


Pos::IdxPair2Coord SurveyInfo::binID2Coord() const
{
    mLock4Read();
    return b2c_;
}


SurveyInfo::Pol2D SurveyInfo::survDataType() const
{
    mLock4Read();
    return pol2d_;
}


void SurveyInfo::setSurvDataType( Pol2D p2d ) const
{
    mLock4Read();
    if ( pol2d_ == p2d )
	return;
    if ( !mLock2Write() && pol2d_ == p2d )
	return;
    const_cast<SurveyInfo*>(this)->pol2d_ = p2d;
    mSendChgNotif( cPol2DChange(), 0 );
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
{
    mLock4Read();
    return coordsystem_;
}


ConstRefMan<Coords::PositionSystem> SurveyInfo::getCoordSystem() const
{
    mLock4Read();
    return coordsystem_;
}


bool SurveyInfo::xyInFeet() const
{
    mLock4Read();
    return mXYInFeet();
}


bool SurveyInfo::setCoordSystem( Coords::PositionSystem* system )
{
    if ( system && !system->isOrthogonal() )
	return false;

    coordsystem_ = system;
    return false;
}


uiRetVal SurveyInfo::isValidDataRoot( const char* inpdirnm )
{
    uiRetVal ret;

    File::Path fp( inpdirnm ? inpdirnm : GetBaseDataDir() );
    const BufferString dirnm( fp.fullPath() );
    const uiString datarootstr = tr("OpendTect Data Directory '%1'")
					.arg( dirnm );
    if ( !File::isDirectory(dirnm) || !File::isWritable(dirnm) )
	mErrRetDoesntExist( datarootstr );

    fp.add( ".omf" );
    const BufferString omffnm( fp.fullPath() );
    if ( !File::exists(omffnm) )
	mErrRetDoesntExist( omffnm );

    fp.setFileName( sSurvFile );
    if ( File::exists(fp.fullPath()) )
    {
	// probably we're in a survey. So let's check:
	fp.setFileName( "Seismics" );
	if ( File::isDirectory(fp.fullPath()) )
	    ret.add( tr("%1 has '%2' file").arg(datarootstr).arg(sSurvFile) );
    }

    return ret;
}


uiRetVal SurveyInfo::isValidSurveyDir( const char* dirnm )
{
    uiRetVal ret;

    File::Path fp( dirnm, ".omf" );
    const BufferString omffnm( fp.fullPath() );
    if ( !File::exists(omffnm) )
	mErrRetDoesntExist( omffnm );

    fp.setFileName( sSurvFile );
    const BufferString survfnm( fp.fullPath() );
    if ( !File::exists(survfnm) )
	mErrRetDoesntExist( survfnm );

    fp.setFileName( "Seismics" );
    const BufferString seisdirnm( fp.fullPath() );
    if ( !File::isDirectory(seisdirnm) )
	mErrRetDoesntExist( seisdirnm );

    return ret;
}


#define mFreshFileName() File::Path(getFullDirPath(),sFreshFile).fullPath()
#define mCrFileName() File::Path(getFullDirPath(),sCreationFile).fullPath()


bool SurveyInfo::isFresh() const
{
    return File::exists( mFreshFileName() );
}


void SurveyInfo::setNotFresh() const
{
    File::rename( mFreshFileName(), mCrFileName() );
}


void SurveyInfo::getFreshSetupData( IOPar& iop ) const
{
    iop.read( mFreshFileName(), sKeyFreshFileType );
}


void SurveyInfo::getCreationData( IOPar& iop ) const
{
    if ( isFresh() )
	getFreshSetupData( iop );
    else
	iop.read( mCrFileName(), sKeyFreshFileType );
}


void SurveyInfo::setFreshSetupData( const IOPar& iop ) const
{
    iop.write( mFreshFileName(), sKeyFreshFileType );
}
