/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seisbuf.cc,v 1.11 2002-11-21 17:10:09 bert Exp $";

#include "seisbuf.h"
#include "seisinfo.h"
#include "seiswrite.h"
#include "xfuncimpl.h"
#include "binidselimpl.h"
#include "ptrman.h"
#include "sorting.h"


void SeisTrcBuf::insert( SeisTrc* t, int insidx )
{
    for ( int idx=insidx; idx<trcs.size(); idx++ )
	t = trcs.replace( t, idx );
    trcs += t;
}


void SeisTrcBuf::fill( SeisTrcBuf& buf ) const
{
    for ( int idx=0; idx<trcs.size(); idx++ )
	buf.add( new SeisTrc( *trcs[idx] ) );
}


void SeisTrcBuf::fill( SeisPacketInfo& spi ) const
{
    const int sz = size();
    if ( sz < 1 ) return;
    spi.binidsampling.start = spi.binidsampling.stop = get(0)->info().binid;
    if ( sz < 2 ) return;

    bool doneinl = false, donecrl = false;
    BinID pbid = spi.binidsampling.start;
    for ( int idx=1; idx<sz; idx++ )
    {
	BinID bid( get(idx)->info().binid );
	spi.binidsampling.include( bid );
	if ( !doneinl && bid.inl != pbid.inl )
	    { spi.binidsampling.step.inl = bid.inl - pbid.inl; doneinl = true; }
	if ( !donecrl && bid.crl != pbid.crl )
	    { spi.binidsampling.step.crl = bid.crl - pbid.crl; donecrl = true; }
    }
}


void SeisTrcBuf::sort( int seisinf_attrnr, bool ascending )
{
    const int sz = size();
    if ( sz < 2 ) return;

    ArrPtrMan<int> idxs = new int [sz];
    ArrPtrMan<float> vals = new float [sz];
    for ( int idx=0; idx<sz; idx++ )
    {
	idxs[idx] = idx;
	vals[idx] = get(idx)->info().getAttr(seisinf_attrnr);
    }
    sort_coupled( (float*)vals, (int*)idxs, sz );
    ObjectSet<SeisTrc> tmp;
    for ( int idx=0; idx<sz; idx++ )
	tmp += get( idxs[idx] );

    erase();
    for ( int idx=0; idx<sz; idx++ )
	add( tmp[ascending ? idx : sz - idx - 1] );
}


void SeisTrcBuf::removeDuplicates( int seisinf_attrnr, bool destroy )
{
    const int sz = size();
    if ( sz < 2 ) return;

    float prevval = get(0)->info().getAttr(seisinf_attrnr);
    for ( int idx=1; idx<sz; idx++ )
    {
	SeisTrc* trc = get(idx);
	float val = trc->info().getAttr(seisinf_attrnr);
	if ( mIS_ZERO(prevval-val) )
	    { remove(trc); idx--; if ( destroy ) delete trc; }
	else
	    prevval = val;
    }
}


void SeisTrcBuf::transferData( FloatList& fl, int takeeach, int icomp ) const
{
    for ( int idx=0; idx<size(); idx+=takeeach )
    {
	const SeisTrc& trc = *get( idx );
	const int trcsz = trc.size( icomp );
	for ( int isamp=0; isamp<trcsz; isamp++ )
	    fl += trc.get( isamp, icomp );
    }
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


void SeisGather::getStack( SeisTrc& trc, const StepInterval<int>& stpi,
			   int icomp ) const
{
    const int nrcomp = trc.data().nrComponents();
    for ( int ic=0; ic<nrcomp; ic++ )
    {
	if ( icomp < 0 || ic == icomp )
	    trc.reSize(0,ic);
    }
    if ( size() < 1 ) return;

    StepInterval<int> si = getSI( stpi );
    const int sz = si.nrSteps() + 1;
    const SeisTrc* trc1 = get( si.start );
    if ( sz == 1 ) { trc = *trc1; return; }

    ArrPtrMan<const SeisTrc*> arr = new const SeisTrc*[sz];
    arr[0] = trc1;
    trc.info().sampling = trc1->info().sampling;

    for ( int ic=0; ic<nrcomp; ic++ )
    {
	if ( icomp >= 0 && ic != icomp )
	    continue;

	Interval<float> tmrg( trc1->samplePos(0,ic),
			      trc1->samplePos(trc1->size(ic)-1,ic) );

	for ( int idx=si.start+si.step; idx<=si.stop; idx+=si.step )
	{
	    int nr = (idx - si.start) / si.step;
	    arr[nr] = get( idx );
	    tmrg.include( arr[nr]->samplePos(0,ic) );
	    tmrg.include( arr[nr]->samplePos(arr[nr]->size(ic)-1,ic) );
	}
	int nrsamps = mNINT(tmrg.width() / trc1->info().sampling.step) + 1;
	if ( ic == 0 ) trc.info().sampling.start = tmrg.start;

	trc.setSampleOffset( trc.info().nearestSample(tmrg.start), ic );
	trc.reSize( nrsamps, ic );

	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
	    float t = trc.samplePos( isamp, ic );
	    float sumvals = 0;
	    int nrvals = 0;
	    for ( int idx=0; idx<sz; idx++ )
	    {
		if ( arr[idx]->dataPresent(t,ic) )
		{
		    sumvals += arr[idx]->getValue( t, ic );
		    nrvals++;
		}
	    }
	    trc.set( isamp, nrvals == 0 ? 0 : sumvals / nrvals, ic );
	}
    }
}


XFunction* SeisGather::getValues( float t, const StepInterval<int>& stpi,
				  int attrnr, int icomp ) const
{
    SegmentXFunction* xf = new SegmentXFunction;
    if ( size() < 1 ) return xf;
    StepInterval<int> si = getSI( stpi );

    for ( int idx=si.start; idx<=si.stop; idx+=si.step )
    {
	const SeisTrc* trc = get( idx );
	if ( trc->dataPresent(t,icomp) )
	    xf->setValue( trc->info().getAttr(attrnr), trc->getValue(t,icomp) );
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
	starter = writer.starter( *trcbuf.get(0) );
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
	int res = starter->doStep();
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
