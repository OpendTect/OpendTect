/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "seisimpps.h"
#include "seispsioprov.h"
#include "seiswrite.h"
#include "seispswrite.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "ioman.h"
#include "ioobj.h"
#include "debug.h"
#include <iostream>

#define mDBGmask 0x1110


class SeisPSImpLineBuf
{
public:

    				SeisPSImpLineBuf( int inl )
				    : inl_(inl)		{}
				~SeisPSImpLineBuf()	{ deepErase(gathers_); }

    void			add(SeisTrc*);

    const int			inl_;
    ObjectSet<SeisTrcBuf>	gathers_;

};


void SeisPSImpLineBuf::add( SeisTrc* trc )
{
    const int crl = trc->info().binid.crl;
    int bufidx = -1;
    for ( int idx=0; idx<gathers_.size(); idx++ )
    {
	SeisTrcBuf& tbuf = *gathers_[idx];
	const int bufcrl = tbuf.get(0)->info().binid.crl;
	if ( bufcrl == crl )
	    { tbuf.add( trc ); return; }
	else if ( bufcrl > crl )
	    { bufidx = idx; break; }
    }

    SeisTrcBuf* newbuf = new SeisTrcBuf( true );
    newbuf->add( trc );
    if ( bufidx == -1 )
	gathers_ += newbuf;
    else
	gathers_.insertAt( newbuf, bufidx );
}


SeisPSImpDataMgr::SeisPSImpDataMgr( const MultiID& pswrid )
    : wrid_(pswrid)
    , wrr_(0)
    , maxinloffs_(-1)
    , gathersize_(0)
{
}


SeisPSImpDataMgr::~SeisPSImpDataMgr()
{
    delete wrr_;
    deepErase( lines_ );
}


void SeisPSImpDataMgr::endReached()
{
    for ( int idx=0; idx<lines_.size(); idx++ )
	towrite_ += lines_[idx]->inl_;
}


void SeisPSImpDataMgr::add( SeisTrc* trc )
{
    const int inl = trc->info().binid.inl;
    int bufidx = -1;
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	SeisPSImpLineBuf& lbuf = *lines_[idx];
	if ( lbuf.inl_ == inl )
	    { lbuf.add( trc ); return; }
	else if ( lbuf.inl_ > inl )
	    { bufidx = idx; break; }
    }

    if ( DBG::isOn(mDBGmask) )
    {
	BufferString str( "Adding inl=" ); str += inl;
	str += ";\tNew nr lines="; str += lines_.size();
	DBG::message( str );
    }
    SeisPSImpLineBuf* newbuf = new SeisPSImpLineBuf( inl );
    newbuf->add( trc );
    if ( bufidx != -1 )
	lines_.insertAt( newbuf, bufidx );
    else
    {
	lines_ += newbuf;
	bufidx = lines_.size() - 1;
    }

    updateStatus( bufidx );
}


void SeisPSImpDataMgr::updateStatus( int bufidx )
{
    if ( maxinloffs_ < 0 ) return;

    const int nrlines = lines_.size();
    for ( int idx=0; idx<nrlines; idx++ )
    {
	const int inloffs = abs( lines_[idx]->inl_ - lines_[bufidx]->inl_ );
	if ( inloffs > maxinloffs_ )
	    towrite_ += lines_[idx]->inl_;
    }
}


bool SeisPSImpDataMgr::writeGather()
{
    if ( towrite_.isEmpty() ) return true;

    bool wrsampnms = false;
    if ( !wrr_ )
    {
	IOObj* ioobj = IOM().get( wrid_ );
	if ( !ioobj )
	    { errmsg_ = "Output data store not in object mgr"; return false; }
	wrr_ = new SeisTrcWriter( ioobj );
	delete ioobj;
	if ( !wrr_ )
	    { errmsg_ = "Cannot write to this data store type"; return false; }
	wrsampnms = true;
    }

    const int curinl = towrite_[0];
    SeisPSImpLineBuf* lbuf = 0;
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	if ( lines_[idx]->inl_ == curinl )
	    { lbuf = lines_[idx]; break; }
    }
    if ( !lbuf || lbuf->gathers_.isEmpty() )
	{ delete lbuf; towrite_.remove(0); return true; } // shouldn't happen

    SeisTrcBuf* gath2write = lbuf->gathers_.remove( 0 );
    const bool lbufempty = lbuf->gathers_.isEmpty();
    if ( lbufempty )
    {
	if ( DBG::isOn(mDBGmask) )
	{
	    BufferString str( "Written inl=" ); str += lbuf->inl_;
	    str += ";\tNew nr lines="; str += lines_.size();
	    DBG::message( str );
	}
	lines_ -= lbuf;
    }

    bool res = true;
    for ( int idx=0; idx<gath2write->size(); idx++ )
    {
	res = wrr_->put( *gath2write->get(idx) );
	if ( !res )
	    { errmsg_ = wrr_->errMsg(); break; }
    }

    if ( gathersize_ == 0 )
	gathersize_ = gath2write->size();
    else if ( gathersize_ > 0 && gath2write->size() != gathersize_ )
	gathersize_ = -1;

    delete gath2write;
    if ( lbufempty ) delete lbuf;

    if ( wrsampnms )
	wrr_->psWriter()->setSampleNames( samplenms_ );
    return res;
}
