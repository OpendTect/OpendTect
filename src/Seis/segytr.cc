/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/


#include "seisrangeseldata.h"
#include "posinfo.h"
#include "segytr.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "segyhdr.h"
#include "segyfiledef.h"
#include "datainterp.h"
#include "conn.h"
#include "iopar.h"
#include "scaler.h"
#include "survinfo.h"
#include "seisseldata.h"
#include "envvars.h"
#include "separstr.h"
#include "settings.h"
#include "zdomain.h"
#include "filepath.h"
#include "od_iostream.h"
#include "bendpoints2coords.h"
#include "uistrings.h"
#include <math.h>
#include <ctype.h>

#define mBPS(cd) (int)cd->datachar_.nrBytes()
#define mInDepth \
    (othdomain_ && SI().zIsTime()) || (!othdomain_ && !SI().zIsTime())
#define mZStepFac ( mInDepth ? 0.001 : 1.e-6 )

#define mIsCoordSysSame (*filepars_.getCoordSys() == *SI().getCoordSystem())

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
	, forcedrev_(-1)
	, useinpsd_(false)
	, inpcd_(0)
	, outcd_(0)
	, estnrtrcs_(-1)
	, curoffs_(-1)
	, curcoord_(mUdf(float),0)
	, prevoffs_(0)
	, othdomain_(false)
	, bp2c_(0)
	, selcomp_(0)
	, coordsys_(0)
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

    deleteAndZeroPtr( bp2c_ );

    mSetUdf(curbid_.inl()); mSetUdf(prevbid_.inl());
    curtrcnr_ = prevtrcnr_ = -1;
    prevoffs_ = curoffs_ = -1.f; mSetUdf(curcoord_.x_);
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
	mErrRet( tr("Cannot read SEG-Y Textual header (aka 'EBCDIC header')") )
    txthead_->setAscii();

    const int revcodeentry = SEGY::BinHeader::EntryRevCode();
    unsigned char binheaderbuf[400];
    if ( !strm.getBin( binheaderbuf, SegyBinHeaderLength ) )
	mErrRet( tr("Cannot read SEG-Y Binary header") )
    binhead_.setInput( binheaderbuf, filepars_.swapHdrs() );
    if ( binhead_.isSwapped() )
	binhead_.unSwap();

    trchead_.setNeedSwap( filepars_.swapHdrs() );
    trchead_.isrev0_ = binhead_.revision() < 1;
    if ( forcedrev_ >= 0 )
	trchead_.isrev0_ = forcedrev_ == 0;

    if ( !trchead_.isrev0_ )
    {
	const int nrstzs = binhead_.skipRev1Stanzas( strm );
	if ( nrstzs > 0 )
	    addWarn( cSEGYFoundStanzas, toUiString(nrstzs) );
	const int fixedtrcflag = binhead_.entryVal( revcodeentry + 1 );
	if ( fixedtrcflag == 0 )
	    addWarn( cSEGYWarnNonFixedLength );
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
	    addWarn( cSEGYWarnBadFmt, toUiString(filepars_.fmt_) );
	    filepars_.fmt_ = 1;
	}
    }

    txthead_->getText( pinfo_.usrinfo );
    pinfo_.nr = binhead_.entryVal( SEGY::BinHeader::EntryLino() );
    pinfo_.zrg.step = binhead_.sampleRate( mInDepth );
    insd_.step = pinfo_.zrg.step;
    innrsamples_ = binhead_.nrSamples();

    od_stream::Pos endpos = strm.endPosition();
    if ( endpos < 0 )
	estnrtrcs_ = -1;
    else
	estnrtrcs_ = mCast( int, (endpos - cEndTapeHeader)
			    / (cTraceHeaderBytes + dataBytes()*innrsamples_));
    if ( estnrtrcs_ < -1 )
	estnrtrcs_ = -1;
    return true;
}


