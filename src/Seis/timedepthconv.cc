/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : September 2007
-*/


#include "timedepthconv.h"

#include "arrayndimpl.h"
#include "binidvalue.h"
#include "trckeyzsampling.h"
#include "datapackbase.h"
#include "genericnumer.h"
#include "indexinfo.h"
#include "ioobj.h"
#include "iopar.h"
#include "samplfunc.h"
#include "seisdatapack.h"
#include "seisprovider.h"
#include "seispreload.h"
#include "seispacketinfo.h"
#include "seisrangeseldata.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "staticstring.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "zdomain.h"


VelocityStretcher::VelocityStretcher( const ZDomain::Def& from,
				      const ZDomain::Def& to )
    : ZAxisTransform(from,to)
{
}


Time2DepthStretcher::Time2DepthStretcher()
    : VelocityStretcher(ZDomain::Time(),ZDomain::Depth())
    , velprovider_( 0 )
    , topvavg_ ( getDefaultVAvg().start, getDefaultVAvg().start )
    , botvavg_ ( getDefaultVAvg().stop, getDefaultVAvg().stop )
{
    voidata_.setNullAllowed( true );
}


Time2DepthStretcher::~Time2DepthStretcher()
{ releaseData(); }


bool Time2DepthStretcher::setVelData( const DBKey& dbkey )
{
    releaseData();

    uiRetVal uirv;
    velprovider_ = Seis::Provider::create( dbkey, &uirv );
    if ( !velprovider_ )
	{ releaseData(); errmsg_ = uirv; return false; }

    fromzdomaininfo_.setID( dbkey.toString() );
    tozdomaininfo_.setID( dbkey.toString() );

    PtrMan<IOObj> velioobj = dbkey.getIOObj();
    veldesc_.type_ = VelocityDesc::Interval;
    veldesc_.usePar( velioobj->pars() );

    topvavg_ = Interval<float>( getDefaultVAvg().start, getDefaultVAvg().start);
    botvavg_ = Interval<float>( getDefaultVAvg().stop, getDefaultVAvg().stop);

    velioobj->pars().get( sKeyTopVavg(), topvavg_ );
    velioobj->pars().get( sKeyBotVavg(), botvavg_ );

    velintime_ = ZDomain::isTime( velioobj->pars() );
    if ( !velintime_ && !ZDomain::isDepth(velioobj->pars()) )
    {
	releaseData();
	return false;
    }

    //TODO: Reload eventual VOIs
    return true;
}


const Interval<float>& Time2DepthStretcher::getVavgRg( bool top ) const
{ return top ? topvavg_ : botvavg_; }


bool Time2DepthStretcher::isOK() const
{
    if ( !TimeDepthConverter::isVelocityDescUseable( veldesc_, velintime_ ) )
    { errmsg_ = tr("Provided velocity is not usable"); return false; }

    return true;
}


Interval<float> Time2DepthStretcher::getDefaultVAvg()
{
    Interval<float> res( 1350, 4500 );
    if ( SI().depthsInFeet() )
	res.scale( mToFeetFactorF );
    return res;
}


void Time2DepthStretcher::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );
    if ( velprovider_ )
    {
	par.set( VelocityDesc::sKeyVelocityVolume(),
		 velprovider_->dbKey() );
    }
}


bool Time2DepthStretcher::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar( par ) )
	return false;

    DBKey vid;
    if ( par.get( VelocityDesc::sKeyVelocityVolume(), vid ) &&
		  !setVelData( vid ) )
	return false;

    return true;
}


int Time2DepthStretcher::addVolumeOfInterest(const TrcKeyZSampling& cs,
					     bool depth )
{
    int id = 0;
    while ( voiids_.isPresent(id) ) id++;

    voidata_ += 0;
    voivols_ += cs;
    voiintime_ += !depth;
    voiids_ += id;

    return id;
}


