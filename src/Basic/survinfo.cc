/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          18-4-1996
________________________________________________________________________

-*/

#include "survinfo.h"

#include "ascstream.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "coordsystem.h"
#include "cubesampling.h"
#include "trckeyzsampling.h"
#include "latlong.h"
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
#include "survgeom3d.h"

mUseType( SurveyInfo, size_type );
mUseType( SurveyInfo, pos_type );
mUseType( SurveyInfo, pos_steprg_type );
mUseType( SurveyInfo, z_type );
mUseType( SurveyInfo, z_steprg_type );
mUseType( SurveyInfo, dist_type );
mUseType( SurveyInfo, area_type );


static const char* sKeySI =		"Survey Info";
static const char* sKeyXTransf =	"Coord-X-BinID";
static const char* sKeyYTransf =	"Coord-Y-BinID";
static const char* sSurvFile =		".survey";
static const char* sDefsFile =		".defs";
static const char* sCommentsFile =	".comments";
static const char* sFreshFile =		".fresh";
static const char* sCreationFile =	".creation";
static const char* sKeySurvDefs =	"Survey defaults";
static const char* sKeyFreshFileType =	"Survey Creation Info";
static const char* sKeyLatLongAnchor =	"Lat/Long anchor";
static const char* sKeySetPointPrefix =	"Set Point";
const char* SurveyInfo::sKeyInlRange()	{ return "In-line range"; }
const char* SurveyInfo::sKeyCrlRange()	{ return "Cross-line range"; }
const char* SurveyInfo::sKeyXRange()	{ return "X range"; }
const char* SurveyInfo::sKeyYRange()	{ return "Y range"; }
const char* SurveyInfo::sKeyZRange()	{ return "Z range"; }
const char* SurveyInfo::sKeyDpthInFt()	{ return "Show depth in feet"; }
const char* SurveyInfo::sKeyXYInFt()	{ return "XY in feet"; }
const char* SurveyInfo::sKeySurvDataType() { return "Survey Data Type"; }
const char* SurveyInfo::sKeySeismicRefDatum()
					{ return "Seismic Reference Datum"; }

uiString SurveyInfo::sInlRange()	{ return tr("In-line range"); }
uiString SurveyInfo::sCrlRange()	{ return tr("Cross-line range"); }
uiString SurveyInfo::sXRange()		{ return tr("X range"); }
uiString SurveyInfo::sYRange()		{ return tr("Y range"); }
uiString SurveyInfo::sZRange()		{ return tr("Z range"); }
uiString SurveyInfo::sDpthInFt()	{ return tr("Show depth in feet"); }
uiString SurveyInfo::sXYInFt()		{ return tr("XY in feet"); }
uiString SurveyInfo::sSurvDataType()	{ return tr("Survey Data Type"); }
uiString SurveyInfo::sSeismicRefDatum()	{ return tr("Seismic Reference Datum");}

mDefineEnumUtils(SurveyInfo,Pol2D3D,"Survey Type")
{ "Only 3D", "Both 2D and 3D", "Only 2D", 0 };

template<>
void EnumDefImpl<SurveyInfo::Pol2D3D>::init()
{
    uistrings_ += mEnumTr("Only 3D",0);
    uistrings_ += mEnumTr("Both 2D and 3D",0);
    uistrings_ += mEnumTr("Only 2D",0);
}

#define mXYInFeet() (coordsystem_ && coordsystem_->isFeet())
#define mXYUnit() (mXYInFeet() ? Feet : Meter)
#define mZUnit() (zdef_.isTime() ? Second : (depthsinfeet_ ? Feet : Meter))
#define mZScale() defaultXYtoZScale( mZUnit(), mXYUnit() )


static PtrMan<SurveyInfo> global_si_ = nullptr;

static void DeleteSI()
{
    global_si_ = nullptr;
}


