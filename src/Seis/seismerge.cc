/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: seismerge.cc,v 1.1 2008-03-07 09:43:42 cvsbert Exp $";

#include "seismerge.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisbuf.h"
#include "seistrcprop.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "survinfo.h"
#include "cubesampling.h"
#include <iostream>


SeisMerge::SeisMerge( const ObjectSet<IOPar>& iops, const IOPar& outiop,
		      bool is2d )
    	: Executor(is2d?"Merging line parts":"Merging cubes")
    	, is2d_(is2d)
    	, wrr_(0)
    	, currdridx_(-1)
    	, nrpos_(0)
    	, totnrpos_(-1)
    	, curbid_(SI().sampling(false).hrg.start)
    	, trcbuf_(*new SeisTrcBuf(false))
{
    if ( !iops.isEmpty() )
	{ errmsg_ = "Nothing to merge"; return; }
    if ( iops.size() == 1 )
	{ errmsg_ = "One single entry to merge: Please use copy"; return; }

    for ( int idx=0; idx<iops.size(); idx++ )
    {
	SeisTrcReader* newrdr = new SeisTrcReader;
	rdrs_ += newrdr;
	newrdr->usePar( *iops[idx] );
	if ( newrdr->errMsg() && *newrdr->errMsg() )
	{
	    errmsg_ = newrdr->errMsg();
	    deepErase( rdrs_ );
	    return;
	}
    }

    wrr_ = new SeisTrcWriter( 0 );
    wrr_->usePar( outiop );
    if ( wrr_->errMsg() && *wrr_->errMsg() )
    {
	errmsg_ = wrr_->errMsg();
	deepErase( rdrs_ );
	return;
    }

    currdridx_ = 0;
    if ( !is2d_ )
	totnrpos_ = SI().sampling(false).hrg.totalNr();
}


SeisMerge::~SeisMerge()
{
    deepErase( rdrs_ );
    delete wrr_;
    trcbuf_.deepErase();
    delete &trcbuf_;
}


const char* SeisMerge::message() const
{
    return errmsg_.isEmpty() ? errmsg_.buf() : "Handling traces";
}


int SeisMerge::nextStep()
{
    if ( currdridx_ < 0 )
	return Executor::ErrorOccurred;

    if ( is2d_ &&  rdrs_.isEmpty() )
	return writeFromBuf();

    SeisTrc* newtrc = getNewTrc();
    if ( !newtrc )
    {
	deepErase( rdrs_ );
	if ( !errmsg_.isEmpty() )
	    return Executor::ErrorOccurred;

	if ( is2d_ )
	{
	    trcbuf_.sort( true, SeisTrcInfo::TrcNr );
	    return Executor::MoreToDo;
	}

	wrr_->close();
	return Executor::Finished;
    }

    if ( is2d_ )
	{ trcbuf_.add( newtrc ); return Executor::MoreToDo; }

    return writeTrc( newtrc );
}


SeisTrc* SeisMerge::getNewTrc()
{
    SeisTrc* ret = 0;

    while ( true )
    {
	nrpos_++;
	if ( is2d_ )
	{
	    ret = getTrcFrom( *rdrs_[currdridx_] );
	    if ( !ret )
	    {
		if ( !errmsg_.isEmpty() )
		    return 0;
		currdridx_++;
		if ( currdridx_ >= rdrs_.size() )
		    return 0;
	    }
	}

	get3DTraces();
	ret = getStacked( trcbuf_ );
	if ( ret || !toNextPos() )
	    break;
    }

    return ret;
}


SeisTrc* SeisMerge::getTrcFrom( SeisTrcReader& rdr )
{
    SeisTrc* newtrc = new SeisTrc;
    if ( !rdr.get(*newtrc) )
    {
	errmsg_ = rdr.errMsg();
	delete newtrc; newtrc = 0;
    }
    return newtrc;
}


void SeisMerge::get3DTraces()
{
    trcbuf_.deepErase();
    for ( int idx=0; idx<rdrs_.size(); idx++ )
    {
	SeisTrcReader& rdr = *rdrs_[idx];
	if ( rdr.seisTranslator()->goTo(curbid_) )
	{
	    SeisTrc* newtrc = getTrcFrom( rdr );
	    if ( newtrc )
		trcbuf_.add( newtrc );
	}
    }
}


SeisTrc* SeisMerge::getStacked( SeisTrcBuf& buf )
{
    if ( buf.isEmpty() )
	return 0;

    SeisTrc& trc( *buf.get(0) );
    const int sz = buf.size();
    if ( sz > 1 )
    {
	SeisTrcPropChg stckr( trc );
	for ( int idx=1; idx<sz; idx++ )
	    stckr.stack( *buf.get(idx), false, 1 / ((float)idx) );
    }

    SeisTrc* ret = new SeisTrc( trc );
    buf.deepErase();
    return ret;
}


bool SeisMerge::toNextPos()
{
    const HorSampling& hs( SI().sampling(false).hrg );

    curbid_.crl += hs.step.crl;
    if ( curbid_.crl > hs.stop.crl )
    {
	curbid_.inl += hs.step.inl;
	curbid_.crl = hs.start.crl;
	if ( curbid_.inl > hs.stop.inl )
	    return false;
    }
    return true;
}


int SeisMerge::writeTrc( SeisTrc* trc )
{
    bool ret = wrr_->put( *trc );
    delete trc;
    if ( ret )
	return Executor::MoreToDo;

    errmsg_ = wrr_->errMsg();
    return Executor::ErrorOccurred;
}


int SeisMerge::writeFromBuf()
{
    if ( trcbuf_.isEmpty() )
    {
	wrr_->close();
	return Executor::Finished;
    }

    SeisTrcBuf tmp( false );
    const int tnr = trcbuf_.get(0)->info().nr;
    for ( int idx=1; idx<trcbuf_.size(); idx++ )
    {
	if ( trcbuf_.get(idx)->info().nr == tnr )
	    { tmp.add( trcbuf_.get(idx) ); trcbuf_.remove( idx ); }
	else
	    break;
    }

    return writeTrc( getStacked(tmp) );
}
