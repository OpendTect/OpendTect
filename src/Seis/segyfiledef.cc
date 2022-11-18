/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "segyfiledef.h"
#include "segyhdr.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "oddirs.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "separstr.h"
#include "ctxtioobj.h"
#include "seistrctr.h"
#include "perthreadrepos.h"

namespace SEGY
{
const char* FilePars::sKeyForceRev0()	   { return "Force Rev0"; }
const char* FilePars::sKeyRevision()	   { return "Revision"; }
const char* FilePars::sKeyNrSamples()	   { return "Nr samples overrule"; }
const char* FilePars::sKeyNumberFormat()   { return "Number format"; }
const char* FilePars::sKeyByteSwap()	   { return "Byte swapping"; }
const char* FileReadOpts::sKeyCoordScale() { return "Coordinate scaling "
						    "overrule"; }
const char* FileReadOpts::sKeyTimeShift()  { return "Start time overrule"; }
const char* FileReadOpts::sKeySampleIntv() { return "Sample rate overrule"; }
const char* FileReadOpts::sKeyICOpt()	   { return "IC -> XY"; }
const char* FileReadOpts::sKeyHaveTrcNrs() { return "Have trace numbers"; }
const char* FileReadOpts::sKeyTrcNrDef()   { return "Generate trace numbers"; }
const char* FileReadOpts::sKeyPSOpt()	   { return "Offset source"; }
const char* FileReadOpts::sKeyCoordOpt()   { return "Coord source"; }
const char* FileReadOpts::sKeyOffsDef()	   { return "Generate offsets"; }
const char* FileReadOpts::sKeyCoordStart() { return "Generate coords.Start"; }
const char* FileReadOpts::sKeyCoordStep() { return "Generate coords.Step"; }
const char* FileReadOpts::sKeyCoordFileName() { return "Coordinate file"; }
}


static const char* allsegyfmtoptions[] = {
	"From file header",
	"1 - Floating point",
	"2 - Integer (32 bits)",
	"3 - Integer (16 bits)",
	"5 - IEEE float (32 bits)",
	"8 - Signed char (8 bits)",
	0
};

SEGY::FileSpec::FileSpec( const char* fnm )
    : ::FileSpec(fnm)
{}


SEGY::FileSpec::FileSpec( const IOPar& iop )
    : ::FileSpec(iop)
{}


SEGY::FileSpec::~FileSpec()
{}


IOObj* SEGY::FileSpec::getIOObj( bool tmp ) const
{
    IOStream* iostrm;
    const MultiID seisdirky( mIOObjContext(SeisTrc).getSelKey() );
    if ( tmp )
    {
	MultiID idstr( seisdirky );
	idstr.setObjectID( IOObj::tmpID() );
	iostrm = new IOStream( usrStr(), idstr.toString() );
    }
    else
    {
	iostrm = new IOStream( usrStr() );
	iostrm->acquireNewKeyIn( seisdirky );
    }

    iostrm->fileSpec() = *this;
    iostrm->setGroup( SeisTrcTranslatorGroup::sGroupName() );
    iostrm->setTranslator( "SEG-Y" );
    iostrm->setDirName( "Seismics" );

    return iostrm;
}


void SEGY::FileSpec::fillParFromIOObj( const IOObj& ioobj, IOPar& iop )
{
    mDynamicCastGet(const IOStream*,iostrm,&ioobj)
    if ( !iostrm )
	iostrm->fileSpec().fillPar( iop );
}


SEGY::FilePars::FilePars( bool forread )
    : fmt_(forread?0:1)
    , forread_(forread)
{
    mAttachCB(IOM().surveyToBeChanged, FilePars::onSurveyChgCB);
}


SEGY::FilePars::FilePars( const FilePars& oth )
{
    *this = oth;
    mAttachCB(IOM().surveyToBeChanged, FilePars::onSurveyChgCB);
}


SEGY::FilePars::~FilePars()
{
    detachAllNotifiers();
}


SEGY::FilePars& SEGY::FilePars::operator=( const FilePars& oth )
{
    if ( &oth == this )
	return *this;

    ns_ = oth.ns_;
    fmt_ = oth.fmt_;
    byteswap_ = oth.byteswap_;
    forread_ = oth.forread_;
    coordsys_ = oth.coordsys_;
    return *this;
}


