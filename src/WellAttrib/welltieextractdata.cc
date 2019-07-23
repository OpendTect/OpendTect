/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

#include "welltieextractdata.h"
#include "welltiegeocalculator.h"

#include "arrayndimpl.h"
#include "trckeyzsampling.h"
#include "interpol1d.h"
#include "ioobj.h"
#include "datapointset.h"
#include "samplingdata.h"
#include "seisbuf.h"
#include "seisprovider.h"
#include "seisrangeseldata.h"
#include "seistrc.h"
#include "survinfo.h"
#include "survgeom2d.h"

namespace WellTie
{

SeismicExtractor::SeismicExtractor( const IOObj& ioobj )
	: Executor("Extracting Seismic positions")
	, prov_(0)
	, trcbuf_(new SeisTrcBuf(false))
	, nrdone_(0)
	, outtrc_(0)
	, tkzs_(new TrcKeyZSampling(false))
	, extrintv_(SI().zRange())
	, linenm_(*new BufferString)
	, radius_(1)
{
    uiRetVal uirv;
    prov_ = Seis::Provider::create( ioobj, &uirv );
    if ( !prov_ )
	errmsg_ = uirv;
}


SeismicExtractor::~SeismicExtractor()
{
    delete tkzs_;
    delete prov_;
    delete trcbuf_;
    delete outtrc_;
}


void SeismicExtractor::setInterval( const StepInterval<float>& itv )
{
    extrintv_ = itv;
    delete outtrc_;
    outtrc_ = new SeisTrc( itv.nrSteps() + 1 );
    outtrc_->info().sampling_ = itv;
    outtrc_->zero();
}

#define mErrRet(msg) { errmsg_ = msg; return false; }
bool SeismicExtractor::collectTracesAroundPath()
{
    if ( !prov_ ) return false;

    if ( bidset_.isEmpty() )
	mErrRet( tr("No position extracted from well track") );

    const bool seisid2D = prov_->is2D();
    if ( seisid2D )
    {
	const auto& geom2d = SurvGeom::get2D( linenm_ );
	if ( geom2d.isEmpty() )
	    mErrRet( tr("2D Line Geometry not found") );

	tkzs_->hsamp_.setTo( geom2d.geomID() );
	const Coord pos = SI().transform( bidset_[0] );
	const auto trcnr = geom2d.nearestTracePosition( pos );
	tkzs_->hsamp_.setCrlRange( Interval<int>(trcnr,trcnr) );
    }
    else
    {
	for ( int idx=0; idx<bidset_.size(); idx++ )
	{
	    BinID bid = bidset_[idx];
	    tkzs_->hsamp_.include( BinID( bid.inl() + radius_,
				     bid.crl() + radius_ ) );
	    tkzs_->hsamp_.include( BinID( bid.inl() - radius_,
				     bid.crl() - radius_ ) );
	}

	tkzs_->hsamp_.snapToSurvey();
    }

    tkzs_->zsamp_ = extrintv_;
    Seis::RangeSelData* sd = new Seis::RangeSelData( *tkzs_ );
    if ( seisid2D && !linenm_.isEmpty() )
	sd->setGeomID( SurvGeom::getGeomID(linenm_) );

    prov_->setSelData( sd );

    SeisBufReader sbfr( *prov_, *trcbuf_ );
    if ( !sbfr.execute() )
	mErrRet( sbfr.message() );

    return true;
}


void SeismicExtractor::setBIDValues( const TypeSet<BinID>& bids )
{
    bidset_.erase();
    for ( int idx=0; idx<bids.size(); idx++ )
    {
	if ( SI().includes( bids[idx] ) )
	    bidset_ += bids[idx];
	else if ( idx )
	    bidset_ += bids[idx-1];
    }
}


int SeismicExtractor::nextStep()
{
    if ( !nrdone_ && !collectTracesAroundPath() )
	return Executor::ErrorOccurred();

    double zval = extrintv_.atIndex( nrdone_ );

    if ( zval>extrintv_.stop || nrdone_ >= extrintv_.nrSteps()
	    || nrdone_ >= bidset_.size() )
	return Executor::Finished();

    const BinID curbid = bidset_[nrdone_];
    float val = 0; float nearestval = 0; int nrtracesinradius = 0;
    int prevradius = mUdf(int);

    for ( int idx=0; idx<trcbuf_->size(); idx++ )
    {
	const SeisTrc* trc = trcbuf_->get(idx);
	BinID b = trc->info().binID();

	const SamplingData<float>& sd = trc->info().sampling_;
	const int trcidx = sd.nearestIndex( zval );

	int xx0 = b.inl()-curbid.inl(); xx0 *= xx0;
	int yy0 = b.crl()-curbid.crl();	yy0 *= yy0;
	const float trcval = ( trcidx < 0 || trcidx >= trc->size() ) ?
						0 : trc->get( trcidx, 0 );

	if ( ( xx0 + yy0 )  < radius_*radius_ )
	{
	    val += mIsUdf(trcval) ? 0 : trcval;
	    nrtracesinradius ++;
	}
	if ( ( xx0 + yy0  ) < prevradius || mIsUdf(prevradius) )
	{
	    prevradius = xx0 + yy0;
	    nearestval = mIsUdf(trcval) ? 0 : trcval;
	}
    }
    if ( !nrtracesinradius )
    {
	val = nearestval;
	nrtracesinradius = 1;
    }
    outtrc_->set( nrdone_, val/nrtracesinradius, 0 );

    nrdone_ ++;
    return Executor::MoreToDo();
}

} // namespace WellTie
