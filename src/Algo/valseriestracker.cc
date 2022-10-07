/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "valseriestracker.h"

#include "genericnumer.h"
#include "iopar.h"
#include "samplfunc.h"

ValSeriesTracker::ValSeriesTracker()
    : sourcevs_(0)
    , sourcedepth_(mUdf(float))
    , sourcesize_(0)
    , targetvs_(0)
    , targetdepth_(mUdf(float))
    , targetsize_(0)
    , targetvalue_(mUdf(float))
{}


ValSeriesTracker::~ValSeriesTracker()
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
    targetvalue_ = mUdf(float);
}



// EventTracker

mDefineEnumUtils(EventTracker,CompareMethod,"Compare Method")
{
	"None",
	"Seed Trace",
	"Adjacent Parent",
	0
};


const char* EventTracker::sKeyPermittedRange()	{ return "Permitted range"; }
const char* EventTracker::sKeyValueThreshold()	{ return "Value threshhold"; }
const char* EventTracker::sKeyValueThresholds()	{ return "Value threshholds"; }
const char* EventTracker::sKeyAllowedVariance()	{ return "Allowed variance"; }
const char* EventTracker::sKeyAllowedVariances(){ return "Allowed variances"; }
const char* EventTracker::sKeyUseAbsThreshold()	{ return "Use abs threshhold"; }
const char* EventTracker::sKeySimWindow()	{ return "Similarity window"; }
const char* EventTracker::sKeySimThreshold() { return "Similarity threshhold"; }
const char* EventTracker::sKeyNormSimi()     { return "Normalize similarity"; }
const char* EventTracker::sKeyTrackByValue()	{ return "Track by value"; }
const char* EventTracker::sKeyTrackEvent()	{ return "Track event"; }
const char* EventTracker::sKeyCompareMethod()	{ return "Compare method"; }
const char* EventTracker::sKeyAttribID()	{ return "Attribute"; }
const char* EventTracker::sKeySnapToEvent()	{ return "Snap to event"; }
const char* EventTracker::sKeyAllowSignChg()	{ return "Allow sign change"; }


static const char* event_names[] = { "Min", "Max", "0+-", "0-+", 0 };

const char** EventTracker::sEventNames()
{
    return event_names;
}


