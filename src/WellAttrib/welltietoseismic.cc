/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "welltietoseismic.h"

#include "ioman.h"
#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "raytrace1d.h"
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
#include "welltrack.h"


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

    bool success = setAIModel() && doFullSynthetics() && extractSeismics(); 
    copyDataToLogSet();

    computeAdditionalInfo( disprg_ );

    return success;
}


#define mErrRet(msg) { errmsg_ = msg; return false; }
bool DataPlayer::processLog( const Well::Log* log, 
			     Well::Log& outplog, const char* nm ) 
{
    BufferString msg;
    if ( !log ) 
	{ msg += "Can not find "; msg += nm; mErrRet( msg ); }

    outplog.setUnitMeasLabel( log->unitMeasLabel() );

    int sz = log->size();
    for ( int idx=1; idx<sz-1; idx++ )
    {
	const float prvval = log->value( idx-1 );
	const float curval = log->value( idx );
	const float nxtval = log->value( idx+1 );
	if ( mIsUdf(prvval) || mIsUdf(curval) || mIsUdf(nxtval) )
	    continue;

	outplog.addValue( log->dah(idx), (prvval + curval + nxtval )/3 );
    }
    sz = outplog.size();
    if ( sz <= 2 )
    {
	msg += nm;
	msg +="log size too small, please check your input log";
	mErrRet(msg)
    }

    GeoCalculator gc; 
    gc.removeSpikes( outplog.valArr(), sz, 10, 3 );

    return true;
}


