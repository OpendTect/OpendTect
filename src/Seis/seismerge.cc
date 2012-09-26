/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seismerge.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisbuf.h"
#include "seistrcprop.h"
#include "dirlist.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "oddirs.h"
#include "survinfo.h"
#include "cubesampling.h"
#include <iostream>


SeisMerger::SeisMerger( const ObjectSet<IOPar>& iops, const IOPar& outiop,
		      bool is2d )
    	: Executor(is2d?"Merging line parts":"Merging cubes")
    	, is2d_(is2d)
    	, wrr_(0)
    	, currdridx_(-1)
    	, nrpos_(0)
    	, totnrpos_(-1)
    	, curbid_(SI().sampling(false).hrg.start)
    	, trcbuf_(*new SeisTrcBuf(false))
    	, stacktrcs_(true)
    	, nrsamps_(-1)
{
    if ( iops.isEmpty() )
	{ errmsg_ = "Nothing to merge"; return; }
    if ( iops.size() == 1 )
	{ errmsg_ = "One single entry to merge: Please use copy"; return; }

    for ( int idx=0; idx<iops.size(); idx++ )
    {
	SeisTrcReader* newrdr = new SeisTrcReader;
	rdrs_ += newrdr;
	newrdr->usePar( *iops[idx] );
	if ( !newrdr->prepareWork() )
	{
	    errmsg_ = newrdr->errMsg();
	    deepErase( rdrs_ );
	    return;
	}
    }

    wrr_ = new SeisTrcWriter( 0 );
    wrr_->usePar( outiop );
    if ( wrr_->errMsg() )
    {
	errmsg_ = wrr_->errMsg();
	deepErase( rdrs_ );
	return;
    }

    currdridx_ = 0;
    if ( !is2d_ )
	totnrpos_ = SI().sampling(false).hrg.totalNr();
}


SeisMerger::SeisMerger( const IOPar& iop )
    	: Executor("Merging cubes")
    	, is2d_(false)
    	, wrr_(0)
    	, currdridx_(-1)
    	, nrpos_(0)
    	, totnrpos_(-1)
	, curbid_(SI().sampling(false).hrg.start)
    	, trcbuf_(*new SeisTrcBuf(false))
    	, stacktrcs_(true)
    	, nrsamps_(-1)
{
    if ( iop.isEmpty() )
	{ errmsg_ = "Nothing to merge"; return; }

    FilePath fp( iop.find(sKey::TmpStor()) );
    DirList dlist( fp.fullPath(), DirList::FilesOnly );
    for ( int idx=0; idx<dlist.size(); idx++ )
    {
	SeisTrcReader* newrdr = new SeisTrcReader( dlist.fullPath(idx) );
	if ( !newrdr->prepareWork() )
	{
	    errmsg_ = newrdr->errMsg();
	    delete newrdr;
	    continue;
	}

	rdrs_ += newrdr;
    }

    PtrMan<IOPar> outiop = iop.subselect( sKey::Output() );
    if ( !outiop )
	return;

    wrr_ = new SeisTrcWriter( 0 );
    wrr_->usePar( *outiop );
    if ( wrr_->errMsg() )
    {
	errmsg_ = wrr_->errMsg();
	deepErase( rdrs_ );
	delete wrr_;
	wrr_ = 0;
	return;
    }

    currdridx_ = 0;
    totnrpos_ = SI().sampling(false).hrg.totalNr();
}


SeisMerger::~SeisMerger()
{
    deepErase( rdrs_ );
    delete wrr_;
    trcbuf_.deepErase();
    delete &trcbuf_;
}


const char* SeisMerger::message() const
{
    return errmsg_.isEmpty() ? errmsg_.buf() : "Handling traces";
}


int SeisMerger::nextStep()
{
    if ( currdridx_ < 0 )
	return Executor::ErrorOccurred();

    if ( is2d_ && rdrs_.isEmpty() )
	return writeFromBuf();

    SeisTrc* newtrc = getNewTrc();
    if ( !newtrc )
    {
	deepErase( rdrs_ );
	if ( !errmsg_.isEmpty() )
	    return Executor::ErrorOccurred();

	if ( is2d_ )
	{
	    trcbuf_.sort( true, SeisTrcInfo::TrcNr );
	    return Executor::MoreToDo();
	}

	wrr_->close();
	return Executor::Finished();
    }

    if ( is2d_ )
	{ trcbuf_.add( newtrc ); return Executor::MoreToDo(); }

    return writeTrc( newtrc );
}


