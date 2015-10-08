/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "segytr.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "segyhdr.h"
#include "segyfiledef.h"
#include "datainterp.h"
#include "conn.h"
#include "iopar.h"
#include "timefun.h"
#include "scaler.h"
#include "survinfo.h"
#include "seisselection.h"
#include "envvars.h"
#include "separstr.h"
#include "keystrs.h"
#include "settings.h"
#include "zdomain.h"
#include "filepath.h"
#include "od_iostream.h"
#include "bendpoints2coords.h"
#include "uistrings.h"
#include <math.h>
#include <ctype.h>

#define mBPS(cd) (int)cd->datachar.nrBytes()
#define mInDepth \
    (othdomain_ && SI().zIsTime()) || (!othdomain_ && !SI().zIsTime())
#define mZStepFac ( mInDepth ? 0.001 : 1.e-6 )

static const int cSEGYWarnBadFmt = 1;
static const int cSEGYWarnPos = 2;
static const int cSEGYWarnZeroSampIntv = 3;
static const int cSEGYWarnDataReadIncomplete = 4;
static const int cSEGYWarnNonrectCoord = 5;
static const int cSEGYWarnSuspiciousCoord = 6;
static const int cSEGYFoundStanzas = 7;
static const int cSEGYWarnNonFixedLength = 8;
static const int cMaxNrSamples = 1000000;
static int maxnrconsecutivebadtrcs = -1;
static const char* sKeyMaxConsBadTrcs = "SEGY.Max Bad Trace Block";
static const od_stream::Pos cEndTapeHeader = 3600;
static const od_int64 cTraceHeaderBytes = 240;


SEGYSeisTrcTranslator::SEGYSeisTrcTranslator( const char* nm, const char* unm )
	: SeisTrcTranslator(nm,unm)
	, trchead_(*new SEGY::TrcHeader(headerbuf_,fileopts_.thdef_,false))
	, txthead_(0)
	, binhead_(*new SEGY::BinHeader)
	, trcscale_(0)
	, curtrcscale_(0)
	, forcedrev_(-1)
	, storinterp_(0)
	, blockbuf_(0)
	, headerdone_(false)
	, useinpsd_(false)
	, inpcd_(0)
	, outcd_(0)
	, estnrtrcs_(-1)
	, curoffs_(-1)
	, curcoord_(mUdf(float),0)
	, prevoffs_(0)
	, othdomain_(false)
	, bp2c_(0)
{
    if ( maxnrconsecutivebadtrcs < 0 )
    {
	maxnrconsecutivebadtrcs = 50;
	Settings::common().get( sKeyMaxConsBadTrcs, maxnrconsecutivebadtrcs );
    }
    curtrcnr_ = prevtrcnr_ = -1;
    mSetUdf(curbid_.inl()); mSetUdf(prevbid_.inl());
}


SEGYSeisTrcTranslator::~SEGYSeisTrcTranslator()
{
    cleanUp();
    delete txthead_;
    delete &binhead_;
    delete &trchead_;
}


void SEGYSeisTrcTranslator::cleanUp()
{
    SeisTrcTranslator::cleanUp();

    delete storinterp_; storinterp_ = 0;
    delete [] blockbuf_; blockbuf_ = 0;
    delete bp2c_; bp2c_ = 0;
    headerdone_ = false;

    prevoffs_ = curoffs_ = forcedrev_ = -1; mSetUdf(curcoord_.x);
}


int SEGYSeisTrcTranslator::dataBytes() const
{
    return SEGY::BinHeader::formatBytes(
		filepars_.fmt_ > 0 ? filepars_.fmt_ : 1 );
}


int SEGYSeisTrcTranslator::traceSizeOnDisk() const
{
    return cTraceHeaderBytes + mBPS(inpcd_) * innrsamples_;
}


bool SEGYSeisTrcTranslator::getFullTrcAsBuf( unsigned char* buf )
{
    return sConn().iStream().getBin( buf, traceSizeOnDisk() );
}


