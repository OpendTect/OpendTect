/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "segyuiscandata.h"
#include "segyhdrkeydata.h"
#include "segyhdr.h"
#include "seisinfo.h"
#include "od_istream.h"
#include "datainterp.h"
#include "dataclipper.h"
#include "posinfodetector.h"
#include "survinfo.h"
#include "executor.h"
#include "coordsystem.h"

static const int cQuickScanNrTrcsAtEnds = 225; // 2 times
static const int cQuickScanNrTrcsInMiddle = 25; // 2 times
// total max 500 traces per file
static const int cQuickScanMaxNrTrcs4LineChg = 10000;
// plus these 10000 worst-case for survey setup


SEGY::BasicFileInfo::BasicFileInfo( bool is2d )
{
    init( is2d );
}


SEGY::BasicFileInfo::BasicFileInfo( const BasicFileInfo& oth )
{
    *this = oth;
}


SEGY::BasicFileInfo::~BasicFileInfo()
{
}


SEGY::BasicFileInfo& SEGY::BasicFileInfo::operator=( const BasicFileInfo& oth )
{
    revision_ = oth.revision_;
    binsr_ = oth.binsr_;
    ns_ = oth.ns_;
    thdrns_ = oth.thdrns_;
    format_ = oth.format_;
    sampling_ = oth.sampling_;
    hdrsswapped_ = oth.hdrsswapped_;
    dataswapped_ = oth.dataswapped_;
    is2d_ = oth.is2d_;
    return *this;
}


void SEGY::BasicFileInfo::init( bool is2d )
{
    is2d_ = is2d;
    revision_ = ns_ = thdrns_ = binsr_ -1;
    format_ = 5;
    sampling_.start = 1.0f;
    sampling_.step = mUdf(float);
    hdrsswapped_ = dataswapped_ = false;
    usenrsampsinfile_ = true;
    useformatinfile_ = true;
}


int SEGY::BasicFileInfo::bytesPerSample() const
{
    return SEGY::BinHeader::formatBytes( format_ );
}


int SEGY::BasicFileInfo::traceDataBytes() const
{
    return ns_ < 0 ? 0 : ns_ * bytesPerSample();
}


DataCharacteristics SEGY::BasicFileInfo::getDataChar() const
{
    return SEGY::BinHeader::getDataChar( format_, dataswapped_ );
}


int SEGY::BasicFileInfo::nrTracesIn( const od_istream& strm,
				 od_stream_Pos startpos ) const
{
    if ( startpos < 0 )
	startpos = SegyTxtHeaderLength + SegyBinHeaderLength;

    const od_int64 databytes = strm.endPosition() - startpos;
    if ( databytes <= 0 )
	return 0;

    const int trcbytes = SegyTrcHeaderLength + traceDataBytes();
    return (int)(databytes / trcbytes);
}


bool SEGY::BasicFileInfo::goToTrace( od_istream& strm, od_stream_Pos startpos,
				     int trcidx ) const
{
    if ( trcidx < 0 )
	return false;

    const int trcbytes = SegyTrcHeaderLength + traceDataBytes();
    od_stream_Pos offs = trcidx; offs *= trcbytes;
    startpos += offs;
    if ( startpos >= strm.endPosition() )
	return false;

    strm.setReadPosition( startpos );
    return true;
}


void SEGY::BasicFileInfo::getFilePars( SEGY::FilePars& fpars ) const
{
    fpars.ns_ = ns_;
    fpars.fmt_ = format_;
    fpars.setSwap( hdrsswapped_, dataswapped_ );
}


const SEGY::TrcHeaderDef& SEGY::BasicFileInfo::getHDef() const
{
    mDefineStaticLocalObject( TrcHeaderDef, trcdef, );
    return trcdef;
}


SEGY::TrcHeader* SEGY::BasicFileInfo::getTrcHdr( od_istream& strm ) const
{
    char* thbuf = new char[ SegyTrcHeaderLength ];
    strm.getBin( thbuf, SegyTrcHeaderLength );
    if ( !strm.isOK() )
	return 0;

    SEGY::TrcHeader* th = new SEGY::TrcHeader( (unsigned char*)thbuf,
						getHDef(), isRev0(), true );
    th->initRead();
    return th;
}


#define mErrRetWithFileName(s) \
    return toUiString( "File:\n%1\n%2" ).arg( strm.fileName() ).arg(s);


