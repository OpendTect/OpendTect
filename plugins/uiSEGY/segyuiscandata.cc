/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "segyuiscandata.h"
#include "segyhdr.h"
#include "segyhdrdef.h"
#include "seisinfo.h"
#include "od_istream.h"
#include "datainterp.h"
#include "dataclipper.h"
#include "posinfodetector.h"
#include "survinfo.h"
#include "executor.h"

#include "uitaskrunner.h"


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
    startpos += trcidx * trcbytes;
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


bool SEGY::LoadDef::findRev0Bytes( od_istream& strm )
{
    if ( strm.position() == 0 )
	strm.setPosition( 3600 );
    const od_stream::Pos firsttrcpos = strm.position();

    // const SEGY::HdrDef& hdrdef = SEGY::TrcHeader::hdrDef();
    do
    {
	PtrMan<TrcHeader> thdr = getTrcHdr( strm );

    } while ( skipData(strm) );

    strm.setPosition( firsttrcpos );
    return true;
}



SEGY::ScanInfo::ScanInfo( const char* fnm )
    : filenm_(fnm)
{
    reInit();
}


void SEGY::ScanInfo::reInit()
{
    usable_ = fullscan_ = false;
    nrtrcs_ = 0;
    infeet_ = false;
    inls_ = Interval<int>( mUdf(int), 0 );
    crls_ = Interval<int>( mUdf(int), 0 );
    trcnrs_ = Interval<int>( mUdf(int), 0 );
    xrg_ = Interval<double>( mUdf(double), 0. );
    yrg_ = Interval<double>( mUdf(double), 0. );
    refnrs_ = Interval<float>( mUdf(float), 0.f );
    offsrg_ = Interval<float>( mUdf(float), 0.f );
}


namespace SEGY
{

class FullUIScanner : public ::Executor
{ mODTextTranslationClass(FullUIScanner)
public:

FullUIScanner( ScanInfo& si, od_istream& strm, const LoadDef& def,
		char* buf, float* vals, DataClipSampler& cs,
		const SEGY::OffsetCalculator& oc, PosInfo::Detector* dt )
    : ::Executor("SEG-Y scanner")
    , si_(si) , strm_(strm), def_(def) , buf_(buf) , vals_(vals)
    , cs_(cs), offscalc_(oc), dtctr_(dt)
    , nrdone_(1)
{
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
	{
	    if ( dtctr_ )
		dtctr_->finish();
	    return Finished();
	}
	if ( !thdr->isusable )
	    continue;

	thdr->fill( ti_, def_.coordscale_ );
	offscalc_.setOffset( ti_, *thdr );
	si_.addPositions( ti_, dtctr_ );
	si_.addValues( cs_, vals_, def_.ns_ );
	nrdone_++;
    }

    return MoreToDo();
}

    ScanInfo&		si_;
    SeisTrcInfo		ti_;
    od_istream&		strm_;
    const LoadDef&	def_;
    char*		buf_;
    float*		vals_;
    DataClipSampler&	cs_;
    PosInfo::Detector*	dtctr_;
    const SEGY::OffsetCalculator& offscalc_;
    od_int64		nrdone_, totalnr_;

}; // end class FullUIScanner

} // namespace SEGY


#define mAdd2Dtector(dt,ti) \
    if ( dt ) \
	dt->add( ti.coord, ti.binid, ti.nr, ti.offset )