void SEGYSeisTrcTranslator::addWarn( int nr, const uiString& detail )
{
    uiString firstoccat = tr("First occurrence at %1");
    uiString msg = toUiString( "[%1] %2" ).arg( nr );
    if ( nr == cSEGYWarnBadFmt )
    {
	msg.arg( tr("SEG-Y format '%1' found").arg(detail) );
	msg.appendPhrase( tr("Replaced with '1' (4-byte floating point)") );
	if ( toInt(toString(detail)) > 254 )
	    msg.appendPhrase(tr("The file may not be SEG-Y, or byte-swapped"));
    }
    else if ( nr == cSEGYWarnPos )
    {
	msg.arg( tr("Bad position(s) found. Such traces are ignored") );
	msg.appendPhrase( firstoccat.arg(detail) );
    }
    else if ( nr == cSEGYWarnZeroSampIntv )
    {
	msg.arg( tr("Zero sample interval found in trace header") );
	msg.appendPhrase( firstoccat.arg(detail) );
    }
    else if ( nr == cSEGYWarnDataReadIncomplete )
    {
	msg.arg( detail );
    }
    else if ( nr == cSEGYWarnNonrectCoord )
    {
	msg.arg( tr("Trace header indicates geographic coords (byte 89).\n\n"
	        "These are not supported as such.\n\n"
	        "Will bluntly load them as rectangular coordinates "
	        "(which they are most often)."
	        "\nBeware that the positions may therefore not be correct.\n"));
	msg.appendPhrase( firstoccat.arg(detail) );
    }
    else if ( nr == cSEGYWarnSuspiciousCoord )
    {
	msg.arg( tr("Suspiciously large coordinate found."
		"\nThis may be incorrect - please check the coordinate scaling."
		"\nOverrule if necessary.\nCoordinate found: %1 %2")
	    .arg( detail ).arg( getTrcPosStr() ) );
    }
    else if ( nr == cSEGYFoundStanzas )
    {
	msg.arg( tr("SEG-Y REV.1 header indicates the presence of "
	        "%1 Extended Textual File Header(s)."
		"\nThis is rarely correct. Please set the variable:"
		"\nOD_SEIS_SEGY_REV1_STANZAS"
		"\nif the file indeed contains these.")
		.arg( detail ) );
    }
    else if ( nr == cSEGYWarnNonFixedLength )
    {
	msg.arg( tr("SEG-Y REV.1 header indicates variable length traces."
	         "\nOpendTect will assume fixed trace length anyway.") );
    }

    SeisTrcTranslator::addWarn( nr, msg );
}


void SEGYSeisTrcTranslator::updateCDFromBuf()
{
    SeisTrcInfo info; trchead_.fill( info, is_2d, fileopts_.coordscale_ );
    if ( othdomain_ )
	info.sampling_.step *= SI().zIsTime() ? 1000 : 0.001f;

    insd_.start = info.sampling_.start;
    insd_.step = pinfo_.zrg.step;
    if ( mIsZero(insd_.step,Seis::cDefZEps()) )
    {
	insd_.step = info.sampling_.step;
	if ( mIsZero(insd_.step,Seis::cDefZEps()) )
	    insd_.step = SI().zRange().step;
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
		innrsamples_ = SI().zRange().nrSteps() + 1;
	}
    }

    addComp( getDataChar(filepars_.fmt_)  );
    DataCharacteristics& dc = tarcds_[0]->datachar_;
    dc.fmt_ = DataCharacteristics::Ieee;
    const float scfac = trchead_.postScale(filepars_.fmt_ ? filepars_.fmt_ : 1);
    if ( !mIsEqual(scfac,1,mDefEps)
      || (dc.isInteger() && dc.nrBytes() == BinDataDesc::N4) )
	dc = DataCharacteristics();
}