#define mErrRet(s) { fillErrMsg(s,false); return false; }
#define mPosErrRet(s) { fillErrMsg(s,true); return false; }


bool SEGYSeisTrcTranslator::readTapeHeader()
{
    od_istream& strm = sConn().iStream();

    if ( !txthead_ )
	txthead_ = new SEGY::TxtHeader;
    if ( !strm.getBin(txthead_->txt_,SegyTxtHeaderLength) )
	mErrRet( tr("Cannot read SEG-Y Text header") )
    txthead_->setAscii();

    const int revcodeentry = SEGY::BinHeader::EntryRevCode();
    unsigned char binheaderbuf[400];
    if ( !strm.getBin( binheaderbuf, SegyBinHeaderLength ) )
	mErrRet( tr("Cannot read SEG-Y Text header") )
    binhead_.setInput( binheaderbuf, filepars_.swapHdrs() );
    if ( binhead_.isSwapped() )
	binhead_.unSwap();

    trchead_.setNeedSwap( filepars_.swapHdrs() );
    trchead_.isrev0_ = binhead_.revision() < 1;
    if ( forcedrev_ != -1 )
	trchead_.isrev0_ = forcedrev_ == 0;

    if ( !trchead_.isrev0_ )
    {
	const int nrstzs = binhead_.skipRev1Stanzas( strm );
	if ( nrstzs > 0 )
	    addWarn( cSEGYFoundStanzas, toString(nrstzs) );
	const int fixedtrcflag = binhead_.entryVal( revcodeentry + 1 );
	if ( fixedtrcflag == 0 )
	    addWarn( cSEGYWarnNonFixedLength, "" );
    }

    if ( filepars_.fmt_ == 0 )
    {
	filepars_.fmt_ = binhead_.format();
	if ( filepars_.fmt_ == 4 && read_mode != Seis::PreScan )
	    mErrRet( tr("SEG-Y format '4' (fixed point/gain code) "
                        "not supported") )
	else if ( filepars_.fmt_<1 || filepars_.fmt_>8
		|| filepars_.fmt_==6 || filepars_.fmt_==7 )
	{
	    addWarn( cSEGYWarnBadFmt, toString(filepars_.fmt_) );
	    filepars_.fmt_ = 1;
	}
    }

    txthead_->getText( pinfo_.usrinfo );
    pinfo_.nr = binhead_.entryVal( SEGY::BinHeader::EntryLino() );
    pinfo_.zrg.step = binhead_.sampleRate( mInDepth );
    insd_.step = pinfo_.zrg.step;
    innrsamples_ = binhead_.nrSamples();

    od_stream::Pos endpos = strm.endPosition();
    estnrtrcs_ = mCast( int, (endpos - cEndTapeHeader)
			/ (cTraceHeaderBytes + dataBytes()*innrsamples_));
    return true;
}


