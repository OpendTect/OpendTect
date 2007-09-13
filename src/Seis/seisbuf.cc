/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seisbuf.cc,v 1.33 2007-09-13 19:38:39 cvsnanne Exp $";

#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seistrcsel.h"
#include "seistrc.h"
#include "ptrman.h"
#include "sorting.h"
#include "flatposdata.h"
#include "survinfo.h"
#include "iopar.h"


void SeisTrcBuf::deepErase()
{
    ::deepErase(trcs);
}


void SeisTrcBuf::insert( SeisTrc* t, int insidx )
{
    for ( int idx=insidx; idx<trcs.size(); idx++ )
	t = trcs.replace( idx, t );
    trcs += t;
}


void SeisTrcBuf::copyInto( SeisTrcBuf& buf ) const
{
    for ( int idx=0; idx<trcs.size(); idx++ )
    {
	const SeisTrc* trc = trcs[idx];
	buf.add( buf.owner_ ? new SeisTrc(*trc) : const_cast<SeisTrc*>(trc) );
    }
}


void SeisTrcBuf::fill( SeisPacketInfo& spi ) const
{
    const int sz = size();
    if ( sz < 1 ) return;
    const SeisTrc* trc = get( 0 );
    BinID bid = trc->info().binid;
    spi.inlrg.start = bid.inl; spi.inlrg.stop = bid.inl;
    spi.crlrg.start = bid.crl; spi.crlrg.stop = bid.crl;
    spi.zrg.start = trc->info().sampling.start;
    spi.zrg.step = trc->info().sampling.step;
    spi.zrg.stop = spi.zrg.start + spi.zrg.step * (trc->size() - 1);
    if ( sz < 2 ) return;

    bool doneinl = false, donecrl = false;
    const BinID pbid = bid;
    for ( int idx=1; idx<sz; idx++ )
    {
	trc = get( idx ); bid = trc->info().binid;
	spi.inlrg.include( bid.inl ); spi.crlrg.include( bid.crl );
	if ( !doneinl && bid.inl != pbid.inl )
	    { spi.inlrg.step = bid.inl - pbid.inl; doneinl = true; }
	if ( !donecrl && bid.crl != pbid.crl )
	    { spi.crlrg.step = bid.crl - pbid.crl; donecrl = true; }
    }
    if ( spi.inlrg.step < 0 ) spi.inlrg.step = -spi.inlrg.step;
    if ( spi.crlrg.step < 0 ) spi.crlrg.step = -spi.crlrg.step;
}


void SeisTrcBuf::add( SeisTrcBuf& tb )
{
    for ( int idx=0; idx<tb.size(); idx++ )
    {
	SeisTrc* trc = tb.get( idx );
	add( owner_ && trc ? new SeisTrc(*trc) : trc );
    }
}


void SeisTrcBuf::stealTracesFrom( SeisTrcBuf& tb )
{
    for ( int idx=0; idx<tb.size(); idx++ )
    {
	SeisTrc* trc = tb.get( idx );
	add( trc );
    }
    tb.trcs.erase();
}


bool SeisTrcBuf::isSorted( bool ascending, SeisTrcInfo::Fld fld ) const
{
    const int sz = size();
    if ( sz < 2 ) return true;

    float prevval = get(0)->getValue(fld);
    for ( int idx=1; idx<sz; idx++ )
    {
	float val = get(idx)->getValue(fld);
	float diff = val - prevval;
	if ( !mIsZero(diff,mDefEps) )
	{
	    if ( (ascending && diff < 0) || (!ascending && diff > 0) )
		return false;
	}
	prevval = val;
    }

    return true;
}


void SeisTrcBuf::sort( bool ascending, SeisTrcInfo::Fld fld )
{
    const int sz = size();
    if ( sz < 2 || isSorted(ascending,fld) )
	return;

    ArrPtrMan<int> idxs = new int [sz];
    ArrPtrMan<float> vals = new float [sz];
    const double offs = get(0)->getValue( fld );
    for ( int idx=0; idx<sz; idx++ )
    {
	idxs[idx] = idx;
	vals[idx] = (float)(get(idx)->getValue( fld ) - offs);
    }
    sort_coupled( (float*)vals, (int*)idxs, sz );
    ObjectSet<SeisTrc> tmp;
    for ( int idx=0; idx<sz; idx++ )
	tmp += get( idxs[idx] );

    erase();
    for ( int idx=0; idx<sz; idx++ )
	add( tmp[ascending ? idx : sz - idx - 1] );
}


