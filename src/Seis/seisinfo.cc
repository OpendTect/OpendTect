/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace informtaion
-*/

static const char* rcsID = "$Id: seisinfo.cc,v 1.16 2003-10-19 13:53:08 bert Exp $";

#include "seisinfo.h"
#include "seistrc.h"
#include "susegy.h"
#include "posauxinfo.h"
#include "binidselimpl.h"
#include "survinfo.h"
#include "strmprov.h"
#include "strmdata.h"
#include "filegen.h"
#include "iopar.h"
#include <math.h>
#include <timeser.h>
#include <float.h>

static BufferString getUsrInfo()
{
    BufferString bs;
    const char* envstr = getenv( "DTECT_SEIS_USRINFO" );
    if ( !envstr || !File_exists(envstr) ) return bs;

    StreamData sd = StreamProvider(envstr).makeIStream();
    if ( sd.usable() )
    {
	char buf[1024];
	while ( *sd.istrm )
	{
	    sd.istrm->getline( buf, 1024 );
	    if ( *(const char*)bs ) bs += "\n";
	    bs += buf;
	}
    }

    return bs;
}

BufferString SeisPacketInfo::defaultusrinfo = getUsrInfo();

const char* SeisTrcInfo::sSamplingInfo = "Sampling information";
const char* SeisTrcInfo::sNrSamples = "Nr of samples";


DefineEnumNames(Seis,WaveType,0,"Wave type")
{
	"P",
	"Sh",
	"Sv",
	"Other",
	0
};

DefineEnumNames(Seis,DataType,0,"Data type")
{
	"Amplitude",
	"Dip",
	"Frequency",
	"Phase",
	"AVO Gradient",
	"Azimuth",
	"Other",
	0
};
 
DefineEnumNames(Seis::Event,Type,0,"Event type")
{
	"None",
	"Peak or trough",
	"Peak (Max)",
	"Trough (Min)",
	"Zero crossing",
	"Zero crossing - to +",
	"Zero crossing + to -",
	"Largest peak",
	"Largest trough",
	0
};


const char* SeisTrcInfo::attrnames[] = {
	"Trace number",
	"Pick position",
	"Reference position",
	"X-coordinate",
	"Y-coordinate",
	"In-line",
	"Cross-line",
	"Offset",
	"Azimuth",
	0
};

const char* SeisPacketInfo::sBinIDs = "BinID range";


SeisPacketInfo::SeisPacketInfo()
	: binidsampling(*new BinIDSampler)
{
    clear();
}


SeisPacketInfo::SeisPacketInfo( const SeisPacketInfo& spi )
	: binidsampling(*new BinIDSampler)
{
    *this = spi;
}



void SeisPacketInfo::clear()
{
    usrinfo = defaultusrinfo;
    nr = 0;
    binidsampling.start = SI().range(false).start;
    binidsampling.stop = SI().range(false).stop;
    binidsampling.step = BinID( SI().inlStep(), SI().crlStep() );
}


SeisPacketInfo& SeisPacketInfo::operator=( const SeisPacketInfo& spi )
{
    stdinfo = spi.stdinfo;
    usrinfo = spi.usrinfo;
    nr = spi.nr;
    binidsampling = spi.binidsampling;
    return *this;
}


SeisPacketInfo::~SeisPacketInfo()
{
    delete &binidsampling;
}


int SeisTrcInfo::attrNr( const char* nm )
{
    return getEnum( nm, attrnames, 0, 1 );
}


double SeisTrcInfo::getAttr( int nr ) const
{
    switch ( nr )
    {
    case 1:	return pick;
    case 2:	return refpos;
    case 3:	return coord.x;
    case 4:	return coord.y;
    case 5:	return binid.inl;
    case 6:	return binid.crl;
    case 7:	return offset;
    case 8:	return azimuth;
    default:	return nr;
    }
}


bool SeisTrcInfo::dataPresent( float t, int trcsz, int soffs ) const
{
    float realstartpos = sampling.atIndex( soffs );
    if ( t < realstartpos || t > samplePos(trcsz-1,soffs) )
	return false;
    if ( mIsUndefined(mute_pos) || t > mute_pos )
	return true;
    return false;
}


