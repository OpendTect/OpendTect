/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltieextractdata.cc,v 1.30 2010-10-15 10:32:42 cvsbruno Exp $";

#include "welltieextractdata.h"
#include "welltiegeocalculator.h"

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "interpol1d.h"
#include "ioman.h"
#include "datapointset.h"
#include "linekey.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "survinfo.h"

#include "welldata.h"
#include "welltiedata.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welltrack.h"


namespace WellTie
{

TrackExtractor::TrackExtractor(const Well::Data* d)
    : Executor("Extracting Well track positions")
    , wd_(*d)
    , track_(wd_.track())
    , d2t_(*wd_.d2TModel())		
    , nrdone_(0)
    , timeintv_(0,0,0)
{
    prevbid_ = SI().transform( track_.pos(0) );     
}


int TrackExtractor::nextStep()
{
    if ( prevbid_.inl<0 || prevbid_.crl<0 )
	return ErrorOccurred();
    double time = timeintv_.atIndex( nrdone_ );
    if ( time > timeintv_.stop )
	return Executor::Finished();

    Coord3 pos = track_.getPos( d2t_.getDah(time) );
    pos.z = time;

    BinID bid = SI().transform( pos );

    if ( time > d2t_.getTime( track_.dah( track_.size()-1 ) ) )
	bid = prevbid_;
    if ( time < d2t_.getTime( track_.dah( 0 ) ) )
	bid = prevbid_;

    if ( !SI().inlRange(true).includes(bid.inl) 
	    || !SI().crlRange(true).includes(bid.crl)  )
    {
	bid = prevbid_;
	bidset_ += bid;
	nrdone_++;
       	return Executor::MoreToDo();
    }
    
    bidset_ += bid;
    prevbid_ = bid;

    nrdone_ ++;
    return Executor::MoreToDo();
}


						 
SeismicExtractor::SeismicExtractor( const IOObj& ioobj ) 
	: Executor("Extracting Seismic positions")
	, rdr_(new SeisTrcReader( &ioobj ))
	, trcbuf_(new SeisTrcBuf(false))				   
	, nrdone_(0)
	, cs_(new CubeSampling())	    
	, timeintv_(0,0,0)
	, linekey_(0)		  
	, radius_(1)		  
{
}


SeismicExtractor::~SeismicExtractor() 
{
    delete vals_; delete dahs_;
    delete cs_;
    delete rdr_;
    delete trcbuf_;
}


void SeismicExtractor::setTimeIntv( const StepInterval<float>& itv )
{
    timeintv_ = itv;
    vals_ = new Array1DImpl<float>( itv.nrSteps() );
    dahs_ = new Array1DImpl<float>( itv.nrSteps() );
}


void SeismicExtractor::collectTracesAroundPath() 
{
    if ( !bidset_.size() ) return;
    Interval<int> inlrg( bidset_[0].inl, bidset_[0].inl );
    Interval<int> crlrg( bidset_[0].crl, bidset_[0].crl );

    if ( rdr_->is2D() )
	crlrg.set( 0, SI().crlRange(true).stop ); 
    else
    {
	for ( int idx=0; idx<bidset_.size(); idx++ )
	{
	    const BinID bid = bidset_[idx];
	    if ( bid.inl < inlrg.start ) inlrg.start = bid.inl;
	    if ( bid.inl > inlrg.stop ) inlrg.stop = bid.inl;
	    if ( bid.crl < crlrg.start ) crlrg.start = bid.crl;
	    if ( bid.crl > crlrg.stop ) crlrg.stop = bid.crl;
	}
	inlrg.start -= radius_; inlrg.stop += radius_;
	crlrg.start -= radius_; crlrg.stop += radius_;
    }
    cs_->hrg.setCrlRange( crlrg );
    cs_->hrg.setInlRange( inlrg );
    cs_->hrg.snapToSurvey();
    cs_->zrg = timeintv_;
    Seis::RangeSelData* sd = new Seis::RangeSelData( *cs_ );
    sd->lineKey() = *linekey_;

    rdr_->setSelData( sd );
    rdr_->prepareWork();

    SeisBufReader sbfr( *rdr_, *trcbuf_ );
    sbfr.execute();
}


void SeismicExtractor::setBIDValues( const TypeSet<BinID>& bids )
{
    bidset_.erase();
    for ( int idx=0; idx<bids.size(); idx++ )
    {	
	if ( idx && ( !bids[idx].crl || !bids[idx].inl ) )
	     bidset_ += bids[idx-1];
	bidset_ += bids[idx];
    }
    collectTracesAroundPath();
}


int SeismicExtractor::nextStep()
{
    double time = timeintv_.atIndex( nrdone_ );

    if ( time>timeintv_.stop || nrdone_ >= timeintv_.nrSteps() )
	return Executor::Finished();

    const int datasz = timeintv_.nrSteps();

    const BinID curbid = bidset_[nrdone_];
    float val = 0; int nrtracesinradius = 0;
    int prevradius = (int) 1e30;

    for ( int idx=0; idx<trcbuf_->size(); idx++ )
    {
	const SeisTrc* trc = trcbuf_->get(idx);
	BinID b = trc->info().binid;
	int xx0 = b.inl-curbid.inl; 	xx0 *= xx0; 
	int yy0 = b.crl-curbid.crl;	yy0 *= yy0;

	if ( rdr_->is2D() )
	{
	    if ( ( xx0 + yy0  ) < prevradius )
	    {
		prevradius = xx0 + yy0;
		val = trc->get( nrdone_, 0 );
		nrtracesinradius = 1;
	    }
	}
	else
	{
	    if ( xx0 + yy0 < radius_*radius_ )
	    {
		nrtracesinradius ++;
		val += trc->get( nrdone_, 0 );
	    }
	}
    }
    if ( mIsUdf(val) ) val =0;
    vals_->set( nrdone_, val/nrtracesinradius );
    dahs_->set( nrdone_, time );

    nrdone_ ++;
    return Executor::MoreToDo();
}



#define mErrRet(s) { errmsg = s; return; }
LogResampler::LogResampler( Well::Log* newl, const Well::Log& orgl, 
			    const Well::Data* d, WellTie::DataHolder* dh )
    	: Executor("Processing log data") 
	, newlog_(newl)
	, orglog_(orgl)	     
	, wd_(*d)	   
	, nrdone_(0)
	, curidx_(0)
	, isavg_(true) 
	, vals_(0)	       
	, dahs_(0)	       
	, prevval_(0)  
{
    if ( newlog_ ) newlog_->erase();

    BufferString exnm = "Processing "; exnm += orgl.name(); exnm += " Data"; 
    setName( exnm );

    fillProcLog( orgl );

    BufferString errmsg;
    errmsg += "no valid "; errmsg += orgl.name();  errmsg += " log selected";

    if ( dh )
    {
	if ( !dh->geoCalc()->isValidLogData( val_ ) ) mErrRet(errmsg);
	dh->geoCalc()->interpolateLogData( dah_, orgl.dahStep(true), true );
	dh->geoCalc()->interpolateLogData( val_, orgl.dahStep(true), false );
	dh->geoCalc()->removeSpikes( val_ );
    }
}


LogResampler::~LogResampler() 
{
    delete vals_; delete dahs_;
}


void LogResampler::fillProcLog( const Well::Log& log )
{
    for ( int idx=0; idx<log.size(); idx++ )
    {
	val_ += log.valArr()[idx];
	dah_ += log.dah(idx);
    }
}


int LogResampler::nextStep()
{
    float curtime = timeintv_.atIndex( nrdone_ );
    if ( curtime >= timeintv_.stop || nrdone_ >= 
	    			timeintv_.nrSteps() || !orglog_.size() ) 
	return Executor::Finished();
   
    float curdah = wd_.d2TModel()->getDah( curtime );
    curidx_ = orglog_.indexOf( curdah );
    if ( curidx_ < 0 ) 
	curidx_ = 0;
    
    float curval = 0;
    if ( curdah < dah_[0] )
	curval = val_[0];
    else if ( curdah > dah_[dah_.size()-1] )
	curval = val_[dah_.size()-1];
    else if ( curidx_>1 && curidx_<dah_.size()-2 && isavg_ )
	curval += ( val_[curidx_+1] + val_[curidx_-1] + val_[curidx_] )/3;
    else
	curval = orglog_.getValue( curdah, true );
    if ( mIsUdf(curval) )
	curval = prevval_;
    
    prevval_ = curval;

    if ( newlog_ )
	newlog_->addValue( curdah, curval );
    if ( nrdone_ < dahs_->info().getSize(0) )
    {
	dahs_->set( nrdone_, curdah );
	vals_->set( nrdone_, curval );
    }

    nrdone_++;
    return Executor::MoreToDo();
}


void LogResampler::setTimeIntv( const StepInterval<float>& itv )
{
    timeintv_ = itv;
    vals_ = new Array1DImpl<float>( itv.nrSteps() );
    dahs_ = new Array1DImpl<float>( itv.nrSteps() );
}

}; //namespace WellTie