void SEGYSeisTrcTranslator::interpretBuf( SeisTrcInfo& ti )
{
    trchead_.fill( ti, is_2d, fileopts_.coordscale_ );
    if ( othdomain_ )
	ti.sampling_.step *= SI().zIsTime() ? 1000 : 0.001f;
    if ( binhead_.isInFeet() ) ti.offset_ *= mFromFeetFactorF;

    float scfac = trchead_.postScale( filepars_.fmt_ ? filepars_.fmt_ : 1 );
    if ( mIsEqual(scfac,1,mDefEps) )
	curtrcscale_ = 0;
    else
    {
	if ( !trcscale_ )
	    trcscale_ = new LinScaler( 0, scfac );
	else
	    trcscale_->factor_ = scfac;

	curtrcscale_ =	trcscale_;
    }

    if ( !mIsUdf(fileopts_.timeshift_) )
	ti.sampling_.start = fileopts_.timeshift_;
    if ( !mIsUdf(fileopts_.sampleintv_) )
	ti.sampling_.step = fileopts_.sampleintv_;

    if ( fileopts_.coorddef_ == SEGY::FileReadOpts::Generate )
    {
	if ( mIsUdf(curcoord_.x_) )
	    curcoord_ = fileopts_.startcoord_;
	else
	    curcoord_ += fileopts_.stepcoord_;
	ti.coord_ = curcoord_;
    }
    else if ( fileopts_.coorddef_ == SEGY::FileReadOpts::ReadFile )
    {
	if ( !bp2c_ )
	{
	    BufferString coordfnm( fileopts_.coordfnm_ );
	    if ( coordfnm.startsWith("ext=") )
	    {
		File::Path fp( ((StreamConn*)conn_)->fileName() );
		fp.setExtension( coordfnm.buf()+4 );
		coordfnm = fp.fullPath();
	    }

	    od_istream stream( coordfnm );
	    if ( !stream.isOK() )
	    {
		errmsg_ = tr("Cannot open coordinate file:\n%1")
				.arg( coordfnm );
		return;
	    }
	    bp2c_ = new BendPoints2Coords( stream );
	    stream.close();
	    if ( bp2c_->getIDs().size() < 2 )
	    {
		errmsg_ =
		    tr("Coordinate file bad (need at least 2 points):\n%1")
			.arg( coordfnm );
		return;
	    }
	}
	ti.coord_ = bp2c_->coordAt( mCast(float,ti.trcNr()) );
    }

    if ( ti.coord_.x_ > 1e9 || ti.coord_.y_ > 1e9 )
	addWarn( cSEGYWarnSuspiciousCoord, toUiString(ti.coord_) );
}


void SEGYSeisTrcTranslator::setTxtHeader( SEGY::TxtHeader* th )
{
    delete txthead_; txthead_ = th;
}


bool SEGYSeisTrcTranslator::writeTapeHeader()
{
    if ( filepars_.fmt_ == 0 ) // Auto-detect
	filepars_.fmt_ = nrFormatFor( trcdata_->getInterpreter()->dataChar() );

    trchead_.isrev0_ = false;

    if ( !txthead_ )
    {
	txthead_ = new SEGY::TxtHeader( trchead_.isrev0_ ? 0 : 1);
	txthead_->setUserInfo( pinfo_.usrinfo );
	fileopts_.thdef_.pinfo = &pinfo_;
	txthead_->setPosInfo( fileopts_.thdef_ );
	txthead_->setStartPos( outsd_.start );
	if ( Settings::common().isTrue(sKeyHdrEBCDIC()) )
	    txthead_->setEbcdic();
    }
    if ( !sConn().oStream().addBin( txthead_->txt_, SegyTxtHeaderLength ) )
	mErrRet(tr("Cannot write SEG-Y Textual header"))

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
	mErrRet(tr("Cannot write SEG-Y Binary header"))

    return true;
}


void SEGYSeisTrcTranslator::fillHeaderBuf( const SeisTrc& trc )
{
    SeisTrcInfo infotouse = trc.info();

     if ( !mIsCoordSysSame )
	infotouse.coord_ = filepars_.getCoordSys()->convertFrom(
				    infotouse.coord_, *SI().getCoordSystem() );
    if ( SI().xyInFeet() )
    {
	infotouse = trc.info();
	infotouse.offset_ *= mToFeetFactorF;
	trchead_.use( infotouse );
    }
    else
    {
	trchead_.use( infotouse );
    }

    SamplingData<float> sdtoput( useinpsd_ ? infotouse.sampling_ : outsd_ );
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
    if ( mIsEqual(outsd_.start,insd_.start,Seis::cDefZEps())
      && mIsEqual(outsd_.step,insd_.step,Seis::cDefZEps()) )
	useinpsd_ = true;

    return forread || writeTapeHeader();
}


bool SEGYSeisTrcTranslator::initRead_()
{
    if ( !readTapeHeader() )
	return false;
    else if ( !readTraceHeadBuffer() )
	mErrRet(tr("Cannot find one full trace in the file."))

    if ( forcedrev_ == 0 )
	trchead_.isrev0_ = true;
    trchead_.initRead();
    if ( tarcds_.isEmpty() )
	updateCDFromBuf();

    if ( innrsamples_ <= 0 || innrsamples_ > cMaxNrSamples )
	mErrRet(tr("Cannot find a reasonable number of samples."
	           "\nFound: %1.\nPlease 'Overrule' to set something usable")
              .arg(innrsamples_))

    offsetcalc_.set( fileopts_ );
    sConn().iStream().setReadPosition( cEndTapeHeader );
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
	tarcds_[idx]->datachar_ = dc;
	if ( idx != selcomp_ )
	    tarcds_[idx]->selected_ = false;
    }

    return true;
}