const SurveyInfo& SI( const SurveyInfo* si )
{
    if ( si )
	return *si;

    if ( !global_si_ && !IsExiting() )
    {
	if ( global_si_.setIfNull(new SurveyInfo,true) )
	    NotifyExitProgram( &DeleteSI );
    }

    return *global_si_;
}


SurveyInfo::SurveyInfo()
    : zdef_(*new ZDomain::Def(ZDomain::Time()) )
    , depthsinfeet_(false)
    , defpars_(sKeySurvDefs)
    , pol2d3d_(OD::Both2DAnd3D)
    , pol2d3dknown_(false)
    , seisrefdatum_(0.f)
    , s3dgeom_(0)
    , work_s3dgeom_(0)
    , fullcs_(*new CubeSampling(false))
    , workcs_(*new CubeSampling(false))
{
    Pos::IdxPair2Coord::DirTransform xtr, ytr;
    xtr.b = ytr.c = 1;
    rdb2c_.setTransforms( xtr, ytr );
    set3binids_[2].crl() = 0;

	// We need a 'reasonable' transform even when no proper SI is available
	// For example DataPointSets need to work
    xtr.b = 1000; xtr.c = 0;
    ytr.b = 0; ytr.c = 1000;
    b2c_.setTransforms( xtr, ytr );

    setToUnlocatedCoordSys( false );
}



SurveyInfo::SurveyInfo( const SurveyInfo& oth )
    : NamedMonitorable( oth )
    , defpars_(sKeySurvDefs)
    , zdef_(*new ZDomain::Def( oth.zDomain() ) )
    , s3dgeom_(0)
    , work_s3dgeom_(0)
    , fullcs_(*new CubeSampling(false))
    , workcs_(*new CubeSampling(false))
    , diskloc_(oth.diskloc_)
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
    diskloc_ = oth.diskloc_;
    coordsystem_ = oth.coordsystem_;
    depthsinfeet_ = oth.depthsinfeet_;
    b2c_ = oth.b2c_;
    pol2d3d_ = oth.pol2d3d_;
    pol2d3dknown_ = oth.pol2d3dknown_;
    for ( int idx=0; idx<3; idx++ )
    {
	set3binids_[idx] = oth.set3binids_[idx];
	set3coords_[idx] = oth.set3coords_[idx];
    }
    fullcs_ = oth.fullcs_;
    workcs_ = oth.workcs_;
    defpars_ = oth.defpars_;
    seisrefdatum_ = oth.seisrefdatum_;
    rdb2c_ = oth.rdb2c_;
    sipnm_ = oth.sipnm_;
    comments_ = oth.comments_;
    updateGeometries();
}


#define mCmpRet(memb,ct) \
    if ( !(memb==oth.memb) ) \
        return ct()
#define mCmpRetDeRef(memb,ct) \
    if ( !((*memb)==(*oth.memb)) ) \
        return ct()

Monitorable::ChangeType SurveyInfo::compareClassData(
					    const SurveyInfo& oth ) const
{
    mCmpRet( diskloc_, cEntireObjectChange );
    mCmpRet( zdef_, cSetupChange );
    mCmpRet( b2c_, cSetupChange );
    mCmpRet( pol2d3d_, cSetupChange );
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
    if ( sipnm_ != oth.sipnm_ )
        return cAuxDataChange();

    mCmpRet( name_, cNameChange );
    mCmpRet( comments_, cCommentChange );

    return cNoChange();
}


void SurveyInfo::setToUnlocatedCoordSys( bool xyinfeet )
{
    RefMan<Coords::UnlocatedXY> undefsystem = new Coords::UnlocatedXY;
    undefsystem->setIsFeet( xyinfeet );
    coordsystem_ = undefsystem;
}


#define mErrRetDoesntExist(fnm) \
    { ret.add( uiStrings::phrFileDoesNotExist(fnm) ); return ret; }