#define mComma ,
const VSEvent::Type* EventTracker::cEventTypes()
{
    mDefineStaticLocalObject( const VSEvent::Type, event_types, [] =
	{ VSEvent::Min mComma VSEvent::Max mComma
	  VSEvent::ZCPosNeg mComma VSEvent::ZCNegPos } );
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



EventTracker::EventTracker()
    : ValSeriesTracker()
    , permrange_( -5, 5 )
    , ampthreshold_( mUdf(float) )
    , allowedvar_( 0.25 )
    , evtype_( VSEvent::Max )
    , useabsthreshold_( false )
    , similaritywin_( -10, 10 )
    , usesimilarity_( true )
    , similaritythreshold_( 0.8 )
    , normalizesimi_( false )
    , dosnap_(true)
    , quality_( 1 )
    , rangestep_( 1 )
    , seedvs_(0)
    , seeddepth_(mUdf(float))
    , seedsize_(0)
    , comparemethod_(SeedTrace)
    , allowamplsignchg_(false)
{
}


EventTracker::~EventTracker()
{}


void EventTracker::setCompareMethod( CompareMethod cm )
{ comparemethod_ = cm; }

EventTracker::CompareMethod EventTracker::getCompareMethod() const
{ return comparemethod_; }


void EventTracker::setPermittedRange( const Interval<float>& rg )
{
    permrange_ = rg;
    permrange_.sort();
}


bool EventTracker::isOK() const
{
    if ( comparemethod_==SeedTrace )
	return seedvs_ && !mIsUdf(seeddepth_);

    if ( comparemethod_==AdjacentParent )
	return sourcevs_ && !mIsUdf(sourcedepth_);

    if ( comparemethod_==None )
	return targetvs_;

    return false;
}


void EventTracker::setSeed( const ValueSeries<float>* vs, int sz, float depth )
{
    seedvs_ = vs;
    seeddepth_ = depth;
    seedsize_ = sz;

    if ( !seedvs_ )
    {
	compareampl_ = mUdf(float);
	return;
    }

    const SampledFunctionImpl<float,ValueSeries<float> >
			sampfunc( *seedvs_, seedsize_ );
    compareampl_ = sampfunc.getValue( seeddepth_ );
}


const Interval<float>& EventTracker::permittedRange() const
{ return permrange_; }


void EventTracker::setTrackEvent( VSEvent::Type ev )
{ evtype_ = ev; }

VSEvent::Type EventTracker::trackEvent() const
{ return evtype_; }


void EventTracker::allowAmplitudeSignChange( bool yn )
{ allowamplsignchg_ = yn; }

bool EventTracker::isAmplitudeSignChangeAllowed() const
{ return allowamplsignchg_; }


void EventTracker::setAmplitudeThreshold( float th )
{ ampthreshold_ = th; }

float EventTracker::amplitudeThreshold() const
{ return ampthreshold_; }


void EventTracker::setAmplitudeThresholds( const TypeSet<float>& ats )
{ ampthresholds_ = ats; }

TypeSet<float>& EventTracker::getAmplitudeThresholds()
{ return ampthresholds_; }


void EventTracker::setAllowedVariance( float v )
{
    allowedvar_ = mIsZero(v,mDefEps) ? 0.25f : Math::Abs(v);
}


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

void EventTracker::setSnapToEvent( bool yn )
{ dosnap_ = yn; }

bool EventTracker::snapToEvent() const
{ return dosnap_; }


bool EventTracker::isTargetValueAllowed() const
{
    if ( isAmplitudeSignChangeAllowed() )
	return true;

    if ( evtype_==VSEvent::Max )
	return targetvalue_>0;
    if ( evtype_==VSEvent::Min )
	return targetvalue_<0;

    return true;
}


bool EventTracker::track()
{
    if ( !usesimilarity_ )
    {
	if ( useAbsThreshold() && !mIsUdf(ampthreshold_) )
	    return snap( amplitudeThreshold() );

	float refampl = mUdf(float);
	if ( comparemethod_==SeedTrace && seedvs_ )
	{
	    refampl = compareampl_;
	}
	else if ( comparemethod_==AdjacentParent && sourcevs_ )
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

	bool res = false;
	const bool hasuplowthreshold = false;
	if ( hasuplowthreshold )
	{
	    Interval<float> amplrg( refampl * (1-allowedvar_),
				    refampl * (1+allowedvar_) );
	    if ( evtype_==VSEvent::Min )
		amplrg.sort();
	    res = snap( amplrg );
	}
	else
	{
	    const float threshold = refampl * (1-allowedvar_);
	    res = snap( threshold );
	}

	if ( res )
	{
	    if ( mIsUdf(targetvalue_) )
	    {
		const SampledFunctionImpl<float,ValueSeries<float> >
				    sampfunc( *targetvs_, targetsize_);
		targetvalue_ = sampfunc.getValue( targetdepth_ );
	    }

	    const float amplvar = Math::Abs(targetvalue_-refampl) / refampl;
	    quality_ = (allowedvar_-amplvar)/allowedvar_;
	    if ( quality_>1 ) quality_ = 1;
	    else if ( quality_<0 ) quality_ = 0;

	    return isTargetValueAllowed();
	}

	return res;
    }

    const Interval<int> permsamplerange( mNINT32(permrange_.start/rangestep_),
				       mNINT32(permrange_.stop/rangestep_) );
    float upsample=mUdf(float), upsim=mUdf(float); bool upflatstart=false;
    const bool findup = permsamplerange.start<=0
	? findMaxSimilarity( -permsamplerange.start, -1, 3,
			     upsample, upsim, upflatstart )
	: false;

    float dnsample=mUdf(float), dnsim=mUdf(float); bool dnflatstart=false;
    const bool finddn = permsamplerange.stop>=0
			    ? findMaxSimilarity( permsamplerange.stop, 1, 3,
						 dnsample, dnsim, dnflatstart )
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

    if ( targetdepth_<0 || mIsUdf(targetdepth_) )
	return false;

    if ( !dosnap_ )
    {
	const SampledFunctionImpl<float,ValueSeries<float> >
					sampfunc( *targetvs_, targetsize_ );
	targetvalue_ = sampfunc.getValue( targetdepth_ );
	return isTargetValueAllowed();
    }

    const int bestidx = mNINT32( targetdepth_ );
    const bool res = snap( (*targetvs_)[bestidx] );
    return res ? isTargetValueAllowed() : false;
}


bool EventTracker::findMaxSimilarity( int nrtests, int step, int nrgracetests,
	float& res, float& maxsim, bool& flatstart ) const
{
    const Interval<int> similaritysamplewin(
		mNINT32(similaritywin_.start/rangestep_),
		mNINT32(similaritywin_.stop/rangestep_) );

    const ValueSeries<float>* refvs =
		comparemethod_==SeedTrace ? seedvs_ : sourcevs_;
    const int refsize =
		comparemethod_==SeedTrace ? seedsize_ : sourcesize_;
    const float refdepth =
		comparemethod_==SeedTrace ? seeddepth_ : sourcedepth_;

    int firstrefsample = mNINT32(refdepth) + similaritysamplewin.start;
    Interval<int> actualsimilaritywin = similaritysamplewin;
    if ( firstrefsample<0 )
    {
	actualsimilaritywin.start -= firstrefsample;
	firstrefsample = 0;
    }

    if ( firstrefsample+actualsimilaritywin.width(false)>=refsize )
	actualsimilaritywin.stop = refsize-firstrefsample;

    int firsttargetsample = mNINT32(targetdepth_)+actualsimilaritywin.start;
    if ( firsttargetsample<0 )
    {
	actualsimilaritywin.start -= firsttargetsample;
	firstrefsample -= firsttargetsample;
	firsttargetsample = 0;
    }

    if ( firsttargetsample+actualsimilaritywin.width(false)>=targetsize_ )
	actualsimilaritywin.stop = targetsize_-firsttargetsample;

    if ( actualsimilaritywin.width(false)<=0 )
	return false;

    int gracecount = 0;
    int nreqsamples = 0;

    const int nrsamples = actualsimilaritywin.width(false)+1;

// Similarity part - Rework!!!

    bool normalize = nrsamples > 1;
    ValSeriesMathFunc reffunc( *refvs, refsize );
    ValSeriesMathFunc targetfunc( *targetvs_, targetsize_ );

    MathFunctionSampler<float,float> refsamp( reffunc );
    refsamp.sd.start = (float)firstrefsample;
    refsamp.sd.step = 1;

    MathFunctionSampler<float,float> targetsamp( targetfunc );
    targetsamp.sd.step = 1;

    double meana = mUdf(double), stddeva = mUdf(double);
    double meanb = mUdf(double), stddevb = mUdf(double);

    double asum = 0;
    mAllocVarLenArr( double, avals, nrsamples );
    mAllocVarLenArr( double, bvals, nrsamples );
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	avals[idx] = refsamp[idx];
	asum += avals[idx];
    }

    meana = asum / nrsamples;
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const double adiff = avals[idx]-meana;
	asum += adiff*adiff;
    }

    stddeva = Math::Sqrt(asum/(nrsamples-1));

    const int subsize = 8;
    const float fstep = (float)step / (float)subsize;
    const int nrsteps = nrtests * subsize;
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	const float targetstart = firsttargetsample + idx*fstep;
	if ( targetstart<0 )
	    break;

	targetsamp.sd.start = targetstart;

	double bsum = 0;
	for ( int sidx=0; sidx<nrsamples; sidx++ )
	{
	    bvals[sidx] = targetsamp[sidx];
	    bsum += bvals[sidx];
	}

	meanb = bsum / nrsamples;
	bsum = 0;
	for ( int sidx=0; sidx<nrsamples; sidx++ )
	{
	    const double bdiff = bvals[sidx]-meanb;
	    bsum += bdiff*bdiff;
	}

	stddevb = Math::Sqrt(bsum/(nrsamples-1));
	if ( mIsZero(stddeva,mDefEps) || mIsZero(stddevb,mDefEps) )
		normalize=false;
