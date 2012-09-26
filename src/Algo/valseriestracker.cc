/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "valseriestracker.h"

#include "genericnumer.h"
#include "iopar.h"
#include "samplfunc.h"


const char** EventTracker::sEventNames() 
{ 
    static const char* event_names[] = { "Min", "Max", "0+-", "0-+", 0 }; 
    return event_names; 
} 
 
 
const VSEvent::Type* EventTracker::cEventTypes() 
{ 
    static const VSEvent::Type event_types[] =
	{ VSEvent::Min, VSEvent::Max, VSEvent::ZCPosNeg, VSEvent::ZCNegPos };
	 
    return event_types; 
} 


int EventTracker::getEventTypeIdx( VSEvent::Type type )
{
    const char** evnames = sEventNames();
    const VSEvent::Type* types = cEventTypes();
    for ( int idx=0; evnames[idx]; idx++ )
    {
	if ( type==types[idx] )
	    return idx;
    }

    return -1;
}
 

ValSeriesTracker::ValSeriesTracker()
    : sourcevs_( 0 )
    , sourcedepth_( mUdf(float) )
    , sourcesize_( 0 )
    , targetvs_( 0 )
    , targetdepth_( mUdf(float) )
    , targetsize_( 0 )
{}


bool ValSeriesTracker::isOK() const
{ return sourcevs_ && targetvs_ && !mIsUdf(sourcedepth_); }


void ValSeriesTracker::setSource( const ValueSeries<float>* vs, int sz,
				  float depth )
{
    sourcevs_ = vs;
    sourcedepth_ = depth;
    sourcesize_ = sz;
}


void ValSeriesTracker::setTarget( const ValueSeries<float>* vs, int sz, 
				  float depth )
{
    targetvs_ = vs;
    targetdepth_ = depth;
    targetsize_ = sz;
}


EventTracker::EventTracker()
    : permrange_( -5, 5 )
    , ampthreshold_( mUdf(float) )
    , allowedvar_( 0.20 )
    , evtype_( VSEvent::Max )
    , useabsthreshold_( false )
    , similaritywin_( -10, 10 )
    , usesimilarity_( false )
    , similaritythreshold_( 0.8 )
    , normalizesimi_( false )
    , quality_( 1 )
    , rangestep_( 1 )
{
#define mAddAV(v) allowedvars_ += v
    mAddAV(0.01); mAddAV(0.02); mAddAV(0.05); mAddAV(0.1); mAddAV(0.2);
#undef mAddAV

}


void EventTracker::setPermittedRange( const Interval<float>& rg )
{
    permrange_ = rg;
    permrange_.sort();
}


bool EventTracker::isOK() const
{
    if ( !sourcevs_ || mIsUdf(sourcedepth_) )
    {
	if ( usesimilarity_ )
	    return false;
    }

    return targetvs_;
}


void EventTracker::setSource( const ValueSeries<float>* vs, int sz,
				  float depth )
{
    ValSeriesTracker::setSource( vs, sz, depth );
    setSourceAmpl( mUdf(float) );
}


const Interval<float>& EventTracker::permittedRange() const
{ return permrange_; }


void EventTracker::setTrackEvent( VSEvent::Type ev )
{ evtype_ = ev; }


VSEvent::Type EventTracker::trackEvent() const
{ return evtype_; }


void EventTracker::setAmplitudeThreshold( float th )
{ ampthreshold_ = th; }


float EventTracker::amplitudeThreshold() const
{ return ampthreshold_; }


void EventTracker::setAmplitudeThresholds( const TypeSet<float>& ats )
{ ampthresholds_ = ats; }


TypeSet<float>& EventTracker::getAmplitudeThresholds()
{ return ampthresholds_; }


void EventTracker::setAllowedVariance( float v )
{ allowedvar_ = v; }


float EventTracker::allowedVariance() const
{ return allowedvar_; }


void EventTracker::setAllowedVariances( const TypeSet<float>& avs )
{
    allowedvars_ = avs;
    //TODO sort the values in descending order
}


TypeSet<float>& EventTracker::getAllowedVariances()
{
    return allowedvars_;
}


void EventTracker::useSimilarity( bool yn )
{ usesimilarity_ = yn; }


bool EventTracker::usesSimilarity() const
{ return usesimilarity_; }


void EventTracker::normalizeSimilarityValues( bool yn )
{ normalizesimi_ = yn; }


bool EventTracker::normalizesSimilarityValues() const
{ return normalizesimi_; }


void EventTracker::setUseAbsThreshold( bool abs )
{ useabsthreshold_ = abs; }


bool EventTracker::useAbsThreshold() const
{ return useabsthreshold_; }