void Time2DepthStretcher::setVolumeOfInterest( int id,
					       const TrcKeyZSampling& cs,
					       bool depth )
{
    const int idx = voiids_.indexOf( id );
    if ( idx<0 )
	return;

    if ( cs==voivols_[idx] && depth!=voiintime_[idx] )
	return;

    delete voidata_[idx];
    voidata_.replace( idx, 0 );
    voivols_[idx] = cs;
    voiintime_[idx] = !depth;
}


void Time2DepthStretcher::removeVolumeOfInterest( int id )
{
    const int idx = voiids_.indexOf( id );
    if ( idx<0 )
	return;

    delete voidata_.removeSingle( idx );
    voivols_.removeSingle( idx );
    voiintime_.removeSingle( idx );
    voiids_.removeSingle( idx );
}


class TimeDepthDataLoader : public SequentialTask
{ mODTextTranslationClass(TimeDepthDataLoader);
public:
		TimeDepthDataLoader( Array3D<float>& arr,
				    Seis::Provider& provider,
				    const TrcKeyZSampling& readcs,
				    const VelocityDesc& vd,
				    const SamplingData<double>& voisd,
				    bool velintime,
				    bool voiintime )
		    : arr_( arr )
		    , provider_( provider )
		    , readcs_( readcs )
		    , hiter_( readcs.hsamp_ )
		    , voisd_( voisd )
		    , veldesc_( vd )
		    , velintime_( velintime )
		    , voiintime_( voiintime )
		    , nrdone_( 0 )
		    , seisdatapack_(0)
		{
		    seisdatapack_ = Seis::PLDM().get<RegularSeisDataPack>(
						    provider.dbKey() );
		    seisdp_ = seisdatapack_.ptr();
		}
protected:

    od_int64		totalNr() const
			{ return readcs_.hsamp_.totalNr(); }
    od_int64		nrDone() const { return nrdone_; }
    uiString		message() const
			{ return errmsg_.isEmpty()
			    ? tr("Reading velocity model") : errmsg_; }
    uiString		nrDoneText() const { return tr("Position read"); }

    int			nextStep()
    {
	const TrcKey trk( hiter_.curTrcKey() );
	const TrcKeySampling& hrg = readcs_.hsamp_;
	const od_int64 offset =
	    arr_.info().getOffset( hrg.lineIdx( trk.lineNr() ),
				   hrg.trcIdx( trk.trcNr() ), 0 );

	OffsetValueSeries<float> arrvs( *arr_.valueSeries(), offset,
					arr_.getSize(2) );

	const BinID& curbid = trk.position();
	const int nrz = arr_.getSize( 2 );
	if ( !seisdp_ )
	{
	    SeisTrc velocitytrc;
	    const uiRetVal uirv = provider_.getNext( velocitytrc );
	    if ( !uirv.isOK() )
	    {
		Time2DepthStretcher::udfFill( arrvs, nrz );
		if ( isFinished(uirv) )
		    return hiter_.next() ? MoreToDo() : Finished();

		errmsg_ = uirv;
		return ErrorOccurred();
	    }

	    const SeisTrcValueSeries trcvs( velocitytrc, 0 );
	    tdc_.setVelocityModel( trcvs, velocitytrc.size(),
				    velocitytrc.info().sampling_, veldesc_,
				    velintime_ );
	}
	else
	{
	    const int globidx = seisdp_->globalIdx( curbid );
	    const OffsetValueSeries<float>& dptrcvs =
		seisdp_->getTrcStorage( 0, globidx );

	    const auto& zss = seisdp_->zSubSel();
	    tdc_.setVelocityModel( dptrcvs, zss.outputSize(), zss.zRange(),
				   veldesc_, velintime_ );
	}

	if ( ( voiintime_ && !tdc_.calcDepths(arrvs,nrz,voisd_) ) ||
	     (!voiintime_ && !tdc_.calcTimes(arrvs,nrz,voisd_) ) )
	    Time2DepthStretcher::udfFill( arrvs, nrz );

	nrdone_++;

	return hiter_.next() ? MoreToDo() : Finished();
    }

