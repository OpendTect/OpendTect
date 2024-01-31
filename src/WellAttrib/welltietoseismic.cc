/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welltietoseismic.h"

#include "ioman.h"
#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "raytrace1d.h"
#include "stratsynthgenparams.h"
#include "synthseis.h"
#include "syntheticdata.h"
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


#define mErrRet(msg) \
{ \
    if ( errmsg_.isEmpty() ) \
	errmsg_ = msg; \
    else \
	errmsg_.append( msg, true ); \
\
    return false; \
}

WellTie::DataPlayer::DataPlayer( Data& data, const MultiID& seisid,
			const BufferString& lnm )
    : data_(data)
    , seisid_(seisid)
    , linenm_(lnm)
{
    zrg_.set( mUdf(float), mUdf(float) );
}


WellTie::DataPlayer::~DataPlayer()
{
    delete [] refarr_;
    delete [] syntarr_;
    delete [] seisarr_;
}


bool WellTie::DataPlayer::computeSynthetics( const Wavelet& wvlt )
{
    errmsg_.setEmpty();

    if ( !data_.wd_ )
	mErrRet( tr( "Cannot read well data" ) )

    if ( !data_.wd_->d2TModel() )
	mErrRet( tr( "No depth/time model computed" ) )

    if ( !setAIModel() )
	mErrRet( tr( "Could not use the density and/or velocity log" ) )

    if ( !doFullSynthetics(wvlt) )
	mErrRet( tr( "Could not compute the synthetic trace" ) )

    if ( !copyDataToLogSet() )
	mErrRet( tr( "Could not copy the AI model to composite logs" ) )

    return true;
}


bool WellTie::DataPlayer::extractSeismics()
{
    errmsg_.setEmpty();

    const PtrMan<IOObj> seisobj = IOM().get( seisid_ );
    const SeisIOObjInfo oinf( seisid_ );
    if ( !seisobj || !oinf.isOK() )
	mErrRet( tr( "Cannot read seismic data" ) )

    TrcKeyZSampling cs;
    oinf.getRanges( cs );
    const ZSampling tracerg = data_.getTraceRange();
    ZSampling seisrg( tracerg.start, tracerg.stop, cs.zsamp_.step );

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

    const SeisTrc rawseis = seisextr.result();
    const int newsz = tracerg.nrSteps()+1;
    auto* newtrc = new SeisTrc( newsz );
    newtrc->info() = rawseis.info();
    newtrc->info().sampling = tracerg;
    for ( int idx=0; idx<newsz; idx++ )
    {
	const float twt = tracerg.atIndex(idx);
	const float outval = rawseis.getValue( twt, 0 );
	newtrc->set( idx, outval, 0 );
    }

    data_.setRealTrc( newtrc );

    return true;
}


bool WellTie::DataPlayer::doFastSynthetics( const Wavelet& wvlt )
{
    errmsg_.setEmpty();

    const SyntheticData* synthetics = data_.getSynthetics();
    if ( !synthetics )
	mErrRet( tr( "Cannot update synthetics without previous" ) );

    const Seis::SynthGenDataPack& synthgendp = synthetics->synthGenDP();

    Seis::RaySynthGenerator synthgen( synthgendp );
    synthgen.usePar( synthetics->getGenParams().synthpars_ );
    synthgen.setWavelet( &wvlt, OD::UsePtr );
    synthgen.setOutSampling( data_.getTraceRange() );

    if ( !TaskRunner::execute(data_.trunner_,synthgen) )
	mErrRet( tr("Cannot update synthetics: %1").arg(synthgen.uiMessage()) )

    SynthGenParams sgp = synthetics->getGenParams();
    sgp.setWavelet( wvlt );
    sgp.createName( sgp.name_ );
    ConstRefMan<SyntheticData> newsynthdp = SyntheticData::get( sgp, synthgen );
    if ( !newsynthdp )
	mErrRet( uiStrings::phrCannotCreate(tr("synthetic")) );

    data_.setSynthetics( newsynthdp.ptr() );

    return true;
}