uiRetVal SurveyInfo::setSurveyLocation( const SurveyDiskLocation& reqloc,
					bool forcerefresh )
{
    uiRetVal ret;
    if ( !forcerefresh && reqloc == SI().diskloc_ )
	return ret;
    if ( !File::isDirectory(reqloc.basePath()) )
	mErrRetDoesntExist(reqloc.basePath())
    File::Path fp( reqloc.basePath(), reqloc.dirName() );
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

    *global_si_ = *newsi;
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

    si->diskloc_ = SurveyDiskLocation( fpsurvdir );
    if ( !survdir || si->diskloc_.isEmpty() )
	return si;

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

    if ( !rdb2c_.isValid() )
    {
	BufferString errmsg( "Survey ", name() );
	errmsg.add( " has an invalid coordinate transformation" );
	ErrMsg( errmsg );
	return false;
    }
    b2c_ = rdb2c_;

    File::Path fp( diskloc_.fullPath(), sCommentsFile );
    od_istream strm( fp.fullPath() );
    if ( strm.isOK() )
	strm.getAll( comments_ );

    updateGeometries();
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
	Pol2D3D p2d3d = OD::Both2DAnd3D;
	Pol2D3DDef().parse( survdatatype, p2d3d );
	setSurvDataType( p2d3d );
    }

    PtrMan<IOPar> crspar = par.subselect( sKey::CoordSys() );
    if ( crspar )
	coordsystem_ = Coords::CoordSystem::createSystem( *crspar );

    if ( !coordsystem_ )
    {
	/*Try to read the parameters, there may be reference latlog and
	  Coordinates in there. */
	bool xyinfeet = false;
	par.getYN( sKeyXYInFt(), xyinfeet );
	BufferString anchor;
	if ( par.get(sKeyLatLongAnchor,anchor) )
	{
	    char* ptr = anchor.find( '=' );
	    if ( ptr )
	    {
		*ptr++ = '\0';
		Coord c; LatLong l;
		if ( c.fromString(anchor) && l.fromString(ptr)
		&& ( !(mIsZero(c.x_,1e-3) && mIsZero(c.y_,1e-3)) ) )
		{
		    RefMan<Coords::AnchorBasedXY> anchoredsystem =
					    new Coords::AnchorBasedXY( l, c );
		    anchoredsystem->setIsFeet( xyinfeet );
		    coordsystem_ = anchoredsystem;
		}
	    }
	}

	if ( !coordsystem_ )
	    setToUnlocatedCoordSys( xyinfeet );
    }

    par.get( sKeySeismicRefDatum(), seisrefdatum_ );
    rdb2c_ = b2c_;
    rdb2c_.usePar( par );

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


IOPar SurveyInfo::getDefaultPars() const
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
    return diskloc_.fullPath();
}


BufferString SurveyInfo::dirNameForName( const char* nm )
{
    BufferString dirnm = nm;
    dirnm.clean( BufferString::AllowDots );
    return dirnm;
}


pos_steprg_type SurveyInfo::inlRange( SurvLimitType slt ) const
{
    mLock4Read();
    return gt3DGeom( slt ).inlRange();
}


pos_steprg_type SurveyInfo::crlRange( SurvLimitType slt ) const
{
    mLock4Read();
    return gt3DGeom( slt ).crlRange();
}


z_steprg_type SurveyInfo::zRange( SurvLimitType slt ) const
{
    mLock4Read();
    return gt3DGeom( slt ).zRange();
}


void SurveyInfo::getHorSampling( HorSampling& hs, SurvLimitType slt ) const
{
    mLock4Read();
    const auto& geom = gt3DGeom( slt );
    hs.start_ = BinID( geom.inlRange().start, geom.crlRange().start );
    hs.stop_ = BinID( geom.inlRange().stop, geom.crlRange().stop );
    hs.step_ = BinID( geom.inlRange().step, geom.crlRange().step );
}


