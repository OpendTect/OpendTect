/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "timedepthconv.h"

#include "arrayndimpl.h"
#include "binidvalue.h"
#include "trckeyzsampling.h"
#include "datapackbase.h"
#include "genericnumer.h"
#include "indexinfo.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "samplfunc.h"
#include "seisbounds.h"
#include "seisdatapack.h"
#include "seisread.h"
#include "seispreload.h"
#include "seispacketinfo.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "separstr.h"
#include "survinfo.h"
#include "timedepthmodel.h"
#include "unitofmeasure.h"
#include "veldescimpl.h"
#include "zdomain.h"
#include "zvalseriesimpl.h"


// TimeDepthDataLoaderNew

class TimeDepthDataLoaderNew : public SequentialTask
{ mODTextTranslationClass(TimeDepthDataLoaderNew);
public:

		    TimeDepthDataLoaderNew(const TrcKeySampling&,
					   const VelocityDesc&,
					   const ZDomain::Info& velzinfo,
					   const RegularZValues& voizvals,
					   const ZDomain::Info& voirevzinfo,
					   SeisTrcReader&,Array3D<float>&);
		    ~TimeDepthDataLoaderNew();

private:

    uiString			msg_;
    int				nrdone_ = 0;
    od_int64			totalnr_;

    uiString	uiMessage() const override		{ return msg_; }
    uiString	uiNrDoneText() const override
					{ return ParallelTask::sPosFinished(); }
    od_int64	nrDone() const override			{ return nrdone_; }
    od_int64	totalNr() const override		{ return totalnr_; }

    int		nextStep() override;

    const TrcKeySampling&	tks_;
    TrcKeySamplingIterator	hiter_;
    SeisTrcReader&		reader_;
    Array3D<float>&		arr_;
    ConstRefMan<RegularSeisDataPack> seisdatapack_;
    Vel::Worker			worker_;
    const ZDomain::Info&	velzinfo_;
    const RegularZValues&	voizvals_;
    const ZDomain::Info&	voirevzinfo_;

};


TimeDepthDataLoaderNew::TimeDepthDataLoaderNew( const TrcKeySampling& tks,
			const VelocityDesc& vd, const ZDomain::Info& velzinfo,
			const RegularZValues& voizvals,
			const ZDomain::Info& voirevzinfo,
			SeisTrcReader& reader, Array3D<float>& arr )
    : tks_(tks)
    , hiter_(tks)
    , totalnr_(tks.totalNr())
    , worker_(vd,SI().seismicReferenceDatum(),
	      UnitOfMeasure::surveyDefSRDStorageUnit())
    , velzinfo_(velzinfo)
    , voizvals_(voizvals)
    , voirevzinfo_(voirevzinfo)
    , reader_(reader)
    , arr_(arr)
{
    msg_ = tr("Reading velocity model");
    seisdatapack_ = Seis::PLDM().get<RegularSeisDataPack>(
						reader.ioObj()->key() );
}


TimeDepthDataLoaderNew::~TimeDepthDataLoaderNew()
{
}


int TimeDepthDataLoaderNew::nextStep()
{
    TrcKey tk;
    if ( !hiter_.next(tk) )
	return Finished();

    const int nrz = arr_.info().getSize( 2 );
    const int icomp = 0;
    const od_int64 offset = arr_.info().getOffset( tks_.inlIdx(tk.inl()),
						   tks_.crlIdx(tk.crl()), 0 );
    OffsetValueSeries<float> arrvs( *arr_.getStorage(), offset, nrz );

    /*TODOVEL: The velocity stretcher should provide an array of t0
      if the velocity is of type RMS and statics are defined in veldesc_.
      Difficult to do here without access to EarthModel    */
    const double t0 = 0.;

    ConstPtrMan<const ValueSeries<double> > Vin;
    ConstPtrMan<const ZValueSeries> zvals_in;
    PtrMan<SeisTrc> veltrace;
    PtrMan<ValueSeries<float> > trcvs;
    if ( seisdatapack_ )
    {
	const int globidx = seisdatapack_->getGlobalIdx( tk );
	const OffsetValueSeries<float> dptrcvs =
				seisdatapack_->getTrcStorage( icomp, globidx );
	trcvs = dptrcvs.clone();
	zvals_in = new RegularZValues( seisdatapack_->sampling().zsamp_,
				       velzinfo_ );
    }
    else
    {
	mDynamicCastGet( SeisTrcTranslator*, veltranslator,
			 reader_.translator() );

	veltrace = new SeisTrc();
	bool res = true;
	if ( veltranslator->supportsGoTo() )
	    res = veltranslator->goTo( tk.position() );

	if ( res )
	    res = reader_.get( *veltrace );

	if ( !res )
	{
	    arrvs.setAll( mUdf(float) );
	    return MoreToDo();
	}

	trcvs = new SeisTrcValueSeries( *veltrace, icomp );
	zvals_in = new RegularZValues( veltrace->info().sampling,
				       veltrace->size(), velzinfo_ );
    }

    Vin = ScaledValueSeries<double,float>::getFrom( *trcvs );
    PtrMan<ValueSeries<float> > zsrc;
    PtrMan<ArrayZValues<float> > zout;
    if ( arrvs.arr() )
    {
	zout = new ArrayZValues<float>( arrvs.arr(), arrvs.size(),
					voirevzinfo_ );
    }
    else
    {
	zsrc = new ArrayValueSeries<float,float>( arrvs.size() );
	if ( !zsrc->isOK() )
	    return MoreToDo();

	arrvs.getValues( *zsrc, arrvs.size() );
	zout = new ArrayZValues<float>( zsrc->arr(), zsrc->size(),
					voirevzinfo_ );
    }

    if ( !worker_.getSampledZ(*Vin,*zvals_in,voizvals_,*zout,t0) )
    {
	arrvs.setAll( mUdf(float) );
	nrdone_++;
	return MoreToDo();
    }

    nrdone_++;
    return MoreToDo();
}


// Time2DepthStretcherProcessorNew

class Time2DepthStretcherProcessorNew : public ParallelTask
{
public:
Time2DepthStretcherProcessorNew( FloatMathFunction& func,
			const ZSampling& zrg, const Interval<float>& trg,
			const SamplingData<float>& sd, float* res, int nriter )
    : samplfunc_(func)
    , zrg_(zrg)
    , trg_(trg)
    , sd_(sd)
    , res_(res)
    , nriter_(nriter)
{
    trg_.sort();
}


private:

od_int64 nrIterations() const override	{ return nriter_; }

int minThreadSize() const override	{ return 50; }

bool doWork( od_int64 start, od_int64 stop, int ) override
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