void EventTracker::setSimilarityWindow( const Interval<float>& rg )
{
    similaritywin_ = rg;
    similaritywin_.sort();
}


const Interval<float>& EventTracker::similarityWindow() const
{ return similaritywin_; }


void EventTracker::setSimilarityThreshold( float th )
{ similaritythreshold_ = th; }


float EventTracker::similarityThreshold() const
{ return similaritythreshold_; }


bool EventTracker::track()
{
    if ( !usesimilarity_ )
    {
	if ( useAbsThreshold() && !mIsUdf(ampthreshold_) )
	    return snap( amplitudeThreshold() );

	float refampl;
	if ( sourcevs_ )
	{
	    const SampledFunctionImpl<float,ValueSeries<float> >
		sampfunc( *sourcevs_, sourcesize_);
	    refampl = sampfunc.getValue( sourcedepth_ );
	}
	else if ( targetvs_ && !mIsUdf(targetdepth_) )
	{
	    const SampledFunctionImpl<float,ValueSeries<float> >
		sampfunc( *targetvs_, targetsize_);
	    refampl = sampfunc.getValue( targetdepth_ );
	}

	const float threshold = refampl * (1-allowedvar_);
	const bool res = snap( threshold );
	if ( res )
	{
	    const SampledFunctionImpl<float,ValueSeries<float> >
				sampfunc( *targetvs_, targetsize_);
	    const float resamp = sampfunc.getValue( targetdepth_ );
	    quality_ = (resamp-threshold)/(refampl-threshold);
	    if ( quality_>1 ) quality_ = 1;
	    else if ( quality_<0 ) quality_ = 0;
	}
	   
	return res;
    }

    const Interval<int> permsamplerange( mNINT32(permrange_.start/rangestep_),
				       mNINT32(permrange_.stop/rangestep_) );
    float upsample, upsim; bool upflatstart;
    const bool findup = permsamplerange.start<=0
	? findMaxSimilarity( -permsamplerange.start, -1, 1,
			     upsample, upsim, upflatstart )
	: false;

    float dnsample, dnsim; bool dnflatstart;
    const bool finddn = permsamplerange.stop>=0
	? findMaxSimilarity( permsamplerange.stop, 1, 1, dnsample,dnsim,dnflatstart)
	: false;

    if ( findup && finddn )
    {
	if ( upsim==dnsim )
	{
	    if ( upflatstart && dnflatstart )
	    {
		targetdepth_ = (upsample+dnsample) / 2;
		quality_ = (upsim+dnsim)/2;
	    }
	    else
	    {
		if ( fabs(dnsample)<fabs(upsample) )
		{
		    targetdepth_ = dnsample;
		    quality_ = dnsim;
		}
		else
		{
		    targetdepth_ = upsample;
		    quality_ = upsim;
		}
	    }
	}
	else
	{
	    if ( dnsim<upsim )
	    {
		targetdepth_ = upsample;
	        quality_ = upsim;
	    }
	    else
	    {
		targetdepth_ = dnsample;
		quality_ = dnsim;
	    }
	}
    }
    else if ( findup )
    {
	targetdepth_ = upsample;
	quality_ = upsim;
    }
    else if ( finddn )
    {
	targetdepth_ = dnsample;
	quality_ = dnsim;
    }
    else
	return false;

    const int bestidx = mNINT32( targetdepth_ );
    return snap( (*targetvs_)[bestidx] );
}


bool EventTracker::findMaxSimilarity( int nrtests, int step, int nrgracetests,
	float& res, float& maxsim, bool& flatstart ) const
{
    const Interval<int> similaritysamplewin( mNINT32(similaritywin_.start/rangestep_),
	    				     mNINT32(similaritywin_.stop/rangestep_) );
    int firstsourcesample = mNINT32(sourcedepth_) + similaritysamplewin.start;
    Interval<int> actualsimilaritywin = similaritysamplewin;
    if ( firstsourcesample<0 )
    {
	actualsimilaritywin.start -= firstsourcesample;
	firstsourcesample = 0;
    }

    if ( firstsourcesample+actualsimilaritywin.width(false)>=sourcesize_ )
	actualsimilaritywin.stop = sourcesize_-firstsourcesample;

    int firsttargetsample = mNINT32(targetdepth_)+actualsimilaritywin.start;
    if ( firsttargetsample<0 )
    {
	actualsimilaritywin.start -= firsttargetsample;
	firstsourcesample -= firsttargetsample;
	firsttargetsample = 0;
    }

    if ( firsttargetsample+actualsimilaritywin.width(false)>=targetsize_ )
	actualsimilaritywin.stop = targetsize_-firsttargetsample;

    if ( actualsimilaritywin.width(false)<=0 )
	return false;

    int gracecount = 0;
    int nreqsamples = 0;

    const int nrsamples = actualsimilaritywin.width(false)+1;

    for ( int idx=0; idx<nrtests; idx++ )
    {
	const int targetstart = firsttargetsample + idx*step;
	if ( targetstart<0 )
	    break;

	const float sim = similarity( *sourcevs_, *targetvs_, nrsamples,
		normalizesimi_, firstsourcesample, targetstart );

	if ( idx && sim<maxsim )
	{
	    if ( gracecount>=nrgracetests )
		break;

	    gracecount++;
	    continue;
	}
	else
	    gracecount = 0;

	if ( !idx || sim>maxsim )
	{
	    maxsim = sim;
	    res = idx;
	    nreqsamples = 0;
	}
	else if ( sim==maxsim )
	    nreqsamples++;
    }

    flatstart = nreqsamples && !res;
    res += ((float)nreqsamples)/2;

    res *= step;
    res += firsttargetsample - actualsimilaritywin.start;

    return maxsim>=similaritythreshold_;
}