void SurveyInfo::getCubeSampling( CubeSampling& cs, SurvLimitType slt ) const
{
    mLock4Read();
    const auto& geom = gt3DGeom( slt );
    cs.hsamp_.start_ = BinID( geom.inlRange().start, geom.crlRange().start );
    cs.hsamp_.stop_ = BinID( geom.inlRange().stop, geom.crlRange().stop );
    cs.hsamp_.step_ = BinID( geom.inlRange().step, geom.crlRange().step );
    cs.zsamp_ = geom.zRange();
}


void SurveyInfo::getSampling( TrcKeySampling& tks, SurvLimitType slt ) const
{
    mLock4Read();
    const auto& geom = gt3DGeom( slt );
    tks.start_ = BinID( geom.inlRange().start, geom.crlRange().start );
    tks.stop_ = BinID( geom.inlRange().stop, geom.crlRange().stop );
    tks.step_ = BinID( geom.inlRange().step, geom.crlRange().step );
}


void SurveyInfo::getSampling( TrcKeyZSampling& tkzs, SurvLimitType slt ) const
{
    mLock4Read();
    const auto& geom = gt3DGeom( slt );
    tkzs.hsamp_.start_ = BinID( geom.inlRange().start, geom.crlRange().start );
    tkzs.hsamp_.stop_ = BinID( geom.inlRange().stop, geom.crlRange().stop );
    tkzs.hsamp_.step_ = BinID( geom.inlRange().step, geom.crlRange().step );
    tkzs.zsamp_ = geom.zRange();
}


size_type SurveyInfo::maxNrTraces( SurvLimitType slt ) const
{
    mLock4Read();
    const auto& geom = gt3DGeom( slt );
    return (geom.inlRange().nrSteps()+1) * (geom.crlRange().nrSteps()+1);
}


pos_type SurveyInfo::inlStep( SurvLimitType slt ) const
{
    mLock4Read();
    return gt3DGeom(slt).inlRange().step;
}


pos_type SurveyInfo::crlStep( SurvLimitType slt ) const
{
    mLock4Read();
    return gt3DGeom(slt).crlRange().step;
}


z_type SurveyInfo::zStep( SurvLimitType slt ) const
{
    mLock4Read();
    return gt3DGeom(slt).zRange().step;
}


BinID SurveyInfo::steps( SurvLimitType slt ) const
{
    mLock4Read();
    return BinID( inlStep(slt), crlStep(slt) );
}


dist_type SurveyInfo::inlDistance() const
{
    mLock4Read();
    return gt3DGeom().inlDistance();
}


dist_type SurveyInfo::crlDistance() const
{
    mLock4Read();
    return gt3DGeom().crlDistance();
}


Coord SurveyInfo::distances() const
{
    mLock4Read();
    return Coord( gt3DGeom().inlDistance(), gt3DGeom().crlDistance() );
}


area_type SurveyInfo::getArea( Interval<pos_type> inlrg,
			       Interval<pos_type> crlrg ) const
{
    mLock4Read();

    const BinID step = BinID( gt3DGeom().inlRange().step,
			      gt3DGeom().crlRange().step );

    const Coord c00 = transform( BinID(inlrg.start,crlrg.start) );
    const Coord c01 = transform( BinID(inlrg.start,crlrg.stop+step.crl()) );
    const Coord c10 = transform( BinID(inlrg.stop+step.inl(),crlrg.start) );

    const dist_type scale = mXYInFeet() ? mFromFeetFactorF : 1;
    const dist_type d01 = c00.distTo<dist_type>( c01 ) * scale;
    const dist_type d10 = c00.distTo<dist_type>( c10 ) * scale;

    return d01 * d10;
}


area_type SurveyInfo::getArea( SurvLimitType slt ) const
{
    return getArea( inlRange(slt), crlRange(slt) );
}


Coord3 SurveyInfo::oneStepTranslation( const Coord3& planenormal ) const
{
    return gt3DGeom().oneStepTranslation( planenormal );
}