void SEGYSeisTrcTranslator::addWarn( int nr, const char* detail )
{
    mDefineStaticLocalObject( const bool, nowarn,
			= Settings::common().isTrue("SEG-Y.No warnings") );
    if ( nowarn || warnnrs_.isPresent(nr) ) return;

    uiString msg;
    if ( nr == cSEGYWarnBadFmt )
    {
	msg = tr("SEG-Y format '%1' "
                 "found.\n\tReplaced with '1' (4-byte floating point)")
            .arg(detail);
	if ( toInt(detail) > 254 )
	    msg.arg("\n-> The file may not be SEG-Y, or byte-swapped");
    }
    else if ( nr == cSEGYWarnPos )
    {
	msg = tr("Bad position found. Such traces are "
                 "ignored.\nFirst occurrence %1").arg(detail);
    }
    else if ( nr == cSEGYWarnZeroSampIntv )
    {
	msg = tr("Zero sample interval found in trace header.\n"
	         "First occurrence ").arg(detail);
    }
    else if ( nr == cSEGYWarnDataReadIncomplete )
    {
	msg = mToUiStringTodo( detail );
    }
    else if ( nr == cSEGYWarnNonrectCoord )
    {
	msg = tr("Trace header indicates Geographic Coordinates (byte 89).\n"
	         "These are not supported.\n"
	         "Will bluntly load them as rectangular coordinates "
	         "(which they are most often)."
	         "\nBeware that the positions may therefore not be correct.\n"
	         "First occurrence %1").arg(detail);
    }
    else if ( nr == cSEGYWarnSuspiciousCoord )
    {
	msg = tr("Suspiciously large coordinate found.\nThis may be incorrect "
	         "- please check the coordinate scaling.\nOverrule "
                 "if necessary.\nCoordinate found: %1 at %2")
	    .arg( detail ).arg( getTrcPosStr() );
    }
    else if ( nr == cSEGYFoundStanzas )
    {
	msg = tr("SEG-Y REV.1 header indicates the presence of\n"
	         "%1 Extended Textual File Header").arg(detail);
	if ( toInt(detail) > 1 )
	msg = tr( "%1s.\nThis is rarely correct. Please set the variable:"
		  "\nOD_SEIS_SEGY_REV1_STANZAS"
		  "\nif the file indeed contains these." ).arg(detail);
    }
    else if ( nr == cSEGYWarnNonFixedLength )
    {
	msg = tr("SEG-Y REV.1 header indicates variable length traces."
	         "\nOpendTect will assume fixed trace length anyway.");
    }

    SeisTrcTranslator::addWarn( nr, detail );
}


void SEGYSeisTrcTranslator::updateCDFromBuf()
{
    SeisTrcInfo info; trchead_.fill( info, fileopts_.coordscale_ );
    if ( othdomain_ )
	info.sampling.step *= SI().zIsTime() ? 1000 : 0.001f;

    insd_.start = info.sampling.start;
    insd_.step = pinfo_.zrg.step;
    if ( mIsZero(insd_.step,1e-8) )
    {
	insd_.step = info.sampling.step;
	if ( mIsZero(insd_.step,1e-8) )
	    insd_.step = SI().zRange(false).step;
    }
    if ( !mIsUdf(fileopts_.timeshift_) )
	insd_.start = fileopts_.timeshift_;
    if ( !mIsUdf(fileopts_.sampleintv_) )
	insd_.step = fileopts_.sampleintv_;

    innrsamples_ = filepars_.ns_;
    if ( innrsamples_ <= 0 || innrsamples_ > cMaxNrSamples )
    {
	innrsamples_ = binhead_.nrSamples();
	if ( innrsamples_ <= 0 || innrsamples_ > cMaxNrSamples )
	{
	    innrsamples_ = trchead_.nrSamples();
	    if ( innrsamples_ <= 0 || innrsamples_ > cMaxNrSamples )
		innrsamples_ = SI().zRange(false).nrSteps() + 1;
	}
    }

    addComp( getDataChar(filepars_.fmt_)  );
    DataCharacteristics& dc = tarcds_[0]->datachar;
    dc.fmt_ = DataCharacteristics::Ieee;
    const float scfac = trchead_.postScale(filepars_.fmt_ ? filepars_.fmt_ : 1);
    if ( !mIsEqual(scfac,1,mDefEps)
      || (dc.isInteger() && dc.nrBytes() == BinDataDesc::N4) )
	dc = DataCharacteristics();
}


