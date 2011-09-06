/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltietoseismic.cc,v 1.68 2011-09-06 15:20:08 cvsbruno Exp $";

#include "welltietoseismic.h"

#include "ioman.h"
#include "synthseis.h"
#include "seistrc.h"
#include "wavelet.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "welltiedata.h"
#include "welltieunitfactors.h"
#include "welltieextractdata.h"


namespace WellTie
{
static const int cDefTimeResampFac = 20;

#define mGetWD() { wd_ = data_.wd_; if ( !wd_ ) return false; }
DataPlayer::DataPlayer( Data& data, const MultiID& seisid, const LineKey* lk ) 
    : data_(data)		    
    , seisid_(seisid)
    , linekey_(lk)
{
    disprg_ = data_.timeintv_;
    dispsz_ = disprg_.nrSteps();

    workrg_ = disprg_; workrg_.step /= cDefTimeResampFac;
    worksz_ = workrg_.nrSteps();
}


bool DataPlayer::computeAll()
{
    mGetWD()

    if ( !setAIModel() || !doFullSynthetics()  || !extractSeismics() )
	return false;

    copyDataToLogSet();

    return true;
}


#define mErrRet(msg) { errmsg_ = msg; return false; }
bool DataPlayer::processLog( const Well::Log* log, 
			     Well::Log& outplog, const char* nm ) 
{
    BufferString msg;
    if ( !log ) 
	{ msg += "Can not find "; msg += nm; mErrRet( msg ); }

    const int sz = log->size();
    if ( sz <= 2 )
    {
	msg += nm;
	msg +="log size too small, please check your input log";
	mErrRet(msg)
    }

    outplog.setUnitMeasLabel( log->unitMeasLabel() );

    for ( int idx=1; idx<sz-1; idx++ )
    {
	const float prvval = log->value( idx-1 );
	const float curval = log->value( idx );
	const float nxtval = log->value( idx+1 );
	outplog.addValue( log->dah(idx), (prvval + curval + nxtval )/3 );
    }

    GeoCalculator gc; 
    gc.removeSpikes( outplog.valArr(), sz, 10, 3 );

    return true;
}


bool DataPlayer::setAIModel()
{
    aimodel_.erase();

    const Well::Log* sonlog = wd_->logs().getLog( data_.usedsonic() );
    const Well::Log* denlog = wd_->logs().getLog( data_.density() );

    Well::Log pslog, pdlog;
    if ( !processLog( sonlog, pslog, data_.usedsonic() ) 
	    || !processLog( denlog, pdlog, data_.density() ) )
	return false;

    if ( data_.isSonic() )
	{ GeoCalculator gc; gc.son2Vel( pslog, true ); }

    if ( !wd_->d2TModel() )
	mErrRet( "No depth/time model computed" );

    for ( int idx=0; idx<worksz_; idx++ )
    {
	const float dah0 = wd_->d2TModel()->getDah( workrg_.atIndex( idx ) );
	const float dah1 = wd_->d2TModel()->getDah( workrg_.atIndex( idx+1 ) );
	const float vel = pslog.getValue( dah1, true ); 
	const float den = pdlog.getValue( dah1, true ); 
	aimodel_ += AILayer( fabs( dah1-dah0 ), vel, den );
    }
    return true;
}


bool DataPlayer::doFullSynthetics()
{
    refmodel_.erase();
    const Wavelet& wvlt = data_.isinitwvltactive_ ? data_.initwvlt_ 
						  : data_.estimatedwvlt_;
    Seis::RaySynthGenerator gen;
    gen.addModel( aimodel_ );
    gen.setWavelet( &wvlt, OD::UsePtr );
    gen.setOutSampling( disprg_ );

    if ( !gen.doRayTracing( data_.trunner_ ) )
	mErrRet( gen.errMsg() )

    Seis::RaySynthGenerator::RayModel& rm = gen.result( 0 );
    rm.forceReflTimes( workrg_ );

    if ( !gen.doSynthetics( data_.trunner_ ) )
	mErrRet( gen.errMsg() )

    rm.getSampledRefs( reflvals_ );
    data_.synthtrc_ = *rm.stackedTrc();
    for ( int idx=0; idx<reflvals_.size(); idx++ )
    {
	refmodel_ += ReflectivitySpike(); 
	refmodel_[idx].reflectivity_ = reflvals_[idx];
    }

    return true;
}


bool DataPlayer::doFastSynthetics()
{
    const Wavelet& wvlt = data_.isinitwvltactive_ ? data_.initwvlt_ 
						  : data_.estimatedwvlt_;
    Seis::SynthGenerator gen;
    gen.setDoResample( false );
    gen.setConvolDomain( false );
    gen.setModel( refmodel_ );
    gen.setWavelet( &wvlt, OD::UsePtr );
    gen.setOutSampling( disprg_ );

    if ( !gen.doWork() )
	mErrRet( gen.errMsg() )

    data_.synthtrc_ = *new SeisTrc( gen.result() );

    return true;
}


bool DataPlayer::extractSeismics()
{
    Well::SimpleTrackSampler wtextr( wd_->track(), wd_->d2TModel() );
    wtextr.setSampling( disprg_ );
    data_.trunner_->execute( wtextr ); 

    const IOObj& ioobj = *IOM().get( seisid_ );
    IOObj* seisobj = ioobj.clone();

    SeismicExtractor seisextr( *seisobj );
    if ( linekey_ )
	seisextr.setLineKey( linekey_ );
    TypeSet<BinID> bids;  wtextr.getBIDs( bids );
    seisextr.setBIDValues( bids );
    seisextr.setInterval( disprg_ );
    data_.trunner_->execute( seisextr );
    data_.seistrc_.copyDataFrom( seisextr.result() );
    return true;
}


bool DataPlayer::copyDataToLogSet()
{
    if ( aimodel_.isEmpty() ) 
	mErrRet( "No data found" )

    TypeSet<float> dah, son, den, ai, synth, refs;
    StepInterval<float> workintv = data_.timeintv_; 
    for ( int idx=0; idx<dispsz_; idx++ )
    {
	const int workidx = idx*cDefTimeResampFac;
	const AILayer& layer = aimodel_[workidx];
	dah += wd_->d2TModel()->getDah( workrg_.atIndex(workidx) );
	son += layer.vel_;
	den += layer.den_;
	ai += layer.vel_*layer.den_;
	refs += reflvals_.validIdx( idx ) ? reflvals_[idx] : 0;
    }
    createLog( data_.usedsonic(), dah.arr(), son.arr(), son.size() ); 
    createLog( data_.density(), dah.arr(), den.arr(), den.size() ); 
    createLog( data_.ai(), dah.arr(), ai.arr(), ai.size() );
    createLog( data_.reflectivity(), dah.arr(), refs.arr(), refs.size()  );

    if ( data_.isSonic() )
    {
	Well::Log* vellog = data_.logset_.getLog( data_.usedsonic() );
	if ( vellog )
	{ 
	    vellog->setUnitMeasLabel( UnitFactors::getStdVelLabel() );
	    GeoCalculator gc; 
	    gc.son2Vel( *vellog, false ); 
	}
    }
    return true;
}


void DataPlayer::createLog( const char* nm, float* dah, float* vals, int sz )
{
    Well::Log* log = 0;
    if ( data_.logset_.indexOf( nm ) < 0 ) 
    {
	log = new Well::Log( nm );
	data_.logset_.add( log );
	const Well::Log* wdlog = wd_->logs().getLog( nm );
    }
    else
	log = data_.logset_.getLog( nm );

    log->erase();

    for( int idx=0; idx<sz; idx ++)
	log->addValue( dah[idx], vals[idx] );
}


}; //namespace WellTie