    TrcKeyZSampling		readcs_;
    Seis::Provider&		provider_;
    Array3D<float>&		arr_;
    TimeDepthConverter		tdc_;
    VelocityDesc		veldesc_;
    bool			velintime_;
    bool			voiintime_;
    ConstRefMan<RegularSeisDataPack> seisdatapack_;
    const RegularSeisDataPack*	seisdp_;

    int				nrdone_;

    SamplingData<double>	voisd_;
    uiString			errmsg_;

    TrcKeySamplingIterator		hiter_;
};


bool Time2DepthStretcher::loadDataIfMissing( int id,
					     const TaskRunnerProvider& trprov )
{
    if ( !velprovider_ ) return true;

    const int idx = voiids_.indexOf( id );
    if ( idx<0 )
	return false;

    const TrcKeyZSampling& voi( voivols_[idx] );
    TrcKeyZSampling readcs( voi );

    const StepInterval<float> filezrg = velprovider_->zRange();
    const int nrsamplesinfile = filezrg.nrSteps()+1;
    if ( velintime_ == voiintime_[idx] )
	readcs.zsamp_.setFrom( filezrg );
    else
    {
	int zstartidx = (int) filezrg.getIndex( readcs.zsamp_.start );
	if ( zstartidx<0 ) zstartidx = 0;
	int zstopidx = (int) filezrg.getIndex( readcs.zsamp_.stop )+1;
	if ( zstopidx>=nrsamplesinfile )
	    zstopidx = nrsamplesinfile-1;

	readcs.zsamp_.start = filezrg.atIndex( zstartidx );
	readcs.zsamp_.stop = filezrg.atIndex( zstopidx );
	readcs.zsamp_.step = filezrg.step;
    }

    Array3D<float>* arr = voidata_[idx];
    if ( !arr )
    {
	arr = new Array3DImpl<float>(voi.nrInl(),voi.nrCrl(),voi.nrZ()+1 );
	if ( !arr->isOK() )
	    return false;

	voidata_.replace( idx, arr );
    }

    if ( velprovider_->is2D() ) // Now geomid is known. Have to recreate reader.
    {
	auto* sd = new Seis::RangeSelData( readcs );
	sd->setGeomID( Pos::GeomID(readcs.hsamp_.start_.lineNr()) );
	velprovider_->setSelData( sd );
    }

    TimeDepthDataLoader loader( *arr, *velprovider_, readcs, veldesc_,
	    SamplingData<double>(voi.zsamp_), velintime_, voiintime_[idx] );
    if ( !trprov.execute( loader ) )
	return false;

    return true;
}


class Time2DepthStretcherProcessor : public ParallelTask
{
public:
Time2DepthStretcherProcessor( FloatMathFunction& func,
    const StepInterval<float>& zrg, const Interval<float>& trg,
    const SamplingData<float>& sd, float* res, int nriter )
    : samplfunc_( func )
    , zrg_( zrg )
    , trg_( trg )
    , sd_( sd )
    , res_( res )
    , nriter_( nriter )
{
    trg_.sort();
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    float depth = zrg_.center();
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	const float t = sd_.atIndex( idx );
	res_[idx] = trg_.includes(t,false) &&
		findValue( samplfunc_, zrg_.start, zrg_.stop, depth, t )
	    ? depth
	    : mUdf(float);
    }

    return true;
}

od_int64 nrIterations() const	{ return nriter_; }

int minThreadSize() const	{ return 50; }

    FloatMathFunction&		samplfunc_;
    const StepInterval<float>	zrg_;
    Interval<float>		trg_;
    const SamplingData<float>	sd_;
    float*			res_;
    int				nriter_;
};