/*
	const float sim = similarity( reffunc, targetfunc,
		(float)firstrefsample, targetstart, 1,
		nrsamples, normalizesimi_ );
*/
	double val1, val2;
	double sqdist = 0, sq1 = 0, sq2 = 0;
	for ( int sidx=0; sidx<nrsamples; sidx++ )
	{
	    val1 = normalize ? (avals[sidx]-meana)/stddeva : avals[sidx];
	    val2 = normalize ? (bvals[sidx]-meanb)/stddevb : bvals[sidx];
	    if ( mIsUdf(val1) || mIsUdf(val2) )
		return mUdf(float);

	    sq1 += val1 * val1;
	    sq2 += val2 * val2;
	    sqdist += (val1-val2) * (val1-val2);
	}

	float sim;
	if ( mIsZero(sq1,mDefEps) && mIsZero(sq2,mDefEps) )
	    sim = 1.f;
	else if ( mIsZero(sq1,mDefEps) || mIsZero(sq2,mDefEps) )
	    sim = 0.f;
	else
	{
	    const float rt = (float) ( Math::Sqrt(sqdist) /
					(Math::Sqrt(sq1) + Math::Sqrt(sq2)) );
	    sim = 1 - rt;
	}

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
	    res = mCast(float,idx);
	    nreqsamples = 0;
	}
	else if ( sim==maxsim )
	    nreqsamples++;
    }

    flatstart = nreqsamples && !res;
    res += ((float)nreqsamples)/2;
    res *= fstep;
    res += firsttargetsample - actualsimilaritywin.start;

    return maxsim>=similaritythreshold_;
}