    FloatMathFunction&		samplfunc_;
    const ZSampling		zrg_;
    Interval<float>		trg_;
    const SamplingData<float>	sd_;
    float*			res_;
    int				nriter_;
};


// VelocityStretcher (new version)

VelocityStretcherNew::VelocityStretcherNew( const ZDomain::Def& from,
					    const ZDomain::Def& to,
					    const MultiID& mid )
    : ZAxisTransform(from,to)
    , veldesc_(*new VelocityDesc(OD::VelocityType::Interval,
			     UnitOfMeasure::surveyDefVelUnit()->getLabel()))
    , topvavg_(getDefaultVAvg().start,getDefaultVAvg().start)
    , botvavg_(getDefaultVAvg().stop,getDefaultVAvg().stop)
{
    voidata_.setNullAllowed();
    if ( !mid.isUdf() )
	setVelData( mid );
}


VelocityStretcherNew::~VelocityStretcherNew()
{
    delete velreader_;
    delete &veldesc_;
    deepErase( voidata_ );
}


void VelocityStretcherNew::releaseData()
{
    deleteAndNullPtr( velreader_ );
    deepErase( voidata_ );
}


bool VelocityStretcherNew::isOK() const
{
    uiRetVal uirv;
    if ( !velzinfo_ ||
	 !VelocityDesc::isUsable(veldesc_.type_,velzinfo_->def_,uirv) )
    {
	errmsg_ = tr("Provided velocity is not usable");
	errmsg_.appendPhrases( uirv );
	return false;
    }

    if ( !velreader_ )
    {
	errmsg_ = tr("No reader set");
	return false;
    }

    return ZAxisTransform::isOK();
}


MultiID VelocityStretcherNew::getVelID() const
{
    MultiID ret;
    if ( velreader_ && velreader_->isOK() )
	ret = velreader_->ioObj()->key();
    return ret;
}


bool VelocityStretcherNew::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar(par) )
	return false;

    MultiID vid;
    if ( par.get(VelocityDesc::sKeyVelocityVolume(),vid) && !setVelData(vid) )
	return false;

    return true;
}


void VelocityStretcherNew::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );
    if ( velreader_ && velreader_->ioObj() )
	par.set( VelocityDesc::sKeyVelocityVolume(),velreader_->ioObj()->key());
}


bool VelocityStretcherNew::setVelData( const MultiID& mid )
{
    releaseData();
    PtrMan<IOObj> velioobj = IOM().get( mid );
    if ( !velioobj )
	return false;

    velreader_ = new SeisTrcReader( *velioobj );
    if ( !velreader_->prepareWork() )
    {
	releaseData();
	return false;
    }

    fromzdomaininfo_.setID( mid );
    tozdomaininfo_.setID( mid );
    veldesc_.usePar( velioobj->pars() );
    velzinfo_ = &velreader_->zDomain();

    topvavg_ = Interval<float>( getDefaultVAvg().start, getDefaultVAvg().start);
    botvavg_ = Interval<float>( getDefaultVAvg().stop, getDefaultVAvg().stop);
    getRange( velioobj->pars(), veldesc_, true, topvavg_ );
    getRange( velioobj->pars(), veldesc_, false, botvavg_ );

    //TODO: Reload eventual VOIs
    return true;
}


bool VelocityStretcherNew::loadDataIfMissing( int id, TaskRunner* taskr )
{
    if ( !velreader_ )
	return true;

    mDynamicCastGet(SeisTrcTranslator*,veltranslator,velreader_->translator())
    if ( !veltranslator )
	return false;

    const int idx = voiids_.indexOf( id );
    if ( idx<0 )
	return false;

    const TrcKeyZSampling& voi = voivols_[idx];
    Array3D<float>* arr = voidata_[idx];
    if ( !arr )
    {
	arr = new Array3DImpl<float>( voi.nrInl(), voi.nrCrl(), voi.nrZ() );
	if ( !arr->isOK() )
	{
	    delete arr;
	    return false;
	}

	delete voidata_.replace( idx, arr );
    }

    const TrcKeySampling& tks = voi.hsamp_;
    if ( velreader_->is2D() ) // Now geomid is known. Have to recreate reader.
    {
	const Seis::GeomType gt = Seis::Line;
	PtrMan<IOObj> ioobj = velreader_->ioObj()->clone();
	delete velreader_;
	velreader_ = new SeisTrcReader( *ioobj, tks.getGeomID(), &gt );
	velreader_->prepareWork();
	velzinfo_ = &velreader_->zDomain();
    }

    if ( !velzinfo_ )
	return false;

    const RegularZValues zvals_out( voi.zsamp_, *voizinfos_[idx] );
    TimeDepthDataLoaderNew loader( tks, veldesc_, *velzinfo_, zvals_out,
				*voirevzinfos_[idx], *velreader_, *arr );
    if ( !TaskRunner::execute(taskr,loader) )
	return false;

    return true;
}


int VelocityStretcherNew::addVolumeOfInterest( const TrcKeyZSampling& tkzs,
					       bool zistrans )
{
    int id = 0;
    while ( voiids_.isPresent(id) )
	id++;

    const ZDomain::Info& zdomain = zistrans ? toZDomainInfo()
					    : fromZDomainInfo();
    const ZDomain::Info& revzdomain = zistrans ? fromZDomainInfo()
					       : toZDomainInfo();
    voidata_ += nullptr;
    voivols_ += tkzs;
    voizinfos_ += &zdomain;
    voirevzinfos_ += &revzdomain;
    voiids_ += id;

    return id;
}


void VelocityStretcherNew::setVolumeOfInterest( int id,
						const TrcKeyZSampling& tkzs,
						bool zistrans )
{
    const int idx = voiids_.indexOf( id );
    if ( idx<0 )
	return;

    const ZDomain::Info& domain = zistrans ? toZDomainInfo()
					   : fromZDomainInfo();
    if ( domain == *voizinfos_[idx] && tkzs==voivols_[idx] )
	return;

    const ZDomain::Info& revzdomain = zistrans ? fromZDomainInfo()
					       : toZDomainInfo();

    delete voidata_.replace( idx, nullptr );
    voivols_[idx] = tkzs;
    voizinfos_.replace( idx, &domain );
    voirevzinfos_.replace( idx, &revzdomain );
}