void Time2DepthStretcher::transformTrc(const TrcKey& trckey,
				    const SamplingData<float>& sd,
				    int sz, float* res ) const
{
    const BinID bid = trckey.binID();
    if ( bid.isUdf() )
	return;

    const Interval<float> resrg = sd.interval(sz);
    int bestidx = -1;
    float largestwidth = mUdf(float);
    for ( int idx=0; idx<voivols_.size(); idx++ )
    {
	if ( !voidata_[idx] || !voivols_[idx].hsamp_.includes(bid) )
	    continue;

	const Interval<float> voirg = getTimeInterval( bid, idx );
	if ( mIsUdf(voirg.start) || mIsUdf(voirg.stop) )
	    continue;

	Interval<float> tmp( resrg );
	if ( !voirg.overlaps( resrg, false ) )
	    continue;

	tmp.limitTo( voirg );

	const float width = tmp.width();
	if ( bestidx==-1 || width>largestwidth )
	{
	    bestidx = idx;
	    largestwidth = width;
	}
    }

    if ( bestidx==-1 )
    {
	ArrayValueSeries<float,float> vs( res, false );
	udfFill( vs, sz );
	return;
    }

    const Array3D<float>& arr = *voidata_[bestidx];
    const TrcKeySampling& hrg = voivols_[bestidx].hsamp_;
    const od_int64 offset = arr.info().getOffset(hrg.inlIdx(bid.inl()),
						 hrg.crlIdx(bid.crl()), 0 );
    const int zsz = arr.getSize(2);
    const OffsetValueSeries<float> vs( *arr.valueSeries(), offset, zsz );

    const StepInterval<float> zrg = voivols_[bestidx].zsamp_;
    SampledFunctionImpl<float,ValueSeries<float> > samplfunc( vs, zsz,
	    zrg.start, zrg.step );

    if ( voiintime_[bestidx] )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const float t = sd.atIndex( idx );
	    res[idx] = zrg.includes(t,false)
		? samplfunc.getValue(t)
		: mUdf(float);
	}
    }
    else
    {
	Time2DepthStretcherProcessor proc( samplfunc, zrg,
		       Interval<float>( vs[0],vs[zsz-1]), sd, res, sz );
	proc.execute();
    }
}


void Time2DepthStretcher::transformTrcBack(const TrcKey& trckey,
					const SamplingData<float>& sd,
					int sz, float* res ) const
{
    const BinID bid = trckey.binID();
    const Interval<float> resrg = sd.interval(sz);
    int bestidx = -1;
    float largestwidth = mUdf(float);
    for ( int idx=0; idx<voivols_.size(); idx++ )
    {
	if ( !voidata_[idx] || !voivols_[idx].hsamp_.includes(bid) )
	    continue;

	const Interval<float> voirg = getDepthInterval( bid, idx );
	if ( mIsUdf(voirg.start) || mIsUdf(voirg.stop) )
	    continue;

	Interval<float> tmp( resrg );
	if ( !voirg.overlaps( resrg, false ) )
	    continue;

	tmp.limitTo( voirg );

	const float width = tmp.width();
	if ( bestidx==-1 || width>largestwidth )
	{
	    bestidx = idx;
	    largestwidth = width;
	}
    }

    if ( bestidx==-1 )
    {
	ArrayValueSeries<float,float> vs( res, false );
	udfFill( vs, sz );
	return;
    }

    const Array3D<float>& arr = *voidata_[bestidx];
    const TrcKeySampling& hrg = voivols_[bestidx].hsamp_;
    const od_int64 offset = arr.info().getOffset(hrg.inlIdx(bid.inl()),
						 hrg.crlIdx(bid.crl()), 0 );
    const int zsz = arr.getSize(2);
    const OffsetValueSeries<float> vs( *arr.valueSeries(), offset,
				       zsz );

    const StepInterval<float> zrg = voivols_[bestidx].zsamp_;
    SampledFunctionImpl<float,ValueSeries<float> > samplfunc( vs, zsz,
	    zrg.start, zrg.step );

    if ( !voiintime_[bestidx] )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const float d = sd.atIndex( idx );
	    res[idx] = zrg.includes(d,false)
		? samplfunc.getValue(d)
		: mUdf(float);
	}
    }
    else
    {
	Time2DepthStretcherProcessor proc( samplfunc, zrg,
		       Interval<float>( vs[0],vs[zsz-1]), sd, res, sz );
	proc.execute();
    }
}