bool EventTracker::snap( float threshold )
{
    if ( evtype_==VSEvent::Max )
	return snap( Interval<float>(threshold,mUdf(float)) );

    if ( evtype_==VSEvent::Min )
	return snap( Interval<float>(-mUdf(float),threshold) );

    return snap( Interval<float>(threshold,threshold) );
}


bool EventTracker::snap( const Interval<float>& amplrg )
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

	targetvalue_ = 0;
    }
    else if ( evtype_==VSEvent::Max || evtype_==VSEvent::Min )
    {
	float upampl;
	bool uploopskip = false;
	float uptroughampl;
	ValueSeriesEvent<float,float> upevent =
	    findExtreme(evfinder,uprg,amplrg,upampl,uploopskip,uptroughampl);

	float dnampl;
	bool dnloopskip = false;
	float dntroughampl;
	ValueSeriesEvent<float,float> dnevent =
	    findExtreme(evfinder,dnrg,amplrg,dnampl,dnloopskip,dntroughampl);

	const float& threshold = amplrg.start;
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
		const bool usednev = uploopskip && dnevent.pos<=dnbound;
		eventpos = usednev ? dnevent.pos : upevent.pos;
		targetvalue_ = usednev ? dnevent.val : upevent.val;
	    }
	    else
	    {
		if ( upampl==dnampl )
		{
		    if ( fabs(upevent.pos-dnevent.pos)<1 )
		    {
			eventpos = (upevent.pos + dnevent.pos) / 2;
			targetvalue_ = upevent.val;
		    }
		    else
		    {
			const float updiff = fabs( targetdepth_-upevent.pos );
			const float dndiff = fabs( targetdepth_-dnevent.pos );
			const bool useupev = updiff<dndiff;
			eventpos = useupev ? upevent.pos : dnevent.pos;
			targetvalue_ = useupev ? upevent.val : dnevent.val;
		    }
		}
		else
		{
		    const bool useupev = upampl>dnampl;
		    eventpos = useupev ? upevent.pos : dnevent.pos;
		    targetvalue_ = useupev ? upevent.val : dnevent.val;
		}
	    }
	}
	else
	{
	    eventpos = upfound ? upevent.pos : dnevent.pos;
	    targetvalue_ = upfound ? upevent.val : dnevent.val;
	}

    }
    else
    {
	pErrMsg("Event not handled");
	return false;
    }

    if ( eventpos<upbound || eventpos>dnbound )
	return false;

    targetdepth_ = eventpos;
    if ( mIsUdf(targetvalue_) )
    {
	const SampledFunctionImpl<float,ValueSeries<float> >
					sampfunc( *targetvs_, targetsize_ );
	targetvalue_ = sampfunc.getValue( targetdepth_ );
    }

    return true;
}


