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
#include "statruncalc.h"
#include "synthseis.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "wavelet.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "welltiedata.h"
#include "welltieextractdata.h"
#include "welltrack.h"


namespace WellTie
{
#define mErrRet(msg) { errmsg_ = msg; return false; }
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
    worksz_ = workrg_.nrSteps()+1;
}


bool DataPlayer::computeSynthetics()
{
    if ( !data_.wd_ )
	mErrRet( "Cannot read well data" );

    if ( !data_.wd_->d2TModel() )
	mErrRet( "No depth/time model computed" );

    if ( !setAIModel() )
	mErrRet( "Could not set AI model for raytracing" );

    if ( !doFullSynthetics() )
	mErrRet( "Could not compute the synthetic trace" );

    if ( !copyDataToLogSet() )
	mErrRet( "Could not copy the AI model to composite logs" );

    return true;
}


bool DataPlayer::extractSeismics()
{
    const IOObj& ioobj = *IOM().get( seisid_ );
    IOObj* seisobj = ioobj.clone();
    SeisIOObjInfo oinf( seisid_ );
    if ( !seisobj || !oinf.isOK() )
	mErrRet( "Cannot read seismic data" );

    CubeSampling cs;
    oinf.getRanges( cs );
    const StepInterval<float> tracerg = data_.getTraceRange();
    StepInterval<float> seisrg( tracerg.start, tracerg.stop, cs.zrg.step );

    Well::SimpleTrackSampler wtextr( data_.wd_->track(), data_.wd_->d2TModel(),
	    			     true, false );
    wtextr.setSampling( seisrg );
    TaskRunner::execute( data_.trunner_, wtextr );

    SeismicExtractor seisextr( *seisobj );
    if ( linekey_ )
	seisextr.setLineKey( linekey_ );

    TypeSet<BinID> bids;  wtextr.getBIDs( bids );
    seisextr.setBIDValues( bids );
    seisextr.setInterval( seisrg );

    if ( !TaskRunner::execute(data_.trunner_,seisextr) )
    {
	BufferString msg;
	msg += "Can not extract seismic: ";
	msg += seisextr.errMsg();
	mErrRet( msg );
    }

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


bool DataPlayer::doFastSynthetics()
{
    const Wavelet& wvlt = data_.isinitwvltactive_ ? data_.initwvlt_ 
						  : data_.estimatedwvlt_;

    Seis::SynthGenerator gen;
    gen.setConvolDomain( true );
    gen.setModel( refmodel_ );
    gen.setWavelet( &wvlt, OD::UsePtr );
    gen.setOutSampling( data_.getTraceRange() );
    if ( !gen.doWork() )
	mErrRet( gen.errMsg() )

    data_.synthtrc_ = *new SeisTrc( gen.result() );

    return true;
}


#define mDelAndReturn(yn) { delete [] seisarr;  delete [] syntharr; return yn;}
bool DataPlayer::computeAdditionalInfo( const Interval<float>& zrg )
{
    if ( !data_.seistrc_.zRange().isEqual(data_.synthtrc_.zRange(), 1e-2) )
	mErrRet( "Synthetic and seismic traces do not have same length" )

    if ( !isOKSynthetic() && !isOKSeismic() )
	mErrRet( "Seismic/Synthetic data too short" )

    const int istartseis = data_.seistrc_.nearestSample( zrg.start );
    const int istopseis = data_.seistrc_.nearestSample( zrg.stop );
    const int nrsamps = istopseis - istartseis + 1;
    if ( nrsamps < 2 )
	mErrRet( "Cross-correlation too short" )

    if ( zrg.start < data_.seistrc_.startPos() ||
	 zrg.stop > data_.seistrc_.endPos() )
    {
	BufferString errmsg = "The cross-correlation window must be smaller ";
	errmsg += "than the synthetic/seismic traces";
	mErrRet( errmsg )
    }

    mDeclareAndTryAlloc( float*, seisarr, float[nrsamps] );
    mDeclareAndTryAlloc( float*, syntharr, float[nrsamps] );
    if ( !seisarr || !syntharr )
	mErrRet( "Cannot allocate memory" )

    TypeSet<float> twt;
    int idy = 0;
    Stats::CalcSetup scalercalc;
    scalercalc.require( Stats::RMS );
    Stats::RunCalc<double> seisstats( scalercalc );
    Stats::RunCalc<double> syntstats( scalercalc );
    for ( int idseis=istartseis; idseis<=istopseis; idseis++ )
    {
	twt += data_.synthtrc_.samplePos( idseis );
	syntharr[idy] = data_.synthtrc_.get( idseis, 0 );
	syntstats += syntharr[idy];
	seisarr[idy] = data_.seistrc_.get( idseis, 0 );
	seisstats += seisarr[idy];
	idy++;
    }
    const double seisrms = seisstats.rms();
    const double syntrms = syntstats.rms();
    data_.setScaler( syntrms > 0 ? mCast( float, seisrms / syntrms )
	    			 : mUdf(float) );

    Data::CorrelData& cd = data_.correl_;
    cd.vals_.erase();
    cd.vals_.setSize( nrsamps, 0 );
    GeoCalculator gccc;
    cd.coeff_ = gccc.crossCorr( seisarr, syntharr, cd.vals_.arr(), nrsamps );

    if ( data_.isinitwvltactive_ )
    {
	const int totnrspikes = refmodel_.size();
	if ( totnrspikes < nrsamps )
	    mErrRet( "Reflectivity serie too short" )

	int firstspike = 0;
	int lastspike = 0;
	const float starttwtseis = twt[0];
	const float stoptwtseis = twt[twt.size()-1];
	while ( lastspike<refmodel_.size() )
	{
	    const ReflectivitySpike spike = refmodel_[lastspike];
	    const float spiketwt = spike.correctedtime_;
	    if ( mIsEqual(spiketwt,stoptwtseis,1e-5 ) )
		break;

	    if ( spiketwt - starttwtseis < -1e-5 )
		firstspike++;

	    lastspike++;
	}

	if ( refmodel_[firstspike].correctedtime_ - starttwtseis < -1e-5 )
	{
	    errmsg_ = "The wavelet estimation window must start ";
	    errmsg_ += "above the first spike at ";
	    errmsg_ += refmodel_[firstspike].correctedtime_;
	    errmsg_ += "ms";
	    mDelAndReturn(false);
	}

	if ( refmodel_[lastspike].correctedtime_ - stoptwtseis > 1e-5 )
	{
	    errmsg_ = "The wavelet estimation window must stop ";
	    errmsg_ += "before the last spike at ";
	    errmsg_ += refmodel_[lastspike].correctedtime_;
	    errmsg_ += "ms";
	    mDelAndReturn(false);
	}

	if ( (lastspike-firstspike+1) != nrsamps )
	{
	    errmsg_ = "The wavelet estimation window must be";
	    errmsg_ += " smaller than the reflectivity serie";
	    mDelAndReturn(false);
	}

	mDeclareAndTryAlloc( float_complex*, refarr, float_complex[nrsamps] );
	if ( !refarr )
	{
	    errmsg_ = "Cannot allocate memory for reflectivity serie";
	    mDelAndReturn(false);
	}

	int nrspikefound = 0;
	for ( int idsp=firstspike; idsp<=lastspike; idsp++ )
	{
	    const ReflectivitySpike spike = refmodel_[idsp];
	    const float twtspike = spike.correctedtime_;
	    if ( !mIsEqual(twtspike,twt[nrspikefound],1e-5) )
	    {
		errmsg_ = "Mismatch between spike twt and seismic twt";
		delete [] refarr;
		mDelAndReturn(false);
	    }

	    refarr[nrspikefound] = spike.isDefined() ? spike.reflectivity_ : 0.;
	    nrspikefound++;
	}

	mDeclareAndTryAlloc( float*, wvltarrfull, float[nrsamps] );
	GeoCalculator gcwvltest;
	gcwvltest.deconvolve( seisarr, refarr, wvltarrfull, nrsamps );

	const int initwvltsz = data_.initwvlt_.size();
	const float sr = data_.initwvlt_.sampleRate();
	int outwvltsz = initwvltsz;
	if ( !(initwvltsz%2) )
	    outwvltsz++;

	Array1DImpl<float> wvltarr( outwvltsz );
	data_.estimatedwvlt_.reSize( outwvltsz );
	for ( int idx=0; idx<outwvltsz; idx++ )
	    wvltarr.set( idx, wvltarrfull[(nrsamps-outwvltsz+1)/2 + 2 + idx] );

	ArrayNDWindow window( Array1DInfoImpl(outwvltsz), false, "CosTaper",
			      0.90 );
	window.apply( &wvltarr );
	memcpy( data_.estimatedwvlt_.samples(), wvltarr.getData(),
		outwvltsz*sizeof(float) );
	data_.estimatedwvlt_.set( (outwvltsz-1)/2, sr );
	delete [] wvltarrfull; delete [] refarr;
    }

    mDelAndReturn(true)
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
    const Well::Log* sonlog = data_.wd_->logs().getLog( data_.sonic() );
    const Well::Log* denlog = data_.wd_->logs().getLog( data_.density() );

    Well::Log* pcvellog = new Well::Log;
    Well::Log* pcdenlog = new Well::Log;
    if ( !processLog(sonlog,*pcvellog,data_.sonic()) ||
	 !processLog(denlog,*pcdenlog,data_.density()) )
	return false;

    if ( data_.isSonic() )
    {
	GeoCalculator gc;
	gc.son2Vel( *pcvellog );
    }

    aimodel_.erase();
    Well::LogSet inplogs;
    inplogs.add( pcvellog );
    inplogs.add( pcdenlog );
    BufferString errmsg;
    if ( !computeElasticModelFromLogs(aimodel_,data_.getModelRange(),true,
	 *data_.wd_,inplogs,errmsg) )
	mErrRet( errmsg );

    return true;
}


bool DataPlayer::doFullSynthetics()
{
    refmodel_.erase();
    const Wavelet& wvlt = data_.isinitwvltactive_ ? data_.initwvlt_ 
						  : data_.estimatedwvlt_;

    Seis::RaySynthGenerator gen;
    gen.addModel( aimodel_ );
    gen.forceReflTimes( data_.getReflRange() );
    gen.setWavelet( &wvlt, OD::UsePtr );
    gen.setOutSampling( data_.getTraceRange() );
    IOPar par;
    par.set(RayTracer1D::sKeySRDepth(),0.f,0.f);
    gen.usePar( par ); 
    if ( !gen.doWork() )
	mErrRet( gen.errMsg() )

    Seis::RaySynthGenerator::RayModel& rm = gen.result( 0 );
    ObjectSet<const ReflectivityModel> refmodels;
    rm.getRefs( refmodels, true );
    if ( refmodels.isEmpty() )
	mErrRet( "Could not retrieve the reflectivities after ray-tracing" )

    refmodel_ = *refmodels[0];
    data_.synthtrc_ = *rm.stackedTrc();

    return true;
}


bool DataPlayer::copyDataToLogSet()
{
    if ( aimodel_.isEmpty() )
	mErrRet( "No data found" )

    data_.logset_.empty();
    const StepInterval<float> dahrg = data_.getDahRange();

    TypeSet<float> dahlog, son, den, ai;
    for ( int idx=0; idx<aimodel_.size(); idx++ )
    {
	const float twt = data_.getModelRange().atIndex(idx);
	const float dah = data_.wd_->d2TModel()->getDah( twt );
	if ( !dahrg.includes(dah,true) )
	    continue;

	dahlog += dah;
	const AILayer& layer = aimodel_[idx];
	son += layer.vel_;
	den += layer.den_;
	ai += layer.vel_ * layer.den_;
    }

    createLog( data_.sonic(), dahlog.arr(), son.arr(), son.size() ); 
    createLog( data_.density(), dahlog.arr(), den.arr(), den.size() ); 
    createLog( data_.ai(), dahlog.arr(), ai.arr(), ai.size() );

    TypeSet<float> dahref, refs;
    for ( int idx=0; idx<refmodel_.size(); idx++ )
    {
	const ReflectivitySpike spike = refmodel_[idx];
	if ( !spike.isDefined() )
	    continue;

	const float twt = spike.correctedtime_;
	const float dah = data_.wd_->d2TModel()->getDah( twt );
	if ( !dahrg.includes(dah,true) )
	    continue;

	dahref += dah;
	refs += spike.reflectivity_.real();
    }

    createLog( data_.reflectivity(), dahref.arr(), refs.arr(), refs.size() );

    TypeSet<float> dahsynth, synth;
    const StepInterval<float> tracerg = data_.getTraceRange();
    for ( int idx=0; idx<=data_.synthtrc_.size(); idx++ )
    {
	const float twt = tracerg.atIndex( idx );
	const float dah = data_.wd_->d2TModel()->getDah( twt );
	if ( !dahrg.includes(dah,true) )
	    continue;

	dahsynth += dah;
	synth += data_.synthtrc_.get( idx, 0 );
    }

    createLog( data_.synthetic(), dahsynth.arr(), synth.arr(), synth.size() );

    const Well::Log* sonlog = data_.wd_->logs().getLog( data_.sonic() );
    const UnitOfMeasure* sonuom = sonlog ? sonlog->unitOfMeasure() : 0;
    Well::Log* vellogfrommodel = data_.logset_.getLog( data_.sonic() );
    if ( vellogfrommodel && sonlog )
    {
	if ( data_.isSonic() )
	{
	    GeoCalculator gc;
	    gc.son2Vel( *vellogfrommodel );
	}
	vellogfrommodel->convertTo( sonuom );
    }

    const Well::Log* denlog = data_.wd_->logs().getLog( data_.density() );
    const UnitOfMeasure* denuom = denlog ? denlog->unitOfMeasure() : 0;
    Well::Log* denlogfrommodel = data_.logset_.getLog( data_.density() );
    if ( denlogfrommodel && denlog )
    {
	const UnitOfMeasure* denuomfrommodel =
	    			UoMR().getInternalFor(PropertyRef::Den);
	if ( denuomfrommodel )
	    denlogfrommodel->setUnitMeasLabel( denuomfrommodel->symbol() );

	denlogfrommodel->convertTo( denuom );
    }

    Well::Log* ailogfrommodel = data_.logset_.getLog( data_.ai() );
    if ( ailogfrommodel && sonuom && denuom )
    {
	const PropertyRef::StdType& impprop = PropertyRef::Imp;
	const UnitOfMeasure* aiuomfrommodel = UoMR().getInternalFor( impprop );
	ailogfrommodel->setUnitMeasLabel( aiuomfrommodel->symbol() );
	float fact = (float)denuom->scaler().factor;
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


bool DataPlayer::computeAll()
{
    mGetWD()

    bool success = setAIModel() && doFullSynthetics() && extractSeismics(); 
    copyDataToLogSet();

    computeAdditionalInfo( disprg_ );

    return success;
}


bool DataPlayer::processLog( const Well::Log* log, 
			     Well::Log& outplog, const char* nm ) 
{
    BufferString msg;
    if ( !log ) 
	{ msg += "Can not find "; msg += nm; mErrRet( msg ); }

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
    {
	msg += nm;
	msg +="log size too small, please check your input log";
	mErrRet(msg)
    }

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

    log->erase();

    for( int idx=0; idx<sz; idx ++)
	log->addValue( dah[idx], vals[idx] );
}


#define mReadLog( val, logidx, layidx ) \
    val = mIsUdf(ls.getLogVal(logidx,layidx)) \
	? lsnearest.getLogVal( logidx, layidx ) \
	: ls.getLogVal( logidx, layidx );
#define	mPVelIdx	0
#define	mDenIdx	1
#define	mVelsIdx	2
bool DataPlayer::computeElasticModelFromLogs( ElasticModel& emodel,
				const StepInterval<float> zrg, bool rgistime,
				const Well::Data& wd, const Well::LogSet& wls,
				BufferString& errmsg )
{
    if ( wls.size() < 1 )
    {
	errmsg = "No logs to compute from";
	return false;
    }

    const float fact = SI().zDomain().isDepth() && SI().depthsInFeet()
		     ? mFromFeetFactorF : 1.0f;

    const float srddepth = wd.info().surfaceelev;
    if ( (!rgistime && zrg.start < srddepth) || (rgistime && zrg.start < 0.f) )
    {
	errmsg = "Extraction interval should not start above SRD";
	return false;
    }

    TypeSet<const UnitOfMeasure*> uomset;
    TypeSet<int> issonic;
    const PropertyRef::StdType& sonpropreftype = PropertyRef::Son;
    for ( int ilog=0; ilog<wls.size(); ilog++ )
    {
	const char* uomlbl = wls.getLog(ilog).unitMeasLabel();
	uomset += UnitOfMeasure::getGuessed( uomlbl );
	issonic += uomset[ilog] && uomset[ilog]->propType() == sonpropreftype
	    	? 1 : 0;
    }

    Well::LogSampler ls( wd, zrg, rgistime, zrg.step, true, Stats::UseAvg,wls);

    Well::LogSampler lsnearest( wd, zrg, rgistime, zrg.step, true,
	    			Stats::TakeNearest, wls );

    if ( !ls.execute() )
    {
	errmsg = ls.errMsg();
	return false;
    }

    if ( !lsnearest.execute() )
    {
	errmsg = lsnearest.errMsg();
	return false;
    }

    const UnitOfMeasure* velpuom = wls.validIdx(mPVelIdx) ? uomset[mPVelIdx]:0;
    const UnitOfMeasure* denuom = wls.validIdx(mDenIdx) ? uomset[mDenIdx] : 0;
    const UnitOfMeasure* sveluom = wls.validIdx(mVelsIdx) ? uomset[mVelsIdx]:0;
    const float dz = zrg.step * fact;

    emodel.erase();
    for ( int idl=0; idl<=zrg.nrSteps(); idl++ )
    {
	float logval = mUdf(float);
	emodel += ElasticLayer( mUdf(float), mUdf(float), mUdf(float),
				mUdf(float) );

	mReadLog( logval, mPVelIdx, idl )
	logval = velpuom ? velpuom->getSIValue( logval ) : logval;
	const float velp = issonic[mPVelIdx] ? 1.f/logval : logval;
	emodel[idl].thickness_ = !rgistime ? dz : dz * velp / 2.f;
	emodel[idl].vel_ = velp;

	if ( wls.size() < 2 )
	    continue;

	mReadLog( logval, mDenIdx, idl )
	emodel[idl].den_ = denuom ? denuom->getSIValue( logval ) : logval;
	if ( wls.size() < 3 )
	    continue;
		
	mReadLog( logval, mVelsIdx, idl )
	logval = sveluom ? sveluom->getSIValue( logval ) : logval;
	emodel[idl].svel_ = issonic[mVelsIdx] ? 1.f/logval : logval;
    }

    if ( emodel.size() == 0 )
    {
	errmsg = "Returning empty elastic model";
	return false;
    }

    float startdepth = zrg.start;
    if ( rgistime )
    {
	const float startdah = wd.d2TModel()->getDah( startdepth );
	startdepth = mCast(float,wd.track().getPos( startdah ).z);
    }

    emodel[0].thickness_ += ( startdepth - srddepth ) * fact;

    return true;
}

}; //namespace WellTie