Interval<float> Time2DepthStretcher::getTimeInterval( const BinID& bid,
						      int idx) const
{
    if ( voiintime_[idx] )
	return voivols_[idx].zsamp_;

    return Interval<float>(
		voidata_[idx]->get(
			voivols_[idx].hsamp_.inlIdx(bid.inl()),
			voivols_[idx].hsamp_.crlIdx(bid.crl()),
			0 ),
		voidata_[idx]->get(
			voivols_[idx].hsamp_.inlIdx(bid.inl()),
			voivols_[idx].hsamp_.crlIdx(bid.crl()),
			voidata_[idx]->getSize(2)-1 ) );
}


Interval<float> Time2DepthStretcher::getDepthInterval( const BinID& bid,
						 int idx) const
{
    if ( !voiintime_[idx] )
	return voivols_[idx].zsamp_;

    const int inlidx = voivols_[idx].hsamp_.inlIdx(bid.inl());
    const int crlidx = voivols_[idx].hsamp_.crlIdx(bid.crl());
    const int zsz = voidata_[idx]->getSize(2);

    return Interval<float>( voidata_[idx]->get( inlidx, crlidx, 0 ),
			    voidata_[idx]->get( inlidx, crlidx, zsz-1 ) );
}


void Time2DepthStretcher::udfFill( ValueSeries<float>& res, int sz )
{
    for ( int idx=0; idx<sz; idx++ )
	res.setValue(idx,  mUdf(float) );
}


Interval<float>& getZRange( Interval<float>& zrg, float step, int userfac )
{
    const int stopidx = zrg.indexOnOrAfter( zrg.stop, step );
    zrg.stop = zrg.atIndex( stopidx, step );
    zrg.stop = mCast(float,mNINT32(zrg.stop*userfac))/userfac;
    return zrg;
}


static float getZStep( const Interval<float>& zrg, int userfac )
{
    const int nrsteps = SI().zRange().nrSteps();
    float zstep = zrg.width() / (nrsteps==0 ? 1 : nrsteps);
    zstep = zstep<1e-3f ? 1.0f : mNINT32(zstep*userfac);
    zstep /= userfac;
    return zstep;
}


Interval<float> Time2DepthStretcher::getZInterval( bool time ) const
{
    const bool survistime = SI().zIsTime();
    float seisrefdatum = SI().seismicReferenceDatum();
    if ( survistime && SI().depthsInFeet() )
	seisrefdatum *= mToFeetFactorF;

    Interval<float> res = SI().zRange();
    if ( survistime && !time )
    {
	res.start *= topvavg_.start/2;
	res.stop *= botvavg_.stop/2;
	res.shift( -seisrefdatum );
    }
    else if ( !survistime && time )
    {
	res.shift( seisrefdatum );
	res.start /= topvavg_.stop/2;
	res.stop /= botvavg_.start/2;
    }

    return getZRange( res, getGoodZStep(), toZDomainInfo().userFactor() );;
}


float Time2DepthStretcher::getGoodZStep() const
{
    if ( SI().zIsTime() )
	return SI().zRange().step * (topvavg_.start+botvavg_.stop) * 0.25f;

    return SI().zRange().step;
}


const char* Time2DepthStretcher::getZDomainID() const
{
    if ( !velprovider_ ) return 0;

    mDeclStaticString( ret );
    ret.set( velprovider_->dbKey().toString() );
    return ret.buf();
}


void Time2DepthStretcher::releaseData()
{
    deleteAndZeroPtr( velprovider_ );
    for ( int idx=0; idx<voidata_.size(); idx++ )
    {
	delete voidata_[idx];
	voidata_.replace( idx, 0 );
    }
}


//Depth2Time


Depth2TimeStretcher::Depth2TimeStretcher()
    : VelocityStretcher(ZDomain::Depth(),ZDomain::Time())
    , stretcher_( new Time2DepthStretcher )
{}