void SEGYSeisTrcTranslator::interpretBuf( SeisTrcInfo& ti )
{
    trchead_.fill( ti, fileopts_.coordscale_ );
    if ( othdomain_ )
	ti.sampling.step *= SI().zIsTime() ? 1000 : 0.001f;
    if ( binhead_.isInFeet() ) ti.offset *= mFromFeetFactorF;

    float scfac = trchead_.postScale( filepars_.fmt_ ? filepars_.fmt_ : 1 );
    if ( mIsEqual(scfac,1,mDefEps) )
	curtrcscale_ = 0;
    else
    {
	if ( !trcscale_ ) trcscale_ = new LinScaler( 0, scfac );
	else		 trcscale_->factor = scfac;
	curtrcscale_ =	trcscale_;
    }

    if ( !mIsUdf(fileopts_.timeshift_) )
	ti.sampling.start = fileopts_.timeshift_;
    if ( !mIsUdf(fileopts_.sampleintv_) )
	ti.sampling.step = fileopts_.sampleintv_;

    if ( fileopts_.coorddef_ == SEGY::FileReadOpts::Generate )
    {
	if ( mIsUdf(curcoord_.x) )
	    curcoord_ = fileopts_.startcoord_;
	else
	    curcoord_ += fileopts_.stepcoord_;
	ti.coord = curcoord_;
    }
    else if ( fileopts_.coorddef_ == SEGY::FileReadOpts::ReadFile )
    {
	if ( !bp2c_ )
	{
	    BufferString coordfnm( fileopts_.coordfnm_ );
	    if ( coordfnm.startsWith("ext=") )
	    {
		FilePath fp( ((StreamConn*)conn_)->fileName() );
		fp.setExtension( coordfnm.buf()+4 );
		coordfnm = fp.fullPath();
	    }

	    od_istream stream( coordfnm );
	    if ( !stream.isOK() )
	    {
		errmsg_ = tr( "Cannot open coordinate file:\n%1" )
				.arg( coordfnm );
		return;
	    }
	    bp2c_ = new BendPoints2Coords( stream );
	    stream.close();
	    if ( bp2c_->getIDs().size() < 2 )
	    {
		errmsg_ =
		    tr( "Coordinate file bad (need at least 2 points):\n%1" )
			.arg( coordfnm );
		return;
	    }
	}
	ti.coord = bp2c_->coordAt( mCast(float,ti.nr) );
    }

    if ( ti.coord.x > 1e9 || ti.coord.y > 1e9 )
	addWarn( cSEGYWarnSuspiciousCoord, ti.coord.toPrettyString() );
}


void SEGYSeisTrcTranslator::setTxtHeader( SEGY::TxtHeader* th )
{
    delete txthead_; txthead_ = th;
}


bool SEGYSeisTrcTranslator::writeTapeHeader()
{
    if ( filepars_.fmt_ == 0 ) // Auto-detect
	filepars_.fmt_ = nrFormatFor( storinterp_->dataChar() );

    trchead_.isrev0_ = false;

    if ( !txthead_ )
    {
	txthead_ = new SEGY::TxtHeader( trchead_.isrev0_ ? 0 : 1);
	txthead_->setUserInfo( pinfo_.usrinfo );
	fileopts_.thdef_.linename = curlinekey_;
	fileopts_.thdef_.pinfo = &pinfo_;
	txthead_->setPosInfo( fileopts_.thdef_ );
	txthead_->setStartPos( outsd_.start );
	if ( Settings::common().isTrue("SEGY.Text Header EBCDIC") )
	    txthead_->setEbcdic();
    }
    if ( !sConn().oStream().addBin( txthead_->txt_, SegyTxtHeaderLength ) )
	mErrRet(tr("Cannot write SEG-Y textual header"))

    binhead_.setForWrite();
    binhead_.setFormat( mCast(short,filepars_.fmt_ < 2 ? 1 : filepars_.fmt_) );
    filepars_.fmt_ = binhead_.format();
    binhead_.setEntryVal( SEGY::BinHeader::EntryLino(), pinfo_.nr );
    mDefineStaticLocalObject( int, jobid, = 0 );
    binhead_.setEntryVal( SEGY::BinHeader::EntryJobID(), ++jobid );
    binhead_.setNrSamples( outnrsamples_ );
    binhead_.setSampleRate( outsd_.step, mInDepth );
    binhead_.setEntryVal( SEGY::BinHeader::EntryTsort(), is_prestack ? 0 : 4 );
					// To make Strata users happy
    binhead_.setInFeet( SI().xyInFeet() );
    if ( !sConn().oStream().addBin( binhead_.buf(), SegyBinHeaderLength ) )
	mErrRet(tr("Cannot write SEG-Y binary header"))

    return true;
}