void VelocityStretcherNew::removeVolumeOfInterest( int id )
{
    const int idx = voiids_.indexOf( id );
    if ( idx<0 )
	return;

    delete voidata_.removeSingle( idx );
    voivols_.removeSingle( idx );
    voizinfos_.removeSingle( idx );
    voirevzinfos_.removeSingle( idx );
    voiids_.removeSingle( idx );
}


void VelocityStretcherNew::transformTrc( const TrcKey& trckey,
					 const SamplingData<float>& sd,
					 int sz, float* res ) const
{
    doTransform( trckey, sd, fromZDomainInfo(), sz, res );
}


void VelocityStretcherNew::transformTrcBack( const TrcKey& trckey,
					     const SamplingData<float>& sd,
					     int sz, float* res ) const
{
    doTransform( trckey, sd, toZDomainInfo(), sz, res );
}


void VelocityStretcherNew::doTransform( const TrcKey& trckey,
					const SamplingData<float>& sd,
					const ZDomain::Info& sdzinfo,
					int sz, float* res ) const
{
    const BinID bid = trckey.position();
    if ( bid.isUdf() )
	return;

    if ( sd.isUdf() )
    {
	OD::sysMemValueSet( res, mUdf(float), sz );
	return;
    }

    const Interval<float> resrg = sd.interval( sz );
    int bestidx = -1;
    float largestwidth = mUdf(float);
    for ( int idx=0; idx<voivols_.size(); idx++ )
    {
	if ( !voidata_[idx] || !voivols_[idx].hsamp_.includes(bid) )
	    continue;

	const Interval<float> voirg = getInterval( bid, idx, sdzinfo );
	if ( voirg.isUdf() )
	    continue;

	Interval<float> tmp( resrg );
	if ( !voirg.overlaps(resrg,false) )
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
	OD::sysMemValueSet( res, mUdf(float), sz );
	return;
    }

    const Array3D<float>& arr = *voidata_[bestidx];
    const TrcKeySampling& hrg = voivols_[bestidx].hsamp_;
    const od_int64 offset = arr.info().getOffset(hrg.inlIdx(bid.inl()),
						 hrg.crlIdx(bid.crl()), 0 );
    const int zsz = arr.info().getSize(2);
    const OffsetValueSeries<float> vs( *arr.getStorage(), offset, zsz );

    const ZSampling zrg = voivols_[bestidx].zsamp_;
    SampledFunctionImpl<float,ValueSeries<float> > samplfunc( vs, zsz,
							zrg.start, zrg.step );
    if ( sdzinfo == *voizinfos_[bestidx] )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const float zval = sd.atIndex( idx );
	    res[idx] = zrg.includes(zval,false) ? samplfunc.getValue(zval)
						: mUdf(float);
	}
    }
    else
    {
	Time2DepthStretcherProcessorNew proc( samplfunc, zrg,
		       Interval<float>( vs[0],vs[zsz-1]), sd, res, sz );
	proc.execute();
    }
}


Interval<float> VelocityStretcherNew::getInterval( const BinID& bid, int idx,
					    const ZDomain::Info& zinfo ) const
{
    if ( zinfo == *voizinfos_[idx] )
	return voivols_[idx].zsamp_;

    const int inlidx = voivols_[idx].hsamp_.inlIdx(bid.inl());
    const int crlidx = voivols_[idx].hsamp_.crlIdx(bid.crl());
    const int zsz = voidata_[idx]->info().getSize(2);

    return Interval<float>( voidata_[idx]->get( inlidx, crlidx, 0 ),
			    voidata_[idx]->get( inlidx, crlidx, zsz-1 ) );
}


Interval<float> VelocityStretcherNew::getZInterval( bool from ) const
{
    return getZInterval( from, true );
}


float VelocityStretcherNew::getGoodZStep() const
{
    return getZInterval( false, true ).step;
}


ZSampling VelocityStretcherNew::getZInterval( bool isfrom,
					      bool makenice ) const
{
    const ZDomain::Info& from = SI().zDomainInfo();
    const ZDomain::Info& to = isfrom ? fromZDomainInfo() : toZDomainInfo();
    return getZInterval( SI().zRange(true), from, to, makenice );
}


ZSampling VelocityStretcherNew::getZInterval( const ZSampling& zsamp,
					       const ZDomain::Info& from,
					       const ZDomain::Info& to,
					       bool makenice ) const
{
    ZSampling ret = getWorkZSampling( zsamp, from, to );
    if ( makenice && from != to )
    {
	const int userfac = to.def_.userFactor();
	float zstep = ret.step;
	zstep = zstep<1e-3f ? 1.0f : mNINT32(zstep*userfac);
	zstep /= userfac;
	ret.step = zstep;

	const Interval<float>& rg = ret;
	const int startidx = rg.indexOnOrAfter( rg.start, zstep );
	ret.start = zstep * mNINT32( rg.atIndex( startidx, zstep ) / zstep );
	const int stopidx = rg.indexOnOrAfter( rg.stop, zstep );
	ret.stop = zstep * mNINT32( rg.atIndex( stopidx, zstep ) / zstep );
    }

    return ret;
}


ZSampling VelocityStretcherNew::getWorkZSampling( const ZSampling& zsamp,
					      const ZDomain::Info& from,
					      const ZDomain::Info& to ) const
{
    if ( topvavg_.isUdf() || botvavg_.isUdf() ) //Not !isOK()
	return ZSampling::udf();

    const UnitOfMeasure* zuom = UnitOfMeasure::surveyDefDepthUnit();
    const float seisrefdatum = getConvertedValue( SI().seismicReferenceDatum(),
			       UnitOfMeasure::surveyDefSRDStorageUnit(), zuom );

    const int nrsamples = zsamp.nrSteps();
    ZSampling ret = zsamp;
    if ( from.isTime() && to.isDepth() )
    {
	ret.start *= topvavg_.start/2.f;
	ret.stop *= botvavg_.stop/2.f;
	ret.shift( -seisrefdatum );
    }
    else if ( from.isDepth() && to.isTime() )
    {
	ret.shift( seisrefdatum );
	ret.start /= topvavg_.stop/2.f;
	ret.stop /= botvavg_.start/2.f;
    }

    if ( to != from )
	ret.step = (ret.width()) / (nrsamples==0 ? 1 : nrsamples);

    return ret;
}