void SEGY::FilePars::onSurveyChgCB(CallBacker*)
{
    if ( coordsys_.ptr() == SI().getCoordSystem().ptr() )
	coordsys_ = nullptr;
}


ConstRefMan<Coords::CoordSystem> SEGY::FilePars::getCoordSys() const
{
    if ( !coordsys_ )
	mSelf().coordsys_ = SI().getCoordSystem();

    return coordsys_;
}


const char** SEGY::FilePars::getFmts( bool fr )
{
    return fr ? allsegyfmtoptions : allsegyfmtoptions+1;
}


void SEGY::FilePars::setForRead( bool fr )
{
    forread_ = fr;
    if ( !forread_ )
    {
	ns_ = 0;
	if ( fmt_ == 0 ) fmt_ = 1;
    }
}


void SEGY::FilePars::fillPar( IOPar& iop ) const
{
    if ( ns_ > 0 )
	iop.set( sKeyNrSamples(), ns_ );
    else
	iop.removeWithKey( sKeyNrSamples() );
    if ( fmt_ != 0 )
	iop.set( sKeyNumberFormat(), nameOfFmt(fmt_,forread_) );
    else
	iop.removeWithKey( sKeyNumberFormat() );
    iop.set( sKeyByteSwap(), byteswap_ );
}


bool SEGY::FilePars::usePar( const IOPar& iop )
{
    const bool foundns = iop.get( sKeyNrSamples(), ns_ );
    const bool foundbs = iop.get( sKeyByteSwap(), byteswap_ );
    const BufferString fmtstr = iop.find( sKeyNumberFormat() );
    const bool foundnf = !fmtstr.isEmpty();
    if ( foundnf )
	fmt_ = fmtOf( fmtstr, forread_ );

    return foundns || foundbs || foundnf;
}


void SEGY::FilePars::getReport( IOPar& iop, bool ) const
{
    if ( ns_ > 0 )
	iop.set( "Number of samples used", ns_ );
    if ( fmt_ > 0 )
	iop.set( forread_ ? "SEG-Y 'format' used" : "SEG-Y 'format'",
		nameOfFmt(fmt_,forread_) );
    if ( byteswap_ )
    {
	const char* str =
	    byteswap_ == 2 ? (forread_?"All bytes are":"All bytes will be")
	 : (byteswap_ == 1 ? (forread_?"Data bytes are":"Data bytes will be")
		       : (forread_?"Header bytes are":"Header bytes will be") );
	iop.set( str, "swapped" );
    }
}


const char* SEGY::FilePars::nameOfFmt( int fmt, bool forread )
{
    const char** fmts = getFmts(true);
    if ( fmt > 0 && fmt < 4 )
	return fmts[fmt];
    if ( fmt == 5 )
	return fmts[4];
    if ( fmt == 8 )
	return fmts[5];

    return forread ? fmts[0] : nameOfFmt( 1, false );
}


int SEGY::FilePars::fmtOf( const char* str, bool forread )
{
    if ( !str || !*str || !iswdigit(*str) )
	return forread ? 0 : 1;

    return (int)(*str - '0');
}


SEGY::FileReadOpts::FileReadOpts( Seis::GeomType gt )
    : forread_(true)
    , coordscale_(mUdf(float))
    , timeshift_(mUdf(float))
    , sampleintv_(mUdf(float))
    , icdef_(Both)
    , havetrcnrs_(true)
    , trcnrdef_(1000,1)
    , psdef_(InFile)
    , offsdef_(0.f,25.f)
    , coorddef_(Present)
    , stepcoord_(1,1)
{
    setGeomType( gt );
    thdef_.fromSettings();
}


SEGY::FileReadOpts::~FileReadOpts()
{}


void SEGY::FileReadOpts::setGeomType( Seis::GeomType gt )
{
    geom_ = gt;
    if ( Seis::is2D(geom_) )
	icdef_ = XYOnly;
}

static int getICOpt( SEGY::FileReadOpts::ICvsXYType opt )
{
    return opt == SEGY::FileReadOpts::XYOnly ? -1
	: (opt == SEGY::FileReadOpts::ICOnly ? 1 : 0);
}


static SEGY::FileReadOpts::ICvsXYType getICType( int opt )
{
    return opt < 0 ? SEGY::FileReadOpts::XYOnly
	: (opt > 0 ? SEGY::FileReadOpts::ICOnly
		   : SEGY::FileReadOpts::Both);
}


