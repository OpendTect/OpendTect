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


SEGY::uiScanDef::uiScanDef()
    : hdrdef_(0)
{
    reInit();
}


void SEGY::uiScanDef::reInit()
{
    revision_ = ns_ = -1;
    format_ = 5;
    hdrsswapped_ = dataswapped_ = false;
    coordscale_ = sampling_.start = sampling_.step = 1.0f;
    delete hdrdef_; hdrdef_ = new TrcHeaderDef;
}


SEGY::uiScanDef::~uiScanDef()
{
    delete hdrdef_;
}


int SEGY::uiScanDef::nrTracesIn( const od_istream& strm,
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


void SEGY::uiScanDef::goToTrace( od_istream& strm, od_stream_Pos startpos,
				 int trcidx ) const
{
    const int trcbytes = SegyTrcHeaderLength + traceDataBytes();
    startpos += trcidx * trcbytes;
    strm.setPosition( startpos );
}


SEGY::TrcHeader* SEGY::uiScanDef::getTrcHdr( od_istream& strm ) const
{
    char* thbuf = new char[ SegyTrcHeaderLength ];
    strm.getBin( thbuf, SegyTrcHeaderLength );
    if ( !strm.isOK() )
	return 0;

    SEGY::TrcHeader* th = new SEGY::TrcHeader(
			 (unsigned char*)thbuf, revision_==1, *hdrdef_, true );
    th->initRead();
    return th;
}


int SEGY::uiScanDef::bytesPerSample() const
{
    return SEGY::BinHeader::formatBytes( format_ );
}


int SEGY::uiScanDef::traceDataBytes() const
{
    return ns_ < 0 ? 0 : ns_ * bytesPerSample();
}


DataCharacteristics SEGY::uiScanDef::getDataChar() const
{
    return SEGY::BinHeader::getDataChar( format_, dataswapped_ );
}


bool SEGY::uiScanDef::getData( od_istream& strm, char* buf, float* vals ) const
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


SEGY::TrcHeader* SEGY::uiScanDef::getTrace( od_istream& strm,
					    char* buf, float* vals ) const
{
    TrcHeader* thdr = getTrcHdr( strm );
    if ( !thdr || !getData(strm,buf,vals) )
	{ delete thdr; return 0; }
    return thdr;
}



SEGY::uiScanData::uiScanData( const char* fnm )
    : filenm_(fnm)
{
    reInit();
}


void SEGY::uiScanData::reInit()
{
    usable_ = false;
    nrtrcs_ = 0;
    inls_ = StepInterval<int>( mUdf(int), 0, 1 );
    crls_ = StepInterval<int>( mUdf(int), 0, 1 );
    trcnrs_ = StepInterval<int>( mUdf(int), 0, 1 );
    xrg_ = Interval<double>( mUdf(double), 0. );
    yrg_ = Interval<double>( mUdf(double), 0. );
    refnrs_ = Interval<float>( mUdf(float), 0.f );
    offsrg_ = Interval<float>( mUdf(float), 0.f );
}


void SEGY::uiScanData::getFromSEGYBody( od_istream& strm, const uiScanDef& def,
					bool isfirst, DataClipSampler* cs )
{
    const od_istream::Pos startpos = strm.position();
    nrtrcs_ = def.nrTracesIn( strm, startpos );
    if ( !def.isValid() )
	return;

    ArrPtrMan<char> buf = new char[ def.traceDataBytes() ];
    ArrPtrMan<float> vals = new float[ def.ns_ ];

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
    SeisTrcInfo ti; thdr->fill( ti, def.coordscale_ );
    inls_.start = inls_.stop = ti.binid.inl();
    crls_.start = crls_.stop = ti.binid.crl();
    trcnrs_.start = trcnrs_.stop = ti.nr;
    xrg_.start = xrg_.stop = ti.coord.x;
    yrg_.start = yrg_.stop = ti.coord.y;
    refnrs_.start = refnrs_.stop = ti.refnr;
    offsrg_.start = offsrg_.stop = ti.offset;
    addValues( cs, vals, def.ns_ );

    if ( isfirst )
    {
	while ( inls_.start == inls_.stop )
	{
	    if ( !addTrace(strm,true,buf,vals,def,cs) )
		break;
	}
    }

    def.goToTrace( strm, startpos, nrtrcs_ / 2 );
    addTrace( strm, false, buf, vals, def, cs );
    def.goToTrace( strm, startpos, nrtrcs_ - 1 );
    addTrace( strm, false, buf, vals, def, cs );
}


bool SEGY::uiScanData::addTrace( od_istream& strm, bool wstep,
				 char* buf, float* vals,
				 const uiScanDef& def, DataClipSampler* cs )
{
    PtrMan<TrcHeader> thdr = def.getTrace( strm, buf, vals );
    if ( !thdr || !thdr->isusable )
	return false;

    SeisTrcInfo ti; thdr->fill( ti, def.coordscale_ );

    inls_.include( ti.binid.inl(), false );
    crls_.include( ti.binid.crl(), false );
    trcnrs_.include( ti.nr, false );
    xrg_.include( ti.coord.x, false );
    yrg_.include( ti.coord.y, false );
    refnrs_.include( ti.refnr, false );
    offsrg_.include( ti.offset, false );

    //TODO handle steps. Not easy.

    addValues( cs, vals, def.ns_ );

    return true;
}


void SEGY::uiScanData::addValues( DataClipSampler* cs,
				  const float* vals, int ns)
{
    if ( !cs )
	return;

    for ( int idx=0; idx<ns; idx++ )
    {
	if ( vals[idx] != 0.f )
	{
	    cs->add( vals, ns );
	    break;
	}
    }
}


void SEGY::uiScanData::merge( const SEGY::uiScanData& sd )
{
    //TODO implement
}
