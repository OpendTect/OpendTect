/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID mUnusedVar = "$Id: segyfiledef.cc,v 1.29 2012-07-23 08:11:14 cvsbert Exp $";

#include "segyfiledef.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "separstr.h"

namespace SEGY
{
const char* FileDef::sKeyForceRev0()	   { return "Force Rev0"; }
const char* FilePars::sKeyNrSamples()	   { return "Nr samples overrule"; }
const char* FilePars::sKeyNumberFormat()   { return "Number format"; }
const char* FilePars::sKeyByteSwap()	   { return "Byte swapping"; }
const char* FileSpec::sKeyFileNrs()	   { return "File numbers"; }
const char* FileReadOpts::sKeyTimeShift()  { return "Start time overrule"; }
const char* FileReadOpts::sKeySampleIntv() { return "Sample rate overrule"; }
const char* FileReadOpts::sKeyICOpt()	   { return "IC -> XY"; }
const char* FileReadOpts::sKeyPSOpt()	   { return "Offset source"; }
const char* FileReadOpts::sKeyCoordOpt()   { return "Coord source"; }
const char* FileReadOpts::sKeyOffsDef()	   { return "Generate offsets"; }
const char* FileReadOpts::sKeyCoordStart() { return "Generate coords.Start"; }
const char* FileReadOpts::sKeyCoordStep() { return "Generate coords.Step"; }
const char* FileReadOpts::sKeyCoordFileName() { return "Coordinate file"; }
const char* FileReadOpts::sKeyCoordScale()
				{ return "Coordinate scaling overrule"; }
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


const char* SEGY::FileSpec::getFileName( int nr ) const
{
    if ( !isMultiFile() )
	return fname_.buf();

    BufferString numbstr( "", nrs_.atIndex(nr) );
    BufferString replstr;
    if ( zeropad_ < 2 )
	replstr = numbstr;
    else
    {
	const int numblen = numbstr.size();
	while ( numblen + replstr.size() < zeropad_ )
	    replstr += "0";
	replstr += numbstr;
    }

    static FileNameString ret; ret = fname_;
    replaceString( ret.buf(), "*", replstr.buf() );
    return ret.buf();
}


IOObj* SEGY::FileSpec::getIOObj( bool tmp ) const
{
    IOStream* iostrm;
    if ( tmp )
    {
	BufferString idstr( "100010.", IOObj::tmpID() );
	iostrm = new IOStream( fname_, idstr );
    }
    else
    {
	iostrm = new IOStream( fname_ );
	iostrm->acquireNewKey();
    }
    iostrm->setFileName( fname_ );
    iostrm->setGroup( "Seismic Data" );
    iostrm->setTranslator( "SEG-Y" );
    iostrm->setDirName( "Seismics" );
    const bool ismulti = !mIsUdf(nrs_.start);
    if ( ismulti )
    {   
	iostrm->fileNumbers() = nrs_;
	iostrm->setZeroPadding( zeropad_ );
    }

    return iostrm;
}


void SEGY::FileSpec::fillPar( IOPar& iop ) const
{
    iop.set( sKey::FileName(), fname_ );
    if ( mIsUdf(nrs_.start) )
	iop.removeWithKey( sKeyFileNrs() );
    else
    {
	FileMultiString fms;
	fms += nrs_.start; fms += nrs_.stop; fms += nrs_.step;
	if ( zeropad_ )
	    fms += zeropad_;
	iop.set( sKeyFileNrs(), fms );
    }
}


bool SEGY::FileSpec::usePar( const IOPar& iop )
{
    if ( !iop.get( sKey::FileName(), fname_ ) )
	return false;

    getMultiFromString( iop.find(sKeyFileNrs()) );
    return true;
}


void SEGY::FileSpec::getReport( IOPar& iop, bool ) const
{
    iop.set( sKey::FileName(), fname_ );
    if ( mIsUdf(nrs_.start) ) return;

    BufferString str;
    str += nrs_.start; str += "-"; str += nrs_.stop;
    str += " step "; str += nrs_.step;
    if ( zeropad_ )
	{ str += "(pad to "; str += zeropad_; str += " zeros)"; }
    iop.set( "Replace '*' with", str );
}


void SEGY::FileSpec::getMultiFromString( const char* str )
{
    FileMultiString fms( str );
    const int len = fms.size();
    nrs_.start = len > 0 ? toInt( fms[0] ) : mUdf(int);
    if ( len > 1 )
	nrs_.stop = toInt( fms[1] );
    if ( len > 2 )
	nrs_.step = toInt( fms[2] );
    if ( len > 3 )
	zeropad_ = toInt( fms[3] );
}


void SEGY::FileSpec::ensureWellDefined( IOObj& ioobj )
{
    mDynamicCastGet(IOStream*,iostrm,&ioobj)
    if ( !iostrm ) return;
    iostrm->setTranslator( "SEG-Y" );
    IOPar& iop = ioobj.pars();
    if ( !iop.find( sKey::FileName() ) ) return;

    SEGY::FileSpec fs; fs.usePar( iop );
    iop.removeWithKey( sKey::FileName() );
    iop.removeWithKey( sKeyFileNrs() );

    iostrm->setFileName( fs.fname_ );
    if ( !fs.isMultiFile() )
	iostrm->fileNumbers().start = iostrm->fileNumbers().stop = 1;
    else
    {
	iostrm->fileNumbers() = fs.nrs_;
	iostrm->setZeroPadding( fs.zeropad_ );
    }
}


void SEGY::FileSpec::fillParFromIOObj( const IOObj& ioobj, IOPar& iop )
{
    mDynamicCastGet(const IOStream*,iostrm,&ioobj)
    if ( !iostrm ) return;

    SEGY::FileSpec fs; fs.fname_ = iostrm->fileName();
    if ( iostrm->isMulti() )
    {
	fs.nrs_ = iostrm->fileNumbers();
	fs.zeropad_ = iostrm->zeroPadding();
    }

    fs.fillPar( iop );
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
    const char* fmtstr = iop.find( sKeyNumberFormat() );
    const bool foundnf = fmtstr && *fmtstr;
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
	const char* str = byteswap_ > 1
	    		? (forread_ ? "All bytes are" : "All bytes will be")
			: (forread_ ? "Data bytes are" : "Data bytes will be");
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
    if ( !str || !*str || !isdigit(*str) )
	return forread ? 0 : 1;

    return (int)(*str - '0');
}


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