#define mFillFromDefIf(cond,def,ky) \
    if ( cond ) \
	def.fillPar( iop, ky ); \
    else \
	def.removeFromPar( iop, ky )

#define mFillIf(cond,key,val) \
    if ( cond ) \
	iop.set( key, val ); \
    else \
	iop.removeWithKey( key )


void SEGY::FileReadOpts::fillPar( IOPar& iop ) const
{
    iop.set( sKeyICOpt(), getICOpt( icdef_ ) );

    mFillFromDefIf(icdef_!=XYOnly,thdef_.inl_,TrcHeaderDef::sInlByte());
    mFillFromDefIf(icdef_!=XYOnly,thdef_.crl_,TrcHeaderDef::sCrlByte());
    mFillFromDefIf(icdef_!=ICOnly,thdef_.xcoord_,TrcHeaderDef::sXCoordByte());
    mFillFromDefIf(icdef_!=ICOnly,thdef_.ycoord_,TrcHeaderDef::sYCoordByte());

    const bool is2d = Seis::is2D( geom_ );
    mFillFromDefIf(is2d,thdef_.trnr_,TrcHeaderDef::sTrNrByte());
    mFillFromDefIf(is2d,thdef_.refnr_,TrcHeaderDef::sRefNrByte());

    mFillIf(!mIsUdf(coordscale_),sKeyCoordScale(),coordscale_);
    mFillIf(!mIsUdf(timeshift_),sKeyTimeShift(),timeshift_);
    mFillIf(!mIsUdf(sampleintv_),sKeySampleIntv(),sampleintv_);

    const bool isps = Seis::isPS( geom_ );

    if ( is2d && !isps )
    {
	iop.setYN( sKeyHaveTrcNrs(), havetrcnrs_ );
	mFillIf(!havetrcnrs_,sKeyTrcNrDef(),trcnrdef_);
	mFillIf(true,sKeyCoordOpt(),(int)coorddef_);
	mFillIf(coorddef_==Generate,sKeyCoordStart(),startcoord_);
	mFillIf(coorddef_==Generate,sKeyCoordStep(),stepcoord_);
	mFillIf(coorddef_==ReadFile,sKeyCoordFileName(),coordfnm_);
    }

    if ( !isps ) return;

    mFillIf(true,sKeyPSOpt(),(int)psdef_);
    mFillIf(psdef_==UsrDef,sKeyOffsDef(),offsdef_);
    mFillFromDefIf(psdef_==InFile,thdef_.offs_,TrcHeaderDef::sOffsByte());
    mFillFromDefIf(psdef_==InFile,thdef_.azim_,TrcHeaderDef::sAzimByte());
}


bool SEGY::FileReadOpts::usePar( const IOPar& iop )
{
    thdef_.usePar( iop );

    iop.get( sKeyCoordScale(), coordscale_ );
    iop.get( sKeyTimeShift(), timeshift_ );
    iop.get( sKeySampleIntv(), sampleintv_ );

    iop.getYN( sKeyHaveTrcNrs(), havetrcnrs_ );
    iop.get( sKeyTrcNrDef(), trcnrdef_ );

    int icopt = getICOpt( icdef_ );
    iop.get( sKeyICOpt(), icopt );
    icdef_ = getICType( icopt );

    int psopt = (int)psdef_;
    iop.get( sKeyPSOpt(), psopt );
    psdef_ = (PSDefType)psopt;
    iop.get( sKeyOffsDef(), offsdef_ );

    int coordopt = (int)coorddef_;
    iop.get( sKeyCoordOpt(), coordopt );
    coorddef_ = (CoordDefType)coordopt;
    iop.get( sKeyCoordStart(), startcoord_ );
    iop.get( sKeyCoordStep(), stepcoord_ );
    iop.get( sKeyCoordFileName(), coordfnm_ );

    return true;
}


void SEGY::FileReadOpts::shallowClear( IOPar& iop )
{
    iop.removeWithKey( sKeyCoordOpt() );
    iop.removeWithKey( sKeyCoordScale() );
    iop.removeWithKey( sKeyTimeShift() );
    iop.removeWithKey( sKeySampleIntv() );
}


