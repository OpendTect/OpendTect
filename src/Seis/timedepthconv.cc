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
#include "veldesc.h"
#include "zdomain.h"
#include "zvalseriesimpl.h"


// TimeDepthDataLoader

class TimeDepthDataLoader : public SequentialTask
{ mODTextTranslationClass(TimeDepthDataLoader);
public:

		    TimeDepthDataLoader(const TrcKeySampling&,
					const VelocityDesc&,
					const ZDomain::Info& velzinfo,
					const RegularZValues& voizvals,
					SeisTrcReader&,Array3D<float>&);
		    ~TimeDepthDataLoader();

private:

    uiString			msg_;
    int				nrdone_ = 0;
    od_int64			totalnr_;

    uiString	uiMessage() const override		{ return msg_; }
    uiString	uiNrDoneText() const override
					{ return ParallelTask::sPosFinished(); }
    od_int64	nrDone() const override			{ return nrdone_; }
    od_int64	totalNr() const override		{ return totalnr_; }

    bool	doPrepare(od_ostream*) override;
    int		nextStep() override;

    const TrcKeySampling&	tks_;
    TrcKeySamplingIterator	hiter_;
    SeisTrcReader&		reader_;
    Array3D<float>&		arr_;
    ConstRefMan<RegularSeisDataPack> seisdatapack_;
    Vel::Worker			worker_;
    const ZDomain::Info&	velzinfo_;
    const RegularZValues&	voizvals_;

};


TimeDepthDataLoader::TimeDepthDataLoader( const TrcKeySampling& tks,
			const VelocityDesc& vd, const ZDomain::Info& velzinfo,
			const RegularZValues& voizvals,
			SeisTrcReader& reader, Array3D<float>& arr )
    : tks_(tks)
    , hiter_(tks)
    , totalnr_(tks.totalNr())
    , worker_(vd,SI().seismicReferenceDatum(),
	      UnitOfMeasure::surveyDefSRDStorageUnit())
    , velzinfo_(velzinfo)
    , voizvals_(voizvals)
    , reader_(reader)
    , arr_(arr)
{
    seisdatapack_ = Seis::PLDM().get<RegularSeisDataPack>(
						reader.ioObj()->key() );
}


TimeDepthDataLoader::~TimeDepthDataLoader()
{
}


bool TimeDepthDataLoader::doPrepare( od_ostream* )
{
    msg_ = tr("Reading velocity model");

    return true;
}


int TimeDepthDataLoader::nextStep()
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
    PtrMan<ValueSeries<double> > Zout =
	ScaledValueSeries<double,float>::getFrom( arrvs );

    if ( !worker_.getSampledZ(*Vin,*zvals_in,voizvals_,*Zout,t0) )
    {
	arrvs.setAll( mUdf(float) );
	nrdone_++;
	return MoreToDo();
    }

    nrdone_++;
    return MoreToDo();
}


// Time2DepthStretcherProcessor

class Time2DepthStretcherProcessor : public ParallelTask
{
public:
Time2DepthStretcherProcessor( FloatMathFunction& func,
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


// VelocityStretcher

VelocityStretcher::VelocityStretcher( const ZDomain::Def& from,
				      const ZDomain::Def& to,
				      const MultiID& mid )
    : ZAxisTransform(from,to)
    , veldesc_(*new VelocityDesc(Vel::Interval,
				 UnitOfMeasure::surveyDefVelUnit()))
    , topvavg_(getDefaultVAvg().start,getDefaultVAvg().start)
    , botvavg_(getDefaultVAvg().stop,getDefaultVAvg().stop)
{
    voidata_.setNullAllowed();
    if ( !mid.isUdf() )
	setVelData( mid );
}


VelocityStretcher::~VelocityStretcher()
{
    delete velreader_;
    delete &veldesc_;
    deepErase( voidata_ );
}


void VelocityStretcher::releaseData()
{
    deleteAndNullPtr( velreader_ );
    deepErase( voidata_ );
}


bool VelocityStretcher::isOK() const
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


MultiID VelocityStretcher::getVelID() const
{
    MultiID ret;
    if ( velreader_ && velreader_->isOK() )
	ret = velreader_->ioObj()->key();
    return ret;
}


bool VelocityStretcher::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar(par) )
	return false;

    MultiID vid;
    if ( par.get(VelocityDesc::sKeyVelocityVolume(),vid) && !setVelData(vid) )
	return false;

    return true;
}


void VelocityStretcher::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );
    if ( velreader_ && velreader_->ioObj() )
	par.set( VelocityDesc::sKeyVelocityVolume(),velreader_->ioObj()->key());
}


bool VelocityStretcher::setVelData( const MultiID& mid )
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


