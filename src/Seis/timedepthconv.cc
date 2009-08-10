/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : September 2007
-*/

static const char* rcsID = "$Id: timedepthconv.cc,v 1.14 2009-08-10 22:10:15 cvskris Exp $";

#include "timedepthconv.h"

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "datapackbase.h"
#include "genericnumer.h"
#include "indexinfo.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "samplfunc.h"
#include "seisread.h"
#include "seispacketinfo.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "zdomain.h"

const char* Time2DepthStretcher::sName() 	{ return "Time2Depth"; }

void Time2DepthStretcher::initClass()
{ ZATF().addCreator( create, sName() ); }


ZAxisTransform* Time2DepthStretcher::create()
{ return new Time2DepthStretcher; }


Time2DepthStretcher::Time2DepthStretcher()
    : velreader_( 0 )
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

    velreader_ = new SeisTrcReader( velioobj );
    if ( !velreader_->prepareWork() )
    {
	releaseData();
	return false;
    }

    veldesc_.type_ = VelocityDesc::Interval;
    veldesc_.usePar ( velioobj->pars() );

    BufferString depthdomain = ZDomain::getDefault().buf();
    velioobj->pars().get( ZDomain::sKey(), depthdomain );

    if ( depthdomain==ZDomain::sKeyTWT() )
	velintime_ = true;
    else if ( depthdomain==ZDomain::sKeyDepth() )
	velintime_ = false;
    else
    {
	releaseData();
	return false;
    }

    //TODO: Reload eventual VOIs
    return true;
}


bool Time2DepthStretcher::isOK() const
{
    if ( !TimeDepthConverter::isVelocityDescUseable( veldesc_, velintime_ ) )
    {
	errmsg_ = "Provided velocity is not useable";
	return false;
    }

    return true;
}



void Time2DepthStretcher::fillPar( IOPar& par ) const
{
    if ( velreader_ && velreader_->ioObj() )
	par.set( sKeyVelData(), velreader_->ioObj()->key() );
}


bool Time2DepthStretcher::usePar( const IOPar& par )
{
    MultiID vid;
    if ( par.get( sKeyVelData(), vid ) && !setVelData( vid ) )
	return false;

    return true;
}


int Time2DepthStretcher::addVolumeOfInterest(const CubeSampling& cs,
					     bool depth )
{
    int id = 0;
    while ( voiids_.indexOf(id)!=-1 ) id++;

    voidata_ += 0;
    voivols_ += cs;
    voiintime_ += !depth;
    voiids_ += id;

    return id;
}


void Time2DepthStretcher::setVolumeOfInterest( int id, const CubeSampling& cs,
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

    delete voidata_[idx];
    voidata_.remove( idx );
    voivols_.remove( idx );
    voiintime_.remove( idx );
    voiids_.remove( idx );
}


class TimeDepthDataLoader : public SequentialTask
{
public:
		TimeDepthDataLoader( Array3D<float>& arr,
				    SeisTrcReader& reader,
				    const CubeSampling& readcs,
				    const VelocityDesc& vd,
				    bool velintime,
				    bool voiintime )
		    : arr_( arr )
		    , reader_( reader )
		    , readcs_( readcs )
		    , hiter_( readcs.hrg )
		    , veldesc_( vd )
		    , velintime_( velintime )
		    , voiintime_( voiintime )
		    , nrdone_( 0 )
		{}
protected:

    od_int64            totalNr() const
			{ return readcs_.hrg.nrCrl()*readcs_.hrg.nrInl(); }
    od_int64            nrDone() const { return nrdone_; }
    const char*         message() const { return "Reading velocity model"; };
    const char*         nrDoneText() const { return "Position read"; }