void SeisTrcBuf::enforceNrTrcs( int nrrequired, SeisTrcInfo::Fld fld )
{
    SeisTrc* prevtrc = get(0);
    if ( !prevtrc ) return;

    float prevval = prevtrc->getValue( fld );
    int nrwithprevval = 1;
    for ( int idx=1; idx<=size(); idx++ )
    {
	SeisTrc* trc = idx==size() ? 0 : get(idx);
	float val = trc ? trc->getValue( fld ) : 0;

	if ( trc && mIsEqual(prevval,val,mDefEps) )
	{
	    nrwithprevval++;
	    if ( nrwithprevval > nrrequired )
	    {
		remove(trc); idx--; delete trc;
		nrwithprevval = nrrequired;
	    }
	}
	else
	{
	    for ( int inew=nrwithprevval; inew<nrrequired; inew++ )
	    {
		SeisTrc* newtrc = new SeisTrc(*prevtrc);
		newtrc->data().zero();
		add( newtrc );
		idx++;
	    }
	    nrwithprevval = 1;
	}

	prevval = val;
    }
}


float* SeisTrcBuf::getHdrVals( SeisTrcInfo::Fld fld, double& offs )
{
    const int sz = size();
    if ( sz < 1 ) return 0;

    float* ret = new float [sz];
    offs = get(0)->getValue( fld );
    for ( int idx=0; idx<sz; idx++ )
	ret[idx] = (float)(get(idx)->getValue( fld ) - offs);

    return ret;
}


void SeisTrcBuf::revert()
{
    int sz = trcs.size();
    int hsz = sz / 2;
    for ( int idx=0; idx<hsz; idx++ )
	trcs.swap( sz-idx-1, idx );
}


int SeisTrcBuf::find( const BinID& binid, bool is2d ) const
{
    int sz = size();
    int startidx = probableIdx( binid, is2d );
    int idx = startidx, pos = 0;
    while ( idx<sz && idx>=0 )
    {
	if ( !is2d && ((SeisTrcBuf*)this)->get(idx)->info().binid == binid )
	    return idx;
	else if ( is2d && ((SeisTrcBuf*)this)->get(idx)->info().nr == binid.crl)
	    return idx;
	if ( pos < 0 ) pos = -pos;
	else	       pos = -pos-1;
	idx = startidx + pos;
	if ( idx < 0 ) { pos = -pos; idx = startidx + pos; }
	else if ( idx >= sz ) { pos = -pos-1; idx = startidx + pos; }
    }
    return -1;
}


int SeisTrcBuf::find( const SeisTrc* trc, bool is2d ) const
{
    if ( !trc ) return -1;

    int tryidx = probableIdx( trc->info().binid, is2d );
    if ( trcs[tryidx] == trc ) return tryidx;

    // Bugger. brute force then
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ((SeisTrcBuf*)this)->get(idx) == trc )
	    return idx;
    }

    return -1;
}