SeisTrc* SeisMerger::getNewTrc()
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
	if ( !toNextPos() || ret )
	    break;
    }

    return ret;
}


SeisTrc* SeisMerger::getTrcFrom( SeisTrcReader& rdr )
{
    SeisTrc* newtrc = new SeisTrc;
    if ( !rdr.get(*newtrc) )
    {
	errmsg_ = rdr.errMsg();
	delete newtrc; newtrc = 0;
    }
    return newtrc;
}


void SeisMerger::get3DTraces()
{
    trcbuf_.deepErase();
    for ( int idx=0; idx<rdrs_.size(); idx++ )
    {
	SeisTrcReader& rdr = *rdrs_[idx];
	if ( rdr.seisTranslator()->goTo(curbid_) )
	{
	    SeisTrc* newtrc = getTrcFrom( rdr );
	    if ( !newtrc )
		continue;
	    
	    trcbuf_.add( newtrc );
	    if ( !stacktrcs_ )
		break;
	}
    }
}


SeisTrc* SeisMerger::getStacked( SeisTrcBuf& buf )
{
    int nrtrcs = buf.size();
    if ( nrtrcs < 1 )
	return 0;
    else if ( nrtrcs == 1 )
	return buf.remove( 0 );

    SeisTrcBuf nulltrcs( false );
    for ( int idx=nrtrcs-1; idx>-1; idx-- )
    {
	if ( buf.get(idx)->isNull() )
	    nulltrcs.add( buf.remove(idx) );
    }

    nrtrcs = buf.size();
    SeisTrc* ret = 0;
    if ( nrtrcs < 1 )
	ret = nulltrcs.remove(0);
    if ( nrtrcs == 1 )
	ret = buf.remove( 0 );
    nulltrcs.deepErase();
    if ( ret )
	return ret;

    SeisTrc& trc( *buf.get(0) );
    if ( stacktrcs_ )
    {
	SeisTrcPropChg stckr( trc );
	for ( int idx=1; idx<nrtrcs; idx++ )
	    stckr.stack( *buf.get(idx), false, idx );
    }

    ret = buf.remove( 0 );
    buf.deepErase();
    return ret;
}


bool SeisMerger::toNextPos()
{
    HorSampling hs = SI().sampling(false).hrg;
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


int SeisMerger::writeTrc( SeisTrc* trc )
{
    if ( nrsamps_ < 0 )
    {
	nrsamps_ = trc->size();
	sd_ = trc->info().sampling;
    }
    else if ( trc->size() != nrsamps_ || trc->info().sampling != sd_ )
    {
	SeisTrc* newtrc = new SeisTrc(*trc);
	newtrc->info().sampling = sd_;
	newtrc->reSize( nrsamps_, false );
	const int nrcomps = trc->nrComponents();
	for ( int isamp=0; isamp<nrsamps_; isamp++ )
	{
	    const float z = newtrc->info().samplePos(isamp);
	    for ( int icomp=0; icomp<nrcomps; icomp++ )
		newtrc->set( isamp, trc->getValue(z,icomp), icomp );
	}
	delete trc; trc = newtrc;
    }
    bool ret = wrr_->put( *trc );
    if ( !ret )
    {
	delete trc;
	errmsg_ = wrr_->errMsg();
	return Executor::ErrorOccurred();
    }

    delete trc;
    return Executor::MoreToDo();

}


int SeisMerger::writeFromBuf()
{
    if ( trcbuf_.isEmpty() )
    {
	wrr_->close();
	return Executor::Finished();
    }

    SeisTrcBuf tmp( false );
    SeisTrc* trc0 = trcbuf_.remove( 0 );
    const int tnr = trc0->info().nr;
    tmp.add( trc0 );

    while ( !trcbuf_.isEmpty() )
    {
	if ( trcbuf_.get(0)->info().nr != tnr )
	    break;
	tmp.add( trcbuf_.remove(0) );
    }

    return writeTrc( getStacked(tmp) );
}