    mFillIf(!mIsUdf(coordscale_),sKeyCoordScale(),coordscale_);
    mFillIf(!mIsUdf(timeshift_),sKeyTimeShift(),timeshift_);
    mFillIf(!mIsUdf(sampleintv_),sKeySampleIntv(),sampleintv_);

    const bool isps = Seis::isPS( geom_ );

    if ( is2d && !isps )
    {
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

    int icopt = getICOpt( icdef_ );
    iop.get( sKeyICOpt(), icopt );
    icdef_ = getICType( icopt );
    int psopt = (int)psdef_;
    iop.get( sKeyPSOpt(), psopt );
    psdef_ = (PSDefType)psopt;
    iop.get( sKeyOffsDef(), offsdef_ );
    iop.get( sKeyCoordScale(), coordscale_ );
    iop.get( sKeyTimeShift(), timeshift_ );
    iop.get( sKeySampleIntv(), sampleintv_ );
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


void SEGY::FileReadOpts::getReport( IOPar& iop, bool rev1 ) const
{
    if ( !mIsUdf(coordscale_) )
	iop.set( sKeyCoordScale(), coordscale_ );
    if ( !mIsUdf(timeshift_) )
	iop.set( sKeyTimeShift(), timeshift_ );
    if ( !mIsUdf(sampleintv_) )
	iop.set( sKeySampleIntv(), sampleintv_ );

    const bool is2d = Seis::is2D( geom_ );
    const bool isps = Seis::isPS( geom_ );

    if ( !rev1 )
    {
	if ( is2d )
	    reportHdrEntry( iop, sKey::TraceNr(), thdef_.trnr_ );
	else
	{
	    iop.set( "Positioning defined by",
		    icdef_ == XYOnly ? "Coordinates" : "Inline/Crossline" );
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

    if ( !isps )
	return;

    iop.set( "Offsets", psdef_ == UsrDef ? "User defined"
	    : (psdef_ == InFile ? "In file" : "Source/Receiver coordinates") );
    if ( psdef_ == UsrDef )
	iop.set( sKeyOffsDef(), offsdef_ );
    else if ( psdef_ != SrcRcvCoords )
    {
	reportHdrEntry( iop, sKey::Offset(), thdef_.offs_ );
	if ( !thdef_.azim_.isUdf() )
	    reportHdrEntry( iop, sKey::Azimuth(), thdef_.azim_ );
    }
}