bool Depth2TimeStretcher::setVelData( const DBKey& dbkey )
{ return stretcher_->setVelData( dbkey ); }


bool Depth2TimeStretcher::isOK() const
{ return stretcher_ && stretcher_->isOK(); }


bool Depth2TimeStretcher::needsVolumeOfInterest() const
{ return stretcher_->needsVolumeOfInterest(); }


void Depth2TimeStretcher::fillPar( IOPar& par ) const
{
    stretcher_->fillPar( par );
    ZAxisTransform::fillPar( par );
}


bool Depth2TimeStretcher::usePar( const IOPar& par )
{ return stretcher_->usePar( par ); }


int Depth2TimeStretcher::addVolumeOfInterest(const TrcKeyZSampling& cs,
					     bool time )
{ return stretcher_->addVolumeOfInterest( cs, !time ); }


void Depth2TimeStretcher::setVolumeOfInterest( int id,
					       const TrcKeyZSampling& cs,
					       bool time )
{ stretcher_->setVolumeOfInterest( id, cs, !time ); }


void Depth2TimeStretcher::removeVolumeOfInterest( int id )
{ stretcher_->removeVolumeOfInterest( id ); }


bool Depth2TimeStretcher::loadDataIfMissing( int id,
					     const TaskRunnerProvider& trprov )
{ return stretcher_->loadDataIfMissing( id, trprov ); }


void Depth2TimeStretcher::transformTrc(const TrcKey& trckey,
				    const SamplingData<float>& sd,
				    int sz, float* res ) const
{ stretcher_->transformTrcBack( trckey, sd, sz, res ); }


void Depth2TimeStretcher::transformTrcBack(const TrcKey& trckey,
					const SamplingData<float>& sd,
					int sz, float* res ) const
{ stretcher_->transformTrc( trckey, sd, sz, res ); }


Interval<float> Depth2TimeStretcher::getZInterval( bool depth ) const
{
    return stretcher_->getZInterval( !depth );
}


float Depth2TimeStretcher::getGoodZStep() const
{
    if ( SI().zIsTime() )
	return SI().zRange().step;

    const Interval<float> topvavg = stretcher_->getVavgRg(true);
    const Interval<float> botvavg = stretcher_->getVavgRg(false);
    return 4 * SI().zRange().step / (topvavg.stop+botvavg.start);
}


const char* Depth2TimeStretcher::getZDomainID() const
{ return stretcher_->getZDomainID(); }


VelocityModelScanner::VelocityModelScanner( const IOObj& input,
					    const VelocityDesc& vd )
    : obj_( input )
    , vd_( vd )
    , msg_(tr("Velocity cube scanning "))
    , startavgvel_( -1, -1 )
    , stopavgvel_( -1, -1 )
    , provider_( 0 )
    , definedv0_( false )
    , definedv1_( false )
    , zistime_ ( SI().zIsTime() )
    , seisrefdatum_( SI().seismicReferenceDatum() )
{
    uiRetVal uirv;
    provider_ = Seis::Provider::create( input, &uirv );
    if ( !provider_ )
	{ msg_ = uirv; return; }

    zistime_ = ZDomain::isTime( input.pars() );
    if ( zistime_ && SI().depthsInFeet() )
	seisrefdatum_ *= mToFeetFactorF;
}


VelocityModelScanner::~VelocityModelScanner()
{
    delete provider_;
}


od_int64 VelocityModelScanner::totalNr() const
{
    return provider_ ? provider_->totalNr() : 0;
}


od_int64 VelocityModelScanner::nrDone() const
{
    return provider_ ? provider_->nrDone() : 0;
}


