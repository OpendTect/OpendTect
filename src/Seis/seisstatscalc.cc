/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2004
-*/

#include "seisstatscalc.h"

#include "rangeposprovider.h"
#include "seisioobjinfo.h"
#include "seisprovider.h"
#include "seistrc.h"
#include "trckeyzsampling.h"
#include "threadwork.h"


// SeisStats
class StatsFiller : public SequentialTask
{
public:
StatsFiller( const Pos::Provider& prov,
	     ObjectSet<Stats::RunCalc<float> >& stats, SeisTrc& trc )
    : prov_(prov)
    , stats_(stats)
    , trc_(trc)
{}


~StatsFiller()
{ delete &trc_; }


int nextStep()
{
    const Coord crd = trc_.info().coord_;
    const StepInterval<float> zrg = trc_.zRange();
    for ( int cidx=0; cidx<trc_.nrComponents(); cidx++ )
    {
	Stats::RunCalc<float>& rc = *stats_[cidx];
	for ( int zidx=0; zidx<trc_.size(); zidx++ )
	{
	    if ( prov_.includes(crd,zrg.atIndex(zidx)) )
		rc.addValue( trc_.get(zidx,cidx) );
	}
    }

    return Finished();
}

protected:

    const Pos::Provider&		prov_;
    ObjectSet<Stats::RunCalc<float> >&	stats_;
    SeisTrc&				trc_;
};


SeisStatsCalc::SeisStatsCalc( const IOObj& ioobj, const Stats::CalcSetup& scs,
			      const Pos::Provider* prov,
			      const TypeSet<int>* comps )
    : Executor("Reader")
    , ioobj_(ioobj.clone())
    , posprov_(0)
    , seisprov_(0)
{
    uiRetVal uirv;
    seisprov_ = Seis::Provider::create( ioobj, &uirv );
    if ( !seisprov_ )
	msg_ = uirv;

    SeisIOObjInfo info( ioobj );
    if ( !comps )
    {
	const int nrcomps = info.nrComponents();
	for ( int idx=0; idx<nrcomps; idx++ )
	    components_ += idx;
    }
    else
	components_ = *comps;

    for ( int idx=0; idx<components_.size(); idx++ )
	stats_ += new Stats::RunCalc<float>( scs );

    if ( !posprov_ )
    {
	if ( info.is2D() )
	{
	    //TODO this only does the first line
	    StepInterval<int> trcrg; StepInterval<float> zrg;
	    GeomIDSet gids;
	    info.getGeomIDs( gids );
	    auto* prov2d = new Pos::RangeProvider2D;
	    if ( !gids.isEmpty() )
	    {
		info.getRanges( gids.first(), trcrg, zrg );
		prov2d->setTrcRange( trcrg );
		prov2d->setZRange( zrg );
		posprov_ = prov2d;
	    }
	}
	else
	{
	    TrcKeyZSampling tkzs;
	    info.getRanges( tkzs );
	    auto* prov3d = new Pos::RangeProvider3D;
	    prov3d->setSampling( tkzs );
	    posprov_ = prov3d;
	}
    }
    else
    {
	IOPar pars; prov->fillPar( pars );
	posprov_ = Pos::Provider::make( pars, info.is2D() );
    }

    totalnr_ = posprov_->estNrPos();
    nrdone_ = 0;

    queueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::SingleThread,
				"Stats Calculator" );
}


SeisStatsCalc::~SeisStatsCalc()
{
    delete seisprov_; delete ioobj_;

    Threads::WorkManager::twm().removeQueue( queueid_, false );
}


const Stats::RunCalc<float>& SeisStatsCalc::getStats( int cidx ) const
{ return *stats_[cidx]; }


int SeisStatsCalc::nextStep()
{
    if ( !seisprov_ ) return ErrorOccurred();

    SeisTrc* trc = new SeisTrc;
    const uiRetVal uirv = seisprov_->getNext( *trc );
    if ( !uirv.isOK() )
    {
	delete trc;
	if ( isFinished(uirv) )
	    return Finished();

	msg_ = uirv;
	return ErrorOccurred();
    }

    Task* task = new StatsFiller( *posprov_, stats_, *trc );
    Threads::WorkManager::twm().addWork(
	Threads::Work(*task,true), 0, queueid_, false, false, true );

    nrdone_++;
    return MoreToDo();
}