static void reportHdrEntry( IOPar& iop, const char* nm,
			    const SEGY::HdrEntry& he )
{
    BufferString keyw( nm, " byte" );
    BufferString val;
    if ( he.isUdf() )
	val += "<undef>";
    else
    {
	val += (int)he.bytepos_ + 1;
	val += " (size "; val += he.byteSize(); val += ")";
	iop.set( keyw.buf(), val.buf() );
    }
}


void SEGY::FileReadOpts::getReport( IOPar& iop, bool isrev0 ) const
{
    if ( !mIsUdf(coordscale_) )
	iop.set( sKeyCoordScale(), coordscale_ );
    if ( !mIsUdf(timeshift_) )
	iop.set( "Data starts at", timeshift_ );
    if ( !mIsUdf(sampleintv_) )
	iop.set( "Sample interval used", sampleintv_ );

    if ( Seis::is2D(geom_) )
    {
	if ( havetrcnrs_ )
	    reportHdrEntry( iop, sKey::TraceNr(), thdef_.trnr_ );
	else
	    iop.set( sKeyTrcNrDef(), trcnrdef_ );
    }
    else
    {
	iop.set( "Positioning defined by",
		icdef_ == XYOnly ? "Coordinates" : "Inline/Crossline" );
	if ( isrev0 )
	{
	    if ( icdef_ != XYOnly )
	    {
		reportHdrEntry( iop, "Inline", thdef_.inl_ );
		reportHdrEntry( iop, "Crossline", thdef_.crl_ );
	    }
	    else
	    {
		reportHdrEntry( iop, "X-coordinate", thdef_.xcoord_ );
		reportHdrEntry( iop, "Y-coordinate", thdef_.ycoord_ );
	    }
	}
    }

    if ( Seis::isPS(geom_) )
    {
	iop.set( "Offsets",
		   psdef_ == UsrDef	? "User defined"
		: (psdef_ == InFile	? "In file"
					: "Source/Receiver coordinates") );
	if ( psdef_ == UsrDef )
	    iop.set( sKeyOffsDef(), offsdef_ );
	else if ( psdef_ != SrcRcvCoords )
	{
	    reportHdrEntry( iop, sKey::Offset(), thdef_.offs_ );
	    if ( !thdef_.azim_.isUdf() )
		reportHdrEntry( iop, sKey::Azimuth(), thdef_.azim_ );
	}
    }
}


SEGY::OffsetCalculator::OffsetCalculator()
    : type_(FileReadOpts::InFile)
    , def_(0.f,1.f)
    , is2d_(false)
    , coordscale_(1.0f)
{
    reset();
}


SEGY::OffsetCalculator::~OffsetCalculator()
{}


void SEGY::OffsetCalculator::set( const SEGY::FileReadOpts& opts )
{
    type_ = opts.psdef_;
    def_ = opts.offsdef_;
    is2d_ = Seis::is2D( opts.geomType() );
    coordscale_ = opts.coordscale_;
    reset();
}


void SEGY::OffsetCalculator::reset()
{
    curoffs_ = def_.start;
    prevbid_.inl() = prevbid_.crl() = mUdf(int);
}


void SEGY::OffsetCalculator::setOffset( SeisTrcInfo& ti,
					const SEGY::TrcHeader& trchdr ) const
{
    if ( type_ == FileReadOpts::InFile )
	return;
    else if ( type_ == FileReadOpts::SrcRcvCoords )
    {
	Coord c1( trchdr.getCoord(true,coordscale_) );
	Coord c2( trchdr.getCoord(false,coordscale_) );
	ti.setPSFlds( c1, c2 );
	return;
    }

    bool diffcrl = false;
    bool diffinl = false;
    if ( is2d_ )
	diffcrl = mIsUdf(prevbid_.trcNr()) || prevbid_.trcNr() != ti.trcNr();
    else
    {
	diffinl = mIsUdf(prevbid_.inl()) || prevbid_.inl() != ti.inl();
	diffcrl = mIsUdf(prevbid_.crl()) || prevbid_.crl() != ti.crl();
    }

    if ( diffcrl || diffinl )
	curoffs_ = def_.start;
    else
	curoffs_ += def_.step;

    ti.offset = curoffs_;
    if ( is2d_ )
	prevbid_.trcNr() = ti.trcNr();
    else
	prevbid_ = ti.binID();
}