uiString SEGY::BasicFileInfo::getFrom( od_istream& strm, bool& inft,
					const bool* knownhdrswap )
{
    strm.setReadPosition( 0 );
    if ( !strm.isOK() )
	mErrRetWithFileName( "is empty" )

    SEGY::TxtHeader txthdr;
    strm.getBin( txthdr.txt_, SegyTxtHeaderLength );
    if ( !strm.isOK() )
	mErrRetWithFileName( "has no textual header" )

    SEGY::BinHeader binhdr;
    strm.getBin( binhdr.buf(), SegyBinHeaderLength );
    if ( strm.isBad() )
	mErrRetWithFileName( "has no binary header" )

    binhdr.guessIsSwapped();
    hdrsswapped_ = dataswapped_ = binhdr.isSwapped();
    if ( (knownhdrswap && *knownhdrswap) || (!knownhdrswap && hdrsswapped_) )
	binhdr.unSwap();
    if ( !binhdr.isRev0() )
	binhdr.skipRev1Stanzas( strm );
    inft = binhdr.isInFeet();

    binsr_ = binhdr.rawSampleRate();
    if ( usenrsampsinfile_ )
	ns_ = binhdr.nrSamples();

    revision_ = binhdr.revision();
    if ( useformatinfile_ )
    {
	short fmt = binhdr.format();
	if ( fmt != 1 && fmt != 2 && fmt != 3 && fmt != 5 && fmt != 8 )
	    fmt = 1;

	format_ = fmt;
    }

    od_stream_Pos firsttrcpos = strm.position();
    PtrMan<SEGY::TrcHeader> thdr = getTrcHdr( strm );
    strm.setReadPosition( firsttrcpos );
    if ( !thdr )
	mErrRetWithFileName( "No traces found" )

    thdrns_ = int(thdr->nrSamples());
    if ( ns_ == 0 )
	ns_ = thdrns_;

    SeisTrcInfo ti;
    if ( thdr->is2D() != is2d_ )
	thdr->geomtype_ = is2d_ ? Seis::Line : Seis::Vol;

    thdr->fill( ti, 1.0f );
    sampling_ = ti.sampling;
    if ( mIsZero(sampling_.step,1.e-8) )
	sampling_.step = binhdr.sampleRate( false );

    return uiString::emptyString();
}



SEGY::LoadDef::LoadDef( bool is2d )
    : BasicFileInfo(is2d)
    , coordsys_(SI().getCoordSystem() )
{
    reInit( is2d, true );
}


SEGY::LoadDef::LoadDef( const LoadDef& oth )
    : BasicFileInfo(oth)
    , hdrdef_(new TrcHeaderDef)
{
    *this = oth;
}


void SEGY::LoadDef::reInit( bool is2d, bool alsohdef )
{
    init( is2d );

    coordscale_ = mUdf(float);
    icvsxytype_ = FileReadOpts::ICOnly;
    havetrcnrs_ = true;
    trcnrdef_ = SamplingData<int>( 1000, 1 );
    psoffssrc_ = FileReadOpts::InFile;
    psoffsdef_ = SamplingData<float>( 0.f, 1.f );
    usezsamplinginfile_ = true;
    coordsys_ = SI().getCoordSystem();
    if ( alsohdef )
    {
	delete hdrdef_;
	hdrdef_ = new TrcHeaderDef;
    }
}


SEGY::LoadDef::~LoadDef()
{
    delete hdrdef_;
}


SEGY::LoadDef& SEGY::LoadDef::operator =( const SEGY::LoadDef& oth )
{
    if ( this != &oth )
    {
	BasicFileInfo::operator =( oth );
	coordscale_ = oth.coordscale_;
	icvsxytype_ = oth.icvsxytype_;
	havetrcnrs_ = oth.havetrcnrs_;
	trcnrdef_ = oth.trcnrdef_;
	psoffssrc_ = oth.psoffssrc_;
	psoffsdef_ = oth.psoffsdef_;
	usezsamplinginfile_ = oth.usezsamplinginfile_;
	hdrdef_ = new TrcHeaderDef( *oth.hdrdef_ );
	coordsys_ = oth.coordsys_;
    }
    return *this;
}