    int                 nextStep()
    {
	const int nrz = arr_.info().getSize( 2 );

	if ( !hiter_.next( curbid_ ) )
	    return Finished();

	const od_int64 offset =
	    arr_.info().getOffset(readcs_.hrg.inlIdx(curbid_.inl),
				  readcs_.hrg.crlIdx(curbid_.crl), 0 );

	OffsetValueSeries<float> arrvs( *arr_.getStorage(), offset );

	mDynamicCastGet( SeisTrcTranslator*, veltranslator,
			reader_.translator() );

	SeisTrc velocitytrc;
	if ( !veltranslator->goTo(curbid_) || !reader_.get(velocitytrc) )
	{
	    Time2DepthStretcher::udfFill( arrvs, nrz );
		return MoreToDo();
	}

	const SeisTrcValueSeries trcvs( velocitytrc, 0 );
	tdc_.setVelocityModel( trcvs, velocitytrc.size(),
				velocitytrc.info().sampling, veldesc_,
				velintime_ );


	nrdone_++;

	if ( voiintime_ )
	{
	    if ( !tdc_.calcDepths(arrvs,nrz,SamplingData<double>(readcs_.zrg)))
	    {
		Time2DepthStretcher::udfFill( arrvs, nrz );
		return MoreToDo();
	    }
	}
	else
	{
	    if ( !tdc_.calcTimes(arrvs,nrz, SamplingData<double>(readcs_.zrg)))
	    {
		Time2DepthStretcher::udfFill( arrvs, nrz );
		return MoreToDo();
	    }
	}

	return MoreToDo();
    }

    CubeSampling        readcs_;
    SeisTrcReader&      reader_;
    Array3D<float>&     arr_;
    TimeDepthConverter  tdc_;
    VelocityDesc        veldesc_;
    bool                velintime_;
    bool                voiintime_;
    BinID               curbid_;
    int                 nrdone_;

    HorSamplingIterator hiter_;
};


bool Time2DepthStretcher::loadDataIfMissing( int id, TaskRunner* tr )
{
    if ( !velreader_ )
	return true;

    mDynamicCastGet( SeisTrcTranslator*, veltranslator,
		     velreader_->translator() );

    if ( !veltranslator || !veltranslator->supportsGoTo() )
	return false;

    const int idx = voiids_.indexOf( id );
    if ( idx<0 )
	return false;

    CubeSampling readcs( voivols_[idx] );

    const StepInterval<float> filezrg = veltranslator->packetInfo().zrg;
    const int nrsamplesinfile = filezrg.nrSteps()+1;
    int zstartidx = (int) filezrg.getIndex( readcs.zrg.start );
    if ( zstartidx<0 ) zstartidx = 0;
    int zstopidx = (int) filezrg.getIndex( readcs.zrg.stop )+1;
    if ( zstopidx>=nrsamplesinfile )
	zstopidx = nrsamplesinfile-1;

    readcs.zrg.start = filezrg.atIndex( zstartidx );
    readcs.zrg.stop = filezrg.atIndex( zstopidx );
    readcs.zrg.step = filezrg.step;

    Array3D<float>* arr = voidata_[idx];
    if ( !arr )
    {
	const int nrz = readcs.nrZ();
	arr =
	    new Array3DImpl<float>(readcs.nrInl(),readcs.nrCrl(),readcs.nrZ());
	if ( !arr->isOK() )
	    return false;

	voidata_.replace( idx, arr );
    }

    TimeDepthDataLoader loader( *arr, *velreader_, readcs, veldesc_,
				velintime_, voiintime_[idx] );
    if ( (tr && !tr->execute( loader ) ) || !loader.execute() )
	return false;

    return true;
}


void Time2DepthStretcher::transform(const BinID& bid,
				    const SamplingData<float>& sd,
				    int sz, float* res ) const
{
    const Interval<float> resrg = sd.interval(sz);
    int bestidx = -1;
    float largestwidth;
    for ( int idx=0; idx<voivols_.size(); idx++ )
    {
	if ( !voidata_[idx] )
	    continue;

	if ( !voivols_[idx].hrg.includes( bid ) )
	    continue;

	const Interval<float> voirg = getTimeInterval( bid, idx );
	if ( mIsUdf(voirg.start) || mIsUdf(voirg.stop) )
	    continue;

	Interval<float> tmp( resrg );
	if ( !voirg.overlaps( resrg ), false )
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
    const HorSampling& hrg = voivols_[bestidx].hrg;
    const od_int64 offset = arr.info().getOffset(hrg.inlIdx(bid.inl),
						 hrg.crlIdx(bid.crl), 0 );
    const OffsetValueSeries<float> vs( *arr.getStorage(), offset );
    const int zsz = arr.info().getSize(2);

    const StepInterval<float> zrg = voivols_[bestidx].zrg;
    SampledFunctionImpl<float,ValueSeries<float> > samplfunc( vs, zsz,
	    zrg.start, zrg.step );

    if ( voiintime_[bestidx] )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const float t = sd.atIndex( idx );
	    res[idx] = zrg.includes(t) ? samplfunc.getValue(t) : mUdf(float);
	}
    }
    else
    {
	const Interval<float> trg( vs[0], vs[zsz-1] );
	float depth;
	for ( int idx=0; idx<sz; idx++ )
	{
	    const float t = sd.atIndex( idx );
	    res[idx] = trg.includes(t) &&
		    findValue( samplfunc, zrg.start, zrg.stop, depth, t )
		?  res[idx] = depth : mUdf(float);
	}
	    
    }
}


