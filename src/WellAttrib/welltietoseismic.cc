/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

#include "welltietoseismic.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "envvars.h"
#include "synthseisgenerator.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "statruncalc.h"
#include "synthseisgenerator.h"
#include "ioobjctxt.h"
#include "ioobj.h"
#include "raysynthgenerator.h"
#include "raytracerrunner.h"
#include "reflectivitymodel.h"
#include "trckeyzsampling.h"
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
#include "welltietoseismic.h"
#include "welltrack.h"



namespace WellTie
{
#define mErrRet(msg) \
{ \
    if ( errmsg_.isEmpty() ) \
	errmsg_ = msg; \
    else \
	errmsg_.appendPhrase( msg ); \
\
    return false; \
}

DataPlayer::DataPlayer( Data& data, const DBKey& seisid,
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
	mErrRet( uiStrings::phrCannotRead(uiStrings::sWellData()) )

    if ( data_.wd_->d2TModel().isEmpty() )
	mErrRet( tr("No depth/time model computed") )

    if ( !setAIModel() )
	mErrRet( tr("Cannot setup for raytracing") )

    if ( !doFullSynthetics(wvlt) )
	mErrRet( tr("Cannot compute the synthetic trace") )

    if ( !copyDataToLogSet() )
	mErrRet( uiStrings::phrCannotCopy(tr(
					   "the AI model to composite logs")) )

    return true;
}


bool DataPlayer::extractSeismics()
{
    errmsg_.setEmpty();

    PtrMan<IOObj> seisobj = seisid_.getIOObj();
    const SeisIOObjInfo oinf( seisid_ );
    if ( !seisobj || !oinf.isOK() )
	mErrRet( uiStrings::phrCannotRead(uiStrings::sSeismicData()) )

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
	mErrRet( uiStrings::phrCannotExtract(uiStrings::sSeismics())
		.addMoreInfo( seisextr.errMsg(), true ) );

    SeisTrc rawseis = SeisTrc( seisextr.result() );
    const int newsz = tracerg.nrSteps()+1;
    data_.seistrc_ = SeisTrc( newsz );
    data_.seistrc_.info().sampling_ = tracerg;
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
    static const bool usefastfourierdom =
				!GetEnvVarYN("DTECT_CONVOLVE_USE_TIME");

    errmsg_.setEmpty();
    if ( !data_.sd_ )
	return false;

    RaySynthGenerator gen( *data_.sd_.ptr() );
    gen.setWavelet( &wvlt );
    IOPar genpar;
    genpar.setYN( SynthSeis::GenBase::sKeyFourier(), usefastfourierdom );
    gen.usePar( genpar );
    if ( !gen.execute() )
	mErrRet( tr("Cannot update synthetics").addMoreInfo(gen.message()) )

    data_.synthtrc_ = *data_.sd_->getTrace( 0 );

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
	mErrRet( tr("Cross-correlation window not set") )

    if ( !data_.seistrc_.zRange().isEqual(data_.synthtrc_.zRange(), 1e-2f) )
	mErrRet( tr("Synthetic and seismic traces do not have same length") )

    if ( !isOKSynthetic() && !isOKSeismic() )
	mErrRet( tr("Seismic/Synthetic data too short") )

    const int istartseis = data_.seistrc_.nearestSample( zrg_.start );
    const int istopseis = data_.seistrc_.nearestSample( zrg_.stop );
    const int nrsamps = istopseis - istartseis + 1;
    if ( nrsamps < 2 )
	mErrRet( tr("Cross-correlation is too short") )

    if ( zrg_.start < data_.seistrc_.startPos() ||
	 zrg_.stop > data_.seistrc_.endPos() )
    {
	const uiString msg = tr("The cross-correlation window must be smaller "
				"than the synthetic/seismic traces");
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
	mErrRet( tr("Cross-correlation window not set") )

    if ( zrg_.isRev() )
	mErrRet( tr("Cross-correlation window is not valid") )

    if ( !extractWvf(false) )
	mErrRet( tr("Cannot extract seismic for cross-correlation") )

    if ( !extractWvf(true) )
	mErrRet( tr("Cannot extract synthetic for cross-correlation") )

    Data::CorrelData& cd = data_.correl_;
    cd.vals_.erase();
    const float step = data_.seistrc_.info().sampling_.step;
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
	mErrRet( tr("Wavelet estimation window not set") )

    if ( zrg_.isRev() )
	mErrRet( tr("Wavelet estimation window is not valid") )

    if ( !extractReflectivity() )
	mErrRet( tr("Cannot extract reflectivity for wavelet estimation") )

    if ( !extractWvf(false) )
	mErrRet( tr("Cannot extract seismic for wavelet estimation") )

    const float step = data_.seistrc_.info().sampling_.step;
    const int nrsamps = mNINT32( zrg_.width(false) / step ) + 1;
    mDeclareAndTryAlloc( float*, wvltarrfull, float[nrsamps] );
    if ( !wvltarrfull )
	mErrRet( tr("Cannot allocate memory for estimated wavelet") )

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

    data_.estimatedwvlt_->setSampleRate( step );
    data_.estimatedwvlt_->setSamples( wvltarr.getData(), outwvltsz );
    data_.estimatedwvlt_->setCenterSample( (outwvltsz-1)/2 );
    delete [] wvltarrfull;

    return true;
}


bool DataPlayer::extractWvf( bool issynt )
{
    errmsg_.setEmpty();

    if ( zrg_.isUdf() )
	mErrRet( tr("Waveform extraction window not set") )

    const SeisTrc& trace = issynt ? data_.synthtrc_ : data_.seistrc_;
    const int istartseis = trace.nearestSample( zrg_.start );
    const int istopseis = trace.nearestSample( zrg_.stop );
    const int nrsamps = istopseis - istartseis + 1;
    mDeclareAndTryAlloc( float*, valarr, float[nrsamps] );
    if ( !valarr )
	mErrRet( uiStrings::phrCannotAllocateMemory() )

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
    if ( !data_.sd_ )
	mErrRet( tr("No synthetic data with reflectivities") )

    ConstRefMan<SynthSeis::RayModel> raymodel = data_.sd_->rayModels().get(0);
    const ReflectivityModel& reflmodel = *raymodel->reflModels().get(0);

    if ( zrg_.isUdf() )
	mErrRet( tr("Extraction window not set for reflectivity computation") )

    const float step = data_.seistrc_.info().sampling_.step;
    const int nrsamps = mNINT32( zrg_.width(false) / step ) + 1;
    const int totnrspikes = reflmodel.size();
    if ( totnrspikes < nrsamps )
	mErrRet( tr("Reflectivity series too short") )

    int firstspike = 0;
    int lastspike = 0;
    while ( lastspike<reflmodel.size() )
    {
	const ReflectivitySpike spike = reflmodel[lastspike];
	const float spiketwt = spike.correctedtime_;
	if ( mIsEqual(spiketwt,zrg_.stop,1e-5f) )
	    break;

	if ( spiketwt - zrg_.start < -1e-5f )
	    firstspike++;

	lastspike++;
    }

    if ( lastspike==reflmodel.size() )
	lastspike--;

    uiString msg;
    if ( reflmodel[firstspike].correctedtime_ - zrg_.start < -1e-5f )
    {
	msg = tr("The wavelet estimation window must start "
		  "above the first spike at %1 ms")
			.arg( toString(reflmodel[firstspike].correctedtime_) );
	mErrRet( msg )
    }

    if ( reflmodel[lastspike].correctedtime_ - zrg_.stop > 1e-5f )
    {
	msg = tr("The wavelet estimation window must stop "
		  "before the last spike at %1 ms")
			.arg( toString(reflmodel[lastspike].correctedtime_) );
	mErrRet( msg )
    }

    if ( (lastspike-firstspike+1) != nrsamps )
    {
	msg = tr("The wavelet estimation window must be"
		  " smaller than the reflectivity series");
	mErrRet( msg )
    }

    mDeclareAndTryAlloc( float_complex*, valarr, float_complex[nrsamps] );
    if ( !valarr )
	mErrRet( uiStrings::phrCannotAllocateMemory() )

    int nrspikefound = 0;
    for ( int idsp=firstspike; idsp<=lastspike; idsp++ )
    {
	const ReflectivitySpike spike = reflmodel[idsp];
	const float twtspike = spike.correctedtime_;
	if ( !mIsEqual(twtspike,zrg_.atIndex(nrspikefound,step),1e-5f) )
	{
	    delete [] valarr;
	    mErrRet( tr("Mismatch between spike twt and seismic twt") )
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
    return seisid_.isValid();
}


bool DataPlayer::setAIModel()
{
    const Well::Log* sonlog = data_.wd_->logs().getLogByName(
						    data_.sKeySonic() );
    const Well::Log* denlog = data_.wd_->logs().getLogByName(
						    data_.sKeyDensity());

    RefMan<Well::Log> pcvellog = new Well::Log;
    RefMan<Well::Log> pcdenlog = new Well::Log;
    if ( !processLog(sonlog,*pcvellog,data_.sKeySonic()) ||
	 !processLog(denlog,*pcdenlog,data_.sKeyDensity()) )
	return false;

    if ( data_.isSonic() )
    {
	GeoCalculator gc;
	gc.son2Vel( *pcvellog );
    }

    elasticmodel_.erase();
    Well::ElasticModelComputer emodelcomputer( *data_.wd_ );
    emodelcomputer.setVelLog( *pcvellog );
    emodelcomputer.setDenLog( *pcdenlog );
    emodelcomputer.setZrange( data_.getModelRange(), true );
    emodelcomputer.setExtractionPars( data_.getModelRange().step, true );
    uiString doeditmsg( tr("Please consider editing your logs") );
    if ( !emodelcomputer.computeFromLogs() )
	mErrRet( uiString( emodelcomputer.errMsg() )
				.appendPhrase(doeditmsg) )

    if ( !emodelcomputer.warnMsg().isEmpty() )
	warnmsg_ = uiString( emodelcomputer.warnMsg() )
						    .appendPhrase(doeditmsg);

    elasticmodel_ = emodelcomputer.elasticModel();

    return true;
}


bool DataPlayer::doFullSynthetics( const Wavelet& wvlt )
{
    errmsg_.setEmpty();
    uiString msg;
    TaskRunner* taskrunner = data_.trunner_;

    const bool updatedataset = data_.sd_;
    PtrMan<RaySynthGenerator> gen = 0;
    if ( updatedataset )
    {
	gen = new RaySynthGenerator( *data_.sd_.ptr() );
    }
    else
    {
	ElasticModelSet elmodels;
	elmodels += new ElasticModel( elasticmodel_ );
	IOPar raypars;
	RayTracerRunner raytracerunner( elmodels, raypars );
	if ( !taskrunner->execute(raytracerunner) )
	    mErrRet( uiStrings::phrCannotCreate(
			tr("synthetic: %1").arg(raytracerunner.message())) )

	const RefObjectSet<RayTracerData>& runnerres = raytracerunner.results();
	RefMan<SynthSeis::RayModelSet> raymodels = new SynthSeis::RayModelSet;
	for ( int imod=0; imod<runnerres.size(); imod++ )
	{
	    RefMan<SynthSeis::RayModel> raymodel =
				new SynthSeis::RayModel(*runnerres.get(imod) );
	    raymodel->forceReflTimes( data_.getReflRange() );
	    raymodels->add( raymodel.ptr() );
	}

	SynthSeis::GenParams sgp;
	sgp.setWaveletName( wvlt.name() );
	gen = new RaySynthGenerator( sgp, *raymodels.ptr() );
    }

    static const bool usefourierdom = !GetEnvVarYN("DTECT_CONVOLVE_USE_TIME");

    gen->setWavelet( &wvlt );
    gen->setOutSampling( data_.getTraceRange() );
    IOPar genpar;
    genpar.setYN( SynthSeis::GenBase::sKeyFourier(), usefourierdom );
    gen->usePar( genpar );

    if ( !taskrunner->execute(*gen) )
	mErrRet( uiStrings::phrCannotCreate(
		tr("synthetic: %1").arg(gen->message())) )

    if ( !gen->isResultOK() )
	return false;

    data_.sd_ = &gen->dataSet();
    data_.synthtrc_ = *data_.sd_->getTrace( 0 );
    data_.setTraceRange( data_.synthtrc_.zRange() );
    //requested range was too small

    return true;
}


bool DataPlayer::copyDataToLogSet()
{
    errmsg_.setEmpty();

    if ( elasticmodel_.isEmpty() || !data_.sd_ )
	mErrRet( toUiString("Internal: No data found") )

    data_.logset_.setEmpty();
    const StepInterval<float> dahrg = data_.getDahRange();

    TypeSet<float> dahlog, son, den, ai;
    for ( int idx=0; idx<elasticmodel_.size(); idx++ )
    {
	const float twt = data_.getModelRange().atIndex(idx);
	const float dah = data_.wd_->d2TModel().getDah(twt,data_.wd_->track());
	if ( !dahrg.includes(dah,true) )
	    continue;

	dahlog += dah;
	const AILayer& layer = elasticmodel_[idx];
	son += layer.vel_;
	den += layer.den_;
	ai += layer.getAI();
    }

    createLog( data_.sKeySonic(), dahlog.arr(), son.arr(), son.size() );
    createLog( data_.sKeyDensity(), dahlog.arr(), den.arr(), den.size() );
    createLog( data_.sKeyAI(), dahlog.arr(), ai.arr(), ai.size() );

    ConstRefMan<SynthSeis::RayModel> raymodel = data_.sd_->rayModels().get(0);
    const ReflectivityModel& reflmodel = *raymodel->reflModels().get(0);
    TypeSet<float> dahref, refs;
    for ( int idx=0; idx<reflmodel.size(); idx++ )
    {
	const ReflectivitySpike spike = reflmodel[idx];
	if ( !spike.isDefined() )
	    continue;

	const float twt = spike.correctedtime_;
	const float dah = data_.wd_->d2TModel().getDah(twt, data_.wd_->track());
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
	const float dah = data_.wd_->d2TModel().getDah(twt,data_.wd_->track());
	if ( !dahrg.includes(dah,true) )
	    continue;

	dahsynth += dah;
	synth += data_.synthtrc_.get( idx, 0 );
    }

    createLog( data_.sKeySynthetic(), dahsynth.arr(), synth.arr(),synth.size());

    const Well::Log* sonlog = data_.wd_->logs().getLogByName(data_.sKeySonic());
    const UnitOfMeasure* sonuom = sonlog ? sonlog->unitOfMeasure() : 0;
    Well::Log* vellogfrommodel = data_.logset_.getLogByName(data_.sKeySonic());
    if ( vellogfrommodel && sonlog )
    {
	if ( data_.isSonic() )
	{
	    GeoCalculator gc;
	    gc.son2Vel( *vellogfrommodel );
	}
	vellogfrommodel->convertTo( sonuom );
    }

    const Well::Log* denlog = data_.wd_->logs().getLogByName(
						data_.sKeyDensity());
    const UnitOfMeasure* denuom = denlog ? denlog->unitOfMeasure() : 0;
    Well::Log* denlogfrommodel = data_.logset_.getLogByName(
						data_.sKeyDensity() );
    if ( denlogfrommodel && denlog )
    {
	const UnitOfMeasure* denuomfrommodel =
				UoMR().getInternalFor(PropertyRef::Den);
	if ( denuomfrommodel )
	    denlogfrommodel->setUnitMeasLabel( denuomfrommodel->symbol() );

	denlogfrommodel->convertTo( denuom );
    }

    Well::Log* ailogfrommodel = data_.logset_.getLogByName( data_.sKeyAI() );
    if ( ailogfrommodel && sonuom && denuom )
    {
	const PropertyRef::StdType& impprop = PropertyRef::Imp;
	const UnitOfMeasure* aiuomfrommodel = UoMR().getInternalFor( impprop );
	ailogfrommodel->setUnitMeasLabel( aiuomfrommodel->symbol() );
	float fact = mCast( float, denuom->scaler().factor_ );
	if ( sonuom->isImperial() )
	    fact *= mFromFeetFactorF;

	ObjectSet<const UnitOfMeasure> relevantunits;
	UoMR().getRelevant( impprop, relevantunits );
	const UnitOfMeasure* aiuom = 0;
	for ( int idx=0; idx<relevantunits.size(); idx++ )
	{
	    const float curfactor = (float)relevantunits[idx]->scaler().factor_;
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
	mErrRet( uiStrings::phrCannotFind(nm) )

    outplog.setUnitMeasLabel( log->unitMeasLabel() );

    Well::LogIter logiter( *log );
    while ( logiter.next() )
    {
	const float logval = logiter.value();
	if ( mIsUdf(logval) )
	    continue;

	outplog.setValueAt( logiter.dah(), logval );
    }
    logiter.retire();

    TypeSet<float> dahs, vals;
    outplog.getData( dahs, vals );
    const Well::Log::size_type sz = vals.size();
    if ( sz <= 2 )
	mErrRet( tr("%1: log size too small, please check your input log")
		     .arg( nm ) )

    GeoCalculator gc;
    gc.removeSpikes( vals.arr(), sz, 10, 3 );
    outplog.setData( dahs, vals );
    outplog.setName( log->name() );

    return true;
}


void DataPlayer::createLog( const char* nm, float* dah, float* vals, int sz )
{
    RefMan<Well::Log> log;
    if ( data_.logset_.isPresent(nm) )
	log = data_.logset_.getLogByName( nm );
    else
    {
	log = new Well::Log( nm );
	data_.logset_.add( log );
    }

    log->setEmpty();
    for( int idx=0; idx<sz; idx ++)
	log->setValueAt( dah[idx], vals[idx] );
}

} // namespace WellTie