void SEGYSeisTrcTranslator::fillHeaderBuf( const SeisTrc& trc )
{
    if ( SI().xyInFeet() )
    {
	SeisTrcInfo info = trc.info();
	info.offset *= mToFeetFactorF;
	trchead_.use( info );
    }
    else
    {
	trchead_.use( trc.info() );
    }

    SamplingData<float> sdtoput( useinpsd_ ? trc.info().sampling : outsd_ );
    const int nstoput = useinpsd_ ? trc.size() : outnrsamples_;
    if ( othdomain_ )
	sdtoput.step *= SI().zIsTime() ? 0.001f : 1000;

    trchead_.putSampling( sdtoput, mCast(unsigned short,nstoput) );
}


void SEGYSeisTrcTranslator::usePar( const IOPar& iopar )
{
    SeisTrcTranslator::usePar( iopar );

    filepars_.usePar( iopar );
    fileopts_.usePar( iopar );
    fileopts_.setGeomType( Seis::geomTypeOf(is_2d,is_prestack) );

    iopar.get( SEGY::FilePars::sKeyRevision(), forcedrev_ );
    if ( iopar.isTrue(SEGY::FilePars::sKeyForceRev0()) )
	forcedrev_ = 0;

    othdomain_ = !ZDomain::isSI( iopar );
}


bool SEGYSeisTrcTranslator::isRev0() const
{
    return trchead_.isrev0_;
}


void SEGYSeisTrcTranslator::toSupported( DataCharacteristics& dc ) const
{
    if ( dc.isInteger() || !dc.isIeee() )
	dc = getDataChar( nrFormatFor(dc) );
}


void SEGYSeisTrcTranslator::selectWriteDataChar( DataCharacteristics& dc ) const
{
    if ( filepars_.fmt_ == 0 )
	const_cast<int&>(filepars_.fmt_) = 1;
    dc = getDataChar( filepars_.fmt_ );
}


int SEGYSeisTrcTranslator::nrFormatFor( const DataCharacteristics& dc ) const
{
    int nrbytes = dc.nrBytes();
    if ( nrbytes > 4 ) nrbytes = 4;
    else if ( !dc.isSigned() && nrbytes < 4 )
	nrbytes *= 2;

    int nf = 1;
    if ( nrbytes == 1 )
	nf = 8;
    else if ( nrbytes == 2 )
	nf = 3;
    else
	nf = dc.isInteger() ? 2 : 1;

    return nf;
}


DataCharacteristics SEGYSeisTrcTranslator::getDataChar( int nf ) const
{
    return SEGY::BinHeader::getDataChar( nf, filepars_.swapData() );
}


bool SEGYSeisTrcTranslator::commitSelections_()
{
    const bool forread = forRead();
    fileopts_.forread_ = forread;
    fileopts_.setGeomType( Seis::geomTypeOf( is_2d, is_prestack ) );

    inpcd_ = inpcds_[0]; outcd_ = outcds_[0];
    storinterp_ = new TraceDataInterpreter( forread ? inpcd_->datachar
						   : outcd_->datachar );
    if ( mIsEqual(outsd_.start,insd_.start,mDefEps)
      && mIsEqual(outsd_.step,insd_.step,mDefEps) )
	useinpsd_ = true;

    if ( blockbuf_ )
	{ delete [] blockbuf_; blockbuf_ = 0; }
    int bufsz = innrsamples_;
    if ( outnrsamples_ > bufsz ) bufsz = outnrsamples_;
    bufsz += 10;
    int nbts = inpcd_->datachar.nrBytes();
    if ( outcd_->datachar.nrBytes() > nbts ) nbts = outcd_->datachar.nrBytes();

    blockbuf_ = new unsigned char [ nbts * bufsz ];
    return forread || writeTapeHeader();
}


