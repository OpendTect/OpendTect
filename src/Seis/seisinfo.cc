/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace informtaion
-*/

static const char* rcsID = "$Id: seisinfo.cc,v 1.1.1.1 1999-09-03 10:11:27 dgb Exp $";

#include "seisinfo.h"
#include "susegy.h"
#include <math.h>
#include <timeser.h>
#include <float.h>

DefineEnumNames(Seis,WaveType,0,"Wave type")
{
	"P",
	"Sh",
	"Sv",
	0
};

DefineEnumNames(Seis,DataType,0,"Data type")
{
	"Amplitude",
	"Dip",
	"Frequency",
	"Phase",
	"AVO Gradient",
	0
};

const char* SeisTrcInfo::attrnames[] = {
	"Trace number",
	"Pick time",
	"Reference time",
	"X-coordinate",
	"Y-coordinate",
	"In-line",
	"Cross-line",
	"Offset",
	0
};

const char* SeisPacketInfo::sBinIDs = "BinID range";
const char* SeisPacketInfo::sNrTrcs = "Nr of traces";
const char* SeisTrcInfo::sSampIntv = "Sample interval (ms)";
const char* SeisTrcInfo::sStartTime = "Start time (ms)";
const char* SeisTrcInfo::sNrSamples = "Nr of samples";


void SeisPacketInfo::fillEmpty( const SeisPacketInfo& tpi )
{
    if ( !client[0] ) client = tpi.client;
    if ( !company[0] ) company = tpi.company;
    if ( !nr ) nr = tpi.nr;
    if ( !ns ) ns = tpi.ns;
    if ( !dt ) dt = tpi.dt;
    if ( starttime < 1e-30 ) starttime = tpi.starttime;
    else		     starttime = 0;
    if ( !range.start.inl && !range.start.crl
      && !range.stop.inl && !range.stop.crl )
	range = tpi.range;
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
    case 2:	return reftime;
    case 3:	return coord.x;
    case 4:	return coord.y;
    case 5:	return binid.inl;
    case 6:	return binid.crl;
    case 7:	return offset;
    default:	return nr;
    }
}


bool SeisTrcInfo::dataPresent( float t, int trcsz ) const
{
    if ( t < starttime || t > sampleTime(trcsz) )
	return NO;
    if ( mIsUndefined(mute_time) || t > mute_time )
	return YES;
    return NO;
}


int SeisTrcInfo::nearestSample( float t ) const
{
    float s = mIsUndefined(t) ? 0 : (t-starttime)*1e6/dt;
    return mNINT(s);
}


SampleGate SeisTrcInfo::sampleGate( const TimeGate& tg ) const
{
    static SampleGate sg;

    sg.start = sg.stop = 0;
    if ( mIsUndefined(tg.start) && mIsUndefined(tg.stop) )
	return sg;

    Interval<float> vals(
		mIsUndefined(tg.start) ? 0 : (tg.start-starttime)*1e6/dt,
		mIsUndefined(tg.stop) ? 0 : (tg.stop-starttime)*1e6/dt );

    if ( vals.start < vals.stop )
    {
	sg.start = (int)floor(vals.start);
	sg.stop =  (int)ceil(vals.stop);
    }
    else
    {
	sg.start = (int)ceil(vals.start);
	sg.stop =  (int)floor(vals.stop);
    }

    if ( sg.start < 0 ) sg.start = 0;
    if ( sg.stop < 0 ) sg.stop = 0;

    return sg;
}


float SeisTrcInfo::sampleTime( int idx ) const
{
    return starttime + idx*dt*1e-6;
}


void SeisTrcInfo::gettr( SUsegy& trc ) const
{
    trc.tracl = trc.fldr = trc.tracf = nr;
    trc.trid = 1;
    trc.dt = dt;
    trc.delrt = (short)mNINT(starttime * 1000);

    SULikeHeader* head = (SULikeHeader*)(&trc);
    head->indic = 123456;
    head->inl = binid.inl;
    head->crl = binid.crl;
    head->inl2 = binid2.inl;
    head->crl2 = binid2.crl;
    head->pick = pick;
    head->pick2 = pick2;
    head->reftime = reftime;
    head->starttime = starttime;
    head->offset = offset;
}


void SeisTrcInfo::puttr( SUsegy& trc )
{
    nr = trc.tracl;
    dt = trc.dt;
    starttime = trc.delrt / 1000.;
    SULikeHeader* head = (SULikeHeader*)(&trc);
    if ( head->indic == 123456 )
    {
	binid.inl = head->inl;
	binid.crl = head->crl;
	binid2.inl = head->inl2;
	binid2.crl = head->crl2;
	pick = head->pick;
	pick2 = head->pick2;
	offset = head->offset;
	reftime = head->reftime;
	starttime = head->starttime;
	if ( starttime == 0 )
	    starttime = trc.delrt * .001;
    }
}
