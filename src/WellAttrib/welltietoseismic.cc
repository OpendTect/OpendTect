/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

#include "welltietoseismic.h"

#include "ioman.h"
#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "envvars.h"
#include "raytrace1d.h"
#include "synthseis.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "statruncalc.h"
#include "ctxtioobj.h"
#include "wavelet.h"
#include "welldata.h"
#include "wellelasticmodelcomputer.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "welltiedata.h"
#include "welltieextractdata.h"
#include "welltiegeocalculator.h"
#include "welltrack.h"

static const char* sKeyAdvancedRayTracer()	{ return "FullRayTracer"; }

namespace WellTie
{
#define mErrRet(msg) \
{ \
    if ( errmsg_.isEmpty() ) \
	errmsg_ = msg; \
    else \
	errmsg_.append( msg, true ); \
\
    return false; \
}

DataPlayer::DataPlayer( Data& data, const MultiID& seisid,
			const BufferString& lnm )
    : data_(data)
    , seisid_(seisid)
    , linenm_(lnm)
    , seisarr_(0)
    , syntarr_(0)
    , refarr_(0)
{
    zrg_.set( mUdf(float), mUdf(float) );
}


DataPlayer::~DataPlayer()
{
    if ( refarr_ )
	delete [] refarr_;

    if ( syntarr_ )
	delete [] syntarr_;

    if ( seisarr_ )
	delete [] seisarr_;
}


bool DataPlayer::computeSynthetics( const Wavelet& wvlt )
{
    errmsg_.setEmpty();

    if ( !data_.wd_ )
	mErrRet( tr( "Cannot read well data" ) )

    if ( !data_.wd_->d2TModel() )
	mErrRet( tr( "No depth/time model computed" ) )

    if ( !setAIModel() )
	mErrRet( tr( "Could not setup for raytracing" ) )

    if ( !doFullSynthetics(wvlt) )
	mErrRet( tr( "Could not compute the synthetic trace" ) )

    if ( !copyDataToLogSet() )
	mErrRet( tr( "Could not copy the AI model to composite logs" ) )

    return true;
}


bool DataPlayer::extractSeismics()
{
    errmsg_.setEmpty();

    const PtrMan<IOObj> seisobj = IOM().get( seisid_ );
    const SeisIOObjInfo oinf( seisid_ );
    if ( !seisobj || !oinf.isOK() )
	mErrRet( tr( "Cannot read seismic data" ) )

    TrcKeyZSampling cs;
    oinf.getRanges( cs );
    const StepInterval<float> tracerg = data_.getTraceRange();
    StepInterval<float> seisrg( tracerg.start, tracerg.stop, cs.zsamp_.step );

    Well::SimpleTrackSampler wtextr( data_.wd_->track(), data_.wd_->d2TModel(),
				     true, false );
    wtextr.setSampling( seisrg );
    TaskRunner::execute( data_.trunner_, wtextr );

    SeismicExtractor seisextr( *seisobj );
    if ( !linenm_.isEmpty() )
	seisextr.setLine( linenm_ );

    TypeSet<BinID> bids;  wtextr.getBIDs( bids );
    seisextr.setBIDValues( bids );
    seisextr.setInterval( seisrg );

    if ( !TaskRunner::execute(data_.trunner_,seisextr) )
	mErrRet( tr( "Can not extract seismic: %1" ).arg( seisextr.errMsg() ) )

    SeisTrc rawseis = SeisTrc( seisextr.result() );
    const int newsz = tracerg.nrSteps()+1;
    data_.seistrc_ = SeisTrc( newsz );
    data_.seistrc_.info().sampling = tracerg;
    for ( int idx=0; idx<newsz; idx++ )
    {
	const float twt = tracerg.atIndex(idx);
	const float outval = rawseis.getValue( twt, 0 );
	data_.seistrc_.set( idx, outval, 0 );
    }

    return true;
}


bool DataPlayer::doFastSynthetics( const Wavelet& wvlt )
{
    errmsg_.setEmpty();

    Seis::SynthGenerator gen;
    gen.setModel( refmodel_ );
    gen.setWavelet( &wvlt, OD::UsePtr );
    gen.enableFourierDomain( !GetEnvVarYN("DTECT_CONVOLVE_USETIME") );
    gen.setOutSampling( data_.getTraceRange() );

    if ( !gen.doWork() )
	mErrRet( tr( "Cannot update synthetics: %1" ).arg( gen.errMsg() ) )

    data_.synthtrc_ = *new SeisTrc( gen.result() );

    return true;
}


bool DataPlayer::computeAdditionalInfo( const Interval<float>& zrg )
{
    setCrossCorrZrg( zrg );
    if ( !checkCrossCorrInps() )
	return false;

    if ( !computeCrossCorrelation() || !computeEstimatedWavelet(0) )
	return false;

    return true;
}


bool DataPlayer::checkCrossCorrInps()
{
    errmsg_.setEmpty();

    if ( zrg_.isUdf() )
	mErrRet( tr( "Cross-correlation window not set" ) )

    if ( !data_.seistrc_.zRange().isEqual(data_.synthtrc_.zRange(), 1e-2f) )
	mErrRet( tr( "Synthetic and seismic traces do not have same length" ) )

    if ( !isOKSynthetic() && !isOKSeismic() )
	mErrRet( tr( "Seismic/Synthetic data too short" ) )

    const int istartseis = data_.seistrc_.nearestSample( zrg_.start );
    const int istopseis = data_.seistrc_.nearestSample( zrg_.stop );
    const int nrsamps = istopseis - istartseis + 1;
    if ( nrsamps < 2 )
	mErrRet( tr( "Cross-correlation too short" ) )

    if ( zrg_.start < data_.seistrc_.startPos() ||
	 zrg_.stop > data_.seistrc_.endPos() )
    {
	const uiString msg = tr( "The cross-correlation window must be smaller "
				 "than the synthetic/seismic traces" );
	mErrRet( msg )
    }

    // clip to nearest sample
    zrg_.start = data_.seistrc_.samplePos( istartseis );
    zrg_.stop = data_.seistrc_.samplePos( istopseis );

    return true;
}


bool DataPlayer::computeCrossCorrelation()
{
    errmsg_.setEmpty();

    if ( zrg_.isUdf() )
	mErrRet( tr( "Cross-correlation window not set" ) )

    if ( zrg_.isRev() )
	mErrRet( tr( "Cross-correlation window is not valid" ) )

    if ( !extractWvf(false) )
	mErrRet( tr( "Cannot extract seismic for cross-correlation" ) )

    if ( !extractWvf(true) )
	mErrRet( tr( "Cannot extract synthetic for cross-correlation" ) )

    Data::CorrelData& cd = data_.correl_;
    cd.vals_.erase();
    const float step = data_.seistrc_.info().sampling.step;
    const int nrsamps = mNINT32( zrg_.width(false) / step ) + 1;
    cd.vals_.setSize( nrsamps, 0 );
    GeoCalculator gccc;
    cd.coeff_ = gccc.crossCorr( seisarr_, syntarr_, cd.vals_.arr(), nrsamps );

    return true;
}


bool DataPlayer::computeEstimatedWavelet( int wvltsz )
{
    errmsg_.setEmpty();

    if ( zrg_.isUdf() )
	mErrRet( tr( "Wavelet estimation window not set" ) )

    if ( zrg_.isRev() )
	mErrRet( tr( "Wavelet estimation window is not valid" ) )

    if ( !extractReflectivity() )
	mErrRet( tr( "Cannot extract reflectivity for wavelet estimation" ) )

    if ( !extractWvf(false) )
	mErrRet( tr( "Cannot extract seismic for wavelet estimation" ) )

    const float step = data_.seistrc_.info().sampling.step;
    const int nrsamps = mNINT32( zrg_.width(false) / step ) + 1;
    mDeclareAndTryAlloc( float*, wvltarrfull, float[nrsamps] );
    if ( !wvltarrfull )
	mErrRet( tr( "Cannot allocate memory for estimated wavelet" ) )

    GeoCalculator gcwvltest;
    gcwvltest.deconvolve( seisarr_, refarr_, wvltarrfull, nrsamps );

    const int outwvltsz = wvltsz%2 ? wvltsz : wvltsz + 1;
    Array1DImpl<float> wvltarr( outwvltsz );
    const int nrsampshift = ( nrsamps - outwvltsz + 1 ) / 2;
    for ( int idx=0; idx<outwvltsz; idx++ )
	wvltarr.set( idx, wvltarrfull[nrsampshift + idx] );

    ArrayNDWindow window( Array1DInfoImpl(outwvltsz), false, "CosTaper",
			  0.90 );
    window.apply( &wvltarr );

    data_.estimatedwvlt_.setSampleRate( step );
    data_.estimatedwvlt_.setCenterSample( (outwvltsz-1)/2 );
    data_.estimatedwvlt_.reSize( outwvltsz );
    OD::memCopy( data_.estimatedwvlt_.samples(), wvltarr.getData(),
		outwvltsz*sizeof(float) );
    delete [] wvltarrfull;

    return true;
}


bool DataPlayer::extractWvf( bool issynt )
{
    errmsg_.setEmpty();

    if ( zrg_.isUdf() )
	mErrRet( tr( "Waveform extraction window not set" ) )

    const SeisTrc& trace = issynt ? data_.synthtrc_ : data_.seistrc_;
    const int istartseis = trace.nearestSample( zrg_.start );
    const int istopseis = trace.nearestSample( zrg_.stop );
    const int nrsamps = istopseis - istartseis + 1;
    mDeclareAndTryAlloc( float*, valarr, float[nrsamps] );
    if ( !valarr )
	mErrRet( tr( "Internal: Cannot allocate memory" ) )

    int idy = 0;
    Stats::CalcSetup scalercalc;
    scalercalc.require( Stats::RMS );
    Stats::RunCalc<double> stats( scalercalc );
    for ( int idseis=istartseis; idseis<=istopseis; idseis++ )
    {
	const float val = trace.get( idseis, 0 );
	valarr[idy] = val;
	stats += val;
	idy++;
    }
    const float wvfrms = mCast( float, stats.rms() );
    if ( issynt )
    {
	if ( syntarr_ )
	    delete [] syntarr_;

	syntarr_ = valarr;
	data_.correl_.scaler_ = mIsUdf(wvfrms) ? mUdf(float)
			      : data_.correl_.scaler_ / wvfrms;
    }
    else
    {
	if ( seisarr_ )
	    delete [] seisarr_;

	seisarr_ = valarr;
	data_.correl_.scaler_ = wvfrms;
    }

    return true;
}


bool DataPlayer::extractReflectivity()
{
    errmsg_.setEmpty();

    if ( zrg_.isUdf() )
	mErrRet( tr( "Extraction window not set for reflectivity computation") )

    const float step = data_.seistrc_.info().sampling.step;
    const int nrsamps = mNINT32( zrg_.width(false) / step ) + 1;
    const int totnrspikes = refmodel_.size();
    if ( totnrspikes < nrsamps )
	mErrRet( tr( "Reflectivity series too short" ) )

    int firstspike = 0;
    int lastspike = 0;
    while ( lastspike<refmodel_.size() )
    {
	const ReflectivitySpike spike = refmodel_[lastspike];
	const float spiketwt = spike.correctedtime_;
	if ( mIsEqual(spiketwt,zrg_.stop,1e-5f) )
	    break;

	if ( spiketwt - zrg_.start < -1e-5f )
	    firstspike++;

	lastspike++;
    }

    uiString msg;
    if ( refmodel_[firstspike].correctedtime_ - zrg_.start < -1e-5f )
    {
	msg = tr( "The wavelet estimation window must start "
		  "above the first spike at %1 ms" )
			.arg( toString(refmodel_[firstspike].correctedtime_) );
	mErrRet( msg )
    }

    if ( refmodel_[lastspike].correctedtime_ - zrg_.stop > 1e-5f )
    {
	msg = tr( "The wavelet estimation window must stop "
		  "before the last spike at %1 ms" )
			.arg( toString(refmodel_[lastspike].correctedtime_) );
	mErrRet( msg )
    }

    if ( (lastspike-firstspike+1) != nrsamps )
    {
	msg = tr( "The wavelet estimation window must be"
		  " smaller than the reflectivity series" );
	mErrRet( msg )
    }

    mDeclareAndTryAlloc( float_complex*, valarr, float_complex[nrsamps] );
    if ( !valarr )
	mErrRet( tr( "Cannot allocate memory for reflectivity series" ) )

    int nrspikefound = 0;
    for ( int idsp=firstspike; idsp<=lastspike; idsp++ )
    {
	const ReflectivitySpike spike = refmodel_[idsp];
	const float twtspike = spike.correctedtime_;
	if ( !mIsEqual(twtspike,zrg_.atIndex(nrspikefound,step),1e-5f) )
	{
	    delete [] valarr;
	    mErrRet( tr( "Mismatch between spike twt and seismic twt" ) )
	}

	valarr[nrspikefound] = spike.isDefined()
			     ? spike.reflectivity_ : float_complex( 0., 0. );
	nrspikefound++;
    }

    if ( refarr_ )
	delete [] refarr_;

    refarr_ = valarr;
    return true;
}


bool DataPlayer::isOKSynthetic() const
{
    return data_.synthtrc_.size();
}


bool DataPlayer::isOKSeismic() const
{
    return data_.seistrc_.size();
}


bool DataPlayer::hasSeisId() const
{
    return !seisid_.isEmpty();
}


bool DataPlayer::setAIModel()
{
    const Well::Log* sonlog = data_.wd_->logs().getLog( data_.sKeySonic() );
    const Well::Log* denlog = data_.wd_->logs().getLog( data_.sKeyDensity());

    Well::Log* pcvellog = new Well::Log;
    Well::Log* pcdenlog = new Well::Log;
    if ( !processLog(sonlog,*pcvellog,data_.sKeySonic()) ||
	 !processLog(denlog,*pcdenlog,data_.sKeyDensity()) )
	return false;

    if ( data_.isSonic() )
    {
	GeoCalculator gc;
	gc.son2Vel( *pcvellog );
    }

    aimodel_.erase();
    Well::ElasticModelComputer emodelcomputer( *data_.wd_ );
    emodelcomputer.setVelLog( *pcvellog );
    emodelcomputer.setDenLog( *pcdenlog );
    emodelcomputer.setZrange( data_.getModelRange(), true );
    emodelcomputer.setExtractionPars( data_.getModelRange().step, true );
    uiString doeditmsg( tr("Please consider editing your logs.") );
    if ( !emodelcomputer.computeFromLogs() )
	mErrRet( uiString( emodelcomputer.errMsg() ).append(doeditmsg,true) )

    if ( emodelcomputer.warnMsg().isSet() )
	warnmsg_ = uiString( emodelcomputer.warnMsg() ).append(doeditmsg,true);

    aimodel_ = emodelcomputer.elasticModel();

    return true;
}


bool DataPlayer::doFullSynthetics( const Wavelet& wvlt )
{
    errmsg_.setEmpty();
    uiString msg;

    refmodel_.erase();
    TypeSet<ElasticModel> aimodels;
    aimodels += aimodel_;
    Seis::RaySynthGenerator gen( &aimodels );
    gen.forceReflTimes( data_.getReflRange() );
    gen.setWavelet( &wvlt, OD::UsePtr );
    gen.setOutSampling( data_.getTraceRange() );
    gen.enableFourierDomain( !GetEnvVarYN("DTECT_CONVOLVE_USETIME") );
    IOPar par;
    FixedString defrayparstr = sKeyAdvancedRayTracer();
    const BufferStringSet& facnms = RayTracer1D::factory().getNames();
    if ( !facnms.isEmpty() )
    {
	const int typeidx = facnms.indexOf( defrayparstr );
	FixedString facnm( typeidx>=0 ? facnms.get(typeidx) : facnms.get(0) );
	par.set( sKey::Type(), facnm );
    }

    gen.usePar( par );
    TaskRunner* taskrunner = data_.trunner_;
    if ( !TaskRunner::execute(taskrunner,gen) )
	mErrRet( uiStrings::phrCannotCreate(
		tr("synthetic: %1").arg(gen.errMsg())) )

    Seis::RaySynthGenerator::RayModel& rm = gen.result( 0 );
    ObjectSet<const ReflectivityModel> refmodels;
    rm.getRefs( refmodels, true );
    if ( refmodels.isEmpty() )
	mErrRet( tr("Could not retrieve the reflectivities after ray-tracing") )

    refmodel_ = *refmodels[0];
    data_.synthtrc_ = *rm.stackedTrc();
    data_.setTraceRange( data_.synthtrc_.zRange() );
    //requested range was too small

    return true;
}


bool DataPlayer::copyDataToLogSet()
{
    errmsg_.setEmpty();

    if ( aimodel_.isEmpty() )
	mErrRet( tr("Internal: No data found") )

    data_.logset_.setEmpty();
    const StepInterval<float> dahrg = data_.getDahRange();

    TypeSet<float> dahlog, son, den, ai;
    for ( int idx=0; idx<aimodel_.size(); idx++ )
    {
	const float twt = data_.getModelRange().atIndex(idx);
	const float dah = data_.wd_->d2TModel()->getDah( twt,
							 data_.wd_->track() );
	if ( !dahrg.includes(dah,true) )
	    continue;

	dahlog += dah;
	const AILayer& layer = aimodel_[idx];
	son += layer.vel_;
	den += layer.den_;
	ai += layer.getAI();
    }

    createLog( data_.sKeySonic(), dahlog.arr(), son.arr(), son.size() );
    createLog( data_.sKeyDensity(), dahlog.arr(), den.arr(), den.size() );
    createLog( data_.sKeyAI(), dahlog.arr(), ai.arr(), ai.size() );

    TypeSet<float> dahref, refs;
    for ( int idx=0; idx<refmodel_.size(); idx++ )
    {
	const ReflectivitySpike spike = refmodel_[idx];
	if ( !spike.isDefined() )
	    continue;

	const float twt = spike.correctedtime_;
	const float dah = data_.wd_->d2TModel()->getDah( twt,
							 data_.wd_->track() );
	if ( !dahrg.includes(dah,true) )
	    continue;

	dahref += dah;
	refs += spike.reflectivity_.real();
    }

    createLog( data_.sKeyReflectivity(), dahref.arr(), refs.arr(),
	       refs.size() );

    TypeSet<float> dahsynth, synth;
    const StepInterval<float> tracerg = data_.getTraceRange();
    for ( int idx=0; idx<=data_.synthtrc_.size(); idx++ )
    {
	const float twt = tracerg.atIndex( idx );
	const float dah = data_.wd_->d2TModel()->getDah( twt,
							 data_.wd_->track() );
	if ( !dahrg.includes(dah,true) )
	    continue;

	dahsynth += dah;
	synth += data_.synthtrc_.get( idx, 0 );
    }

    createLog( data_.sKeySynthetic(), dahsynth.arr(), synth.arr(),synth.size());

    const Well::Log* sonlog = data_.wd_->logs().getLog( data_.sKeySonic() );
    const UnitOfMeasure* sonuom = sonlog ? sonlog->unitOfMeasure() : 0;
    Well::Log* vellogfrommodel = data_.logset_.getLog( data_.sKeySonic() );
    if ( vellogfrommodel && sonlog )
    {
	if ( data_.isSonic() )
	{
	    GeoCalculator gc;
	    gc.son2Vel( *vellogfrommodel );
	}
	vellogfrommodel->convertTo( sonuom );
    }

    const Well::Log* denlog = data_.wd_->logs().getLog( data_.sKeyDensity());
    const UnitOfMeasure* denuom = denlog ? denlog->unitOfMeasure() : 0;
    Well::Log* denlogfrommodel = data_.logset_.getLog( data_.sKeyDensity() );
    if ( denlogfrommodel && denlog )
    {
	const UnitOfMeasure* denuomfrommodel =
				UoMR().getInternalFor(Mnemonic::Den);
	if ( denuomfrommodel )
	{
	    denlogfrommodel->setUnitMeasLabel( denuomfrommodel->symbol() );
	    denlogfrommodel->setMnemonic( Mnemonic::defDEN() );
	}

	denlogfrommodel->convertTo( denuom );
    }

    Well::Log* ailogfrommodel = data_.logset_.getLog( data_.sKeyAI() );
    if ( ailogfrommodel && sonuom && denuom )
    {
	const Mnemonic::StdType& impprop = Mnemonic::Imp;
	const UnitOfMeasure* aiuomfrommodel = UoMR().getInternalFor( impprop );
	ailogfrommodel->setUnitMeasLabel( aiuomfrommodel->symbol() );
	ailogfrommodel->setMnemonic( Mnemonic::defAI() );
	float fact = mCast( float, denuom->scaler().factor );
	if ( sonuom->isImperial() )
	    fact *= mFromFeetFactorF;

	ObjectSet<const UnitOfMeasure> relevantunits;
	UoMR().getRelevant( impprop, relevantunits );
	const UnitOfMeasure* aiuom = 0;
	for ( int idx=0; idx<relevantunits.size(); idx++ )
	{
	    const float curfactor = (float)relevantunits[idx]->scaler().factor;
	    const float eps = curfactor / 100.f;
	    if ( mIsEqual(curfactor,fact,eps) )
		aiuom = relevantunits[idx];
	}

	if ( aiuom )
	    ailogfrommodel->convertTo( aiuom );
    }

    return true;
}


bool DataPlayer::processLog( const Well::Log* log,
			     Well::Log& outplog, const char* nm )
{
    errmsg_.setEmpty();
    uiString msg;

    if ( !log )
	mErrRet( tr( "Can not find log '%1'" ).arg( nm ) )

    outplog.setMnemLabel( log->mnemLabel() );
    outplog.setUnitMeasLabel( log->unitMeasLabel() );

    int sz = log->size();
    for ( int idx=0; idx<sz; idx++ )
    {
	const float logval = log->value( idx );
	if ( mIsUdf(logval) )
	    continue;

	outplog.addValue( log->dah(idx), logval );
    }

    sz = outplog.size();
    if ( sz <= 2 )
	mErrRet( tr( "%1: log size too small, please check your input log" )
		     .arg( nm ) )

    GeoCalculator gc;
    gc.removeSpikes( outplog.valArr(), sz, 10, 3 );
    outplog.setName( log->name() );

    return true;
}


void DataPlayer::createLog( const char* nm, float* dah, float* vals, int sz )
{
    Well::Log* log = 0;
    if ( data_.logset_.indexOf( nm ) < 0 )
    {
	log = new Well::Log( nm );
	data_.logset_.add( log );
    }
    else
	log = data_.logset_.getLog( nm );

    log->setEmpty();
    for( int idx=0; idx<sz; idx ++)
	log->addValue( dah[idx], vals[idx] );
}

} // namespace WellTie