SEGY::LoadDef SEGY::LoadDef::getPrepared( od_istream& strm ) const
{
    od_stream_Pos orgpos = strm.position();
    LoadDef rddef( *this ); bool dum;
    uiString msg = rddef.getFrom( strm, dum, &hdrsswapped_ );
    strm.setReadPosition( orgpos );
    if ( !msg.isEmpty() )
	return *this;

    LoadDef ret( *this );
    ret.ns_ = rddef.ns_;
    ret.sampling_ = rddef.sampling_;
    ret.format_ = rddef.format_;
    return ret;
}


void SEGY::LoadDef::getTrcInfo( SEGY::TrcHeader& thdr, SeisTrcInfo& ti,
				const SEGY::OffsetCalculator& offscalc ) const
{
    if ( thdr.is2D() != is2d_ )
	thdr.geomtype_ = is2d_ ? Seis::Line : Seis::Vol;

    thdr.fill( ti, coordscale_ );
    offscalc.setOffset( ti, thdr );
    if ( icvsxytype_ == FileReadOpts::ICOnly )
	ti.calcCoord();
    else if ( !is2d_ && icvsxytype_ == FileReadOpts::XYOnly )
    {
	if ( coordsys_ && !(*SI().getCoordSystem() == *coordsys_) )
	    ti.coord = SI().getCoordSystem()->convertFrom(ti.coord,*coordsys_);
	ti.setPos( SI().transform( ti.coord ) );
    }
}


bool SEGY::LoadDef::getData( od_istream& strm, char* buf, float* vals ) const
{
    const int trcbytes = traceDataBytes();
    if ( !strm.getBin(buf,trcbytes) )
	return false;

    if ( vals )
    {
	const int bps = bytesPerSample();
	const DataInterpreter<float> di( getDataChar() );
	const char* bufend = buf + trcbytes;
	while ( buf != bufend )
	{
	    *vals = di.get( buf, 0 );
	    buf += bps; vals++;
	}
    }

    return true;
}


bool SEGY::LoadDef::skipData( od_istream& strm ) const
{
    strm.ignore( traceDataBytes() );
    return strm.isOK();
}


SEGY::TrcHeader* SEGY::LoadDef::getTrace( od_istream& strm,
					    char* buf, float* vals ) const
{
    TrcHeader* thdr = getTrcHdr( strm );
    if ( !thdr || !getData(strm,buf,vals) )
	{ delete thdr; return 0; }
    thdr->setNeedSwap( hdrsswapped_ );
    return thdr;
}


void SEGY::LoadDef::getFilePars( SEGY::FilePars& fpars ) const
{
    BasicFileInfo::getFilePars( fpars );
    if ( coordsys_ )
      fpars.setCoordSys( coordsys_ );
    if ( usenrsampsinfile_ )
	fpars.ns_ = 0;
    if ( useformatinfile_ )
	fpars.fmt_ = 0;
}


void SEGY::LoadDef::getFileReadOpts( SEGY::FileReadOpts& readopts ) const
{
    readopts.thdef_ = *hdrdef_;
    readopts.coordscale_ = coordscale_;
    readopts.timeshift_ = usezsamplinginfile_ ? mUdf(float) : sampling_.start;
    readopts.sampleintv_ = usezsamplinginfile_ ? mUdf(float) : sampling_.step;
    readopts.icdef_ = icvsxytype_;
    readopts.psdef_ = psoffssrc_;
    readopts.havetrcnrs_ = havetrcnrs_;
    readopts.trcnrdef_ = trcnrdef_;
    readopts.offsdef_ = psoffsdef_;
}


void SEGY::LoadDef::usePar( const IOPar& iop )
{
    FilePars filepars; getFilePars( filepars );
    filepars.usePar( iop );
    ns_ = filepars.ns_; format_ = (short)filepars.fmt_;
    hdrsswapped_ = filepars.swapHdrs();
    dataswapped_ = filepars.swapData();

    iop.get( FilePars::sKeyRevision(), revision_ );
    if ( iop.isTrue(FilePars::sKeyForceRev0()) )
	revision_ = 0;

    FileReadOpts readopts( Seis::Vol ); getFileReadOpts( readopts );
    readopts.usePar( iop );
    *hdrdef_ = readopts.thdef_;
    coordscale_ = readopts.coordscale_;
    sampling_.start = readopts.timeshift_;
    sampling_.step = readopts.sampleintv_;
    icvsxytype_ = readopts.icdef_;
    havetrcnrs_ = readopts.havetrcnrs_;
    trcnrdef_ = readopts.trcnrdef_;
    psoffssrc_ = readopts.psdef_;
    psoffsdef_ = readopts.offsdef_;
    if ( coordsys_ )
	coordsys_.getNonConstPtr()->usePar( iop );
}