bool WellTie::DataPlayer::computeAdditionalInfo( const Interval<float>& zrg )
{
    setCrossCorrZrg( zrg );
    if ( !checkCrossCorrInps() )
	return false;

    if ( !computeCrossCorrelation() || !computeEstimatedWavelet(0) )
	return false;

    return true;
}


bool WellTie::DataPlayer::checkCrossCorrInps()
{
    errmsg_.setEmpty();

    if ( zrg_.isUdf() )
	mErrRet( tr( "Cross-correlation window not set" ) )

    const Data& data = data_;
    const SeisTrc* realtrc = data.getRealTrc();
    const SeisTrc* synthtrc = data.getSynthTrc();
    if ( !realtrc || !synthtrc )
	return false;

    if ( !realtrc->zRange().isEqual(synthtrc->zRange(), 1e-2f) )
	mErrRet( tr( "Synthetic and seismic traces do not have same length" ) )

    if ( !isOKSynthetic() && !isOKSeismic() )
	mErrRet( tr( "Seismic/Synthetic data too short" ) )

    const int istartseis = realtrc->nearestSample( zrg_.start );
    const int istopseis = realtrc->nearestSample( zrg_.stop );
    const int nrsamps = istopseis - istartseis + 1;
    if ( nrsamps < 2 )
	mErrRet( tr( "Cross-correlation too short" ) )

    if ( zrg_.start < realtrc->startPos() || zrg_.stop > realtrc->endPos() )
    {
	const uiString msg = tr( "The cross-correlation window must be smaller "
				 "than the synthetic/seismic traces" );
	mErrRet( msg )
    }

    // clip to nearest sample
    zrg_.start = realtrc->samplePos( istartseis );
    zrg_.stop = realtrc->samplePos( istopseis );

    return true;
}


bool WellTie::DataPlayer::computeCrossCorrelation()
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
    const float step = data_.getZStep();
    if ( mIsUdf(step) )
	return false;

    const int nrsamps = mNINT32( zrg_.width(false) / step ) + 1;
    cd.vals_.setSize( nrsamps, 0 );
    cd.coeff_ = WellTie::GeoCalculator::crossCorr( seisarr_, syntarr_,
						   cd.vals_.arr(), nrsamps );

    return true;
}


bool WellTie::DataPlayer::computeEstimatedWavelet( int wvltsz )
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

    const float step = data_.getZStep();
    if ( mIsUdf(step) )
	return false;

    const int nrsamps = mNINT32( zrg_.width(false) / step ) + 1;
    mDeclareAndTryAlloc( float*, wvltarrfull, float[nrsamps] );
    if ( !wvltarrfull )
	mErrRet( tr( "Cannot allocate memory for estimated wavelet" ) )

    WellTie::GeoCalculator::deconvolve( seisarr_, refarr_, wvltarrfull,nrsamps);

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


bool WellTie::DataPlayer::extractWvf( bool issynt )
{
    errmsg_.setEmpty();

    if ( zrg_.isUdf() )
	mErrRet( tr( "Waveform extraction window not set" ) )

    const Data& data = data_;
    const SeisTrc* trace = data.getTrc( issynt );
    if ( !trace )
	return false;

    const int istartseis = trace->nearestSample( zrg_.start );
    const int istopseis = trace->nearestSample( zrg_.stop );
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
	const float val = trace->get( idseis, 0 );
	valarr[idy] = val;
	stats += val;
	idy++;
    }
    const float wvfrms = mCast( float, stats.rms() );
    if ( issynt )
    {
	delete [] syntarr_;
	syntarr_ = valarr;
	data_.correl_.scaler_ = mIsUdf(wvfrms) ? mUdf(float)
			      : data_.correl_.scaler_ / wvfrms;
    }
    else
    {
	delete [] seisarr_;
	seisarr_ = valarr;
	data_.correl_.scaler_ = wvfrms;
    }

    return true;
}