void EventTracker::fillPar( IOPar& iopar ) const
{
    ValSeriesTracker::fillPar( iopar );
    iopar.set( sKeyTrackEvent(), VSEvent::getTypeString(evtype_) );
    iopar.set( sKeyPermittedRange(), permrange_ );
    iopar.set( sKeyValueThreshold(), ampthreshold_ );
    iopar.set( sKeyValueThresholds(), ampthresholds_ ); 
    iopar.set( sKeyAllowedVariance(), allowedvar_);
    iopar.set( sKeyAllowedVariances(), allowedvars_);
    iopar.setYN( sKeyUseAbsThreshold(), useabsthreshold_ );
    iopar.set( sKeySimWindow(), similaritywin_ );
    iopar.set( sKeySimThreshold(), similaritythreshold_ );
    iopar.setYN( sKeyTrackByValue(), !usesimilarity_ );
    iopar.setYN( sKeyNormSimi(), normalizesimi_ );
}


bool EventTracker::usePar( const IOPar& iopar )
{
    if ( !ValSeriesTracker::usePar( iopar ) )
	return false;

    VSEvent::parseEnumType( iopar.find( sKeyTrackEvent() ), evtype_ );
    iopar.get( sKeyPermittedRange(), permrange_ );
    iopar.get( sKeyValueThreshold(), ampthreshold_ );
    TypeSet<float> storedampthresholds;
    iopar.get( sKeyValueThresholds(), storedampthresholds);
    if ( storedampthresholds.size() != 0 )
	ampthresholds_ = storedampthresholds;
    iopar.get( sKeyAllowedVariance(), allowedvar_);
    TypeSet<float> storedallowedvars;
    iopar.get( sKeyAllowedVariances(), storedallowedvars );
    if ( storedallowedvars.size() != 0 )
	allowedvars_ = storedallowedvars;
    iopar.getYN( sKeyUseAbsThreshold(), useabsthreshold_ );
    iopar.get( sKeySimWindow(),similaritywin_ );
    iopar.getYN( sKeyNormSimi(), normalizesimi_ );
    iopar.get( sKeySimThreshold(), similaritythreshold_ );
    bool trackbyvalue;
    if ( iopar.getYN( sKeyTrackByValue(), trackbyvalue ) )
	usesimilarity_ = !trackbyvalue;

    return true;
}


