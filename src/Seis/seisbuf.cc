/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seisbuf.cc,v 1.1.1.2 1999-09-16 09:35:10 arend Exp $";

#include "seisbuf.h"
#include "seisinfo.h"
#include "seiswrite.h"
#include "xfuncimpl.h"


void SeisTrcBuf::insert( SeisTrc* t, int insidx )
{
    for ( int idx=insidx; idx<trcs.size(); idx++ )
	t = trcs.replace( t, idx );
    trcs += t;
}


void SeisTrcBuf::fill( SeisTrcBuf& buf ) const
{
    for ( int idx=0; idx<trcs.size(); idx++ )
	buf.add( trcs[idx]->clone() );
}


void SeisTrcBuf::fill( SeisPacketInfo& spi ) const
{
    if ( !size() ) return;

    const SeisTrc* trc0 = get( 0 );
    spi.ns = trc0->size();
    spi.dt = trc0->info().dt;
    spi.starttime = trc0->info().starttime;

    spi.range.start = trc0->info().binid;
    spi.range.stop = trc0->info().binid;
    for ( int idx=1; idx<size(); idx++ )
	spi.range.include( get(idx)->info().binid );
}


void SeisTrcBuf::revert()
{
    int sz = trcs.size();
    int hsz = sz / 2;
    for ( int idx=0; idx<hsz; idx++ )
	trcs.replace( trcs.replace(trcs[sz-idx-1],idx), sz-idx-1 );
}


int SeisTrcBuf::find( const BinID& binid ) const
{
    int sz = size();
    int startidx = probableIdx( binid );
    int idx = startidx, pos = 0;
    while ( idx<sz && idx>=0 )
    {
	if ( ((SeisTrcBuf*)this)->get(idx)->info().binid == binid )
	    return idx;
	if ( pos < 0 ) pos = -pos;
	else	       pos = -pos-1;
	idx = startidx + pos;
	if ( idx < 0 ) { pos = -pos; idx = startidx + pos; }
	else if ( idx >= sz ) { pos = -pos-1; idx = startidx + pos; }
    }
    return -1;
}


int SeisTrcBuf::find( SeisTrc* trc ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ((SeisTrcBuf*)this)->get(idx) == trc )
	    return idx;
    }
    return -1;
}


int SeisTrcBuf::probableIdx( const BinID& bid ) const
{
    int sz = size(); if ( sz < 2 ) return 0;
    const BinID& start = trcs[0]->info().binid;
    const BinID& stop = trcs[sz-1]->info().binid;

    BinID dist( start.inl - stop.inl, start.crl - stop.crl );
    if ( !dist.inl && !dist.crl )
	return 0;

    int n1  = dist.inl ? start.inl : start.crl;
    int n2  = dist.inl ? stop.inl  : stop.crl;
    int pos = dist.inl ? bid.inl   : bid.crl;
 
    float fidx = ((sz-1.) * (pos - n1)) / (n2-n1);
    int idx = mNINT(fidx);
    if ( idx < 0 ) idx = 0;
    if ( idx >= sz ) idx = sz-1;
    return idx;
}


StepInterval<int> SeisGather::getSI( const StepInterval<int>& stpi ) const
{
    const int sz = size();
    StepInterval<int> si( stpi );
    si.sort();
    if ( si.stop >= sz ) si.stop = sz - 1;
    if ( si.start >= sz ) si.start = sz - 1;
    return si;
}


void SeisGather::getStack( SeisTrc& trc, const StepInterval<int>& stpi ) const
{
    if ( size() < 1 ) return;

    StepInterval<int> si = getSI( stpi );
    const int sz = si.nrSteps();
    trc.reSize(0);
    if ( sz < 1 ) return;

    const SeisTrc* trc1 = get( si.start );
    if ( sz == 1 )
    {
	trc.copyData( *trc1 );
	trc.info().mute_time = trc1->info().mute_time;
	return;
    }

    const SeisTrc* arr[sz];
    arr[0] = trc1;
    Interval<float> tmrg( trc1->info().sampleTime(0),
			  trc1->info().sampleTime(trc1->size()-1) );
    for ( int idx=si.start+si.step; idx<=si.stop; idx+=si.step )
    {
	int nr = (idx - si.start) / si.step;
	arr[nr] = get( idx );
	tmrg.include( arr[nr]->info().sampleTime(0) );
	tmrg.include( arr[nr]->info().sampleTime(arr[nr]->size()-1) );
    }
    int nrsamps = mNINT(tmrg.width() / (trc1->info().dt*1.e-6)) + 1;

    trc.reSize( nrsamps );
    trc.info().starttime = tmrg.start;
    trc.info().dt = trc1->info().dt;
    for ( int isamp=0; isamp<nrsamps; isamp++ )
    {
	float t = trc.info().sampleTime( isamp );
	float sumvals = 0;
	int nrvals = 0;
	for ( int idx=0; idx<sz; idx++ )
	{
	    if ( arr[idx]->dataPresent(t) )
	    {
		sumvals += arr[idx]->getValue( t );
		nrvals++;
	    }
	}
	trc.set( isamp, nrvals == 0 ? 0 : sumvals / nrvals );
    }
}


XFunction* SeisGather::getValues( float t, const StepInterval<int>& stpi,
				  int attrnr ) const
{
    SegmentXFunction* xf = new SegmentXFunction;
    if ( size() < 1 ) return xf;
    StepInterval<int> si = getSI( stpi );

    for ( int idx=si.start; idx<=si.stop; idx+=si.step )
    {
	const SeisTrc* trc = get( idx );
	if ( trc->dataPresent(t) )
	    xf->setValue( trc->info().getAttr(attrnr), trc->getValue(t) );
    }

    return xf;
}


SeisTrcBufWriter::SeisTrcBufWriter( const SeisTrcBuf& b, SeisTrcWriter& w )
	: Executor("Trace storage")
	, trcbuf(b)
	, writer(w)
	, starter(0)
	, nrdone(0)
{
    if ( trcbuf.size() )
	starter = writer.starter();
}


SeisTrcBufWriter::~SeisTrcBufWriter()
{
    delete starter;
}

const char* SeisTrcBufWriter::message() const
{
    return starter ? starter->message() : "Writing traces";
}
const char* SeisTrcBufWriter::nrDoneText() const
{
    return starter ? starter->nrDoneText() : "Traces written";
}
int SeisTrcBufWriter::totalNr() const
{
    return starter ? starter->totalNr() : trcbuf.size();
}
int SeisTrcBufWriter::nrDone() const
{
    return starter ? starter->nrDone() : nrdone;
}


int SeisTrcBufWriter::nextStep()
{
    if ( starter )
    {
	int res = starter->nextStep();
	if ( res ) return res;
	delete starter; starter = 0;
	return 1;
    }

    Interval<int> nrs( nrdone, nrdone+9 );
    if ( nrs.start >= trcbuf.size() ) return 0;
    if ( nrs.stop >= trcbuf.size() ) nrs.stop = trcbuf.size() - 1;
    nrdone = nrs.stop + 1;
    
    for ( int idx=nrs.start; idx<=nrs.stop; idx++ )
	writer.put( *trcbuf.get(idx) );

    return nrdone >= trcbuf.size() ? 0 : 1;
}