bool SEGYSeisTrcTranslator::initRead_()
{
    if ( !readTapeHeader() || !readTraceHeadBuffer() )
	return false;

    trchead_.initRead();
    if ( tarcds_.isEmpty() )
	updateCDFromBuf();

    if ( innrsamples_ <= 0 || innrsamples_ > cMaxNrSamples )
	mErrRet(tr("Cannot find a reasonable number of samples."
	           "\nFound: %1.\nPlease 'Overrule' to set something usable")
              .arg(innrsamples_))

    offsetcalc_.set( fileopts_ );
    sConn().iStream().setPosition( cEndTapeHeader );
    return true;
}


bool SEGYSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    filepars_.setForRead( false );

    for ( int idx=0; idx<trc.data().nrComponents(); idx++ )
    {
	DataCharacteristics dc(trc.data().getInterpreter(idx)->dataChar());
	addComp( dc );
	toSupported( dc );
	selectWriteDataChar( dc );
	tarcds_[idx]->datachar = dc;
	if ( idx ) tarcds_[idx]->destidx = -1;
    }

    return true;
}


bool SEGYSeisTrcTranslator::goToTrace( int nr )
{
    od_stream::Pos so = nr;
    so *= (cTraceHeaderBytes + dataBytes() * innrsamples_);
    so += cEndTapeHeader;
    od_istream& strm = sConn().iStream();
    strm.setPosition( so );
    headerdone_ = false;
    return strm.isOK();
}



const char* SEGYSeisTrcTranslator::getTrcPosStr() const
{
    mDeclStaticString( ret );
    int usecur = 1; const bool is2d = Seis::is2D(fileopts_.geomType());
    if ( is2d )
    {
	if ( mIsUdf(curtrcnr_) || curtrcnr_ < 0 )
	    usecur = prevtrcnr_ < 0 ? -1 : 0;
    }
    else
    {
	if ( mIsUdf(curbid_.inl()) )
	    usecur = mIsUdf(prevbid_.inl()) ? -1 : 0;
    }

    ret = usecur ? "at " : "after ";
    if ( usecur < 0 )
	{ ret += "start of data"; return ret.buf(); }

    if ( is2d )
	{ ret += "trace number "; ret += usecur ? curtrcnr_ : prevtrcnr_; }
    else
    {
	const BinID bid( usecur ? curbid_ : prevbid_ );
	ret.add( "position " ).add( bid.toString() );
    }

    if ( Seis::isPS(fileopts_.geomType()) )
	{ ret += " (offset "; ret += usecur ? curoffs_ : prevoffs_; ret += ")";}

    return ret.buf();
}


bool SEGYSeisTrcTranslator::tryInterpretBuf( SeisTrcInfo& ti )
{
    errmsg_.setEmpty();
    interpretBuf( ti );
    return errmsg_.isEmpty();
}


bool SEGYSeisTrcTranslator::skipThisTrace( SeisTrcInfo& ti, int& nrbadtrcs )
{
    addWarn( cSEGYWarnPos, getTrcPosStr() );
    nrbadtrcs++;
    if ( nrbadtrcs >= maxnrconsecutivebadtrcs )
    {
	const uiString str =
		tr( "More than %1 traces with invalid position found." )
		  .arg( maxnrconsecutivebadtrcs );
	mPosErrRet(str);
    }

    sConn().iStream().setPosition( innrsamples_*mBPS(inpcd_), od_stream::Rel );
    if ( !readTraceHeadBuffer() )
	return false;
    if ( !tryInterpretBuf(ti) )
	return false;
    return (headerdone_ = true);
}


#define mBadCoord(ti) \
	(ti.coord.x < 0.01 && ti.coord.y < 0.01)