bool WellTie::DataPlayer::extractReflectivity()
{
    errmsg_.setEmpty();

    if ( zrg_.isUdf() )
	mErrRet( tr( "Extraction window not set for reflectivity computation") )

    const float step = data_.getZStep();
    if ( mIsUdf(step) )
	return false;

    const int nrsamps = mNINT32( zrg_.width(false) / step ) + 1;
    const ReflectivityModelBase* refmodel = data_.getRefModel();
    const int totnrspikes = refmodel->nrSpikes();
    if ( totnrspikes < nrsamps )
	mErrRet( tr( "Reflectivity series too short" ) )

    const int ioff = 0;
    const ReflectivityModelTrace* reflectivities =
				  refmodel->getReflectivities( ioff );
    const float_complex* refarr = reflectivities
				? reflectivities->arr() : nullptr;
    const float* times = refmodel->getReflTimes();
    if ( !refarr || !times )
	return false;

    int firstspike = 0;
    int lastspike = 0;
    while ( lastspike<totnrspikes )
    {
	const float spiketwt = times[lastspike];
	if ( mIsEqual(spiketwt,zrg_.stop,1e-5f) )
	    break;

	if ( spiketwt - zrg_.start < -1e-5f )
	    firstspike++;

	lastspike++;
    }

    uiString msg;
    if ( times[firstspike] - zrg_.start < -1e-5f )
    {
	msg = tr( "The wavelet estimation window must start "
		  "above the first spike at %1 ms" )
			.arg( toString(times[firstspike]) );
	mErrRet( msg )
    }

    if ( times[lastspike] - zrg_.stop > 1e-5f )
    {
	msg = tr( "The wavelet estimation window must stop "
		  "before the last spike at %1 ms" )
			.arg( toString(times[lastspike]) );
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
	const float twtspike = times[idsp];
	if ( !mIsEqual(twtspike,zrg_.atIndex(nrspikefound,step),1e-5f) )
	{
	    delete [] valarr;
	    mErrRet( tr( "Mismatch between spike twt and seismic twt" ) )
	}

	const float_complex refval = refarr[idsp];
	valarr[nrspikefound] = mIsUdf(refval) ? float_complex(0.f,0.f) : refval;
	nrspikefound++;
    }

    delete [] refarr_;
    refarr_ = valarr;
    return true;
}


bool WellTie::DataPlayer::isOKSynthetic() const
{
    const Data& data = data_;
    const SeisTrc* synthtrc = data.getSynthTrc();
    return synthtrc && synthtrc->size() != 0;
}


bool WellTie::DataPlayer::isOKSeismic() const
{
    const Data& data = data_;
    const SeisTrc* realtrc = data.getRealTrc();
    return realtrc && realtrc->size() != 0;
}


bool WellTie::DataPlayer::hasSeisId() const
{
    return !seisid_.isUdf();
}