const UnitOfMeasure* VelocityStretcherNew::velUnit() const
{
    return Vel::Worker::getUnit( veldesc_ );
}


Interval<float> VelocityStretcherNew::getDefaultVAvg() const
{
    return getDefaultVAvg( velUnit() );
}


Interval<float> VelocityStretcherNew::getDefaultVAvg(
					const UnitOfMeasure* veluom )
{
    Interval<float> res( 1350.f, 4500.f );
    res.start = veluom->getUserValueFromSI( res.start );
    res.stop = veluom->getUserValueFromSI( res.stop );
    return res;
}


void VelocityStretcherNew::setRange( const Interval<float>& rg,
				     const VelocityDesc& desc, bool top,
				     IOPar& par )
{
    const char* velrgkey = top ? sKeyTopVavg() : sKeyBotVavg();
    if ( !desc.isVelocity() )
    {
	par.removeWithKey( velrgkey );
	return;
    }

    FileMultiString fms;
    fms.add( rg.start ).add( rg.stop )
       .add( Vel::Worker::getUnit( desc )->getLabel() );
    par.set( velrgkey, fms );
}


bool VelocityStretcherNew::getRange( const IOPar& par, const VelocityDesc& desc,
				     bool top, Interval<float>& rg )
{
    const char* velrgkey = top ? sKeyTopVavg() : sKeyBotVavg();
    FileMultiString fms;
    if ( !par.get(velrgkey,fms) || fms.size() < 2 )
	return false;

    Interval<float> ret;
    ret.start = fms.getFValue( 0 );
    ret.stop = fms.getFValue( 1 );
    if ( ret.isUdf() )
	return false;

    const UnitOfMeasure* veluom = Vel::Worker::getUnit( desc );
    const UnitOfMeasure* velstoruom = veluom;
    if ( fms.size() > 2 )
    {
	const StringView unitlbl = fms[2];
	const UnitOfMeasure* rgstoruom = UoMR().get( unitlbl );
	if ( rgstoruom && rgstoruom->isCompatibleWith(*veluom) )
	    velstoruom = rgstoruom;
    }

    convValue( ret.start, velstoruom, veluom );
    convValue( ret.stop, velstoruom, veluom );

    rg = ret;
    return true;
}


ZSampling VelocityStretcherNew::getWorkZSampling( const ZSampling& zsamp,
					    const ZDomain::Info& from,
					    const ZDomain::Info& to,
					    const IOPar& par, bool makenice )
{
    VelocityDesc desc;
    if ( !desc.usePar(par) || !desc.isVelocity() )
	return ZSampling::udf();

    Interval<float> topvavg = Interval<float>::udf();
    Interval<float> botvavg = Interval<float>::udf();
    if ( !getRange(par,desc,true,topvavg) || !getRange(par,desc,false,botvavg) )
	return ZSampling::udf();

    return getWorkZSampling( zsamp, from, to, topvavg, botvavg,
			     Vel::Worker::getUnit( desc ), makenice );
}


ZSampling VelocityStretcherNew::getWorkZSampling( const ZSampling& zsamp,
					    const ZDomain::Info& from,
					    const ZDomain::Info& to,
					    const Interval<float>& topvelrg,
					    const Interval<float>& botvelrg,
					    const UnitOfMeasure* vavguom,
					    bool makenice )
{
    RefMan<VelocityStretcherNew> stretcher;
    if ( to.isDepth() )
	stretcher = new Time2DepthStretcherNew();
    else if ( to.isTime() )
	stretcher = new Depth2TimeStretcherNew();
    else
	return ZSampling::udf();

    stretcher->topvavg_ = topvelrg;
    stretcher->botvavg_ = botvelrg;
    const UnitOfMeasure* veluom = stretcher->velUnit();
    convValue( stretcher->topvavg_.start, vavguom, veluom );
    convValue( stretcher->topvavg_.stop, vavguom, veluom );
    convValue( stretcher->botvavg_.start, vavguom, veluom );
    convValue( stretcher->botvavg_.stop, vavguom, veluom );

    return stretcher->getZInterval( zsamp, from, to, makenice );
}


// Time2DepthStretcher (new version)

Time2DepthStretcherNew::Time2DepthStretcherNew( const MultiID& mid )
    : VelocityStretcherNew(ZDomain::Time(),ZDomain::Depth(),mid)
{}


Time2DepthStretcherNew::Time2DepthStretcherNew()
    : Time2DepthStretcherNew(MultiID::udf())
{}


Time2DepthStretcherNew::~Time2DepthStretcherNew()
{}


// Depth2TimeStretcher (new version)

Depth2TimeStretcherNew::Depth2TimeStretcherNew( const MultiID& mid )
    : VelocityStretcherNew(ZDomain::Depth(),ZDomain::Time(),mid)
{}


Depth2TimeStretcherNew::Depth2TimeStretcherNew()
    : Depth2TimeStretcherNew(MultiID::udf())
{}


Depth2TimeStretcherNew::~Depth2TimeStretcherNew()
{}


// VelocityModelScanner (new version)

VelocityModelScannerNew::VelocityModelScannerNew( const IOObj& input,
						  const VelocityDesc& vd )
    : Executor("Velocity Scanner")
    , obj_(input)
    , srd_(SI().seismicReferenceDatum())
    , srduom_(UnitOfMeasure::surveyDefSRDStorageUnit())
    , veldesc_(vd)
    , vavgdesc_(*new VelocityDesc(OD::VelocityType::Avg,
				  Vel::Worker::getUnit(vd)->getLabel()))
    , startavgvel_(Interval<float>::udf())
    , stopavgvel_(Interval<float>::udf())
    , subsel_(true)
    , msg_(tr("Velocity cube scanning "))
{
}


VelocityModelScannerNew::~VelocityModelScannerNew()
{
    delete reader_;
    delete &vavgdesc_;
}


const UnitOfMeasure* VelocityModelScannerNew::velUnit() const
{
    return Vel::Worker::getUnit( vavgdesc_ );
}


bool VelocityModelScannerNew::goImpl( od_ostream* strm, bool first, bool last,
				      int delay )
{
    if ( !doPrepare(strm) )
	return false;

    const bool res = Executor::goImpl( strm, first, last, delay );
    return doFinish( res, strm );
}