int SeisTrcBuf::probableIdx( const BinID& bid, bool is2d ) const
{
    int sz = size(); if ( sz < 2 ) return 0;
    BinID start = trcs[0]->info().binid;
    BinID stop = trcs[sz-1]->info().binid;
    if ( is2d )
    {
	start.inl = stop.inl = 0;
	start.crl = trcs[0]->info().nr;
	stop.crl = trcs[sz-1]->info().nr;
    }

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


struct SeisTrcBufArray2DInfo : public Array2DInfo
{
SeisTrcBufArray2DInfo( const SeisTrcBuf& tb )
    : buf_(tb)
{
}

SeisTrcBufArray2DInfo( const SeisTrcBufArray2DInfo& ai )
    : buf_(ai.buf_)
{
}

ArrayNDInfo* clone() const
{
    return new SeisTrcBufArray2DInfo(buf_);
}

int getSize( int dim ) const
{
    if ( dim == 0 )
	return buf_.size();

    const SeisTrc* trc = buf_.get( 0 );
    return trc ? trc->size() : 0;
}

// Mandatory functions ... why?
bool setSize( int, int ) { return false; }
// Are these really necessary?
od_uint64 getMemPos( const int* ) const { return 0; }
bool validPos( const int* ) const { return false; }
od_uint64 getMemPos( int ) const { return 0; }
bool validPos( int ) const { return false; }
od_uint64 getMemPos( int, int ) const { return 0; }
bool validPos( int, int ) const { return false; }

    const SeisTrcBuf&	buf_;

};


SeisTrcBufArray2D::SeisTrcBufArray2D( SeisTrcBuf& tbuf, bool mine, int icomp )
    : buf_(tbuf)
    , info_(*new SeisTrcBufArray2DInfo(tbuf))
    , comp_(icomp)
    , bufmine_(mine)
{
}


SeisTrcBufArray2D::SeisTrcBufArray2D( const SeisTrcBuf& tbuf, int icomp )
    : buf_(const_cast<SeisTrcBuf&>(tbuf))
    , info_(*new SeisTrcBufArray2DInfo(tbuf))
    , comp_(icomp)
    , bufmine_(false)
{
}


SeisTrcBufArray2D::~SeisTrcBufArray2D()
{
    if ( bufmine_ )
	delete &buf_;
    delete &info_;
}


float SeisTrcBufArray2D::get( int itrc, int isamp ) const
{
    const SeisTrc* trc = buf_.get( itrc );
    return trc && trc->size() > isamp ? trc->get(isamp,comp_) : mUdf(float);
}


void SeisTrcBufArray2D::set( int itrc, int isamp, float val )
{
    SeisTrc* trc = buf_.get( itrc );
    if ( trc && trc->size() > isamp )
	trc->set( isamp, val, comp_ );
}


void SeisTrcBufArray2D::getAuxInfo( Seis::GeomType gt, int itrc,
				    IOPar& iop ) const
{
    SeisTrc* trc = buf_.get( itrc );
    if ( trc )
	trc->info().getInterestingFlds( gt, iop );
}


SeisTrcBufDataPack::SeisTrcBufDataPack( SeisTrcBuf& tbuf,
					Seis::GeomType gt,
					SeisTrcInfo::Fld fld,
					const char* cat, int icomp )
    : FlatDataPack(cat,new SeisTrcBufArray2D(tbuf,true,icomp))
    , gt_(gt)
    , posfld_(fld)
{
    const int tbufsz = tbuf.size();
    if ( tbufsz < 1 ) return;
    SeisTrcInfo::getAxisCandidates( gt, flds_ );

    FlatPosData& pd = posData();
    double ofv; float* hdrvals = tbuf.getHdrVals( posfld_, ofv );
    pd.setX1Pos( hdrvals, tbufsz, ofv );
    SeisPacketInfo pinf; tbuf.fill( pinf );
    StepInterval<double> zrg; assign( zrg, pinf.zrg );
    zrg.scale( SI().zFactor() );
    pd.setRange( false, zrg );
}


void SeisTrcBufDataPack::getAltDim0Keys( BufferStringSet& bss ) const
{
    for ( int idx=0; idx<flds_.size(); idx++ )
	bss.add( eString(SeisTrcInfo::Fld,flds_[idx]) );
}



double SeisTrcBufDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    const SeisTrcBuf& buf = trcBuf();
    return i0 < 0 || i0 >= buf.size() || ikey >= flds_.size()
	 ? FlatDataPack::getAltDim0Value( ikey, i0 )
	 : buf.get(i0)->getValue( flds_[ikey] );
}


void SeisTrcBufDataPack::getAuxInfo( int itrc, int isamp, IOPar& iop ) const
{
    const SeisTrcBuf& buf = trcBuf();
    const int bufsz = buf.size();
    if ( itrc >= bufsz || itrc < 0 ) return;

    const SeisTrc* trc = buf.get( itrc );
    trc->info().getInterestingFlds( gt_, iop );
    iop.set( "Z-Coord", trc->info().samplePos(isamp) );
}


Coord3 SeisTrcBufDataPack::getCoord( int itrc, int isamp ) const
{
    const SeisTrcBuf& buf = trcBuf();
    if ( buf.isEmpty() ) return Coord3();
    if ( itrc >= buf.size() ) itrc = buf.size() - 1;
    if ( itrc < 0 ) itrc = 0;
    const SeisTrc* trc = buf.get( itrc );
    return Coord3( trc->info().coord, trc->info().samplePos(isamp) );
}


const char* SeisTrcBufDataPack::dimName( bool dim0 ) const
{
    return dim0 ? eString(SeisTrcInfo::Fld,posfld_) : "Z";
}