void Time2DepthStretcher::transformBack(const BinID& bid,
					const SamplingData<float>& sd,
				        int sz, float* res ) const
{
    const Interval<float> resrg = sd.interval(sz);
    int bestidx = -1;
    float largestwidth;
    for ( int idx=0; idx<voivols_.size(); idx++ )
    {
	if ( !voidata_[idx] )
	    continue;

	if ( !voivols_[idx].hrg.includes( bid ) )
	    continue;

	const Interval<float> voirg = getDepthInterval( bid, idx );
	if ( mIsUdf(voirg.start) || mIsUdf(voirg.stop) )
	    continue;

	Interval<float> tmp( resrg );
	if ( !voirg.overlaps( resrg ), false )
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
    const HorSampling& hrg = voivols_[bestidx].hrg;
    const od_int64 offset = arr.info().getOffset(hrg.inlIdx(bid.inl),
						 hrg.crlIdx(bid.crl), 0 );
    const OffsetValueSeries<float> vs( *arr.getStorage(), offset );
    const int zsz = arr.info().getSize(2);

    const StepInterval<float> zrg = voivols_[bestidx].zrg;
    SampledFunctionImpl<float,ValueSeries<float> > samplfunc( vs, zsz,
	    zrg.start, zrg.step );

    if ( !voiintime_[bestidx] )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const float d = sd.atIndex( idx );
	    res[idx] = zrg.includes(d) ? samplfunc.getValue(d) : mUdf(float);
	}
    }
    else
    {
	const Interval<float> drg( vs[0], vs[zsz-1] );
	float time;
	for ( int idx=0; idx<sz; idx++ )
	{
	    const float d = sd.atIndex( idx );
	    res[idx] = drg.includes(d) &&
		    findValue( samplfunc, zrg.start, zrg.stop, time, d )
		?  res[idx] = time : mUdf(float);
	}
    }
}


Interval<float> Time2DepthStretcher::getTimeInterval( const BinID& bid,
						      int idx) const
{
    if ( voiintime_[idx] )
	return voivols_[idx].zrg;

    return
	Interval<float>( voidata_[idx]->get( voivols_[idx].hrg.inlIdx(bid.inl),
				    voivols_[idx].hrg.crlIdx(bid.crl), 0 ),
			 voidata_[idx]->get( voivols_[idx].hrg.inlIdx(bid.inl),
				    voivols_[idx].hrg.crlIdx(bid.crl), 
		   		    voidata_[idx]->info().getSize(2) ) );
}


Interval<float> Time2DepthStretcher::getDepthInterval( const BinID& bid,
						 int idx) const
{
    if ( !voiintime_[idx] )
	return voivols_[idx].zrg;

    const int inlidx = voivols_[idx].hrg.inlIdx(bid.inl);
    const int crlidx = voivols_[idx].hrg.crlIdx(bid.crl);
    const int zsz = voidata_[idx]->info().getSize(2);

    return Interval<float>( voidata_[idx]->get( inlidx, crlidx, 0 ),
			    voidata_[idx]->get( inlidx, crlidx, zsz-1 ) );
}


void Time2DepthStretcher::udfFill( ValueSeries<float>& res, int sz )
{
    for ( int idx=0; idx<sz; idx++ )
	res.setValue(idx,  mUdf(float) );
}