bool VelocityModelScannerNew::doPrepare( od_ostream* )
{
    msg_ = tr("Velocity cube scanning ");

    delete reader_;
    reader_ = new SeisTrcReader( obj_ );
    if ( !reader_->prepareWork() )
    {
	msg_ = reader_->errMsg();
	return false;
    }

    velzinfo_ = &reader_->zDomain();

    mDynamicCastGet(Seis::Bounds2D*,b2d,reader_->getBounds());
    mDynamicCastGet(Seis::Bounds3D*,b3d,reader_->getBounds());
    if ( b2d )
	subsel_.set( Interval<int>(1,1), b2d->nrrg_ );
    else if ( b3d )
	subsel_ = b3d->tkzs_.hsamp_;

    hsiter_.setSampling( subsel_ );

    return true;
}


int VelocityModelScannerNew::nextStep()
{
    BinID curbid;
    if ( !hsiter_.next(curbid) )
	return Finished();

    mDynamicCastGet(SeisTrcTranslator*,veltranslator,reader_->translator());
    if ( !veltranslator )
    {
	msg_ = tr("Cannot read velocity volume");
	return ErrorOccurred();
    }

    nrdone_++;

    bool res = true;
    SeisTrc veltrace;
    if ( veltranslator->supportsGoTo() )
	res = veltranslator->goTo( curbid );

    if ( res )
	res = reader_->get( veltrace );

    const int sz = veltrace.size();
    if ( !res || sz<2 )
	return MoreToDo();

    const int icomp = 0;
    if ( !veltrace.updateVelocities(veldesc_,vavgdesc_,*velzinfo_,srd_,
				    srduom_,icomp) )
	return MoreToDo();

    for ( int idx=0; idx<sz; idx++ )
    {
	const float vavg = veltrace.get( idx, icomp );
	if ( mIsUdf(vavg) )
	    continue;

	startavgvel_.include( vavg );
	break;
    }

    for ( int idx=sz-1; idx>=0; idx-- )
    {
	const float vavg = veltrace.get( idx, icomp );
	if ( mIsUdf(vavg) )
	    continue;

	stopavgvel_.include( vavg );
	break;
    }

    return MoreToDo();
}


bool VelocityModelScannerNew::doFinish( bool success, od_ostream* )
{
    deleteAndNullPtr( reader_ );
    if ( success && (startavgvel_.isUdf() || stopavgvel_.isUdf()) )
    {
	msg_ = tr("Could not get a valid velocity range for that dataset");
	return false;
    }

    startavgvel_.sort();
    stopavgvel_.sort();

    return success;
}


// LinearVelTransform (new version)

const char* lineartranskey = "V0,dV";

LinearVelTransformNew::LinearVelTransformNew( const ZDomain::Def& from,
					      const ZDomain::Def& to,
					      double v0, double k )
    : ZAxisTransform(from,to)
    , v0_(v0)
    , k_(k)
{
    worker_ = new Vel::Worker( v0_, k_, SI().seismicReferenceDatum(), velUnit(),
			       UnitOfMeasure::surveyDefSRDStorageUnit() );
}


LinearVelTransformNew::~LinearVelTransformNew()
{
    delete worker_;
}


bool LinearVelTransformNew::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar(par) )
	return false;

    return par.get( lineartranskey, v0_, k_ );
}


void LinearVelTransformNew::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );
    par.set( lineartranskey, v0_, k_ );
}


bool LinearVelTransformNew::isOK() const
{
    return !mIsUdf(v0_) && !mIsUdf(k_) && worker_;
}


void LinearVelTransformNew::transformTrc( const TrcKey&,
					  const SamplingData<float>& sd,
					  int sz, float* res ) const
{
    doTransform( sd, fromZDomainInfo(), sz, res );
}


void LinearVelTransformNew::transformTrcBack( const TrcKey&,
					      const SamplingData<float>& sd,
					      int sz, float* res ) const
{
    doTransform( sd, toZDomainInfo(), sz, res );
}


void LinearVelTransformNew::doTransform( const SamplingData<float>& sd_out,
					 const ZDomain::Info& sdzinfo,
					 int sz, float* res ) const
{
    if ( sd_out.isUdf() )
    {
	OD::sysMemValueSet( res, mUdf(float), sz );
	return;
    }

    const RegularZValues zvals( sd_out, sz, sdzinfo );
    ArrayZValues<float> zout( res, sz, sdzinfo == toZDomainInfo()
				? fromZDomainInfo() : toZDomainInfo() );
    worker_->calcZLinear( zvals, zout );
}


Interval<float> LinearVelTransformNew::getZInterval( bool from ) const
{
    return getZInterval( from, true );
}


float LinearVelTransformNew::getGoodZStep() const
{
    return getZInterval( false, true ).step;
}


ZSampling LinearVelTransformNew::getZInterval( bool isfrom,
					       bool makenice ) const
{
    const ZDomain::Info& from = SI().zDomainInfo();
    const ZDomain::Info& to = isfrom ? fromZDomainInfo() : toZDomainInfo();
    return getZInterval( SI().zRange(true), from, to, makenice );
}


ZSampling LinearVelTransformNew::getZInterval( const ZSampling& zsamp,
					       const ZDomain::Info& from,
					       const ZDomain::Info& to,
					       bool makenice ) const
{
    ZSampling ret = getWorkZSampling( zsamp, from, to );
    if ( makenice && from != to )
    {
	const int userfac = to.def_.userFactor();
	float zstep = ret.step;
	zstep = zstep<1e-3f ? 1.0f : mNINT32(zstep*userfac);
	zstep /= userfac;
	ret.step = zstep;

	const Interval<float>& rg = ret;
	const int startidx = rg.indexOnOrAfter( rg.start, zstep );
	ret.start = zstep * mNINT32( rg.atIndex( startidx, zstep ) / zstep );
	const int stopidx = rg.indexOnOrAfter( rg.stop, zstep );
	ret.stop = zstep * mNINT32( rg.atIndex( stopidx, zstep ) / zstep );
    }

    return ret;
}


ZSampling LinearVelTransformNew::getWorkZSampling( const ZSampling& zsamp,
					     const ZDomain::Info& from,
					     const ZDomain::Info& to ) const
{
    if ( !isOK() )
	return ZSampling::udf();

    const int nrsamples = zsamp.nrSteps();
    ZSampling ret;
    if ( (from.isTime() && to.isDepth()) ||
	 (from.isDepth() && to.isTime()) )
    {
	const ArrayZValues<float> zvals( (float*)( &zsamp.start ), 2, from );
	ArrayZValues<float> zout( &ret.start, 2, to );
	worker_->calcZLinear( zvals, zout );
	ret.step = (ret.width()) / (nrsamples==0 ? 1 : nrsamples);
    }
    else
	ret = zsamp;

    return ret;
}