bool SEGY::LoadDef::needXY() const
{
    return icvsxytype_ != FileReadOpts::ICOnly;
}


void SEGY::ScanRangeInfo::reInit()
{
    inls_ = Interval<int>( mUdf(int), 0 );
    crls_ = Interval<int>( mUdf(int), 0 );
    trcnrs_ = Interval<int>( mUdf(int), 0 );
    xrg_ = Interval<double>( mUdf(double), 0. );
    yrg_ = Interval<double>( mUdf(double), 0. );
    refnrs_ = Interval<float>( mUdf(float), 0.f );
    offs_ = Interval<float>( mUdf(float), 0.f );
    azims_ = Interval<float>( mUdf(float), 0.f );
}


void SEGY::ScanRangeInfo::use( const PosInfo::Detector& dtector )
{
    const Coord cmin( dtector.minCoord() );
    const Coord cmax( dtector.maxCoord() );

    xrg_.start = cmin.x; xrg_.stop = cmax.x;
    yrg_.start = cmin.y; yrg_.stop = cmax.y;

    BinID startbid( dtector.start() );
    BinID stopbid( dtector.stop() );
    trcnrs_.start = startbid.crl(); trcnrs_.stop = stopbid.crl();
    if ( dtector.is2D() )
    {
	startbid = SI().transform( cmin );
	stopbid = SI().transform( cmax );
    }
    inls_.start = startbid.inl(); inls_.stop = stopbid.inl();
    crls_.start = startbid.crl(); crls_.stop = stopbid.crl();
    inls_.sort(); crls_.sort();

    offs_ = dtector.offsRg();
    azims_ = dtector.azimuthRg();
}


void SEGY::ScanRangeInfo::merge( const SEGY::ScanRangeInfo& si )
{
    inls_.include( si.inls_, false );
    crls_.include( si.crls_, false );
    trcnrs_.include( si.trcnrs_, false );
    xrg_.include( si.xrg_, false );
    yrg_.include( si.yrg_, false );
    refnrs_.include( si.refnrs_, false );
    offs_.include( si.offs_, false );
    azims_.include( si.azims_, false );
}



SEGY::ScanInfo::ScanInfo( const char* fnm, bool is2d )
    : filenm_(fnm)
    , keydata_(*new HdrEntryKeyData)
    , basicinfo_(is2d)
{
    init( is2d );
}


void SEGY::ScanInfo::init( bool is2d )
{
    full_ = false;
    nrtrcs_ = idxfirstlive_ = 0;

    rgs_.reInit();
    keydata_.setEmpty();

    PosInfo::Detector::Setup pisu( is2d );
    pisu.isps( true ).reqsorting( false );
    delete pidetector_;
    pidetector_ = new PosInfo::Detector( pisu );
}


SEGY::ScanInfo::~ScanInfo()
{
    delete pidetector_;
    delete &keydata_;
}


bool SEGY::ScanInfo::is2D() const
{
    return pidetector_ && pidetector_->is2D();
}


namespace SEGY
{

class FullUIScanner : public ::Executor
{ mODTextTranslationClass(FullUIScanner)
public:

FullUIScanner( ScanInfo& si, od_istream& strm, const LoadDef& def,
		char* buf, float* vals, DataClipSampler& cs,
		const OffsetCalculator& oc )
    : ::Executor("SEG-Y scanner")
    , si_(si) , strm_(strm), def_(def) , buf_(buf) , vals_(vals)
    , clipsampler_(cs), offscalc_(oc)
    , nrdone_(1)
{
    si_.full_ = true;
    totalnr_ = def_.nrTracesIn( strm );
}

virtual uiString uiNrDoneText() const	{ return tr("Traces handled"); }
virtual od_int64 nrDone() const		{ return nrdone_; }
virtual od_int64 totalNr() const	{ return totalnr_; }

virtual uiString uiMessage() const
{
    uiString ret( tr("Scanning traces in %1") );
    ret.arg( strm_.fileName() );
    return ret;
}

virtual int nextStep()
{
    for ( int idx=0; idx<10; idx++ )
    {
	PtrMan<TrcHeader> thdr = def_.getTrace( strm_, buf_, vals_ );
	if ( !thdr )
	    return Finished();
	else if ( !thdr->isusable )
	    continue; // dead trace

	nrdone_++;
	si_.addTrace( *thdr, vals_, def_, clipsampler_, offscalc_,
			(int)nrdone_ );
    }

    return MoreToDo();
}