bool WellTie::DataPlayer::setAIModel()
{
    const Well::Log* sonlog = data_.wd_->logs().getLog( data_.sKeySonic() );
    const Well::Log* denlog = data_.wd_->logs().getLog( data_.sKeyDensity());

    auto* pcvellog = new Well::Log;
    auto* pcdenlog = new Well::Log;
    if ( !processLog(sonlog,*pcvellog,data_.sKeySonic()) ||
	 !processLog(denlog,*pcdenlog,data_.sKeyDensity()) )
	return false;

    if ( data_.isSonic() )
	WellTie::GeoCalculator::son2Vel( *pcvellog );

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


bool WellTie::DataPlayer::setTargetModel( TimeDepthModel& tdmodel ) const
{
    const ZSampling& reflzrg = data_.getReflRange();
    const int nrlayers = aimodel_.size()+1;
    TypeSet<float> refldepths( nrlayers, mUdf(float) );
    TypeSet<float> refltimes( nrlayers, mUdf(float) );
    float* refldepthsarr = refldepths.arr();
    float* refltimesarr = refltimes.arr();
    const UnitOfMeasure* depthuom = UnitOfMeasure::surveyDefDepthUnit();
    const float kbelev = getConvertedValue( data_.wd_->track().getKbElev(),
				UnitOfMeasure::surveyDefDepthStorageUnit(),
				depthuom );
    const float srd = getConvertedValue( SI().seismicReferenceDatum(),
				    UnitOfMeasure::surveyDefSRDStorageUnit(),
				    depthuom );
    if ( kbelev < srd )
    {
	const float replvel = getConvertedValue( data_.wd_->info().replvel_,
			UnitOfMeasure::surveyDefDepthStorageUnit(), depthuom );
	refldepthsarr[0] = -kbelev;
	refltimesarr[0] = 2.f * (srd-kbelev) / replvel;
    }
    else
    {
	refldepthsarr[0] = -srd;
	refltimesarr[0] = 0.f;
    }

    float dz = depthuom->getUserValueFromSI( aimodel_.first()->getThickness() );
    refldepthsarr[1] = -srd + dz;
    refltimesarr[1] = reflzrg.start;
    for ( int idz=2; idz<nrlayers; idz++ )
    {
	dz = depthuom->getUserValueFromSI( aimodel_.get(idz-1)->getThickness());
	refldepthsarr[idz] = refldepthsarr[idz-1] + dz;
	refltimesarr[idz] = reflzrg.atIndex( idz-1 );
    }

    tdmodel.setModel( refldepthsarr, refltimesarr, nrlayers );
    return tdmodel.isOK();
}


bool WellTie::DataPlayer::doFullSynthetics( const Wavelet& wvlt )
{
    errmsg_.setEmpty();
    uiString msg;
    TaskRunner* taskrunner = data_.trunner_;
    ElasticModelSet aimodels;
    aimodels.add( aimodel_.clone() );

    ObjectSet<const TimeDepthModel> forcedtdmodels;
    TimeDepthModel tdmodel;
    if ( setTargetModel(tdmodel) )
	forcedtdmodels.add( &tdmodel );

    const SynthGenParams& sgp = data_.setup().sgp_;
    const float srd = getConvertedValue( SI().seismicReferenceDatum(),
				    UnitOfMeasure::surveyDefSRDStorageUnit(),
				    UnitOfMeasure::surveyDefDepthUnit() );
    const Seis::OffsetType offstyp = SI().xyInFeet()
				   ? Seis::OffsetType::OffsetFeet
				   : Seis::OffsetType::OffsetMeter;
    const ZDomain::DepthType depthtype = SI().depthType();
    ConstRefMan<ReflectivityModelSet> refmodels =
	Seis::RaySynthGenerator::getRefModels( aimodels, *sgp.reflPars(),
		       msg, taskrunner, srd, offstyp, depthtype,
		       forcedtdmodels.isEmpty() ? nullptr : &forcedtdmodels );
    if ( !refmodels )
	mErrRet( uiStrings::phrCannotCreate(tr("synthetic: %1").arg(msg)) );

    Seis::RaySynthGenerator synthgen( *refmodels.ptr() );
    synthgen.usePar( sgp.synthpars_ );
    synthgen.setWavelet( &wvlt, OD::UsePtr );
    synthgen.setOutSampling( data_.getTraceRange() );
    if ( !TaskRunner::execute(taskrunner,synthgen) )
	mErrRet( uiStrings::phrCannotCreate(
		tr("synthetic: %1").arg(synthgen.uiMessage())) )

    ConstRefMan<SyntheticData> synthdp = SyntheticData::get( sgp, synthgen );
    if ( !synthdp )
	mErrRet( uiStrings::phrCannotCreate(tr("synthetic")) );

    data_.setSynthetics( synthdp.ptr() );
    const Data& data = data_;
    const SeisTrc* firstrc = data.getSynthTrc();
    if ( firstrc )
	data_.setTraceRange( firstrc->zRange() );
	//requested range was too small

    return true;
}


bool WellTie::DataPlayer::copyDataToLogSet()
{
    errmsg_.setEmpty();

    if ( aimodel_.isEmpty() )
	mErrRet( tr("Internal: No data found") )

    data_.logset_.setEmpty();
    const Data& data = data_;
    const ZSampling dahrg = data.getDahRange();

    TypeSet<float> dahlog, son, den, ai;
    for ( int idx=0; idx<aimodel_.size(); idx++ )
    {
	const float twt = data.getModelRange().atIndex(idx);
	const float dah = data.wd_->d2TModel()->getDah( twt,
							data.wd_->track() );
	if ( !dahrg.includes(dah,true) )
	    continue;

	dahlog += dah;
	const RefLayer& layer = *aimodel_.get(idx);
	son += layer.getPVel();
	den += layer.getDen();
	ai += layer.getAI();
    }

    createLog( data.sKeySonic(), dahlog.arr(), son.arr(), son.size() );
    createLog( data.sKeyDensity(), dahlog.arr(), den.arr(), den.size() );
    createLog( data.sKeyAI(), dahlog.arr(), ai.arr(), ai.size() );

    TypeSet<float> dahref, refs;
    const ReflectivityModelBase* refmodel = data.getRefModel();
    if ( !refmodel || !refmodel->isOK() )
	return false;

    const int nrspikes = refmodel->nrSpikes();
    const int ioff = 0;
    const ReflectivityModelTrace* reflectivities =
				  refmodel->getReflectivities( ioff );
    const float_complex* refarr = reflectivities
				? reflectivities->arr() : nullptr;
    const float* times = refmodel->getReflTimes();
    if ( !refarr || !times )
	return false;

    for ( int idz=0; idz<nrspikes; idz++ )
    {
	const float refval = refarr[idz].real();
	const float twt = times[idz];
	if ( mIsUdf(refval) || mIsUdf(twt) )
	    continue;

	const float dah = data.wd_->d2TModel()->getDah( twt,
							data.wd_->track() );
	if ( !dahrg.includes(dah,true) )
	    continue;

	dahref += dah;
	refs += refval;
    }

    createLog( data.sKeyReflectivity(), dahref.arr(), refs.arr(), refs.size() );

    const SeisTrc* synthtrc = data.getSynthTrc();
    if ( !synthtrc )
	return false;

    TypeSet<float> dahsynth, synth;
    const ZSampling tracerg = data.getTraceRange();
    for ( int idx=0; idx<=synthtrc->size(); idx++ )
    {
	const float twt = tracerg.atIndex( idx );
	const float dah = data.wd_->d2TModel()->getDah( twt,
							data.wd_->track() );
	if ( !dahrg.includes(dah,true) )
	    continue;

	dahsynth += dah;
	synth += synthtrc->get( idx, 0 );
    }

    createLog( data.sKeySynthetic(), dahsynth.arr(), synth.arr(),synth.size());

    const Well::Log* sonlog = data.wd_->logs().getLog( data.sKeySonic() );
    const UnitOfMeasure* sonuom = sonlog ? sonlog->unitOfMeasure() : 0;
    Well::Log* vellogfrommodel = data.logset_.getLog( data.sKeySonic() );
    if ( vellogfrommodel && sonlog )
    {
	if ( data.isSonic() )
	    WellTie::GeoCalculator::son2Vel( *vellogfrommodel );

	vellogfrommodel->convertTo( sonuom );
    }

    const Well::Log* denlog = data.wd_->logs().getLog( data.sKeyDensity());
    const UnitOfMeasure* denuom = denlog ? denlog->unitOfMeasure() : 0;
    Well::Log* denlogfrommodel = data.logset_.getLog( data.sKeyDensity() );
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

    Well::Log* ailogfrommodel = data.logset_.getLog( data.sKeyAI() );
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


bool WellTie::DataPlayer::processLog( const Well::Log* log,
				      Well::Log& outplog, const char* nm )
{
    errmsg_.setEmpty();
    uiString msg;

    if ( !log )
	mErrRet( tr( "Can not find log '%1'" ).arg( nm ) )

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

    outplog.setUnitMeasLabel( log->unitMeasLabel() );
    outplog.setMnemonicLabel( log->mnemonicLabel() );

    WellTie::GeoCalculator::removeSpikes( outplog.valArr(), sz, 10, 3 );
    outplog.setName( log->name() );

    return true;
}


void WellTie::DataPlayer::createLog( const char* nm, float* dah,
				     float* vals, int sz )
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