const UnitOfMeasure* LinearVelTransformNew::velUnit()
{
    return UnitOfMeasure::surveyDefVelUnit();
}


//LinearT2DTransform (new version)

LinearT2DTransformNew::LinearT2DTransformNew( double startvel, double dv )
    : LinearVelTransformNew(ZDomain::Time(),ZDomain::Depth(),startvel,dv)
{}


LinearT2DTransformNew::~LinearT2DTransformNew()
{}


//LinearD2TTransform (new version)

LinearD2TTransformNew::LinearD2TTransformNew( double startvel, double dv )
    : LinearVelTransformNew(ZDomain::Depth(),ZDomain::Time(),startvel,dv)
{}


LinearD2TTransformNew::~LinearD2TTransformNew()
{}


// OLD classes
#include "velocitycalc.h"

mStartAllowDeprecatedSection

VelocityStretcher::VelocityStretcher( const ZDomain::Def& from,
				      const ZDomain::Def& to )
    : ZAxisTransform(from,to)
{
}


VelocityStretcher::~VelocityStretcher()
{}


Time2DepthStretcher::Time2DepthStretcher()
    : VelocityStretcher(ZDomain::Time(),ZDomain::Depth())
    , velreader_( 0 )
    , topvavg_ ( getDefaultVAvg().start, getDefaultVAvg().start )
    , botvavg_ ( getDefaultVAvg().stop, getDefaultVAvg().stop )
{
    voidata_.allowNull( true );
}


Time2DepthStretcher::~Time2DepthStretcher()
{ releaseData(); }


bool Time2DepthStretcher::setVelData( const MultiID& mid )
{
    releaseData();
    PtrMan<IOObj> velioobj = IOM().get( mid );
    if ( !velioobj )
	return false;

    velreader_ = new SeisTrcReader( *velioobj );
    if ( !velreader_->prepareWork() )
    {
	releaseData();
	return false;
    }

    fromzdomaininfo_.setID( mid );
    tozdomaininfo_.setID( mid );

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
    if ( velreader_ && velreader_->ioObj() )
    {
	par.set( VelocityDesc::sKeyVelocityVolume(),
		 velreader_->ioObj()->key() );
    }
}


bool Time2DepthStretcher::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar( par ) )
	return false;

    MultiID vid;
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
				    SeisTrcReader& reader,
				    const TrcKeyZSampling& readcs,
				    const VelocityDesc& vd,
				    const SamplingData<double>& voisd,
				    bool velintime,
				    bool voiintime,
				    const Scaler* scaler )
		    : arr_(arr)
		    , reader_(reader)
		    , readcs_(readcs)
		    , hiter_(readcs.hsamp_)
		    , voisd_(voisd)
		    , veldesc_(vd)
		    , velintime_(velintime)
		    , voiintime_(voiintime)
		    , nrdone_(0)
		    , seisdatapack_(nullptr)
		    , scaler_(scaler)
		{
		    seisdatapack_ = Seis::PLDM().get<RegularSeisDataPack>(
						    reader.ioObj()->key() );
		}
protected:

    int		nextStep() override;
    od_int64	totalNr() const override
		{ return readcs_.hsamp_.nrCrl()*readcs_.hsamp_.nrInl(); }
    od_int64	nrDone() const override		{ return nrdone_; }
    uiString	uiMessage() const override
		{ return tr("Reading velocity model"); }
    uiString	uiNrDoneText() const override	{ return tr("Position read"); }

    TrcKeyZSampling		readcs_;
    SeisTrcReader&		reader_;
    Array3D<float>&		arr_;
    TimeDepthConverter		tdc_;
    VelocityDesc		veldesc_;
    bool			velintime_;
    bool			voiintime_;
    const RegularSeisDataPack*	seisdatapack_;

    int				nrdone_;

    SamplingData<double>	voisd_;

    TrcKeySamplingIterator	hiter_;

    const Scaler*		scaler_ = nullptr;
};


int TimeDepthDataLoader::nextStep()
{
    const int nrz = arr_.info().getSize( 2 );

    TrcKey tk;
    if ( !hiter_.next(tk) )
	return Finished();

    const od_int64 offset =
	arr_.info().getOffset(readcs_.hsamp_.inlIdx(tk.inl()),
				readcs_.hsamp_.crlIdx(tk.crl()), 0 );

    OffsetValueSeries<float> arrvs( *arr_.getStorage(), offset, nrz );

    if ( !seisdatapack_ )
    {
	mDynamicCastGet( SeisTrcTranslator*, veltranslator,
			reader_.translator() );

	SeisTrc velocitytrc;
	bool res = true;
	if ( veltranslator->supportsGoTo() )
	    res = veltranslator->goTo( tk.position() );

	if ( res )
	    res = reader_.get( velocitytrc );

	if ( !res )
	{
	    Time2DepthStretcher::udfFill( arrvs, nrz );
	    return MoreToDo();
	}

	const SeisTrcValueSeries trcvs( velocitytrc, 0 );
	tdc_.setVelocityModel( trcvs, velocitytrc.size(),
				velocitytrc.info().sampling, veldesc_,
				velintime_, scaler_ );
    }
    else
    {
	const int globidx = seisdatapack_->getGlobalIdx( tk );
	const OffsetValueSeries<float>& dptrcvs =
	    seisdatapack_->getTrcStorage( 0, globidx );

	const SamplingData<float> sd = seisdatapack_->sampling().zsamp_;
	tdc_.setVelocityModel( dptrcvs,
			       seisdatapack_->sampling().zsamp_.nrSteps()+1,
			       sd, veldesc_, velintime_, scaler_ );
    }

    nrdone_++;

    if ( voiintime_ )
    {
	if ( !tdc_.calcDepths(arrvs,nrz,voisd_) )
	{
	    Time2DepthStretcher::udfFill( arrvs, nrz );
	    return MoreToDo();
	}
    }
    else
    {
	if ( !tdc_.calcTimes(arrvs,nrz,voisd_) )
	{
	    Time2DepthStretcher::udfFill( arrvs, nrz );
	    return MoreToDo();
	}
    }

    return MoreToDo();
}