void SeisTrcInfo::usePar( const IOPar& iopar )
{
    iopar.get( attrnames[0], nr );
    iopar.get( attrnames[1], pick );
    iopar.get( attrnames[2], refpos );
    iopar.get( attrnames[3], coord.x );
    iopar.get( attrnames[4], coord.y );
    iopar.get( attrnames[5], binid.inl );
    iopar.get( attrnames[6], binid.crl );
    iopar.get( attrnames[7], offset );
    iopar.get( attrnames[8], azimuth );

    iopar.get( sSamplingInfo, sampling.start, sampling.step );
}

void SeisTrcInfo::fillPar( IOPar& iopar ) const
{
    iopar.set( attrnames[0], nr );
    iopar.set( attrnames[1], pick );
    iopar.set( attrnames[2], refpos );
    iopar.set( attrnames[3], coord.x );
    iopar.set( attrnames[4], coord.y );
    iopar.set( attrnames[5], binid.inl );
    iopar.set( attrnames[6], binid.crl );
    iopar.set( attrnames[7], offset );
    iopar.set( attrnames[8], azimuth );

    iopar.set( sSamplingInfo, sampling.start, sampling.step );
}


int SeisTrcInfo::nearestSample( float t, int soffs ) const
{
    float s = mIsUndefined(t) ? 0 : (t - sampling.start) / sampling.step;
    s -= soffs;
    return mNINT(s);
}


SampleGate SeisTrcInfo::sampleGate( const Interval<float>& tg, int soffs ) const
{
    SampleGate sg;

    sg.start = sg.stop = 0;
    if ( mIsUndefined(tg.start) && mIsUndefined(tg.stop) )
	return sg;

    Interval<float> vals(
	mIsUndefined(tg.start) ? 0 : (tg.start-sampling.start) / sampling.step,
	mIsUndefined(tg.stop) ? 0 : (tg.stop-sampling.start) / sampling.step );
    vals.shift( -soffs );

    if ( vals.start < vals.stop )
    {
	sg.start = (int)floor(vals.start+1e-3);
	sg.stop =  (int)ceil(vals.stop-1e-3);
    }
    else
    {
	sg.start =  (int)ceil(vals.start-1e-3);
	sg.stop = (int)floor(vals.stop+1e-3);
    }

    if ( sg.start < 0 ) sg.start = 0;
    if ( sg.stop < 0 ) sg.stop = 0;

    return sg;
}



void SeisTrcInfo::gettr( SUsegy& trc ) const
{
    trc.tracl = trc.fldr = trc.tracf = nr;
    trc.trid = 1;
    trc.dt = (int)(sampling.step*1e6 + .5);
    trc.delrt = (short)mNINT(sampling.start * 1000);

    SULikeHeader* head = (SULikeHeader*)(&trc);
    head->indic = 123456;
    head->inl = binid.inl;
    head->crl = binid.crl;
    head->pick = pick;
    head->refpos = refpos;
    head->startpos = sampling.start;
    head->offset = offset;
    head->azimuth = azimuth;
}


void SeisTrcInfo::puttr( const SUsegy& trc )
{
    nr = trc.tracl;
    sampling.step = trc.dt * 1e-6;
    sampling.start = trc.delrt * .001;
    SULikeHeader* head = (SULikeHeader*)(&trc);
    if ( head->indic == 123456 )
    {
	binid.inl = head->inl;
	binid.crl = head->crl;
	pick = head->pick;
	offset = head->offset;
	azimuth = head->azimuth;
	refpos = head->refpos;
	sampling.start = head->startpos;
	if ( mIS_ZERO(sampling.start) )
	    sampling.start = trc.delrt * .001;
    }
}


void SeisTrcInfo::putTo( PosAuxInfo& auxinf ) const
{
    auxinf.binid = binid;
    auxinf.startpos = sampling.start;
    auxinf.coord = coord;
    auxinf.offset = offset;
    auxinf.azimuth = azimuth;
    auxinf.pick = pick;
    auxinf.refpos = refpos;
}


void SeisTrcInfo::getFrom( const PosAuxInfo& auxinf )
{
    binid = auxinf.binid;
    sampling.start = auxinf.startpos;
    coord = auxinf.coord;
    offset = auxinf.offset;
    azimuth = auxinf.azimuth;
    pick = auxinf.pick;
    refpos = auxinf.refpos;
}