bool EventTracker::snap( float threshold )
{
    if ( targetdepth_< 0 || targetdepth_>=targetsize_ )
	return false;

    const SamplingData<float> sd( 0, 1 );
    ValueSeriesEvFinder<float, float> evfinder( *targetvs_, targetsize_-1, sd );

    const Interval<int> permsamplerange( mNINT32(permrange_.start/rangestep_),
				       mNINT32(permrange_.stop/rangestep_) );
    const float upbound = targetdepth_ + permsamplerange.start - 0.01f;
    const float dnbound = targetdepth_ + permsamplerange.stop  + 0.01f;

    const Interval<float> uprg( targetdepth_, mMAX(0,upbound-1) );
    const Interval<float> dnrg( targetdepth_, mMIN(targetsize_-1, dnbound+1) );

    float eventpos;
    if ( evtype_==VSEvent::ZCNegPos || evtype_==VSEvent::ZCPosNeg )
    {
	ValueSeriesEvent<float, float> upevent =
	    evfinder.find( evtype_, uprg, 1 );
	ValueSeriesEvent<float, float> dnevent =
	    evfinder.find( evtype_, dnrg, 1 );

	const bool upfound = !mIsUdf(upevent.pos);
	const bool dnfound = !mIsUdf(dnevent.pos);

	if ( !upfound && !dnfound )
	    return false;
	else if ( upfound && dnfound )
	    eventpos =
		fabs(targetdepth_-upevent.pos)<fabs(targetdepth_-dnevent.pos)
		? upevent.pos : dnevent.pos;
	else
	    eventpos = upfound ? upevent.pos : dnevent.pos;
    }
    else if ( evtype_==VSEvent::Max || evtype_==VSEvent::Min )
    {
	float upampl;
	bool uploopskip = false;
	float uptroughampl;
	ValueSeriesEvent<float,float> upevent =
	    findExtreme(evfinder,uprg,threshold,upampl,uploopskip,uptroughampl);

	float dnampl;
	bool dnloopskip = false;
	float dntroughampl;
	ValueSeriesEvent<float,float> dnevent =
	    findExtreme(evfinder,dnrg,threshold,dnampl,dnloopskip,dntroughampl);

	float troughthreshold = !mIsUdf(threshold) ? -0.1f*threshold : 0;
	if ( evtype_==VSEvent::Min )
	{
	    troughthreshold *= -1;
	    uptroughampl *= -1;
	    dntroughampl *= -1;
	    upampl *= -1;
	    dnampl *= -1;
	}

	const bool uptrough = uploopskip && uptroughampl<=troughthreshold;
	const bool dntrough = dnloopskip && dntroughampl<=troughthreshold;

	const bool upfound = !mIsUdf(upevent.pos) && !uptrough;
	const bool dnfound = !mIsUdf(dnevent.pos) && !dntrough;

	if ( !upfound && !dnfound )
	    return false;
	else if ( upfound && dnfound )
	{
	    if ( uploopskip!=dnloopskip )
	    {
		eventpos = uploopskip && dnevent.pos<=dnbound
		    ? dnevent.pos : upevent.pos;
	    }
	    else
	    {
		if ( upampl==dnampl )
		{
		    if( fabs(upevent.pos-dnevent.pos)<1 )
			eventpos = (upevent.pos + dnevent.pos) / 2;
		    else 
		    {
			const float updiff = fabs( targetdepth_-upevent.pos );
			const float dndiff = fabs( targetdepth_-dnevent.pos );
			eventpos = updiff<dndiff ? upevent.pos : dnevent.pos;
		    }
		}
		else
		    eventpos = upampl>dnampl ? upevent.pos : dnevent.pos;
	    }
	}
	else
	    eventpos = upfound ? upevent.pos : dnevent.pos;

    }
    else
    {
	pErrMsg("Event not handled");
	return false;
    }

    if ( eventpos<upbound || eventpos>dnbound )
	return false;

    targetdepth_ = eventpos;
    return true;
}


ValueSeriesEvent<float,float> EventTracker::findExtreme(
    const ValueSeriesEvFinder<float, float>& eventfinder,
    const Interval<float>& rg, float threshold, float& avgampl,
    bool& hasloopskips, float& troughampl ) const
{
    const SamplingData<float>& sd = eventfinder.samplingData();
    const ValueSeries<float>& valser = eventfinder.valueSeries();

    ValueSeriesEvent<float,float> ev;
    int occ=1;
    while ( true )
    {
	ev = eventfinder.find( evtype_, rg, occ );
	if ( mIsUdf(ev.pos) )
	    return ev;

	if ( !mIsUdf(threshold) &&
		( (evtype_==VSEvent::Min && ev.val>threshold) ||
		    (evtype_==VSEvent::Max && ev.val<threshold)) )
	{
	    occ++;
	    continue;
	}

	break;
    }

    Interval<int> amplsumrg( sd.nearestIndex(rg.start),
    sd.nearestIndex(ev.pos) );
    const int inc = amplsumrg.start>amplsumrg.stop ? -1 : 1;
    avgampl = 0;
    hasloopskips = false;
    bool troughamplset = false;

    float prev = valser.value(amplsumrg.start);
    for ( int idx=amplsumrg.start;
	inc>0?idx<=amplsumrg.stop:idx>=amplsumrg.stop; idx+=inc )
    {
	const float val = valser.value(idx);
	if ( !troughamplset || (evtype_==VSEvent::Min && val>troughampl ) ||
			       (evtype_==VSEvent::Max && val<troughampl ) )
	{
	    troughamplset = true;
	    troughampl = val;
	}

	if ( !hasloopskips && ((evtype_==VSEvent::Min && val>prev ) ||
				(evtype_==VSEvent::Max && val<prev )) )
	{
	    hasloopskips = true;
	}


	avgampl += val;
    }

    avgampl /= amplsumrg.width()+1;
    return ev;
}

