/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace informtaion
-*/

static const char* rcsID = "$Id: seisinfo.cc,v 1.4 2001-02-13 17:21:08 bert Exp $";

#include "seisinfo.h"
#include "seistrc.h"
#include "susegy.h"
#include "binidselimpl.h"
#include "survinfo.h"
#include <math.h>
#include <timeser.h>
#include <float.h>

FixedString<32>  SeisPacketInfo::defaultclient( getenv("dGB_SEGY_CLIENT") );
FixedString<32>  SeisPacketInfo::defaultcompany( getenv("dGB_SEGY_COMPANY") );
FixedString<180> SeisPacketInfo::defaultauxinfo( getenv("dGB_SEGY_AUXINFO") );
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
	0
};

const char* SeisPacketInfo::sBinIDs = "BinID range";


SeisPacketInfo::SeisPacketInfo()
	: range(*new BinIDRange)
{
    clear();
}


SeisPacketInfo::SeisPacketInfo( const SeisPacketInfo& spi )
	: range(*new BinIDRange)
{
    *this = spi;
}



void SeisPacketInfo::clear()
{
    client = defaultclient;
    company = defaultcompany;
    auxinfo = defaultauxinfo;
    nr = 0;
    range = SI().range();
}


SeisPacketInfo& SeisPacketInfo::operator=( const SeisPacketInfo& spi )
{
    client = spi.client;
    company = spi.company;
    auxinfo = spi.auxinfo;
    nr = spi.nr;
    range = spi.range;
    return *this;
}


SeisPacketInfo::~SeisPacketInfo()
{
    delete &range;
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


int SeisTrcInfo::nearestSample( float t, int soffs ) const
{
    float s = mIsUndefined(t) ? 0 : (t - sampling.start) / sampling.step;
    s -= soffs;
    return mNINT(s);
}


SampleGate SeisTrcInfo::sampleGate( const Interval<float>& tg, int soffs ) const
{
    static SampleGate sg;

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
	refpos = head->refpos;
	sampling.start = head->startpos;
	if ( mIS_ZERO(sampling.start) )
	    sampling.start = trc.delrt * .001;
    }
}