    ScanInfo&		si_;
    od_istream&		strm_;
    const LoadDef&	def_;
    char*		buf_;
    float*		vals_;
    DataClipSampler&	clipsampler_;
    const OffsetCalculator& offscalc_;
    od_int64		nrdone_, totalnr_;

}; // class FullUIScanner

} // namespace SEGY


void SEGY::ScanInfo::getFromSEGYBody( od_istream& strm, const LoadDef& indef,
			bool forsurvsetup,
		    DataClipSampler& clipsampler, TaskRunner* fullscanrunner )
{
    reInit();
    const LoadDef def( indef.getPrepared(strm) );

    startpos_ = strm.position();
    nrtrcs_ = def.nrTracesIn( strm, startpos_ );
    if ( !def.isValid() || nrtrcs_ == 0 )
	return;

    mAllocLargeVarLenArr( char, buf, def.traceDataBytes() );
    mAllocLargeVarLenArr( float, vals, def.ns_ );

    PtrMan<TrcHeader> thdr = def.getTrace( strm, buf, vals );
    idxfirstlive_ = 0;
    if ( !thdr )
	{ finishGet(strm); return; }
    while ( !thdr->isusable )
    {
	// skip dead traces to get at least the first good trace
	idxfirstlive_++;
	thdr = def.getTrace( strm, buf, vals );
	if ( !thdr )
	    { finishGet(strm); return; }
    }

    OffsetCalculator offscalc;
    offscalc.type_ = def.psoffssrc_; offscalc.def_ = def.psoffsdef_;
    offscalc.is2d_ = is2D(); offscalc.coordscale_ = def.coordscale_;

    addTrace( *thdr, vals, def, clipsampler, offscalc, idxfirstlive_ );

    if ( fullscanrunner )
    {
	FullUIScanner scanner( *this, strm, def, buf, vals,
			       clipsampler, offscalc );
	TaskRunner::execute( fullscanrunner, scanner );
    }
    else
    {
#define	mAddTrcs() addTraces(strm,trcrg,buf,vals,def,clipsampler,offscalc)
	Interval<int> trcrg( 1, cQuickScanNrTrcsAtEnds );
	mAddTrcs();
	if ( !is2D() && forsurvsetup )
	    ensureStepsFound( strm, buf, vals, def, clipsampler, offscalc );
	trcrg.start = nrtrcs_/3 - cQuickScanNrTrcsInMiddle/2;
	trcrg.stop = trcrg.start + cQuickScanNrTrcsInMiddle - 1;
	mAddTrcs();
	trcrg.start = (2*nrtrcs_)/3 - cQuickScanNrTrcsInMiddle/2;
	trcrg.stop = trcrg.start + cQuickScanNrTrcsInMiddle - 1;
	mAddTrcs();
	trcrg.start = nrtrcs_ - cQuickScanNrTrcsAtEnds;
	trcrg.stop = nrtrcs_ - 1;
	mAddTrcs();
    }

    finishGet( strm );
}


void SEGY::ScanInfo::addTrace( TrcHeader& thdr, const float* vals,
			const LoadDef& def, DataClipSampler& clipsampler,
			const OffsetCalculator& offscalc, int nrinfile )
{
    SeisTrcInfo ti;
    def.getTrcInfo( thdr, ti, offscalc );

    const bool isfirst = nrinfile == idxfirstlive_;
    if ( !def.havetrcnrs_ )
	ti.setTrcNr( def.trcnrdef_.atIndex( nrinfile - idxfirstlive_ ) );

    if ( !full_ )
	keydata_.add( thdr, def.hdrsswapped_, isfirst );
    pidetector_->add( ti.coord, ti.binID(), ti.trcNr(), ti.offset, ti.azimuth );
    addValues( clipsampler, vals, def.ns_ );

    if ( isfirst )
	rgs_.refnrs_.start = rgs_.refnrs_.stop = ti.refnr;
    else
	rgs_.refnrs_.include( ti.refnr, false );
}