ValueSeriesEvent<float,float> EventTracker::findExtreme(
    const ValueSeriesEvFinder<float, float>& eventfinder,
    const Interval<float>& rg, float threshold, float& avgampl,
    bool& hasloopskips, float& troughampl ) const
{
    return findExtreme( eventfinder, rg, Interval<float>(threshold,threshold),
			avgampl, hasloopskips, troughampl );
}



ValueSeriesEvent<float,float> EventTracker::findExtreme(
    const ValueSeriesEvFinder<float, float>& eventfinder,
    const Interval<float>& rg, const Interval<float>& amplrg, float& avgampl,
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

	if ( !amplrg.includes(ev.val,false) )
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


void EventTracker::fillPar( IOPar& iopar ) const
{
    ValSeriesTracker::fillPar( iopar );
    iopar.set( sKeyTrackEvent(), VSEvent::getTypeString(evtype_) );
    iopar.set( sKeyCompareMethod(), getCompareMethodString(comparemethod_) );
    iopar.set( sKeyPermittedRange(), permrange_ );
    iopar.set( sKeyValueThreshold(), ampthreshold_ );
    iopar.set( sKeyValueThresholds(), ampthresholds_ );
    iopar.set( sKeyAllowedVariance(), allowedvar_);
    iopar.set( sKeyAllowedVariances(), allowedvars_);
    iopar.setYN( sKeyUseAbsThreshold(), useabsthreshold_ );
    iopar.set( sKeySimWindow(), similaritywin_ );
    iopar.set( sKeySimThreshold(), similaritythreshold_ );
    iopar.setYN( sKeySnapToEvent(), dosnap_ );
    iopar.setYN( sKeyAllowSignChg(), allowamplsignchg_ );
    iopar.setYN( sKeyTrackByValue(), !usesimilarity_ );
    iopar.setYN( sKeyNormSimi(), normalizesimi_ );
}


bool EventTracker::usePar( const IOPar& iopar )
{
    if ( !ValSeriesTracker::usePar(iopar) )
	return false;

    VSEvent::parseEnumType( iopar.find(sKeyTrackEvent()), evtype_ );
    parseEnumCompareMethod( iopar.find(sKeyCompareMethod()), comparemethod_ );
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
    iopar.getYN( sKeySnapToEvent(), dosnap_ );
    iopar.getYN( sKeyAllowSignChg(), allowamplsignchg_ );
    bool trackbyvalue;
    if ( iopar.getYN( sKeyTrackByValue(), trackbyvalue ) )
	usesimilarity_ = !trackbyvalue;

    return true;
}