bool Time2DepthStretcher::loadDataIfMissing( int id, TaskRunner* taskr )
{
    if ( !velreader_ )
	return true;

    mDynamicCastGet(SeisTrcTranslator*,veltranslator,velreader_->translator())
    if ( !veltranslator )
	return false;

    const int idx = voiids_.indexOf( id );
    if ( idx<0 )
	return false;

    const TrcKeyZSampling& voi( voivols_[idx] );
    TrcKeyZSampling readcs( voi );

    const StepInterval<float> filezrg = veltranslator->packetInfo().zrg;
    const int nrsamplesinfile = filezrg.nrSteps()+1;
    if ( velintime_!=voiintime_[idx] )
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
    else
    {
	readcs.zsamp_.setFrom( filezrg );
    }

    Array3D<float>* arr = voidata_[idx];
    if ( !arr )
    {
	arr = new Array3DImpl<float>(voi.nrInl(),voi.nrCrl(),voi.nrZ()+1 );
	if ( !arr->isOK() )
	    return false;

	voidata_.replace( idx, arr );
    }

    if ( velreader_->is2D() ) // Now geomid is known. Have to recreate reader.
    {
	const Seis::GeomType gt = Seis::Line;
	const Pos::GeomID gid = readcs.hsamp_.getGeomID();
	PtrMan<Seis::SelData> sd = new Seis::RangeSelData( readcs );

	sd->setGeomID( Pos::GeomID(readcs.hsamp_.start_.lineNr()) );
	PtrMan<IOObj> ioobj = velreader_->ioObj()->clone();
	delete velreader_;
	velreader_ = new SeisTrcReader( *ioobj, gid, &gt );
	velreader_->prepareWork();
	if ( sd && !sd->isAll() )
	    velreader_->setSelData( sd.release() );
    }

    TimeDepthDataLoader loader( *arr, *velreader_, readcs, veldesc_,
	    SamplingData<double>(voi.zsamp_), velintime_, voiintime_[idx],
	    getVelUnitOfMeasure() );
    if ( !TaskRunner::execute( taskr, loader ) )
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


bool doWork( od_int64 start, od_int64 stop, int ) override
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

od_int64 nrIterations() const override	{ return nriter_; }

int minThreadSize() const override	{ return 50; }

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
    const BinID bid = trckey.position();

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
    const int zsz = arr.info().getSize(2);
    const OffsetValueSeries<float> vs( *arr.getStorage(), offset, zsz );

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
    const BinID bid = trckey.position();

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
    const int zsz = arr.info().getSize(2);
    const OffsetValueSeries<float> vs( *arr.getStorage(), offset, zsz );

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

    return
	Interval<float>( voidata_[idx]->get(
		     voivols_[idx].hsamp_.inlIdx(bid.inl()),
		     voivols_[idx].hsamp_.crlIdx(bid.crl()), 0 ),
		     voidata_[idx]->get( voivols_[idx].hsamp_.inlIdx(bid.inl()),
			voivols_[idx].hsamp_.crlIdx(bid.crl()),
			voidata_[idx]->info().getSize(2)-1 ) );
}


Interval<float> Time2DepthStretcher::getDepthInterval( const BinID& bid,
						 int idx) const
{
    if ( !voiintime_[idx] )
	return voivols_[idx].zsamp_;

    const int inlidx = voivols_[idx].hsamp_.inlIdx(bid.inl());
    const int crlidx = voivols_[idx].hsamp_.crlIdx(bid.crl());
    const int zsz = voidata_[idx]->info().getSize(2);

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
    return zrg;
}


float getZStep( const Interval<float>& zrg, int userfac )
{
    const int nrsteps = SI().zRange( true ).nrSteps();
    const float zstep = zrg.width() / (nrsteps==0 ? 1 : nrsteps);
    return zstep;
}


Interval<float> Time2DepthStretcher::getZInterval( bool time ) const
{
    const bool survistime = SI().zIsTime();
    float seisrefdatum = SI().seismicReferenceDatum();
    if ( survistime && SI().depthsInFeet() )
	seisrefdatum *= mToFeetFactorF;

    Interval<float> res = SI().zRange(true);
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
	return SI().zRange(true).step * (topvavg_.start+botvavg_.stop) * 0.25f;

    return SI().zRange(true).step;
}


MultiID Time2DepthStretcher::getZDomainID() const
{
    return velreader_ && velreader_->ioObj()
	? velreader_->ioObj()->key() : MultiID::udf();
}


void Time2DepthStretcher::releaseData()
{
    delete velreader_;
    velreader_ = 0;

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
{
}


Depth2TimeStretcher::~Depth2TimeStretcher()
{}


bool Depth2TimeStretcher::setVelData( const MultiID& mid )
{ return stretcher_->setVelData( mid ); }


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


bool Depth2TimeStretcher::loadDataIfMissing( int id, TaskRunner* trans )
{
    stretcher_->setVelUnitOfMeasure( getVelUnitOfMeasure() );
    return stretcher_->loadDataIfMissing( id, trans );
}


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
	return SI().zRange(true).step;

    const Interval<float> topvavg = stretcher_->getVavgRg(true);
    const Interval<float> botvavg = stretcher_->getVavgRg(false);
    return 4 * SI().zRange(true).step / (topvavg.stop+botvavg.start);
}


MultiID Depth2TimeStretcher::getZDomainID() const
{ return stretcher_->getZDomainID(); }


VelocityModelScanner::VelocityModelScanner( const IOObj& input,
					    const VelocityDesc& vd )
    : obj_( input )
    , vd_( vd )
    , msg_(tr("Velocity cube scanning "))
    , startavgvel_( -1, -1 )
    , stopavgvel_( -1, -1 )
    , subsel_( true )
    , reader_( new SeisTrcReader(obj_) )
    , definedv0_( false )
    , definedv1_( false )
    , zistime_ ( SI().zIsTime() )
    , nrdone_( 0 )
{
    reader_->prepareWork();
    mDynamicCastGet(Seis::Bounds2D*,b2d,reader_->getBounds());
    mDynamicCastGet(Seis::Bounds3D*,b3d,reader_->getBounds());
    if ( b2d ) subsel_.set( Interval<int>(1,1), b2d->nrrg_ );
    if ( b3d ) subsel_ = b3d->tkzs_.hsamp_;

    hsiter_.setSampling(  subsel_ );
    zistime_ = ZDomain::isTime( input.pars() );
}