int VelocityModelScanner::nextStep()
{
    if ( !provider_ )
	return ErrorOccurred();

    SeisTrc veltrace;
    const uiRetVal uirv = provider_->getNext( veltrace );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	{
	    if ( startavgvel_.start<0 || stopavgvel_.start<0 )
	    {
		msg_ = tr("Velocity volume is not defined");
		return ErrorOccurred();
	    }
	    return Finished();
	}

	msg_ = uirv;
	return ErrorOccurred();
    }

    const int sz = veltrace.size();
    if ( sz<2 )
	return MoreToDo();

    const SeisTrcValueSeries trcvs( veltrace, 0 );
    const SamplingData<double> sd = veltrace.info().sampling_;

    TimeDepthConverter tdconverter;
    if ( !tdconverter.setVelocityModel(trcvs,sz,sd,vd_,zistime_).isOK() )
	return MoreToDo();

    ArrayValueSeries<float, float> resvs( sz );

    if ( zistime_ )
    {
	if ( !tdconverter.calcDepths( resvs, sz, sd ) )
	    return MoreToDo();
    }
    else
    {
	if ( !tdconverter.calcTimes( resvs, sz, sd ) )
	    return MoreToDo();
    }

    int first = -1, last = -1;
    for ( int idx=0; idx<sz; idx++ )
	if ( !mIsUdf(resvs.value(idx) ) )
	    { first = idx; break; }

    for ( int idx=sz-1; idx>=0; idx-- )
	if ( !mIsUdf(resvs.value(idx) ) )
	    { last = idx; break; }

    if ( first!=-1 && last!=-1 && first!=last )
    {
	float firsttime = mCast(float,sd.atIndex(first));
	if ( !zistime_ ) firsttime -= seisrefdatum_;

	float v0 = -1;
	if ( firsttime > 0 )
	{
	    float firstvalue = resvs.value( first );
	    if ( zistime_ ) firstvalue += seisrefdatum_;
	    v0 = zistime_ ? 2*firstvalue/firsttime
			  : (firstvalue>1e-4 ? 2*firsttime/firstvalue : 1500);
	}
	else
	{
	    const float diff0 = resvs.value(first+1) - resvs.value(first);
	    v0 = (float)( zistime_ ? 2 * diff0 / sd.step : 2 * sd.step / diff0);
	}

	if ( v0 > 0 )
	{
	    if ( !definedv0_ )
	    {
		definedv0_ = true;
		startavgvel_.start = startavgvel_.stop = v0;
	    }
	    else
		startavgvel_.include( v0 );
	}

	float lasttime = mCast(float,sd.atIndex(last));
	if ( !zistime_ ) lasttime -= seisrefdatum_;
	float lastvalue = resvs.value( last );
	if ( zistime_ ) lastvalue += seisrefdatum_;

	const float v1 = zistime_ ? 2*lastvalue/lasttime : 2*lasttime/lastvalue;
	if ( !definedv1_ )
	{
	    definedv1_ = true;
	    stopavgvel_.start = stopavgvel_.stop = v1;
	}
	else
	    stopavgvel_.include( v1 );
    }

    return MoreToDo();
}


const char* lineartranskey = "V0,dV";


LinearVelTransform::LinearVelTransform(const ZDomain::Def& from,
				       const ZDomain::Def& to,
				       float v0, float dv)
    : ZAxisTransform( from, to )
    , startvel_( v0 )
    , dv_( dv )
{}



bool LinearVelTransform::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar( par ) )
	return false;

    return par.get( lineartranskey, startvel_, dv_ );
}


void LinearVelTransform::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );
    par.set( lineartranskey, startvel_, dv_ );
}


void LinearVelTransform::transformT2D( const SamplingData<float>& sd,
				       int sz, float* res ) const
{
    float seisrefdatum = SI().seismicReferenceDatum();
    if ( SI().zIsTime() && SI().depthsInFeet() )
	seisrefdatum *= mToFeetFactorF;

    if ( !computeLinearT2D( startvel_, dv_, -seisrefdatum, sd, sz, res ) )
    {
	for ( int idx=0; idx<sz; idx++ )
	    res[idx] = mUdf(float);
    }
}