void SurveyInfo::setSipName( const uiString& str )
{
    mLock4Read();
    if ( sipnm_ == str )
	return;
    if ( mLock2Write() || sipnm_ != str )
    {
	sipnm_ = str;
	mSendChgNotif( cAuxDataChange(), ChangeData::cUnspecChgID() );
    }
}


void SurveyInfo::setRanges( const CubeSampling& cs )
{
    auto& geom = gt3DGeom( OD::FullSurvey );
    geom.setRanges( cs.hsamp_.inlRange(), cs.hsamp_.crlRange(), cs.zsamp_ );
    auto& workgeom = gt3DGeom( OD::UsrWork );
    workgeom.setRanges( geom.inlRange(), geom.crlRange(), geom.zRange() );
}


void SurveyInfo::setWorkRanges( const CubeSubSel& css ) const
{
    mLock4Read();
    if ( css.isAll() )
	return;
    if ( !mLock2Write() && css.isAll() )
	return;

    const_cast<SurveyInfo*>(this)->workcs_ = CubeSampling( css );
    mSendChgNotif( cWorkRangeChange(), 0 );
}


Interval<pos_type> SurveyInfo::reasonableRange( bool inl ) const
{
    mLock4Read();
    const auto& rg = inl ? gt3DGeom().inlRange() : gt3DGeom().crlRange();
    const pos_type w = rg.stop - rg.start;
    return Interval<pos_type>( rg.start - 3*w, rg.stop +3*w );
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

Coord SurveyInfo::minCoord( SurvLimitType slt ) const
{
    CubeSampling cs; getCubeSampling( cs, slt );
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

Coord SurveyInfo::maxCoord( SurvLimitType slt ) const
{
    CubeSampling cs; getCubeSampling( cs, slt );
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


void SurveyInfo::checkInlRange( Interval<pos_type>& intv, SurvLimitType slt ) const
{
    intv.sort();
    mLock4Read();
    const auto& inlrg = gt3DGeom( slt ).inlRange();
    intv.limitTo( inlrg );
    intv.start = inlrg.snap( intv.start, OD::SnapUpward );
    intv.stop = inlrg.snap( intv.stop, OD::SnapDownward );
}


void SurveyInfo::checkCrlRange( Interval<pos_type>& intv, SurvLimitType slt ) const
{
    intv.sort();
    mLock4Read();
    const auto& crlrg = gt3DGeom( slt ).crlRange();
    intv.limitTo( crlrg );
    intv.start = crlrg.snap( intv.start, OD::SnapUpward );
    intv.stop = crlrg.snap( intv.stop, OD::SnapDownward );
}



void SurveyInfo::checkZRange( Interval<z_type>& intv, SurvLimitType slt ) const
{
    intv.sort();
    mLock4Read();
    const auto& zrg = gt3DGeom( slt ).zRange();
    intv.limitTo( zrg );
    intv.start = zrg.snap( intv.start, OD::SnapUpward );
    intv.stop = zrg.snap( intv.stop, OD::SnapDownward );
}


bool SurveyInfo::includes( const BinID& bid, SurvLimitType slt ) const
{
    mLock4Read();
    const CubeSampling& cs = isWork(slt) ? workcs_ : fullcs_;
    return cs.hsamp_.includes( bid );
}


bool SurveyInfo::includes( const BinID& bid, const z_type z,
			   SurvLimitType slt ) const
{
    mLock4Read();
    const CubeSampling& cs = isWork(slt) ? workcs_ : fullcs_;
    const z_type eps = 1e-8;
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


z_type SurveyInfo::showZ2UserFactor() const
{
    mLock4Read();
    return z_type(zDomain().userFactor());
}


const char* SurveyInfo::fileZUnitString( bool wp ) const
{
    mLock4Read();
    return zDomain().fileUnitStr( wp );
}


uiString SurveyInfo::zUnitString() const
{
    mLock4Read();
    return zDomain().unitStr();
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


uiString SurveyInfo::xyUnitString( bool abbrvt ) const
{
    return uiStrings::sDistUnitString( xyInFeet(), abbrvt );
}


const char* SurveyInfo::fileXYUnitString( bool wp ) const
{
    return getDistUnitString( xyInFeet(), wp );
}


void SurveyInfo::setZUnit( bool istime, bool infeet )
{
    zdef_ = istime ? ZDomain::Time() : ZDomain::Depth();
    depthsinfeet_ = infeet;
}


z_type SurveyInfo::defaultXYtoZScale( Unit zunit, Unit xyunit )
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


z_type SurveyInfo::zScale() const
{
    mLock4Read();
    return mZScale();
}


Coord SurveyInfo::transform( const BinID& bid ) const
{
    return gt3DGeom().transform( bid );
}


BinID SurveyInfo::transform( const Coord& crd, SurvLimitType slt ) const
{
    return gt3DGeom(slt).transform( crd );
}


void SurveyInfo::get3Pts( Coord c[3], BinID b[2], pos_type& xline ) const
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


uiString SurveyInfo::set3Pts( const Coord c[3], const BinID b[2],
			      Index_Type xline )
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
    set3binids_[0] = fullcs_.hsamp_.start_;
    set3binids_[1] = fullcs_.hsamp_.stop_;
    set3binids_[2] = BinID( fullcs_.hsamp_.start_.inl(),
			    fullcs_.hsamp_.stop_.crl() );
    set3coords_[0] = transform( set3binids_[0] );
    set3coords_[1] = transform( set3binids_[1] );
    set3coords_[2] = transform( set3binids_[2] );
}


void SurveyInfo::snap( BinID& binid, OD::SnapDir dir ) const
{
    gt3DGeom().snap( binid, dir );
}


void SurveyInfo::snapStep( BinID& step ) const
{
    gt3DGeom().snapStep( step );
}


void SurveyInfo::snapZ( z_type& z, OD::SnapDir dir ) const
{
    gt3DGeom().snapZ( z, dir );
}


bool SurveyInfo::isRightHandSystem() const
{
    return gt3DGeom().isRightHandSystem();
}


bool SurveyInfo::write( const char* basedir ) const
{
    mLock4Read();
    BufferString basedirstr( basedir );
    if ( basedirstr.isEmpty() )
	basedirstr = diskloc_.basePath();

    File::Path fp( basedirstr, diskloc_.dirName(), sSetupFileName() );
    const BufferString dotsurvfnm( fp.fullPath() );
    SafeFileIO sfio( dotsurvfnm, false );
    if ( !sfio.open(false) )
    {
	ErrMsg( tr("Cannot open survey info file for write:\n%1\n\n%2")
		      .arg( dotsurvfnm ).arg( sfio.errMsg() ) );
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
    BufferString commentsbuf( comments_ );
    char* lineptr = commentsbuf.getCStr();
    if ( lineptr && *lineptr )
    {
	while ( *lineptr )
	{
	    char* nlptr = firstOcc( lineptr, '\n' );
	    if ( !nlptr )
	    {
		if ( *lineptr )
		    strm << lineptr;
		break;
	    }
	    *nlptr = '\0';
	    strm << lineptr << '\n';
	    *nlptr = '\n';
	    lineptr = nlptr + 1;
	}
	strm << '\n';
    }

    if ( !strm.isOK() )
    {
	sfio.closeFail();
	BufferString msg( "Error during write of survey info file.\n" );
	msg += ::toString( strm.errMsg() );
	ErrMsg( msg );
	return false;
    }
    else if ( !sfio.closeSuccess() )
    {
	BufferString msg( "Error closing survey info file:\n" );
	msg += ::toString( sfio.errMsg() );
	ErrMsg( msg );
	return false;
    }

    fp.set( basedirstr ).add( diskloc_.dirName() );
    const BufferString savedir( fp.fullPath() );
    saveDefaultPars( savedir );
    saveComments( savedir );
    return true;
}


static void putTrfInIOPar( const Pos::IdxPair2Coord::DirTransform& trans,
			   IOPar& par, const char* key )
{
    const BufferString stra( toStringPrecise(trans.a) );
    const BufferString strb( toStringPrecise(trans.b) );
    const BufferString strc( toStringPrecise(trans.c) );
    par.set( key, stra, strb, strc );
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

    putTrfInIOPar( b2c_.getTransform(true), par, sKeyXTransf );
    putTrfInIOPar( b2c_.getTransform(false), par, sKeyYTransf );

    for ( int idx=0; idx<3; idx++ )
    {
	SeparString ky( sKeySetPointPrefix, '.' );
	ky += idx + 1;
	fms = set3binids_[idx].toString();
	fms += set3coords_[idx].toString();
	par.set( ky.buf(), fms.buf() );
    }

    par.removeSubSelection( sKey::CoordSys() );
    IOPar crspar;
    coordsystem_->fillPar( crspar );
    par.mergeComp( crspar, sKey::CoordSys() );

    // To prevent overwring by v6.0 and older
    IOPar& defpars = const_cast<SurveyInfo*>(this)->defpars_;
    defpars.removeSubSelection( sKey::CoordSys() );
    defpars.mergeComp( crspar, sKey::CoordSys() );

    // Needed by v6.0 and older
    par.setYN( sKeyXYInFt(), xyInFeet() );
    par.set( sKeySeismicRefDatum(), seisrefdatum_ );
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
    else
    {
	IOPar iop( defpars_ );
	iop.sortOnKeys();
	if ( !iop.write( defsfnm, sKeySurvDefs ) )
	    uiErrMsg( defsfnm );
    }
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
    return ::has2D( pol2d3d_ );
}


bool SurveyInfo::has3D() const
{
    mLock4Read();
    return ::has3D( pol2d3d_ );
}


void SurveyInfo::updateGeometry( Geometry3D& g3d, const CubeSampling& cs )
{
    BufferString nm( name() );
    if ( &g3d == work_s3dgeom_ )
	nm.add( " [work]" );
    g3d.setName( nm );
    g3d.setTransform( b2c_ );
    g3d.setRanges( cs.inlRange(), cs.crlRange(), cs.zRange() );
}


void SurveyInfo::updateGeometries()
{
    if ( s3dgeom_ )
	updateGeometry( *s3dgeom_, fullcs_ );
    if ( work_s3dgeom_ )
	updateGeometry( *work_s3dgeom_, workcs_ );
}


RefMan<SurvGeom3D> SurveyInfo::get3DGeometry( SurvLimitType slt )
{
    return &gt3DGeom( slt );
}


ConstRefMan<SurvGeom3D> SurveyInfo::get3DGeometry( SurvLimitType slt ) const
{
    return &gt3DGeom( slt );
}


SurvGeom3D& SurveyInfo::gt3DGeom( SurvLimitType slt ) const
{
    // No read locking here, and callers may choose not to lock.
    // The locking/unlocking can simply be too big of a performance hit
    // In practice, this means we can only be hurt in a very, very unlucky case

    const bool work = isWork( slt );
    const Geometry3D* sgeom = work ? work_s3dgeom_ : s3dgeom_;

    if ( !sgeom )
    {
	Threads::Locker locker( make3dgeomlock_ );
	sgeom = work ? work_s3dgeom_ : s3dgeom_;
	if ( !sgeom )
	{
	    auto* newsgeom = new Geometry3D( name() );
	    const_cast<SurveyInfo*>(this)->updateGeometry( *newsgeom,
					    work ? workcs_ : fullcs_ );

	    SurveyInfo& self = *const_cast<SurveyInfo*>( this );
	    if ( work )
		self.work_s3dgeom_ = newsgeom;
	    else
		self.s3dgeom_ = newsgeom;

	    newsgeom->ref();
	    sgeom = newsgeom;
	}
    }

    return *const_cast<Geometry3D*>( sgeom );
}


Pos::IdxPair2Coord SurveyInfo::binID2Coord() const
{
    mLock4Read();
    return b2c_;
}


SurveyInfo::Pol2D3D SurveyInfo::survDataType() const
{
    mLock4Read();
    return pol2d3d_;
}


void SurveyInfo::setSurvDataType( Pol2D3D p2d3d ) const
{
    mLock4Read();
    if ( pol2d3d_ == p2d3d )
	return;
    if ( !mLock2Write() && pol2d3d_ == p2d3d )
	return;
    const_cast<SurveyInfo*>(this)->pol2d3d_ = p2d3d;
    mSendChgNotif( cPol2D3DChange(), 0 );
}


float SurveyInfo::angleXInl() const
{
    Coord xy1 = transform( BinID(inlRange().start, crlRange().start));
    Coord xy2 = transform( BinID(inlRange().start, crlRange().stop) );
    const auto xdiff = xy2.x_ - xy1.x_;
    const auto ydiff = xy2.y_ - xy1.y_;
    return (float)Math::Atan2( ydiff, xdiff );
}


float SurveyInfo::angleXCrl() const
{
    Coord xy1 = transform( BinID(inlRange().start, crlRange().start));
    Coord xy2 = transform( BinID(inlRange().stop, crlRange().start) );
    const double xdiff = xy2.x_ - xy1.x_;
    const double ydiff = xy2.y_ - xy1.y_;
    return (float)Math::Atan2( ydiff, xdiff );
}


RefMan<Coords::CoordSystem> SurveyInfo::getCoordSystem()
{
    mLock4Read();
    return coordsystem_;
}


ConstRefMan<Coords::CoordSystem> SurveyInfo::getCoordSystem() const
{
    mLock4Read();
    return coordsystem_;
}


bool SurveyInfo::xyInFeet() const
{
    mLock4Read();
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
    File::Path fp( getFullDirPath() );
    fp.add( sSetupFileName() );
    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(true) )
	return;

    ascistream astream( sfio.istrm() );
    if ( !astream.isOfFileType(sKeySI) )
	{ sfio.closeSuccess(); return; }

    astream.next();
    const IOPar survpar( astream );


    PtrMan<IOPar> crspar = survpar.subselect( sKey::CoordSys() );
    if ( !crspar )
	defpars_.subselect( sKey::CoordSys() );

    RefMan<Coords::CoordSystem> newsys = !crspar ? 0
				: Coords::CoordSystem::createSystem( *crspar );
    if ( newsys )
	const_cast<SurveyInfo*>(this)->coordsystem_ = newsys;

    sfio.closeSuccess();
}


uiRetVal SurveyInfo::isValidDataRoot( const char* inpdirnm )
{
    uiRetVal ret;

    File::Path fp( inpdirnm ? inpdirnm : GetBaseDataDir() );
    const BufferString dirnm( fp.fullPath() );
    if ( !File::isDirectory(dirnm) || !File::isWritable(dirnm) )
	mErrRetDoesntExist( dirnm );

    fp.add( ".omf" );
    const BufferString omffnm( fp.fullPath() );
    if ( !File::exists(omffnm) )
	mErrRetDoesntExist( omffnm );

    fp.setFileName( sSurvFile );
    if ( File::exists(fp.fullPath()) )
    {
	// probably we're in a survey. So let's check:
	fp.setFileName( "Misc" );
	if ( File::isDirectory(fp.fullPath()) )
	    ret.add( tr("'%1' has '%2' file").arg(dirnm).arg(sSurvFile) );
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

    fp.setFileName( sSeismicSubDir() );
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

	const File::Path fp( basedir, dirnm, SurveyInfo::sSetupFileName() );
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