bool DataPlayer::setAIModel()
{
    aimodel_.erase();

    const Well::Log* sonlog = wd_->logs().getLog( data_.sonic() );
    const Well::Log* denlog = wd_->logs().getLog( data_.density() );

    Well::Log pslog, pdlog;
    if ( !processLog( sonlog, pslog, data_.sonic() ) 
	    || !processLog( denlog, pdlog, data_.density() ) )
	return false;

    if ( data_.isSonic() )
	{ GeoCalculator gc; gc.son2Vel( pslog, true ); }

    if ( !wd_->d2TModel() )
	mErrRet( "No depth/time model computed" );

    for ( int idx=0; idx<worksz_; idx++ )
    {
	const float dah0 = wd_->d2TModel()->getDah(workrg_.atIndex(idx) );
	const float dah1 = wd_->d2TModel()->getDah(workrg_.atIndex(idx+1) );
	const bool inside = data_.dahrg_.includes(dah1,true);
	const float vel = inside ? pslog.getValue(dah1,true) : mUdf(float);
	const float den = inside ? pdlog.getValue(dah1,true) : mUdf(float);
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
    IOPar par;
    const float replveldz = -1. * wd_->info().surfaceelev
			    - wd_->track().getKbElev();
    const float sourrecz = replveldz > 0 ?
			   wd_->info().surfaceelev :
			   -1. * wd_->track().getKbElev();
    par.set(RayTracer1D::sKeySRDepth(),sourrecz,sourrecz);
    gen.usePar( par ); 
    gen.setTaskRunner( data_.trunner_ );
    if ( !gen.doRayTracing() )
	mErrRet( gen.errMsg() )

    Seis::RaySynthGenerator::RayModel& rm = gen.result( 0 );
    StepInterval<float> reflrg = disprg_;
    reflrg.step  = workrg_.step;
    reflrg.start = wd_->d2TModel()->getTime( data_.dahrg_.start, wd_->track() );
    reflrg.stop  = wd_->d2TModel()->getTime( data_.dahrg_.stop, wd_->track() );
    if ( disprg_.start > reflrg.start )
	reflrg.start = disprg_.start; 
    if ( disprg_.stop < reflrg.stop )
	reflrg.stop = disprg_.stop; 
    rm.forceReflTimes( reflrg );

    if ( !gen.doSynthetics() )
	mErrRet( gen.errMsg() )

    data_.synthtrc_ = *rm.stackedTrc();
    rm.getSampledRefs( reflvals_ );
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
    Well::SimpleTrackSampler wtextr( wd_->track(), wd_->d2TModel(), true, true);
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

    const bool success = data_.trunner_->execute( seisextr );
    data_.seistrc_ = SeisTrc( seisextr.result() );
    BufferString msg;
    if ( !success )
    { 
	msg += "Can not extract seismic: "; 
	msg += seisextr.errMsg(); 
	mErrRet( msg ); 
    }
    return success;
}


bool DataPlayer::copyDataToLogSet()
{
    if ( aimodel_.isEmpty() ) 
	mErrRet( "No data found" )

    TypeSet<float> dahlog, son, den, ai, synth, refs;
    for ( int idx=0; idx<dispsz_; idx++ )
    {
	const int workidx = idx*cDefTimeResampFac;
	const float dah = wd_->d2TModel()->getDah(workrg_.atIndex(workidx));
	const bool inside = data_.dahrg_.includes( dah, true );
	if ( !inside )
	    continue;

	dahlog += dah;
	refs += reflvals_.validIdx( idx ) ? reflvals_[idx] : mUdf(float);
	synth += data_.synthtrc_.size() > idx ? data_.synthtrc_.get(idx,0) 
					      : mUdf(float);
	const AILayer& layer = aimodel_[workidx];
	son += layer.vel_;
	den += layer.den_;
	ai += layer.vel_*layer.den_;
    }
    createLog( data_.sonic(), dahlog.arr(), son.arr(), son.size() ); 
    createLog( data_.density(), dahlog.arr(), den.arr(), den.size() ); 
    createLog( data_.ai(), dahlog.arr(), ai.arr(), ai.size() );
    createLog( data_.reflectivity(), dahlog.arr(), refs.arr(), refs.size()  );
    createLog( data_.synthetic(), dahlog.arr(), synth.arr(), synth.size()  );

    if ( data_.isSonic() )
    {
	Well::Log* vellog = data_.logset_.getLog( data_.sonic() );
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


#define mDelAndReturn(yn) { delete [] seisarr;  delete [] syntharr; return yn;}
bool DataPlayer::computeAdditionalInfo( const Interval<float>& zrg )  
{
    const float step = disprg_.step;
    const int nrsamps = mNINT32( zrg.width()/step )+1;
    const int istart = mNINT32( zrg.start/step );

    if ( nrsamps <= 1 )
    {
	errmsg_ = "Invalid time or depth range specified";
	return false;
    }

    Data::CorrelData& cd = data_.correl_;
    cd.vals_.setSize( nrsamps, 0 ); 

    mDeclareAndTryAlloc( float*, seisarr, float[nrsamps] );
    mDeclareAndTryAlloc( float*, syntharr, float[nrsamps] );

    for ( int idx=0; idx<nrsamps; idx++ )
    {
	if ( idx+istart >= data_.synthtrc_.size() ) break;
	syntharr[idx] = data_.synthtrc_.get( idx + istart, 0 );
	seisarr[idx] =  data_.seistrc_.get( idx + istart, 0 );
    }
    GeoCalculator gc;
    cd.coeff_ = gc.crossCorr( seisarr, syntharr, cd.vals_.arr(), nrsamps );
    if ( data_.isinitwvltactive_ )
    {
	int wvltsz = data_.estimatedwvlt_.size();
	wvltsz += wvltsz%2 ? 0 : 1;
	data_.estimatedwvlt_.reSize( wvltsz );
	if ( data_.timeintv_.nrSteps() < wvltsz )
	{
	    errmsg_ = "Seismic trace shorter than wavelet";
	    mDelAndReturn(false)
	}

	const Well::Log* log = data_.logset_.getLog( data_.reflectivity() );
	if ( !log )
	{
	    errmsg_ = "No reflectivity to estimate wavelet";
	    mDelAndReturn(false);
	}

	mDeclareAndTryAlloc( float*, refarr, float[nrsamps] );
	mDeclareAndTryAlloc( float*, wvltarr, float[nrsamps] );
	mDeclareAndTryAlloc( float*, wvltshiftedarr, float[wvltsz] );

	mGetWD()

	const Well::D2TModel* d2t = wd_->d2TModel();
	for ( int idx=0; idx<nrsamps; idx++ )
	    refarr[idx]= log->getValue(d2t->getDah(zrg.atIndex(idx,step)),true);

	gc.deconvolve( seisarr, refarr, wvltarr, nrsamps );
	for ( int idx=0; idx<wvltsz; idx++ )
	    wvltshiftedarr[idx] = wvltarr[(nrsamps-wvltsz)/2 + idx];

	Array1DImpl<float> wvltvals( wvltsz );
	memcpy( wvltvals.getData(), wvltshiftedarr, wvltsz*sizeof(float) );
	ArrayNDWindow window( Array1DInfoImpl(wvltsz), false, "CosTaper", .05 );
	window.apply( &wvltvals );
	memcpy( data_.estimatedwvlt_.samples(),
	wvltvals.getData(), wvltsz*sizeof(float) );
	delete [] wvltarr;      delete [] wvltshiftedarr;  delete [] refarr;
    }
    mDelAndReturn(true)
}

}; //namespace WellTie
