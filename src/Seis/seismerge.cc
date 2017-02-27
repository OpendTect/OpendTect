/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

#include "seismerge.h"
#include "seisprovider.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "scaler.h"

SeisMerger::SeisMerger( const ObjectSet<IOPar>& iops, const IOPar& outiop,
			bool stacktrcs, Seis::MultiProvider::ZPolicy zpol )
	: Executor(!iops.isEmpty() &&
		SeisIOObjInfo(Seis::Provider::dbKey(*iops[0])).is2D()
		? "Merging line parts" : "Merging cubes")
	, wrr_(0)
	, multiprov_(0)
	, nrpos_(0)
	, totnrpos_(-1)
	, stacktrcs_(stacktrcs)
        , scaler_(0)
	, nrsamps_(-1)
{
    if ( iops.isEmpty() )
    { errmsg_ = tr("Nothing to merge"); return; }
    if (iops.size() == 1)
    { errmsg_ = tr("One single entry to merge: Please use copy"); return; }

    const Seis::MultiProvider::Policy policy = stacktrcs_
				? Seis::MultiProvider::RequireAtLeastOne
				: Seis::MultiProvider::RequireOnlyOne;
    /*
    const bool is2d = SeisIOObjInfo(Provider::dbKey(iops[0])).is2D();
    if ( is2d )
	multiprov_ = new Seis::MultiProvider2D( policy, zpol );
    else*/
	multiprov_ = new Seis::MultiProvider3D( policy, zpol );

    const uiRetVal uirv = multiprov_->usePar( iops );
    if ( !uirv.isOK() )
	{ errmsg_ = uirv; multiprov_->setEmpty(); return; }

    const ZSampling zrg = multiprov_->getZRange();
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
	multiprov_->setEmpty();
	return;
    }

    totnrpos_ = multiprov_->totalNr();
}


SeisMerger::~SeisMerger()
{
    delete wrr_;
    delete multiprov_;
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
    if ( !errmsg_.isEmpty() )
	return Executor::ErrorOccurred();

    nrpos_++;
    SeisTrc* newtrc = new SeisTrc;
    const uiRetVal uirv = multiprov_->getNext( *newtrc, stacktrcs_ );
    if ( isFinished(uirv) )
    {
	delete newtrc;
	wrr_->close();
	return Executor::Finished();
    }

    return writeTrc( newtrc );
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