void LinearVelTransform::transformD2T(const SamplingData<float>& sd,
				      int sz, float* res ) const
{
    float seisrefdatum = SI().seismicReferenceDatum();
    if ( SI().zIsTime() && SI().depthsInFeet() )
	seisrefdatum *= mToFeetFactorF;

    if ( !computeLinearD2T( startvel_, dv_, -seisrefdatum, sd, sz, res ) )
    {
	for ( int idx=0; idx<sz; idx++ )
	    res[idx] = mUdf(float);
    }
}


//LinearT2DTransform
LinearT2DTransform::LinearT2DTransform( float startvel, float dv )
: LinearVelTransform(ZDomain::Time(),ZDomain::Depth(), startvel, dv )

{}


void LinearT2DTransform::transformTrc( const TrcKey&,
				    const SamplingData<float>& sd,
				    int sz, float* res ) const
{ transformT2D( sd, sz, res ); }


void LinearT2DTransform::transformTrcBack( const TrcKey&,
					const SamplingData<float>& sd,
					int sz, float* res ) const
{ transformD2T( sd, sz, res ); }


Interval<float> LinearT2DTransform::getZInterval( bool time ) const
{
    Interval<float> zrg = SI().zRange();
    const bool survistime = SI().zIsTime();
    if ( time && survistime ) return zrg;

    TrcKey tk( BinID(0,0) );
    if ( survistime && !time )
    {
	zrg.start = ZAxisTransform::transformTrc( tk, zrg.start );
	zrg.stop = ZAxisTransform::transformTrc( tk, zrg.stop );
    }
    else if ( !survistime && time )
    {
	zrg.start = ZAxisTransform::transformTrcBack( tk, zrg.start );
	zrg.stop = ZAxisTransform::transformTrcBack( tk, zrg.stop );
    }

    return getZRange( zrg, getGoodZStep(), toZDomainInfo().userFactor() );
}


float LinearT2DTransform::getGoodZStep() const
{
    if ( !SI().zIsTime() )
	return SI().zRange().step;

    TrcKey tk( BinID(0,0) );
    Interval<float> zrg = SI().zRange();
    zrg.start = ZAxisTransform::transformTrc( tk, zrg.start );
    zrg.stop = ZAxisTransform::transformTrc( tk, zrg.stop );
    return getZStep( zrg, toZDomainInfo().userFactor() );
}



//LinearD2TTransform
LinearD2TTransform::LinearD2TTransform( float startvel, float dv )
    : LinearVelTransform(ZDomain::Depth(),ZDomain::Time(), startvel, dv)
{}


void LinearD2TTransform::transformTrc( const TrcKey&,
				    const SamplingData<float>& sd,
				    int sz, float* res ) const
{ transformD2T( sd, sz, res ); }


void LinearD2TTransform::transformTrcBack( const TrcKey&,
					const SamplingData<float>& sd,
					int sz, float* res ) const
{ transformT2D( sd, sz, res ); }


Interval<float> LinearD2TTransform::getZInterval( bool depth ) const
{
    Interval<float> zrg = SI().zRange();
    const bool survistime = SI().zIsTime();
    if ( !survistime && depth ) return zrg;

    TrcKey tk( BinID(0,0) );
    if ( survistime && depth )
    {
	zrg.start = ZAxisTransform::transformTrcBack( tk, zrg.start );
	zrg.stop = ZAxisTransform::transformTrcBack( tk, zrg.stop );
    }
    else if ( !survistime && !depth )
    {
	zrg.start = ZAxisTransform::transformTrc( tk, zrg.start );
	zrg.stop = ZAxisTransform::transformTrc( tk, zrg.stop );
    }

    return getZRange( zrg, getGoodZStep(), toZDomainInfo().userFactor() );
}


float LinearD2TTransform::getGoodZStep() const
{
    if ( SI().zIsTime() )
	return SI().zRange().step;

    TrcKey tk( BinID(0,0) );
    Interval<float> zrg = SI().zRange();
    zrg.start = ZAxisTransform::transformTrc( tk, zrg.start );
    zrg.stop = ZAxisTransform::transformTrc( tk, zrg.stop );
    return getZStep( zrg, toZDomainInfo().userFactor() );
}