bool VelocityStretcher::loadDataIfMissing( int id, TaskRunner* taskr )
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
    TimeDepthDataLoader loader( tks, veldesc_, *velzinfo_, zvals_out,
				*velreader_, *arr );
    if ( !TaskRunner::execute(taskr,loader) )
	return false;

    return true;
}


int VelocityStretcher::addVolumeOfInterest( const TrcKeyZSampling& tkzs,
					    bool zistrans )
{
    int id = 0;
    while ( voiids_.isPresent(id) )
	id++;

    const ZDomain::Info& zdomain = zistrans ? toZDomainInfo()
					    : fromZDomainInfo();
    voidata_ += nullptr;
    voivols_ += tkzs;
    voizinfos_ += &zdomain;
    voiids_ += id;

    return id;
}


void VelocityStretcher::setVolumeOfInterest( int id,
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

    delete voidata_.replace( idx, nullptr );
    voivols_[idx] = tkzs;
    voizinfos_.replace( idx, &domain );
}


void VelocityStretcher::removeVolumeOfInterest( int id )
{
    const int idx = voiids_.indexOf( id );
    if ( idx<0 )
	return;

    delete voidata_.removeSingle( idx );
    voivols_.removeSingle( idx );
    voizinfos_.removeSingle( idx );
    voiids_.removeSingle( idx );
}


void VelocityStretcher::transformTrc( const TrcKey& trckey,
				      const SamplingData<float>& sd,
				      int sz, float* res ) const
{
    doTransform( trckey, sd, fromZDomainInfo(), sz, res );
}


void VelocityStretcher::transformTrcBack(const TrcKey& trckey,
					const SamplingData<float>& sd,
					int sz, float* res ) const
{
    doTransform( trckey, sd, toZDomainInfo(), sz, res );
}


void VelocityStretcher::doTransform( const TrcKey& trckey,
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
	Time2DepthStretcherProcessor proc( samplfunc, zrg,
		       Interval<float>( vs[0],vs[zsz-1]), sd, res, sz );
	proc.execute();
    }
}


Interval<float> VelocityStretcher::getInterval( const BinID& bid, int idx,
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


ZSampling VelocityStretcher::getWorkZSampling( const ZSampling& zsamp,
					       const ZDomain::Info& from,
					       const ZDomain::Info& to ) const
{
    if ( topvavg_.isUdf() || botvavg_.isUdf() ) //Not !isOK()
	return ZSampling::udf();

    const UnitOfMeasure* zuom = UnitOfMeasure::surveyDefDepthUnit();
    const float seisrefdatum = getConvertedValue( SI().seismicReferenceDatum(),
			       UnitOfMeasure::surveyDefSRDStorageUnit(), zuom );

    ZSampling ret = zsamp;
    const int nrsamples = ret.nrSteps();
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


const UnitOfMeasure* VelocityStretcher::velUnit() const
{
    return veldesc_.getUnit();
}


Interval<float> VelocityStretcher::getDefaultVAvg() const
{
    return getDefaultVAvg( velUnit() );
}


Interval<float> VelocityStretcher::getDefaultVAvg( const UnitOfMeasure* veluom )
{
    Interval<float> res( 1350.f, 4500.f );
    res.start = veluom->getUserValueFromSI( res.start );
    res.stop = veluom->getUserValueFromSI( res.stop );
    return res;
}


void VelocityStretcher::setRange( const Interval<float>& rg,
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
    fms.add( rg.start ).add( rg.stop ).add( desc.getUnit()->getLabel() );
    par.set( velrgkey, fms );
}


bool VelocityStretcher::getRange( const IOPar& par, const VelocityDesc& desc,
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

    const UnitOfMeasure* veluom = desc.getUnit();
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


ZSampling VelocityStretcher::getWorkZSampling( const ZSampling& zsamp,
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
			     desc.getUnit(), makenice );
}


ZSampling VelocityStretcher::getWorkZSampling( const ZSampling& zsamp,
					 const ZDomain::Info& from,
					 const ZDomain::Info& to,
					 const Interval<float>& topvelrg,
					 const Interval<float>& botvelrg,
					 const UnitOfMeasure* vavguom,
					 bool makenice )
{
    RefMan<VelocityStretcher> stretcher;
    if ( to.isDepth() )
	stretcher = new Time2DepthStretcher();
    else if ( to.isTime() )
	stretcher = new Depth2TimeStretcher();
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


// Time2DepthStretcher

Time2DepthStretcher::Time2DepthStretcher( const MultiID& mid )
    : VelocityStretcher(ZDomain::Time(),ZDomain::Depth(),mid)
{}


Time2DepthStretcher::Time2DepthStretcher()
    : Time2DepthStretcher(MultiID::udf())
{}


Time2DepthStretcher::~Time2DepthStretcher()
{}


// Depth2TimeStretcher

Depth2TimeStretcher::Depth2TimeStretcher( const MultiID& mid )
    : VelocityStretcher(ZDomain::Depth(),ZDomain::Time(),mid)
{}


Depth2TimeStretcher::Depth2TimeStretcher()
    : Depth2TimeStretcher(MultiID::udf())
{}


Depth2TimeStretcher::~Depth2TimeStretcher()
{}


// VelocityModelScanner

VelocityModelScanner::VelocityModelScanner( const IOObj& input,
					    const VelocityDesc& vd )
    : obj_(input)
    , srd_(SI().seismicReferenceDatum())
    , srduom_(UnitOfMeasure::surveyDefSRDStorageUnit())
    , veldesc_(vd)
    , vavgdesc_(*new VelocityDesc(Vel::Avg,vd.getUnit()))
    , startavgvel_(Interval<float>::udf())
    , stopavgvel_(Interval<float>::udf())
    , subsel_(true)
    , msg_(tr("Velocity cube scanning "))
{
}


VelocityModelScanner::~VelocityModelScanner()
{
    delete reader_;
    delete &vavgdesc_;
}


const UnitOfMeasure* VelocityModelScanner::velUnit() const
{
    return vavgdesc_.getUnit();
}


bool VelocityModelScanner::doPrepare( od_ostream* )
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


int VelocityModelScanner::nextStep()
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


bool VelocityModelScanner::doFinish( bool success, od_ostream* )
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


// LinearVelTransform

const char* lineartranskey = "V0,dV";

LinearVelTransform::LinearVelTransform(const ZDomain::Def& from,
				       const ZDomain::Def& to,
				       double v0, double k )
    : ZAxisTransform(from,to)
    , v0_(v0)
    , k_(k)
    , srd_(SI().seismicReferenceDatum())
{
}


LinearVelTransform::~LinearVelTransform()
{}


bool LinearVelTransform::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar(par) )
	return false;

    return par.get( lineartranskey, v0_, k_ );
}


void LinearVelTransform::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );
    par.set( lineartranskey, v0_, k_ );
}