void SEGY::ScanInfo::getFromSEGYBody( od_istream& strm, const LoadDef& def,
				bool ismulti, bool is2d, DataClipSampler& cs,
				bool full, PosInfo::Detector* dtctr,
				uiParent* uiparent )
{
    startpos_ = strm.position();
    nrtrcs_ = def.nrTracesIn( strm, startpos_ );
    if ( !def.isValid() )
	return;

    mAllocLargeVarLenArr( char, buf, def.traceDataBytes() );
    mAllocLargeVarLenArr( float, vals, def.ns_ );

    PtrMan<TrcHeader> thdr = def.getTrace( strm, buf, vals );
    if ( !thdr )
	return;
    while ( !thdr->isusable )
    {
	thdr = def.getTrace( strm, buf, vals );
	if ( !thdr )
	    return;
    }

    usable_ = true;
    SEGY::OffsetCalculator offscalc;
    offscalc.type_ = def.psoffssrc_; offscalc.def_ = def.psoffsdef_;
    offscalc.is2d_ = is2d; offscalc.coordscale_ = def.coordscale_;

    SeisTrcInfo ti;
    thdr->fill( ti, def.coordscale_ );
    offscalc.setOffset( ti, *thdr );

    inls_.start = inls_.stop = ti.binid.inl();
    crls_.start = crls_.stop = ti.binid.crl();
    trcnrs_.start = trcnrs_.stop = ti.nr;
    xrg_.start = xrg_.stop = ti.coord.x;
    yrg_.start = yrg_.stop = ti.coord.y;
    refnrs_.start = refnrs_.stop = ti.refnr;
    offsrg_.start = offsrg_.stop = ti.offset;
    mAdd2Dtector( dtctr, ti );
    addValues( cs, vals, def.ns_ );

    if ( full )
    {
	FullUIScanner fscnnr( *this, strm, def, buf, vals, cs, offscalc, dtctr);
	uiTaskRunner tr( uiparent );
	tr.execute( fscnnr );
	return;
    }

    addTraces( strm, 1, is2d, buf, vals, def, cs, offscalc, dtctr );
    if ( !ismulti )
	addTraces( strm, nrtrcs_/2, is2d, buf, vals, def, cs, offscalc, dtctr );

    addTraces( strm, nrtrcs_-1, is2d, buf, vals, def, cs, offscalc, dtctr,true);

    if ( dtctr )
	dtctr->finish();
}


void SEGY::ScanInfo::addTraces( od_istream& strm, int trcidx, bool is2d,
				char* buf, float* vals,
				const LoadDef& def, DataClipSampler& cs,
				const SEGY::OffsetCalculator& offscalc,
				PosInfo::Detector* dtctr, bool rev )
{
    if ( !def.goToTrace(strm,startpos_,trcidx) )
	return;

    const int minnrtrcs = 10; const int maxnrtrcs = 10000;
    SeisTrcInfo ti; int previnl, prevcrl, prevnr;
    bool haveinlchg = false, havecrlchg = false, havenrchg = false;
    for ( int itrc=0; itrc<maxnrtrcs; itrc++ )
    {
	PtrMan<TrcHeader> thdr = def.getTrace( strm, buf, vals );
	if ( !thdr )
	    break;
	if ( !thdr->isusable )
	    continue;

	thdr->fill( ti, def.coordscale_ );
	offscalc.setOffset( ti, *thdr );
	addPositions( ti, dtctr );
	addValues( cs, vals, def.ns_ );

	int curinl = ti.binid.inl();
	int curcrl = ti.binid.crl();
	int curnr = ti.nr;
	if ( itrc == 0 )
	    { previnl = curinl; prevcrl = curcrl; prevnr = curnr; }
	else
	{
	    if ( is2d )
		havenrchg = havenrchg || curnr != prevnr;
	    else
	    {
		haveinlchg = haveinlchg || curinl != previnl;
		havecrlchg = havecrlchg || curcrl != prevcrl;
	    }
	}

	const bool foundranges = is2d ? havenrchg : haveinlchg && havecrlchg;
	const bool founddata = cs.nrVals() > 1000;
	if ( foundranges && founddata && itrc >= minnrtrcs )
	    break;

	if ( rev && !def.goToTrace(strm,startpos_,nrtrcs_-trcidx-2) )
	    break;
    }
}


void SEGY::ScanInfo::addPositions( const SeisTrcInfo& ti,
				   PosInfo::Detector* dtctr )
{
    mAdd2Dtector( dtctr, ti );

    inls_.include( ti.binid.inl(), false );
    crls_.include( ti.binid.crl(), false );
    trcnrs_.include( ti.nr, false );
    xrg_.include( ti.coord.x, false );
    yrg_.include( ti.coord.y, false );
    refnrs_.include( ti.refnr, false );
    offsrg_.include( ti.offset, false );
}


void SEGY::ScanInfo::addValues( DataClipSampler& cs,
				  const float* vals, int ns )
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


void SEGY::ScanInfo::merge( const SEGY::ScanInfo& si )
{
    if ( si.nrtrcs_ < 1 )
	return;

    nrtrcs_ += si.nrtrcs_;
    inls_.include( si.inls_, false );
    crls_.include( si.crls_, false );
    trcnrs_.include( si.trcnrs_, false );
    xrg_.include( si.xrg_, false );
    yrg_.include( si.yrg_, false );
    refnrs_.include( si.refnrs_, false );
    offsrg_.include( si.offsrg_, false );
}