VelocityModelScanner::~VelocityModelScanner()
{
    delete reader_;
}


int VelocityModelScanner::nextStep()
{
    BinID curbid;
    if ( !hsiter_.next( curbid ) )
    {
	if ( startavgvel_.start<0 || stopavgvel_.start<0 )
	{
	    msg_ = tr("Velocity volume is not defined for the selected type.");
	    return ErrorOccurred();
	}

	return Finished();
    }

    mDynamicCastGet(SeisTrcTranslator*,veltranslator,reader_->translator());
    if ( !veltranslator )
    {
	msg_ = tr("Cannot read velocity volume");
	return ErrorOccurred();
    }

    nrdone_++;

    bool res = true;
    SeisTrc veltrace;
    if ( veltranslator->supportsGoTo() )
	res = veltranslator->goTo( curbid );

    if ( res )
	res = reader_->get( veltrace );

    if ( !res )
	return MoreToDo();

    const SeisTrcValueSeries trcvs( veltrace, 0 );

    const int sz = veltrace.size();
    if ( sz<2 ) return MoreToDo();

    const SamplingData<double> sd = veltrace.info().sampling;

    TimeDepthConverter tdconverter;
    if ( !tdconverter.setVelocityModel(trcvs,sz,sd,vd_,zistime_,
			    &UnitOfMeasure::surveyDefVelUnit()->scaler()) )
	return MoreToDo();

    ArrayValueSeries<float,float> resvs( sz );
    if ( zistime_ )
    {
	if ( !tdconverter.calcDepths(resvs,sz,sd) )
	    return MoreToDo();
    }
    else
    {
	if ( !tdconverter.calcTimes(resvs,sz,sd) )
	    return MoreToDo();
    }

    int first = -1, last = -1;
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !mIsUdf(resvs.value(idx) ) )
	{
	    first = idx;
	    break;
	}
    }


    for ( int idx=sz-1; idx>=0; idx-- )
    {
	if ( !mIsUdf(resvs.value(idx) ) )
	{
	    last = idx;
	    break;
	}
    }

    float seisrefdatum = SI().seismicReferenceDatum();
    if ( zistime_ && SI().depthsInFeet() )
	seisrefdatum *= mToFeetFactorF;

    if ( first!=-1 && last!=-1 && first!=last )
    {
	float firsttime = mCast(float,sd.atIndex(first));
	if ( !zistime_ ) firsttime -= seisrefdatum;

	float v0 = -1;
	if ( firsttime > 0 )
	{
	    float firstvalue = resvs.value( first );
	    if ( zistime_ ) firstvalue += seisrefdatum;
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
	if ( !zistime_ ) lasttime -= seisrefdatum;
	float lastvalue = resvs.value( last );
	if ( zistime_ ) lastvalue += seisrefdatum;

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



LinearVelTransform::LinearVelTransform(const ZDomain::Def& from,
				       const ZDomain::Def& to,
				       float v0, float dv)
    : ZAxisTransform( from, to )
    , startvel_( v0 )
    , dv_( dv )
{}


LinearVelTransform::~LinearVelTransform()
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

    if ( sd.isUdf() ||
	    !computeLinearT2D( startvel_, dv_, -seisrefdatum, sd, sz, res ) )
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

    if ( sd.isUdf() ||
	    !computeLinearD2T( startvel_, dv_, -seisrefdatum, sd, sz, res ) )
    {
	for ( int idx=0; idx<sz; idx++ )
	    res[idx] = mUdf(float);
    }
}


//LinearT2DTransform
LinearT2DTransform::LinearT2DTransform( float startvel, float dv )
: LinearVelTransform(ZDomain::Time(),ZDomain::Depth(), startvel, dv )

{}


LinearT2DTransform::~LinearT2DTransform()
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
    Interval<float> zrg = SI().zRange( true );
    const bool survistime = SI().zIsTime();
    if ( time && survistime ) return zrg;

    BinIDValue startbidval( 0, 0, zrg.start );
    BinIDValue stopbidval( 0, 0, zrg.stop );
    if ( survistime && !time )
    {
	zrg.start = ZAxisTransform::transform( startbidval );
	zrg.stop = ZAxisTransform::transform( stopbidval );
    }
    else if ( !survistime && time )
    {
	zrg.start = ZAxisTransform::transformBack( startbidval );
	zrg.stop = ZAxisTransform::transformBack( stopbidval );
    }

    return getZRange( zrg, getGoodZStep(), toZDomainInfo().userFactor() );
}


float LinearT2DTransform::getGoodZStep() const
{
    if ( !SI().zIsTime() )
	return SI().zRange(true).step;

    Interval<float> zrg = SI().zRange( true );
    zrg.start = transform( BinIDValue(0,0,zrg.start) );
    zrg.stop = transform( BinIDValue(0,0,zrg.stop) );
    return getZStep( zrg, toZDomainInfo().userFactor() );
}



//LinearD2TTransform
LinearD2TTransform::LinearD2TTransform( float startvel, float dv )
    : LinearVelTransform(ZDomain::Depth(),ZDomain::Time(), startvel, dv)
{}


LinearD2TTransform::~LinearD2TTransform()
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
    Interval<float> zrg = SI().zRange( true );
    const bool survistime = SI().zIsTime();
    if ( !survistime && depth )	return zrg;

    BinIDValue startbidval( 0, 0, zrg.start );
    BinIDValue stopbidval( 0, 0, zrg.stop );
    if ( survistime && depth )
    {
	zrg.start = ZAxisTransform::transformBack( startbidval );
	zrg.stop = ZAxisTransform::transformBack( stopbidval );
    }
    else if ( !survistime && !depth )
    {
	zrg.start = ZAxisTransform::transform( startbidval );
	zrg.stop = ZAxisTransform::transform( stopbidval );
    }

    return getZRange( zrg, getGoodZStep(), toZDomainInfo().userFactor() );
}


float LinearD2TTransform::getGoodZStep() const
{
    if ( SI().zIsTime() )
	return SI().zRange(true).step;

    Interval<float> zrg = SI().zRange( true );
    zrg.start = transform( BinIDValue(0,0,zrg.start) );
    zrg.stop = transform( BinIDValue(0,0,zrg.stop) );
    return getZStep( zrg, toZDomainInfo().userFactor() );
}

mStopAllowDeprecatedSection
