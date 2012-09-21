/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seispacketinfo.h"
#include "seisread.h"
#include "seisselection.h"
#include "ptrman.h"
#include "sorting.h"
#include "flatposdata.h"
#include "survinfo.h"
#include "iopar.h"
#include "cubesampling.h"
#include "bufstringset.h"
#include "strmprov.h"
#include <iostream>


void SeisTrcBuf::deepErase()
{
    ::deepErase(trcs_);
}


void SeisTrcBuf::insert( SeisTrc* t, int insidx )
{
    for ( int idx=insidx; idx<trcs_.size(); idx++ )
	t = trcs_.replace( idx, t );
    trcs_ += t;
}


void SeisTrcBuf::copyInto( SeisTrcBuf& buf ) const
{
    for ( int idx=0; idx<trcs_.size(); idx++ )
    {
	const SeisTrc* trc = trcs_[idx];
	buf.add( buf.owner_ ? new SeisTrc(*trc) : const_cast<SeisTrc*>(trc) );
    }
}


void SeisTrcBuf::fill( SeisPacketInfo& spi ) const
{
    const int sz = size();
    if ( sz < 1 ) return;
    const SeisTrc* trc = first();
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
    tb.trcs_.erase();
}


bool SeisTrcBuf::isSorted( bool ascending, SeisTrcInfo::Fld fld ) const
{
    const int sz = size();
    if ( sz < 2 ) return true;

    float prevval = (float) first()->info().getValue(fld);
    for ( int idx=1; idx<sz; idx++ )
    {
	float val = (float) get(idx)->info().getValue(fld);
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

    mAllocVarLenArr( int, idxs, sz );
    mAllocVarLenArr( float, vals, sz );
    const double offs = trcs_[0]->info().getValue( fld );
    for ( int idx=0; idx<sz; idx++ )
    {
	idxs[idx] = idx;
	vals[idx] = (float)(trcs_[idx]->info().getValue( fld ) - offs);
    }
    sort_coupled( (float*)vals, (int*)idxs, sz );
    ObjectSet<SeisTrc> tmp;
    for ( int idx=0; idx<sz; idx++ )
	tmp += trcs_[idxs[idx]];

    trcs_.erase();

    for ( int idx=0; idx<sz; idx++ )
	trcs_ += tmp[ascending ? idx : sz - idx - 1];
}


void SeisTrcBuf::enforceNrTrcs( int nrrequired, SeisTrcInfo::Fld fld,
				bool dostack )
{
    SeisTrc* prevtrc = first();
    if ( !prevtrc ) return;

    float prevval = (float) prevtrc->info().getValue( fld );
    int nrwithprevval = 1;
    for ( int idx=1; idx<=size(); idx++ )
    {
	SeisTrc* trc = idx==size() ? 0 : get(idx);
	float val = (float) (trc ? trc->info().getValue( fld ) : 0);

	if ( trc && mIsEqual(prevval,val,mDefEps) )
	{
	    nrwithprevval++;
	    if ( nrwithprevval > nrrequired )
	    {
		if ( dostack )
		{
		    float wt = nrrequired; wt /= nrwithprevval - 1;
		    SeisTrcPropChg( *get(idx-1) ).stack(*trc,false,wt);
		}
		remove(trc); idx--; delete trc;
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
    offs = first()->info().getValue( fld );
    for ( int idx=0; idx<sz; idx++ )
	ret[idx] = (float)(get(idx)->info().getValue( fld ) - offs);

    return ret;
}


void SeisTrcBuf::revert()
{
    int sz = trcs_.size();
    int hsz = sz / 2;
    for ( int idx=0; idx<hsz; idx++ )
	trcs_.swap( sz-idx-1, idx );
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
    if ( trcs_[tryidx] == trc ) return tryidx;

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
    BinID start = trcs_[0]->info().binid;
    BinID stop = trcs_[sz-1]->info().binid;
    if ( is2d )
    {
	start.inl = stop.inl = 0;
	start.crl = trcs_[0]->info().nr;
	stop.crl = trcs_[sz-1]->info().nr;
    }

    BinID dist( start.inl - stop.inl, start.crl - stop.crl );
    if ( !dist.inl && !dist.crl )
	return 0;

    int n1  = dist.inl ? start.inl : start.crl;
    int n2  = dist.inl ? stop.inl  : stop.crl;
    int pos = dist.inl ? bid.inl   : bid.crl;
 
    float fidx = ((sz-1.f) * (pos - n1)) / (n2-n1);
    int idx = mNINT32(fidx);
    if ( idx < 0 ) idx = 0;
    if ( idx >= sz ) idx = sz-1;
    return idx;
}


bool SeisTrcBuf::dump( const char* fnm, bool is2d, bool isps, int icomp ) const
{
    if ( isEmpty() ) return false;

    StreamData sd = StreamProvider( fnm ).makeOStream();
    if ( !sd.usable() ) return false;
    std::ostream& strm = *sd.ostrm;

    const SeisTrc& trc0 = *first();
    strm << trc0.info().sampling.start
	 << ' ' << trc0.info().sampling.step * SI().zDomain().userFactor()
	 << ' ' << trc0.size();

    for ( int itrc=0; itrc<size(); itrc++ )
    {
	strm << '\n';
	const SeisTrc& trc = *get( itrc );
	if ( !is2d )
	    strm << trc.info().binid.inl << ' ' << trc.info().binid.crl;
	else
	{
	    BufferString postxt;
	    postxt += trc.info().nr; postxt += " ";
	    postxt += trc.info().coord.x; postxt += " ";
	    postxt += trc.info().coord.y;
	    strm << postxt;
	}
	if ( isps )
	    strm << ' ' << trc.info().offset;

	for ( int isamp=0; isamp<trc.size(); isamp++ )
	    strm << ' ' << trc.get( isamp, icomp );
    }

    sd.close();
    return true;
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

    const SeisTrc* trc = buf_.first();
    return trc ? trc->size() : 0;
}

// Mandatory functions ... why?
bool setSize( int, int ) { return false; }
// Are these really necessary?
od_uint64 getMemPos( const int* ) const { return 0; }
bool validPos( const int* pos ) const { return Array2DInfo::validPos(pos); }
od_uint64 getMemPos( int ) const { return 0; }
bool validPos( int ) const { return false; }
od_uint64 getMemPos( int, int ) const { return 0; }
bool validPos( int p0, int p1 ) const { return Array2DInfo::validPos(p0,p1); }

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


SeisTrcBufDataPack::SeisTrcBufDataPack( SeisTrcBuf* tbuf,
					Seis::GeomType gt,
					SeisTrcInfo::Fld fld,
					const char* cat, int icomp )
    : FlatDataPack( cat )
    , gt_(gt)
    , posfld_(fld)
{
    setBuffer( tbuf, gt, fld, icomp, true );
}


SeisTrcBufDataPack::SeisTrcBufDataPack( const SeisTrcBuf& tbuf,
					Seis::GeomType gt,
					SeisTrcInfo::Fld fld,
					const char* cat, int icomp )
    : FlatDataPack( cat )
    , gt_(gt)
    , posfld_(fld)
{
    setBuffer( const_cast<SeisTrcBuf*>(&tbuf), gt, fld, icomp, false );
}


SeisTrcBufDataPack::SeisTrcBufDataPack( const SeisTrcBufDataPack& b )
    : FlatDataPack( b.category(), 0 )
{
    const bool bufisours =
		b.arr2d_ && ((SeisTrcBufArray2D*)b.arr2d_)->bufIsMine();
    SeisTrcBuf* buf = const_cast<SeisTrcBuf*>( &b.trcBuf() );
    if ( bufisours )
	buf = buf->clone();
    setBuffer( buf, b.gt_, b.posfld_, b.trcBufArr2D().getComp(), bufisours );
    setName( b.name() );
}
    

void SeisTrcBufDataPack::setBuffer( SeisTrcBuf* tbuf, Seis::GeomType gt,
				    SeisTrcInfo::Fld fld, int icomp, bool mine )
{
    delete arr2d_; arr2d_ = 0;
    if ( !tbuf )
	return;

    arr2d_ = new SeisTrcBufArray2D( *tbuf, mine, icomp );
    const int tbufsz = tbuf->size();
    posfld_ = fld;
    gt_ = gt;

    if ( tbufsz<1 ) return;

    SeisTrcInfo::getAxisCandidates( gt, flds_ );

    FlatPosData& pd = posData();
    double ofv; float* hdrvals = tbuf->getHdrVals( posfld_, ofv );
    pd.setX1Pos( hdrvals, tbufsz, ofv );
    SeisPacketInfo pinf; tbuf->fill( pinf );
    StepInterval<double> zrg; assign( zrg, pinf.zrg );
    pd.setRange( false, zrg );
}



void SeisTrcBufDataPack::getAltDim0Keys( BufferStringSet& bss ) const
{
    for ( int idx=0; idx<flds_.size(); idx++ )
	bss.add( SeisTrcInfo::getFldString(flds_[idx]) );
}



double SeisTrcBufDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    const SeisTrcBuf& buf = trcBuf();
    return i0 < 0 || i0 >= buf.size() || ikey >= flds_.size()
	 ? FlatDataPack::getAltDim0Value( ikey, i0 )
	 : buf.get(i0)->info().getValue( flds_[ikey] );
}


void SeisTrcBufDataPack::getAuxInfo( int itrc, int isamp, IOPar& iop ) const
{
    const SeisTrcBuf& buf = trcBuf();
    const int bufsz = buf.size();
    if ( itrc >= bufsz || itrc < 0 ) return;

    const SeisTrc* trc = buf.get( itrc );
    trc->info().getInterestingFlds( gt_, iop );

    float z = trc->info().samplePos(isamp);
    if ( SI().zIsTime() ) z *= 1000;
    int z100 = mNINT32(z*100); z = z100 / 100;
    iop.set( "Z", z );
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


bool SeisTrcBufDataPack::getCubeSampling( CubeSampling& cs ) const
{
    const SeisTrcBuf& buf = trcBuf();
    if ( buf.isEmpty() )
	return false;

    cs.hrg.start.inl = cs.hrg.stop.inl = buf.first()->info().binid.inl;
    cs.hrg.start.crl = cs.hrg.stop.crl = buf.first()->info().binid.crl;
    cs.hrg.step.inl = SI().inlStep();
    cs.hrg.step.crl = SI().crlStep();

    for ( int idx=1; idx<buf.size(); idx++ )
	cs.hrg.include( buf.get( idx )->info().binid );

    cs.zrg.setFrom( posData().range(false) );

    return true;
}


const char* SeisTrcBufDataPack::dimName( bool dim0 ) const
{
    return dim0 ? SeisTrcInfo::getFldString(posfld_) : "Z";
}


SeisBufReader::SeisBufReader( SeisTrcReader& rdr, SeisTrcBuf& buf )
    : Executor("Collecting traces")
    , rdr_(rdr)
    , buf_(buf)
    , totnr_(-1)
    , msg_("Reading traces")
{
    if ( rdr.selData() && !rdr.selData()->isAll() )
	totnr_ = rdr.selData()->expectedNrTraces( rdr.is2D() );
}


int SeisBufReader::nextStep()
{
    SeisTrc* newtrc = new SeisTrc;

    int res = rdr_.get( newtrc->info() );
    if ( res > 1 ) return Executor::MoreToDo();
    if ( res == 0 ) return Executor::Finished();

    if ( res < 0 || !rdr_.get(*newtrc) )
	{ msg_ = rdr_.errMsg(); return Executor::ErrorOccurred(); }

    buf_.add( newtrc );
    return Executor::MoreToDo();
}
