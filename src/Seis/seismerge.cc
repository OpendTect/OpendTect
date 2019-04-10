/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2003
-*/

#include "seismerge.h"
#include "seisprovider.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seisstorer.h"
#include "scaler.h"
#include "keystrs.h"

SeisMerger::SeisMerger( const ObjectSet<IOPar>& iops, const IOPar& outiop,
			bool stacktrcs, Seis::MultiProvider::ZPolicy zpol )
    : Executor(!iops.isEmpty() &&
	    SeisIOObjInfo(Seis::Provider::dbKey(*iops[0])).is2D()
	    ? "Merging line parts" : "Merging cubes")
    , stacktrcs_(stacktrcs)
    , totnrpos_(-1)
    , nrsamps_(-1)
{
    if ( iops.isEmpty() )
	{ errmsg_ = tr("Nothing to merge"); return; }
    if (iops.size() == 1)
	{ errmsg_ = tr("One single entry to merge: Please use copy"); return; }

    const Seis::MultiProvider::Policy policy = stacktrcs_
				? Seis::MultiProvider::RequireAtLeastOne
				: Seis::MultiProvider::RequireOnlyOne;

    const bool is2d = SeisIOObjInfo(Seis::Provider::dbKey(*iops[0])).is2D();
    if ( is2d )
	multiprov_ = new Seis::MultiProvider2D( policy, zpol );
    else
	multiprov_ = new Seis::MultiProvider3D( policy, zpol );

    IOPar iop;
    for ( int idx=0; idx<iops.size(); idx++ )
    {
	const FixedString key( IOPar::compKey(sKey::Provider(),idx) );
	iop.mergeComp( *iops[idx], key );
    }

    const uiRetVal uirv = multiprov_->usePar( iop );
    if ( !uirv.isOK() )
	{ errmsg_ = uirv; multiprov_->setEmpty(); return; }

    const ZSampling zrg = multiprov_->getZRange();
    if ( !mIsUdf(zrg.start) && !mIsUdf(zrg.start) )
    {
	sd_.start = zrg.start; sd_.step = zrg.step;
	nrsamps_ = zrg.nrSteps() + 1;
    }

    storer_ = new Storer;
    storer_->usePar( outiop );
    if ( !storer_->isUsable() )
	{ errmsg_ = storer_->errNotUsable(); multiprov_->setEmpty(); }
    else
	totnrpos_ = multiprov_->totalNr();
}


SeisMerger::~SeisMerger()
{
    delete storer_;
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
	storer_->close();
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
    auto ret = storer_->put( *trc );
    delete trc;
    if ( !ret.isOK() )
	{ errmsg_ = ret; return Executor::ErrorOccurred(); }

    return Executor::MoreToDo();
}