bool SEGYSeisTrcTranslator::goToTrace( int nr )
{
    od_stream::Pos so = nr;
    so *= (cTraceHeaderBytes + dataBytes() * innrsamples_);
    so += cEndTapeHeader;
    od_istream& strm = sConn().iStream();
    strm.setReadPosition( so );
    headerdone_ = false;
    return strm.isOK();
}



uiString SEGYSeisTrcTranslator::getTrcPosStr() const
{
    uiString ret;
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


    if ( usecur < 0 )
	return tr("at start of data");

    ret = toUiString( "@ %1 %2%3" );
    if ( is2d )
	ret.arg( uiStrings::sTraceNumber().toLower() )
	    .arg( usecur ? curtrcnr_ : prevtrcnr_ );
    else
    {
	const BinID bid( usecur ? curbid_ : prevbid_ );
	ret.arg( uiStrings::sPosition() ).arg( toUiString(bid) );
    }

    if ( !Seis::isPS(fileopts_.geomType()) )
	ret.arg( "" );
    else
	ret.arg( toUiString(" (%1 %2)")
		.arg( uiStrings::sOffset().toLower() )
		.arg( usecur ? curoffs_ : prevoffs_ ) );

    return ret;
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

    sConn().iStream().ignore( innrsamples_*mBPS(inpcd_) );
    if ( !readTraceHeadBuffer() )
	return false;
    if ( !tryInterpretBuf(ti) )
	return false;
    return (headerdone_ = true);
}


#define mBadCoord(ti) \
	(ti.coord_.x_ < 0.01 && ti.coord_.y_ < 0.01)
#define mBadBid(ti) \
	(ti.lineNr() <= 0 && ti.trcNr() <= 0)
#define mSkipThisTrace() { if ( !skipThisTrace(ti,nrbadtrcs) ) return false; }

bool SEGYSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !ensureSelectionsCommitted() )
	return false;
    if ( headerdone_ )
	return true;

    const int oldcurinl = curbid_.inl();
    const int oldcurtrcnr = curtrcnr_;
    if ( read_mode != Seis::Scan )
	{ mSetUdf(curbid_.inl()); mSetUdf(curtrcnr_); }

    if ( !readTraceHeadBuffer() )
	return false;
    if ( !tryInterpretBuf(ti) )
	return false;

    if ( !fileopts_.havetrcnrs_ )
    {
	if ( prevtrcnr_ < 0 )
	    curtrcnr_ = fileopts_.trcnrdef_.start;
	else
	    curtrcnr_ = prevtrcnr_ + fileopts_.trcnrdef_.step;
	ti.setTrcNr( curtrcnr_ );
    }

    bool goodpos = true;
    int nrbadtrcs = 0;

    if ( !is_2d && fileopts_.icdef_ == SEGY::FileReadOpts::XYOnly )
    {
	if ( !mIsCoordSysSame )
	    ti.coord_ = SI().getCoordSystem()->convertFrom( ti.coord_,
						    *filepars_.getCoordSys() );
	if ( read_mode == Seis::Scan )
	    goodpos = !mBadCoord(ti);
	else if ( read_mode == Seis::Prod )
	{
	    while ( mBadCoord(ti) )
		mSkipThisTrace()
	}
	ti.setPos( SI().transform(ti.coord_) );
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
	ti.coord_ = SI().transform( ti.binID() );
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
		ti.setPos( SI().transform(ti.coord_) );
	    else if ( mBadCoord(ti) )
		ti.coord_ = SI().transform( ti.binID() );
	}
    }

    if ( trchead_.isusable )
	trchead_.isusable = goodpos;

    if ( !useinpsd_ ) ti.sampling_ = outsd_;

    offsetcalc_.setOffset( ti, trchead_ );

    curbid_.inl() = oldcurinl;
    curtrcnr_ = oldcurtrcnr;

    if ( goodpos )
    {
	prevtrcnr_ = curtrcnr_; curtrcnr_ = ti.trcNr();
	prevbid_ = curbid_; curbid_ = ti.binID();
	prevoffs_ = curoffs_; curoffs_ = ti.offset_;
    }

    if ( mIsZero(ti.sampling_.step,Seis::cDefZEps()) )
    {
	addWarn(cSEGYWarnZeroSampIntv,getTrcPosStr());
	ti.sampling_.step = insd_.step;
    }

    if ( trchead_.nonrectcoords )
	addWarn(cSEGYWarnNonrectCoord,getTrcPosStr());

    if ( seldata_ && ti.is2D() )
	ti.setLineNr( seldata_->geomID().getI() );

    return (headerdone_ = true);
}