void SEGY::ScanInfo::addTraces( od_istream& strm, Interval<int> trcidxs,
				char* buf, float* vals, const LoadDef& def,
				DataClipSampler& clipsampler,
				const OffsetCalculator& offscalc )
{
    int curtrcidx = trcidxs.start;
    if ( !def.goToTrace(strm,startpos_,curtrcidx) )
	return;

    for ( ; curtrcidx<=trcidxs.stop; curtrcidx++ )
	if ( !addNextTrace(strm,buf,vals,def,clipsampler,offscalc) )
	    break;
}


bool SEGY::ScanInfo::addNextTrace( od_istream& strm,
				    char* buf, float* vals, const LoadDef& def,
				    DataClipSampler& clipsampler,
				    const OffsetCalculator& offscalc )
{
    PtrMan<TrcHeader> thdr = def.getTrace( strm, buf, vals );
    if ( !thdr )
	return false;

    if ( thdr->isusable )
    {
	od_stream_Pos tidx = strm.position() - startpos_;
	tidx /= SegyTrcHeaderLength + def.traceDataBytes();
	addTrace( *thdr, vals, def, clipsampler, offscalc, (int)tidx );
    }

    return true;
}


void SEGY::ScanInfo::ensureStepsFound( od_istream& strm,
				char* buf, float* vals, const LoadDef& def,
				DataClipSampler& clipsampler,
				const OffsetCalculator& offscalc )
{
    for ( int itrc=0; itrc<cQuickScanMaxNrTrcs4LineChg; itrc++ )
    {
	const bool haveinlstep = pidetector_->haveStep( true );
	const bool havecrlstep = pidetector_->haveStep( false );
	if ( (haveinlstep && havecrlstep)
	  || !addNextTrace(strm,buf,vals,def,clipsampler,offscalc) )
	    break;
    }
}


void SEGY::ScanInfo::addValues( DataClipSampler& cs, const float* vals, int ns )
{
    if ( !vals || ns < 1 )
	return;

    // avoid null traces
    for ( int idx=0; idx<ns; idx++ )
    {
	if ( vals[idx] != 0.f )
	{
	    cs.add( vals, ns );
	    break;
	}
    }
}


void SEGY::ScanInfo::finishGet( od_istream& strm )
{
    pidetector_->finish();
    rgs_.use( *pidetector_ );
    strm.setReadPosition( startpos_ );
}


SEGY::ScanInfoSet::ScanInfoSet( bool is2d, bool isps )
    : is2d_(is2d)
    , isps_(isps)
    , keydata_(*new HdrEntryKeyData)
    , detector_(*new PosInfo::Detector(is2d))
{
    setEmpty();
}


SEGY::ScanInfoSet::~ScanInfoSet()
{
    deepErase( sis_ );
    delete &keydata_;
    delete &detector_;
}


void SEGY::ScanInfoSet::setEmpty()
{
    nrtrcs_ = 0;
    infeet_ = false;
    keydata_.setEmpty();
    rgs_.reInit();
    deepErase( sis_ );
    detector_.reInit();
}


SEGY::ScanInfo& SEGY::ScanInfoSet::add( const char* fnm )
{
    ScanInfo* si = new ScanInfo( fnm, is2d_ );
    sis_ += si;
    return *si;
}


void SEGY::ScanInfoSet::removeLast()
{
    const int sz = sis_.size();
    if ( sz > 0 )
	sis_.removeSingle( sz-1 );
}


void SEGY::ScanInfoSet::finish()
{
    if ( sis_.isEmpty() )
	{ nrtrcs_ = 0; return; }

    const ScanInfo& sis0 = *sis_[0];
    nrtrcs_ = sis0.nrTraces();
    rgs_ = sis0.ranges();
    keydata_ = sis0.keyData();
    detector_ = sis0.piDetector();

    for ( int idx=1; idx<sis_.size(); idx++ )
    {
	const ScanInfo& sis = *sis_[idx];
	nrtrcs_ += sis.nrTraces();
	rgs_.merge( sis.ranges() );
	keydata_.merge( sis.keyData() );
	detector_.appendResults( sis.piDetector() );
    }

    keydata_.finish( isps_ );
    detector_.finish();
}


static const SEGY::BasicFileInfo dummyfileinfo( false );

const SEGY::BasicFileInfo& SEGY::ScanInfoSet::basicInfo() const
{
    return sis_.isEmpty() ? dummyfileinfo : sis_[0]->basicInfo();
}


bool SEGY::ScanInfoSet::isFull() const
{
    for ( int idx=0; idx<sis_.size(); idx++ )
	if ( !sis_[idx]->isFull() )
	    return false;
    return true;
}
