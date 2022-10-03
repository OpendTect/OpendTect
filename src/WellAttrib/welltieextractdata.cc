/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welltieextractdata.h"
#include "welltiegeocalculator.h"

#include "arrayndimpl.h"
#include "trckeyzsampling.h"
#include "interpol1d.h"
#include "ioman.h"
#include "datapointset.h"
#include "samplingdata.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "survinfo.h"


// WellTie

WellTie::SeismicExtractor::SeismicExtractor( const IOObj& ioobj )
    : Executor("Extracting Seismic positions")
    , rdr_(new SeisTrcReader(ioobj ))
    , trcbuf_(new SeisTrcBuf(false))
    , tkzs_(new TrcKeyZSampling(false))
    , extrintv_(SI().zRange(false))
{}


WellTie::SeismicExtractor::~SeismicExtractor()
{
    delete tkzs_;
    delete rdr_;
    delete trcbuf_;
    delete outtrc_;
}


void WellTie::SeismicExtractor::setInterval( const StepInterval<float>& itv )
{
    extrintv_ = itv;
    delete outtrc_;
    outtrc_ = new SeisTrc( itv.nrSteps() + 1 );
    outtrc_->info().sampling = itv;
    outtrc_->zero();
}

#define mErrRet(msg) { errmsg_ = msg; return false; }
bool WellTie::SeismicExtractor::collectTracesAroundPath()
{
    if ( bidset_.isEmpty() )
	mErrRet( tr("No position extracted from well track") );

    const bool seisid2D = rdr_->is2D();
    if ( seisid2D )
    {
	const Survey::Geometry* geom = linenm_.isEmpty() ? 0
			: Survey::GM().getGeometry( linenm_.buf() );
	if ( !geom )
	    mErrRet( tr("2D Line Geometry not found") );

	tkzs_->hsamp_.init( geom->getID() );
	const Coord pos = SI().transform( bidset_[0] );
	const TrcKey trckey = geom->nearestTrace( pos );
	tkzs_->hsamp_.setCrlRange( Interval<int>(trckey.trcNr(),
						 trckey.trcNr()) );
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
	sd->setGeomID( Survey::GM().getGeomID(linenm_) );

    rdr_->setSelData( sd );
    rdr_->prepareWork();

    SeisBufReader sbfr( *rdr_, *trcbuf_ );
    if ( !sbfr.execute() )
	mErrRet( sbfr.uiMessage() );

    return true;
}


void WellTie::SeismicExtractor::setBIDValues( const TypeSet<BinID>& bids )
{
    bidset_.erase();
    for ( int idx=0; idx<bids.size(); idx++ )
    {
	if ( SI().isInside( bids[idx], true ) )
	    bidset_ += bids[idx];
	else if ( idx )
	    bidset_ += bids[idx-1];
    }
}


int WellTie::SeismicExtractor::nextStep()
{
    if ( !nrdone_ && !collectTracesAroundPath() )
	return ErrorOccurred();

    double zval = extrintv_.atIndex( nrdone_ );

    if ( zval>extrintv_.stop || nrdone_ >= extrintv_.nrSteps()
	    || nrdone_ >= bidset_.size() )
	return Finished();

    const BinID curbid = bidset_[nrdone_];
    float val = 0; float nearestval = 0; int nrtracesinradius = 0;
    int prevradius = mUdf(int);

    for ( int idx=0; idx<trcbuf_->size(); idx++ )
    {
	const SeisTrc* trc = trcbuf_->get(idx);
	BinID b = trc->info().binID();

	const SamplingData<float>& sd = trc->info().sampling;
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
    return MoreToDo();
}