bool SEGYSeisTrcTranslator::skip( int ntrcs )
{
    if ( ntrcs < 1 )
	return true;
    if ( !trcdata_ )
	commitSelections();

    od_istream& strm = sConn().iStream();
    if ( !headerdone_ )
	strm.ignore( mSEGYTraceHeaderBytes );
    strm.ignore( innrsamples_ * mBPS(inpcd_) );
    if ( ntrcs > 1 )
	strm.setReadPosition( (ntrcs-1)
		* (mSEGYTraceHeaderBytes + innrsamples_ * mBPS(inpcd_)) );

    headerdone_ = false;

    if ( strm.isBad() )
	mPosErrRet( uiStrings::phrErrDuringRead() )
    return true;
}


#define mErrRetDiskFull(s) \
    mErrRet( s.appendPhrase(uiStrings::phrDiskSpace()) )


bool SEGYSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    OD::memZero( headerbuf_, mSEGYTraceHeaderBytes );
    fillHeaderBuf( trc );

    if ( !sConn().oStream().addBin( headerbuf_, mSEGYTraceHeaderBytes ) )
	mErrRetDiskFull(tr("Cannot write trace header"))

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


bool SEGYSeisTrcTranslator::readData( TraceData* tdtofill )
{
    if ( !ensureSelectionsCommitted() )
	return false;

    const bool directread = tdtofill && samprg_.step < 2;
    TraceData& tdata = directread ? *tdtofill : *trcdata_;
    int tdsz = samprg_.nrSteps() + 1;
    if ( tdata.size(0) != tdsz
      || tdata.nrComponents() != 1
      || tdata.getInterpreter(0)->dataChar() != inpcd_->datachar_ )
    {
	tdata.setNrComponents( 0, OD::AutoDataRep );
	tdata.addComponent( 1, inpcd_->datachar_ );
	tdata.reSize( tdsz );
    }

    od_istream& strm = sConn().iStream();
    if ( samprg_.start > 0 )
	strm.ignore( samprg_.start * mBPS(inpcd_) );

    int rdsz = nrSamps2Read() * mBPS(inpcd_);
    if ( !sConn().iStream().getBin(tdata.getComponent()->data(),rdsz) )
    {
	if ( strm.lastNrBytesRead() != rdsz )
	    addWarn( cSEGYWarnDataReadIncomplete, strm.lastNrBytesRead()
	    ? tr("Last trace is incomplete") : tr("No data in last trace") );
	return noErrMsg();
    }

    if ( samprg_.stop < innrsamples_-1 )
	strm.ignore( (innrsamples_-samprg_.stop-1) * mBPS(inpcd_) );

    if ( !strm.isBad() )
	datareaddone_ = true;

    if ( tdtofill && !directread )
    {
	const auto nrsamps2read = nrSamps2Read();
	for ( int isamp=0; isamp<nrsamps2read; isamp++ )
	    tdtofill->setValue( isamp,
				trcdata_->getValue(isamp*samprg_.step,0), 0 );
    }

    headerdone_ = false;

    return datareaddone_;
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
	trcdata_->setValue( idx, val );
    }

    if ( !sConn().oStream().addBin( trcdata_->getComponent()->data(),
			 outnrsamples_ * outcd_->datachar_.nrBytes() ) )
	mErrRetDiskFull(tr("Cannot write trace data"))

    headerdone_ = false;
    return true;
}


void SEGYSeisTrcTranslator::fillErrMsg( const uiString& s, bool withpos )
{
    const BufferString fnm = sConn().odStream().fileName();

    errmsg_ = toUiString("%1%2:\n%3")
		.arg( fnm.isEmpty()
		     ? toUiString(usrname_)
		     : tr("In file '%1'").arg( fnm ) )
		.arg( withpos
		    ? toUiString(" %1").arg( getTrcPosStr() )
		    : uiString::empty() )
		.arg( s );
}


bool SEGYSeisTrcTranslator::noErrMsg()
{
    errmsg_.setEmpty(); return false;
}


bool SEGYSeisTrcTranslator::getGeometryInfo( PosInfo::LineCollData& lcd ) const
{
    PosInfo::LineCollData* lcdptr = LineCollData::create(
			    seldata_->asRange()->fullSubSel().fullHorSubSel() );
    lcd = *lcdptr;
    return lcdptr ? true : false;
}
