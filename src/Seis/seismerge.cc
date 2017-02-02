/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/


#include "seismerge.h"
#include "seisbounds.h"
#include "seisprovider.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisbuf.h"
#include "seistrcprop.h"
#include "dirlist.h"
#include "filepath.h"
#include "dbman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "oddirs.h"
#include "scaler.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include <iostream>


SeisMerger::SeisMerger( const ObjectSet<IOPar>& iops, const IOPar& outiop,
		      bool is2d )
	: Executor(is2d?"Merging line parts":"Merging cubes")
	, is2d_(is2d)
	, wrr_(0)
	, curprovidx_(-1)
	, nrpos_(0)
	, totnrpos_(-1)
	, curbid_(SI().sampling(false).hsamp_.start_)
	, trcbuf_(*new SeisTrcBuf(false))
	, stacktrcs_(true)
        , scaler_(0)
	, nrsamps_(-1)
{
    if ( iops.isEmpty() )
    { errmsg_ = tr("Nothing to merge"); return; }
    if (iops.size() == 1)
    { errmsg_ = tr("One single entry to merge: Please use copy"); return; }

    StepInterval<float> zrg( mUdf(float), -mUdf(float), SI().zStep() );
    for ( int idx=0; idx<iops.size(); idx++ )
    {
	const Seis::GeomType geomtype = is2d_ ? Seis::Line : Seis::Vol;
	Seis::Provider* prov = Seis::Provider::create( geomtype );
	const uiRetVal uirv = prov->usePar( *iops[idx] );
	if ( !uirv.isOK() )
	    { errmsg_ = uirv; delete prov; continue; }

	if ( is2d_ )
	{
	    StepInterval<int> rg;
	    ZSampling zsamp;
	    mDynamicCastGet(const Seis::Provider2D&,prov2d,*prov);
	    if ( prov2d.getRanges(prov2d.curGeomID(),rg,zsamp) )
		zrg.include( zsamp, false );
	}
	else
	{
	    TrcKeyZSampling tkzs;
	    mDynamicCastGet(const Seis::Provider3D&,prov3d,*prov);
	    if ( prov3d.getRanges(tkzs) )
		zrg.include( tkzs.zsamp_, false );
	}

	provs_ += prov;
    }

    if ( !mIsUdf(zrg.start) && !mIsUdf(zrg.start) )
    {
	sd_.start = zrg.start; sd_.step = zrg.step;
	nrsamps_ = zrg.nrSteps() + 1;
    }

    wrr_ = new SeisTrcWriter( 0 );
    wrr_->usePar( outiop );
    if ( !wrr_->errMsg().isEmpty() )
    {
	errmsg_ = wrr_->errMsg();
	deepErase( provs_ );
	return;
    }

    curprovidx_ = 0;
    if ( !is2d_ )
	totnrpos_ = mCast( int, SI().sampling(false).hsamp_.totalNr() );
}


SeisMerger::~SeisMerger()
{
    deepErase( provs_ );
    delete wrr_;
    trcbuf_.deepErase();
    delete &trcbuf_;
    delete scaler_;
}


uiString SeisMerger::message() const
{
    return errmsg_.isEmpty() ? errmsg_ : tr("Handling traces");
}


void SeisMerger::setScaler( Scaler* scaler )
{
    delete scaler_;
    scaler_ = scaler;
}


int SeisMerger::nextStep()
{
    if ( curprovidx_ < 0 )
	return Executor::ErrorOccurred();

    if ( is2d_ && provs_.isEmpty() )
	return writeFromBuf();

    SeisTrc* newtrc = getNewTrc();
    if ( !newtrc )
    {
	deepErase( provs_ );
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
	    ret = getTrcFrom( *provs_[curprovidx_] );
	    if ( !ret )
	    {
		if ( !errmsg_.isEmpty() )
		    return 0;
		curprovidx_++;
		if ( curprovidx_ >= provs_.size() )
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


SeisTrc* SeisMerger::getTrcFrom( Seis::Provider& prov )
{
    SeisTrc* newtrc = new SeisTrc;
    const uiRetVal uirv = prov.getNext( *newtrc );
    if ( !uirv.isOK() )
    {
	deleteAndZeroPtr( newtrc );
	if ( !isFinished(uirv) )
	    errmsg_ = uirv;
    }

    return newtrc;
}


void SeisMerger::get3DTraces()
{
    trcbuf_.deepErase();
    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	SeisTrc* newtrc = getTrcFrom( *provs_[idx] );
	if ( !newtrc )
	    continue;

	trcbuf_.add( newtrc );
	if ( !stacktrcs_ )
	    break;
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
	    stckr.stack( *buf.get(idx), false, mCast(float,idx) );
    }

    ret = buf.remove( 0 );
    buf.deepErase();
    return ret;
}


bool SeisMerger::toNextPos()
{
    TrcKeySampling hs = SI().sampling(false).hsamp_;
    curbid_.crl() += hs.step_.crl();
    if ( curbid_.crl() > hs.stop_.crl() )
    {
	curbid_.inl() += hs.step_.inl();
	curbid_.crl() = hs.start_.crl();
	if ( curbid_.inl() > hs.stop_.inl() )
	    return false;
    }
    return true;
}


int SeisMerger::writeTrc( SeisTrc* trc )
{
    if ( nrsamps_ < 0 )
    {
	nrsamps_ = trc->size();
	sd_ = trc->info().sampling_;
    }
    else if ( trc->size() != nrsamps_ || trc->info().sampling_ != sd_ )
    {
	SeisTrc* newtrc = new SeisTrc(*trc);
	newtrc->info().sampling_ = sd_;
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

    if ( scaler_ ) trc->data().scale( *scaler_ );
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
    const TrcKey trckey = trc0->info().trckey_;
    tmp.add( trc0 );

    while ( !trcbuf_.isEmpty() )
    {
	if ( trcbuf_.get(0)->info().trckey_ != trckey )
	    break;
	tmp.add( trcbuf_.remove(0) );
    }

    return writeTrc( getStacked(tmp) );
}