Interval<float> Time2DepthStretcher::getZInterval( bool time ) const
{
    const float factor = SI().defaultXYtoZScale(SurveyInfo::Second,
	    					SurveyInfo::Meter);

    Interval<float> res = SI().zRange(true);

    const bool survistime = SI().zIsTime();

    if ( survistime && !time )
    {
	res.start *= factor;
	res.stop *= factor;
    }
    else if ( !survistime && time )
    {
	res.start /= factor;
	res.stop /= factor;
    }

    return res;
}


float Time2DepthStretcher::getGoodZStep() const
{
    if ( SI().zIsTime() )
    {
	const float factor = SI().defaultXYtoZScale(SurveyInfo::Second,
						    SurveyInfo::Meter);
	return SI().zRange(true).step * factor;
    }

    return SI().zRange(true).step;
}


const char* Time2DepthStretcher::getToZDomainString() const
{ return ZDomain::sKeyDepth(); }


const char* Time2DepthStretcher::getFromZDomainString() const
{ return ZDomain::sKeyTWT(); }


const char* Time2DepthStretcher::getZDomainID() const
{ return velreader_ && velreader_->ioObj() ? velreader_->ioObj()->key() : 0; }


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



const char* Depth2TimeStretcher::sName() 	{ return "Depth2Time"; }

void Depth2TimeStretcher::initClass()
{ ZATF().addCreator( create, sName() ); }


ZAxisTransform* Depth2TimeStretcher::create()
{ return new Depth2TimeStretcher; }


Depth2TimeStretcher::Depth2TimeStretcher()
    : stretcher_( new Time2DepthStretcher )
{}


bool Depth2TimeStretcher::setVelData( const MultiID& mid )
{ return stretcher_->setVelData( mid ); }


bool Depth2TimeStretcher::isOK() const
{ return stretcher_ && stretcher_->isOK(); }


bool Depth2TimeStretcher::needsVolumeOfInterest() const
{ return stretcher_->needsVolumeOfInterest(); }


void Depth2TimeStretcher::fillPar( IOPar& par ) const
{ stretcher_->fillPar( par ); }


bool Depth2TimeStretcher::usePar( const IOPar& par )
{ return stretcher_->usePar( par ); }


int Depth2TimeStretcher::addVolumeOfInterest(const CubeSampling& cs, bool time )
{ return stretcher_->addVolumeOfInterest( cs, !time ); }


void Depth2TimeStretcher::setVolumeOfInterest( int id, const CubeSampling& cs,
					       bool time )
{ stretcher_->setVolumeOfInterest( id, cs, !time ); }


void Depth2TimeStretcher::removeVolumeOfInterest( int id )
{ stretcher_->removeVolumeOfInterest( id ); }


bool Depth2TimeStretcher::loadDataIfMissing( int id, TaskRunner* tr )
{ return stretcher_->loadDataIfMissing( id, tr ); }


void Depth2TimeStretcher::transform(const BinID& bid,
				    const SamplingData<float>& sd,
				    int sz, float* res ) const
{ stretcher_->transformBack( bid, sd, sz, res ); }


void Depth2TimeStretcher::transformBack(const BinID& bid,
					const SamplingData<float>& sd,
				        int sz, float* res ) const
{ stretcher_->transform( bid, sd, sz, res ); }


Interval<float> Depth2TimeStretcher::getZInterval( bool depth ) const
{
    const float factor = SI().defaultXYtoZScale(SurveyInfo::Second,
	    					SurveyInfo::Meter);

    Interval<float> res = SI().zRange(true);

    const bool survisdepth = !SI().zIsTime();

    if ( survisdepth && !depth )
    {
	res.start /= factor;
	res.stop /= factor;
    }
    else if ( !survisdepth && depth )
    {
	res.start *= factor;
	res.stop *= factor;
    }

    return res;
}


float Depth2TimeStretcher::getGoodZStep() const
{
    if ( SI().zIsTime() )
	return SI().zRange(true).step;

    const float factor = SI().defaultXYtoZScale(SurveyInfo::Second,
	    					SurveyInfo::Meter);
    return SI().zRange(true).step / factor;
}


const char* Depth2TimeStretcher::getToZDomainString() const
{ return stretcher_->getFromZDomainString(); }


const char* Depth2TimeStretcher::getFromZDomainString() const
{ return stretcher_->getToZDomainString(); }


const char* Depth2TimeStretcher::getZDomainID() const
{ return stretcher_->getZDomainID(); }


