/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismerge.h"
#include "seisbounds.h"
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
#include "scaler.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include <iostream>


SeisMerger::SeisMerger( const ObjectSet<IOPar>& iops, const IOPar& outiop,
			bool is2d )
    : Executor(is2d?"Merging line parts":"Merging cubes")
    , is2d_(is2d)
    , wrr_(0)
    , currdridx_(-1)
    , nrpos_(0)
    , totnrpos_(-1)
    , curbid_(SI().sampling(false).hsamp_.start_)
    , trcbuf_(*new SeisTrcBuf(false))
    , stacktrcs_(true)
    , scaler_(0)
    , nrsamps_(-1)
{
    if ( iops.isEmpty() )
    {
	errmsg_ = tr("Nothing to merge");
	return;
    }

    if ( iops.size() == 1 )
    {
	errmsg_ = tr("One single entry to merge: Please use copy");
	return;
    }

    const Seis::GeomType gt = Seis::geomTypeOf( is2d_, false );
    StepInterval<float> zrg( mUdf(float), -mUdf(float), SI().zStep() );
    for ( const auto* iop : iops )
    {
	PtrMan<IOObj> newobj = SeisStoreAccess::getFromPar( *iop );
	if ( !newobj )
	    continue;

	auto* newrdr = new SeisTrcReader( *newobj, &gt );
	newrdr->usePar( *iop );
	if ( !newrdr->prepareWork() )
	{
	    errmsg_ = newrdr->errMsg();
	    delete newrdr;
	    continue;
	}

	PtrMan<Seis::Bounds> rgs = newrdr->getBounds();
	if ( rgs )
	    zrg.include( rgs->getZRange(), false );

	rdrs_ += newrdr;
    }

    if ( !mIsUdf(zrg.start_) && !mIsUdf(zrg.start_) )
    {
	sd_.start_ = zrg.start_; sd_.step_ = zrg.step_;
	nrsamps_ = zrg.nrSteps() + 1;
    }

    PtrMan<IOObj> newobj = SeisStoreAccess::getFromPar( outiop );
    if ( !newobj )
    {
	deepErase( rdrs_ );
	return;
    }

    wrr_ = new SeisTrcWriter( *newobj.ptr(), &gt );
    wrr_->usePar( outiop );
    if ( !wrr_->errMsg().isEmpty() )
    {
	errmsg_ = wrr_->errMsg();
	deepErase( rdrs_ );
	deleteAndNullPtr( wrr_ );
	return;
    }

    currdridx_ = 0;
    if ( !is2d_ )
	totnrpos_ = mCast( int, SI().sampling(false).hsamp_.totalNr() );
}


SeisMerger::SeisMerger( const IOPar& iop )
    : Executor("Merging cubes")
    , is2d_(false)
    , wrr_(0)
    , currdridx_(-1)
    , nrpos_(0)
    , totnrpos_(-1)
    , curbid_(SI().sampling(false).hsamp_.start_)
    , trcbuf_(*new SeisTrcBuf(false))
    , stacktrcs_(true)
    , nrsamps_(-1)
{
    if ( iop.isEmpty() )
    {
	errmsg_ = tr("Nothing to merge");
	return;
    }

    FilePath fp( iop.find(sKey::TmpStor()) );
    DirList dlist( fp.fullPath(), File::FilesInDir );
    StepInterval<float> zrg( mUdf(float), -mUdf(float), SI().zStep() );
    const Seis::GeomType gt = Seis::geomTypeOf( is2d_, false );
    for ( int idx=0; idx<dlist.size(); idx++ )
    {
	auto* newrdr = new SeisTrcReader( dlist.fullPath(idx) );
	if ( !newrdr->prepareWork() )
	{
	    errmsg_ = newrdr->errMsg();
	    delete newrdr;
	    continue;
	}

	PtrMan<Seis::Bounds> rgs = newrdr->getBounds();
	if ( rgs )
	    zrg.include( rgs->getZRange(), false );

	rdrs_ += newrdr;
    }

    if ( !mIsUdf(zrg.start_) && !mIsUdf(zrg.start_) )
    {
	sd_.start_ = zrg.start_; sd_.step_ = zrg.step_;
	nrsamps_ = zrg.nrSteps() + 1;
    }

    PtrMan<IOPar> outiop = iop.subselect( sKey::Output() );
    if ( !outiop )
    {
	deepErase( rdrs_ );
	return;
    }

    PtrMan<IOObj> newobj = SeisStoreAccess::getFromPar( *outiop.ptr() );
    if ( !newobj )
    {
	deepErase( rdrs_ );
	return;
    }

    wrr_ = new SeisTrcWriter( *newobj.ptr(), &gt );
    wrr_->usePar( *outiop );
    if ( !wrr_->errMsg().isEmpty() )
    {
	errmsg_ = wrr_->errMsg();
	deepErase( rdrs_ );
	deleteAndNullPtr( wrr_ );
	return;
    }

    currdridx_ = 0;
    totnrpos_ = mCast( int, SI().sampling(false).hsamp_.totalNr() );
}


SeisMerger::~SeisMerger()
{
    deepErase( rdrs_ );
    delete wrr_;
    trcbuf_.deepErase();
    delete &trcbuf_;
    delete scaler_;
}


uiString SeisMerger::uiMessage() const
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
    if ( currdridx_ < 0 )
	return ErrorOccurred();

    if ( is2d_ && rdrs_.isEmpty() )
	return writeFromBuf();

    SeisTrc* newtrc = getNewTrc();
    if ( !newtrc )
    {
	deepErase( rdrs_ );
	if ( !errmsg_.isEmpty() )
	    return ErrorOccurred();

	if ( is2d_ )
	{
	    trcbuf_.sort( true, SeisTrcInfo::TrcNr );
	    return MoreToDo();
	}

	wrr_->close();
	return Finished();
    }

    if ( is2d_ )
	{ trcbuf_.add( newtrc ); return MoreToDo(); }

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

    if ( scaler_ ) trc->data().scale( *scaler_ );
    bool ret = wrr_->put( *trc );
    if ( !ret )
    {
	delete trc;
	errmsg_ = wrr_->errMsg();
	return ErrorOccurred();
    }

    delete trc;
    return MoreToDo();

}


int SeisMerger::writeFromBuf()
{
    if ( trcbuf_.isEmpty() )
    {
	wrr_->close();
	return Finished();
    }

    SeisTrcBuf tmp( false );
    SeisTrc* trc0 = trcbuf_.remove( 0 );
    const int tnr = trc0->info().trcNr();
    tmp.add( trc0 );

    while ( !trcbuf_.isEmpty() )
    {
	if ( trcbuf_.get(0)->info().trcNr() != tnr )
	    break;

	tmp.add( trcbuf_.remove(0) );
    }

    return writeTrc( getStacked(tmp) );
}
