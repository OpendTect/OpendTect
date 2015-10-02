/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "segyuiscandata.h"
#include "segyhdrkeydata.h"
#include "segyhdr.h"
#include "segyhdrdef.h"
#include "seisinfo.h"
#include "od_istream.h"
#include "datainterp.h"
#include "dataclipper.h"
#include "posinfodetector.h"
#include "sortedlist.h"
#include "survinfo.h"
#include "executor.h"

#include "uitaskrunner.h"

static const int cQuickScanNrTrcsAtEnds = 225; // 2 times
static const int cQuickScanNrTrcsInMiddle = 25; // 2 times
// total max 500 traces per file


void SEGY::BasicFileInfo::init()
{
    revision_ = ns_ = -1;
    format_ = 5;
    sampling_.start = 1.0f;
    sampling_.step = mUdf(float);
    hdrsswapped_ = dataswapped_ = false;
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

    strm.setPosition( startpos );
    return true;
}


SEGY::LoadDef::LoadDef()
    : hdrdef_(0)
{
    reInit(true);
}


void SEGY::LoadDef::reInit( bool alsohdef )
{
    init();

    coordscale_ = mUdf(float);
    psoffssrc_ = FileReadOpts::InFile;
    icvsxytype_ = FileReadOpts::ICOnly;
    psoffsdef_ = SamplingData<float>( 0.f, 1.f );
    if ( alsohdef )
	{ delete hdrdef_; hdrdef_ = new TrcHeaderDef; }
}


SEGY::LoadDef::~LoadDef()
{
    delete hdrdef_;
}


SEGY::TrcHeader* SEGY::LoadDef::getTrcHdr( od_istream& strm ) const
{
    char* thbuf = new char[ SegyTrcHeaderLength ];
    strm.getBin( thbuf, SegyTrcHeaderLength );
    if ( !strm.isOK() )
	return 0;

    SEGY::TrcHeader* th = new SEGY::TrcHeader(
			 (unsigned char*)thbuf, *hdrdef_, isRev0(), true );
    th->initRead();
    return th;
}



void SEGY::LoadDef::getTrcInfo( SEGY::TrcHeader& thdr, SeisTrcInfo& ti,
				const SEGY::OffsetCalculator& offscalc ) const
{
    thdr.fill( ti, coordscale_ );
    offscalc.setOffset( ti, thdr );
    if ( icvsxytype_ == FileReadOpts::ICOnly )
	ti.coord = SI().transform( ti.binid );
    else if ( icvsxytype_ == FileReadOpts::XYOnly )
	ti.binid = SI().transform( ti.coord );
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
    return thdr;
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
}



SEGY::ScanInfo::ScanInfo( const char* fnm, bool is2d )
    : filenm_(fnm)
    , keydata_(*new HdrEntryKeyData)
    , pidetector_(0)
{
    init( is2d );
}


void SEGY::ScanInfo::init( bool is2d )
{
    full_ = false;
    nrtrcs_ = 0;

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
    si_.full_ = false;
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
	    { si_.full_ = true; return Finished(); }
	else if ( !thdr->isusable )
	    continue; // dead trace

	si_.addTrace( *thdr, vals_, def_, clipsampler_, offscalc_ );
	nrdone_++;
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

}; // end class FullUIScanner

} // namespace SEGY


void SEGY::ScanInfo::getFromSEGYBody( od_istream& strm, const LoadDef& def,
		    DataClipSampler& clipsampler, TaskRunner* fullscanrunner )
{
    reInit();

    startpos_ = strm.position();
    nrtrcs_ = def.nrTracesIn( strm, startpos_ );
    if ( !def.isValid() || nrtrcs_ == 0 )
	return;

    mAllocLargeVarLenArr( char, buf, def.traceDataBytes() );
    mAllocLargeVarLenArr( float, vals, def.ns_ );

    PtrMan<TrcHeader> thdr = def.getTrace( strm, buf, vals );
    if ( !thdr )
	{ finishGet(strm); return; }
    while ( !thdr->isusable )
    {
	// skip dead traces to get at least the first good trace
	thdr = def.getTrace( strm, buf, vals );
	if ( !thdr )
	    { finishGet(strm); return; }
    }

    OffsetCalculator offscalc;
    offscalc.type_ = def.psoffssrc_; offscalc.def_ = def.psoffsdef_;
    offscalc.is2d_ = is2D(); offscalc.coordscale_ = def.coordscale_;

    addTrace( *thdr, vals, def, clipsampler, offscalc, true );

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
			const OffsetCalculator& offscalc, bool isfirst )
{
    SeisTrcInfo ti;
    def.getTrcInfo( thdr, ti, offscalc );

    keydata_.add( thdr, def.hdrsswapped_ );
    pidetector_->add( ti.coord, ti.binid, ti.nr, ti.offset );
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
    {
	PtrMan<TrcHeader> thdr = def.getTrace( strm, buf, vals );
	if ( !thdr )
	    break;

	if ( thdr->isusable )
	    addTrace( *thdr, vals, def, clipsampler, offscalc );
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
    keydata_.finish();
    pidetector_->finish();
    rgs_.use( *pidetector_ );
    strm.setPosition( startpos_ );
}


SEGY::ScanInfoSet::ScanInfoSet( bool is2d )
    : is2d_(is2d)
    , keydata_(*new HdrEntryKeyData)
    , detector_(*new PosInfo::Detector(is2d))
{
    setEmpty();
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

    keydata_.finish();
    detector_.finish();
}


static const SEGY::BasicFileInfo dummyfileinfo;

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