#define mBadBid(ti) \
	(ti.binid.inl() <= 0 && ti.binid.crl() <= 0)
#define mSkipThisTrace() { if ( !skipThisTrace(ti,nrbadtrcs) ) return false; }


bool SEGYSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !storinterp_ ) commitSelections();
    if ( headerdone_ ) return true;

    const int oldcurinl = curbid_.inl();
    const int oldcurtrcnr = curtrcnr_;
    if ( read_mode != Seis::Scan )
	{ mSetUdf(curbid_.inl()); mSetUdf(curtrcnr_); }

    if ( !readTraceHeadBuffer() )
	return false;
    if ( !tryInterpretBuf(ti) )
	return false;

    bool goodpos = true;
    int nrbadtrcs = 0;
    if ( fileopts_.icdef_ == SEGY::FileReadOpts::XYOnly )
    {
	if ( read_mode == Seis::Scan )
	    goodpos = !mBadCoord(ti);
	else if ( read_mode == Seis::Prod )
	{
	    while ( mBadCoord(ti) )
		mSkipThisTrace()
	}
	ti.binid = SI().transform( ti.coord );
    }
    else if ( fileopts_.icdef_ == SEGY::FileReadOpts::ICOnly )
    {
	if ( read_mode == Seis::Scan )
	    goodpos = !mBadBid(ti);
	else if ( read_mode == Seis::Prod )
	{
	    while ( mBadBid(ti) )
		mSkipThisTrace()
	}
	ti.coord = SI().transform( ti.binid );
    }
    else
    {
	if ( read_mode == Seis::Scan )
	    goodpos = !mBadBid(ti) || !mBadCoord(ti);
	if ( read_mode == Seis::Prod )
	{
	    while ( mBadBid(ti) && mBadCoord(ti) )
		mSkipThisTrace()
	    if ( mBadBid(ti) )
		ti.binid = SI().transform( ti.coord );
	    else if ( mBadCoord(ti) )
		ti.coord = SI().transform( ti.binid );
	}
    }

    if ( trchead_.isusable )
	trchead_.isusable = goodpos;

    if ( !useinpsd_ ) ti.sampling = outsd_;

    offsetcalc_.setOffset( ti, trchead_ );

    curbid_.inl() = oldcurinl;
    curtrcnr_ = oldcurtrcnr;

    if ( goodpos )
    {
	prevtrcnr_ = curtrcnr_; curtrcnr_ = ti.nr;
	prevbid_ = curbid_; curbid_ = ti.binid;
	prevoffs_ = curoffs_; curoffs_ = ti.offset;
    }

    if ( mIsZero(ti.sampling.step,mDefEps) )
    {
	addWarn(cSEGYWarnZeroSampIntv,getTrcPosStr());
	ti.sampling.step = insd_.step;
    }

    if ( trchead_.nonrectcoords )
	addWarn(cSEGYWarnNonrectCoord,getTrcPosStr());

    return (headerdone_ = true);
}


bool SEGYSeisTrcTranslator::read( SeisTrc& trc )
{
    if ( !readInfo(trc.info()) )
	return false;

    return readData( trc );
}


bool SEGYSeisTrcTranslator::skip( int ntrcs )
{
    if ( ntrcs < 1 ) return true;
    if ( !storinterp_ ) commitSelections();

    od_istream& strm = sConn().iStream();
    if ( !headerdone_ )
	strm.setPosition( mSEGYTraceHeaderBytes, od_stream::Rel );
    strm.setPosition( innrsamples_ * mBPS(inpcd_), od_stream::Rel );
    if ( ntrcs > 1 )
	strm.setPosition( (ntrcs-1)
		* (mSEGYTraceHeaderBytes + innrsamples_ * mBPS(inpcd_)) );

    headerdone_ = false;

    if ( strm.isBad() )
	mPosErrRet(tr("Read error during trace skipping"))
    return true;
}


