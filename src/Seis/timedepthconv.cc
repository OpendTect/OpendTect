/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : September 2007
-*/

static const char* rcsID = "$Id: timedepthconv.cc,v 1.3 2008-01-07 22:38:19 cvskris Exp $";

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
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "velocitycalc.h"

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
    veldesc_.samplespan_ = VelocityDesc::Centered;

    veldesc_.usePar ( velioobj->pars() );

    BufferString depthdomain = SI().zIsTime() ? sKey::Time : sKey::Depth;
    velioobj->pars().get( sKey::ZDomain, depthdomain );

    if ( depthdomain==sKey::Time )
	velintime_ = true;
    else if ( depthdomain==sKey::Depth )
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
{ return veldesc_.type_==VelocityDesc::Interval; }


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


bool Time2DepthStretcher::loadDataIfMissing( int id )
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

    const CubeSampling& cs( voivols_[idx] );

    TimeDepthConverter tdc;
    Array3D<float>* arr = voidata_[idx];
    if ( !arr )
    {
	const int nrz = cs.nrZ();
	arr = new Array3DImpl<float>( cs.nrInl(), cs.nrCrl(), cs.nrZ() );
	if ( !arr->isOK() )
	    return false;

	voidata_.replace( idx, arr );
    }

    HorSamplingIterator hiter( cs.hrg );
    BinID curbid;
    const int nrz = arr->info().getSize( 2 );
    while ( hiter.next( curbid ) )
    {
	const od_int64 offset =
	    arr->info().getOffset(cs.hrg.inlIdx(curbid.inl),
				  cs.hrg.crlIdx(curbid.crl), 0 );

	OffsetValueSeries<float> arrvs( *arr->getStorage(), offset );

	SeisTrc velocitytrc;
	if ( !veltranslator->goTo(curbid) || !velreader_->get(velocitytrc) )
	{
	    udfFill( arrvs, nrz );
	    continue;
	}

	const SeisTrcValueSeries trcvs( velocitytrc, 0 );
	tdc.setVelocityModel( trcvs, velocitytrc.size(),
			      velocitytrc.info().sampling, veldesc_,
			      velintime_ );

	if ( voiintime_[idx] )
	{
	    if ( !tdc.calcDepths( arrvs, nrz, SamplingData<double>(cs.zrg) ) )
	    {
		udfFill( arrvs, nrz );
		continue;
	    }
	}
	else
	{
	    if ( !tdc.calcTimes( arrvs, nrz, SamplingData<double>(cs.zrg) ) )
	    {
		udfFill( arrvs, nrz );
		continue;
	    }
	}
    }


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

    return
	Interval<float>( voidata_[idx]->get( voivols_[idx].hrg.inlIdx(bid.inl),
				    voivols_[idx].hrg.crlIdx(bid.crl), 0 ),
			 voidata_[idx]->get( voivols_[idx].hrg.inlIdx(bid.inl),
				    voivols_[idx].hrg.crlIdx(bid.crl), 
		   		    voidata_[idx]->info().getSize(2) ) );
}


void Time2DepthStretcher::udfFill( ValueSeries<float>& res, int sz )
{
    for ( int idx=0; idx<sz; idx++ )
	res.setValue(idx,  mUdf(float) );
}



Interval<float> Time2DepthStretcher::getZInterval(bool from) const
{
    return Interval<float>( 0, 1 ); //TODO implement something real
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

    