bool LinearVelTransform::isOK() const
{
    return !mIsUdf(v0_) && !mIsUdf(k_);
}


void LinearVelTransform::transformTrc( const TrcKey&,
				       const SamplingData<float>& sd,
				       int sz, float* res ) const
{
    doTransform( sd, fromZDomainInfo(), sz, res );
}


void LinearVelTransform::transformTrcBack( const TrcKey&,
					   const SamplingData<float>& sd,
					   int sz, float* res ) const
{
    doTransform( sd, toZDomainInfo(), sz, res );
}


void LinearVelTransform::doTransform( const SamplingData<float>& sd_out,
				      const ZDomain::Info& sdzinfo,
				      int sz, float* res ) const
{
    if ( sd_out.isUdf() )
    {
	OD::sysMemValueSet( res, mUdf(float), sz );
	return;
    }

    const RegularZValues zvals( sd_out, sz, sdzinfo );
    ArrayValueSeries<double,float> zout( res, false, sz );
    const Vel::Worker worker( v0_, k_, srd_, velUnit(),
			      UnitOfMeasure::surveyDefSRDStorageUnit() );
    worker.calcZLinear( zvals, zout );
}


ZSampling LinearVelTransform::getWorkZSampling( const ZSampling& zsamp,
						const ZDomain::Info& from,
						const ZDomain::Info& to ) const
{
    if ( !isOK() )
	return ZSampling::udf();

    ZSampling ret = zsamp;;
    const int nrsamples = ret.nrSteps();
    if ( (from.isTime() && to.isDepth()) ||
	 (from.isDepth() && to.isTime()) )
    {
	SamplingData<float> sd( ret.start, 1.f );
	doTransform( sd, from, 1, &ret.start );
	sd.start = ret.stop;
	doTransform( sd, from, 1, &ret.stop );
    }

    if ( to != from )
	ret.step = (ret.width()) / (nrsamples==0 ? 1 : nrsamples);

    return ret;
}


const UnitOfMeasure* LinearVelTransform::velUnit()
{
    return UnitOfMeasure::surveyDefVelUnit();
}


//LinearT2DTransform

LinearT2DTransform::LinearT2DTransform( double startvel, double dv )
    : LinearVelTransform(ZDomain::Time(),ZDomain::Depth(),startvel,dv)
{}


LinearT2DTransform::LinearT2DTransform( float startvel, float dv )
    : LinearT2DTransform(double(startvel),double(dv))
{}


LinearT2DTransform::~LinearT2DTransform()
{}



//LinearD2TTransform

LinearD2TTransform::LinearD2TTransform( double startvel, double dv )
    : LinearVelTransform(ZDomain::Depth(),ZDomain::Time(),startvel,dv)
{}


LinearD2TTransform::LinearD2TTransform( float startvel, float dv )
    : LinearD2TTransform(double(startvel),double(dv))
{}


LinearD2TTransform::~LinearD2TTransform()
{}