bool SEGYSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    OD::memZero( headerbuf_, mSEGYTraceHeaderBytes );
    fillHeaderBuf( trc );

    if ( !sConn().oStream().addBin( headerbuf_, mSEGYTraceHeaderBytes ) )
	mErrRet(tr("Cannot write trace header"))

    return writeData( trc );
}


bool SEGYSeisTrcTranslator::readTraceHeadBuffer()
{
    if ( !conn_ || conn_->isBad() || !conn_->isStream() )
	return noErrMsg();
    od_istream& strm = sConn().iStream();
    if ( !strm.getBin(headerbuf_,mSEGYTraceHeaderBytes) )
	return noErrMsg();

    return true;
}


bool SEGYSeisTrcTranslator::readDataToBuf()
{
    od_istream& strm = sConn().iStream();
    if ( samprg_.start > 0 )
	strm.setPosition( samprg_.start * mBPS(inpcd_), od_stream::Rel );

    int rdsz = (samprg_.width()+1) *  mBPS(inpcd_);
    if ( !sConn().iStream().getBin(blockbuf_,rdsz) )
    {
	if ( strm.lastNrBytesRead() != rdsz )
	    addWarn( cSEGYWarnDataReadIncomplete, strm.lastNrBytesRead()
		    ? "Last trace is incomplete" : "No data in last trace" );
	return noErrMsg();
    }

    if ( samprg_.stop < innrsamples_-1 )
	strm.setPosition( (innrsamples_-samprg_.stop-1) * mBPS(inpcd_),
				od_stream::Rel );

    return !strm.isBad();
}


bool SEGYSeisTrcTranslator::readData( SeisTrc& trc )
{
    noErrMsg();
    const int curcomp = selComp();
    prepareComponents( trc, outnrsamples_ );
    headerdone_ = false;

    if ( !readDataToBuf() )
	return false;

    for ( int isamp=0; isamp<outnrsamples_; isamp++ )
	trc.set( isamp, storinterp_->get(blockbuf_,isamp), curcomp );

    if ( curtrcscale_ )
    {
	trc.convertToFPs();
	const int tsz = trc.size();
	for ( int idx=0; idx<tsz; idx++ )
	    trc.set( idx, (float)curtrcscale_->scale(trc.get(idx,curcomp)),
		     curcomp );
    }

    return true;
}


bool SEGYSeisTrcTranslator::writeData( const SeisTrc& trc )
{
    const int curcomp = selComp();

    mDefineStaticLocalObject( bool, allowudfs,
			      = GetEnvVarYN("OD_SEIS_SEGY_ALLOW_UDF") );
    mDefineStaticLocalObject( float, udfreplaceval,
		= (float) GetEnvVarDVal( "OD_SEIS_SEGY_UDF_REPLACE", 0 ) );
    for ( int idx=0; idx<outnrsamples_; idx++ )
    {
	float val = trc.getValue( outsd_.atIndex(idx), curcomp );
	if ( !allowudfs && mIsUdf(val) )
	    val = udfreplaceval;
	storinterp_->put( blockbuf_, idx, val );
    }

    if ( !sConn().oStream().addBin( blockbuf_,
			 outnrsamples_ * outcd_->datachar.nrBytes() ) )
	mErrRet(tr("Cannot write trace data"))

    headerdone_ = false;
    return true;
}


void SEGYSeisTrcTranslator::fillErrMsg( const uiString& s, bool withpos )
{
    const BufferString fnm = sConn().odStream().fileName();

    errmsg_ = tr("%1%2:\n%3")
	    .arg( fnm.isEmpty()
		 ? toUiString(usrname_)
		 : tr("In file '%1'").arg( fnm ) )
	    .arg( withpos
		? tr(" %1").arg( getTrcPosStr() )
		: uiString::emptyString() )
	    .arg( s );
}


bool SEGYSeisTrcTranslator::noErrMsg()
{
    errmsg_.setEmpty(); return false;
}
