/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizonadjuster.cc,v 1.24 2005-12-21 22:14:19 cvskris Exp $";

#include "horizonadjuster.h"

#include "attribdatacubes.h"
#include "attribsel.h"
#include "emhorizon.h"
#include "genericnumer.h"
#include "iopar.h"
#include "linear.h"
#include "mpeengine.h"
#include "samplingdata.h"
#include "survinfo.h"
#include "valseriesevent.h"

#include <math.h>

namespace MPE {


HorizonAdjuster::HorizonAdjuster( EM::Horizon& hor,
	const EM::SectionID& sid )
    : SectionAdjuster( sid )
    , horizon_( hor )
    , attribsel( *new Attrib::SelSpec )
    , attrdata( 0 )
{
    float dist = 5 * SI().zStep();
    permzrange_ = Interval<float>(-dist,dist);
    dist = 10 * SI().zStep();
    similaritywin_ = Interval<float>(-dist, dist);
    
    ampthreshold_ = mUdf(float);
    allowedvar_ = 0.20;
    similaritythreshold_ = 0.80;
    trackbyvalue_ = true;
    evtype = VSEvent::Max;
    useabsthreshold_ = false;
}


HorizonAdjuster::~HorizonAdjuster()
{
    delete &attribsel;
    if ( attrdata ) attrdata->unRef();
}


void HorizonAdjuster::setPermittedZRange(const Interval<float>& rg)
{ permzrange_ = rg; }


Interval<float> HorizonAdjuster::permittedZRange() const
{ return permzrange_; }


void HorizonAdjuster::setTrackByValue(bool yn)
{ trackbyvalue_ = yn; }


bool HorizonAdjuster::trackByValue() const
{ return trackbyvalue_; }


void HorizonAdjuster::setTrackEvent( VSEvent::Type ev )
{ evtype = ev; }


VSEvent::Type HorizonAdjuster::trackEvent() const
{ return evtype; }


void HorizonAdjuster::setAmplitudeThreshold(float th)
{ ampthreshold_ = th; }


float HorizonAdjuster::amplitudeTreshold() const
{ return ampthreshold_; }


void HorizonAdjuster::setAllowedVariance(float v)
{ allowedvar_ = v; }


float HorizonAdjuster::allowedVariance() const
{ return allowedvar_; }


void HorizonAdjuster::setUseAbsThreshold(bool abs)
{ useabsthreshold_ = abs; }


bool HorizonAdjuster::useAbsThreshold() const
{ return useabsthreshold_; }


void HorizonAdjuster::setSimilarityWindow(const Interval<float>& rg)
{ similaritywin_ = rg; }


Interval<float> HorizonAdjuster::similarityWindow() const
{ return similaritywin_; }


void HorizonAdjuster::setSimiliarityThreshold(float th)
{ similaritythreshold_ = th; }


float HorizonAdjuster::similarityThreshold()
{ return similaritythreshold_; }


int HorizonAdjuster::getNrAttributes() const
{ return 1; }


const Attrib::SelSpec* HorizonAdjuster::getAttributeSel( int idx ) const
{ return !idx ? &attribsel : 0; }


void  HorizonAdjuster::reset()
{
    if ( attrdata ) attrdata->unRef();
    attrdata = engine().getAttribCache( attribsel );
    if ( attrdata ) attrdata->ref();
}


int HorizonAdjuster::nextStep()
{
    if ( !attrdata )
	return cErrorOccurred();

    int count = 0;
    for ( int idx=0; idx<pids_.size(); idx++ )
    {
	BinID bid;
	bid.setSerialized( pids_[idx] );
	BinID refbid;
	if ( pidsrc_.size() > idx )
	    refbid.setSerialized( pidsrc_[idx] );
	else
	    refbid = BinID(-1,-1);
	
	float targetz;
	const bool res = trackByValue()
	    ? trackByAmplitude( refbid, bid, targetz )
	    : trackBySimilarity( refbid, bid, targetz );

	if ( res )
	    setHorizonPick( bid, targetz );
	else if ( removeonfailure_ )
	    setHorizonPick(bid, mUdf(float) );
    }

    return cFinished();
}


bool HorizonAdjuster::findMaxSimilarity( const float* fixedarr,
					 const float* slidingarr,
					 int nrsamples, int nrtests, int step,
					 int nrgracetests,
       					 float& res, float& maxsim,
       					 bool& flatstart ) const
{
    if ( !nrtests )
	return false;

    int gracecount = 0;
    int nreqsamples = 0;

    for ( int idx=0; idx<nrtests; idx++, slidingarr += step )
    {
	const float sim = similarity( fixedarr, slidingarr, nrsamples, false,
				      0, 0 );

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

    return maxsim>=similaritythreshold_;
}


void HorizonAdjuster::getNeededAttribs(
	ObjectSet<const Attrib::SelSpec>& specs ) const
{
    for ( int idx=specs.size()-1; idx>=0; idx-- )
    {
	if ( *specs[idx]==attribsel )
	    return;
    }

    specs += &attribsel;
}


CubeSampling HorizonAdjuster::getAttribCube( const Attrib::SelSpec& sp ) const
{
    if ( sp!=attribsel )
	return SectionAdjuster::getAttribCube( sp );

    CubeSampling res = engine().activeVolume();

    res.zrg.start += permzrange_.start;
    res.zrg.stop += permzrange_.stop;
    if ( !trackByValue() )
    {
	res.zrg.start += similaritywin_.start;
	res.zrg.stop += similaritywin_.stop;
    }

    return res;
}


bool HorizonAdjuster::snap( const BinID& bid,
			    float threshold,
			    float& targetz ) const
{
    const StepInterval<float> zrg( attrdata->z0*attrdata->zstep,
			   (attrdata->z0+attrdata->getZSz())* attrdata->zstep,
			   attrdata->zstep );

    if ( !zrg.includes(targetz) )
	return false;

    const int inlidx = attrdata->inlsampling.nearestIndex( bid.inl );
    if ( inlidx<0 || inlidx>=attrdata->getInlSz() )
	return false;

    const int crlidx = attrdata->crlsampling.nearestIndex( bid.crl );
    if ( crlidx<0 || crlidx>=attrdata->getCrlSz() )
	return false;

    const Array3D<float>& cube = attrdata->getCube(0);
    const float* arr = cube.getData() +
		       cube.info().getMemPos( inlidx, crlidx, 0 );

    const ArrayValueSeries<float> valarr( const_cast<float*>(arr) );
    const SamplingData<float> sd( zrg.start, zrg.step );
    ValueSeriesEvFinder<float, float>
			    evfinder( valarr, attrdata->getZSz()-1, sd);

    const Interval<float> uprg( targetz,
	    			mMAX(zrg.start,targetz+permzrange_.start) );
    const Interval<float> dnrg( targetz,
	    			mMIN(zrg.stop,targetz+permzrange_.stop) );

    if ( evtype==VSEvent::ZCNegPos || evtype==VSEvent::ZCPosNeg )
    {
	ValueSeriesEvent<float, float> upevent =
	    				evfinder.find( evtype, uprg, 1 );
	ValueSeriesEvent<float, float> dnevent =
	    				evfinder.find( evtype, dnrg, 1 );

	const bool upfound = !Values::isUdf(upevent.pos);
	const bool dnfound = !Values::isUdf(dnevent.pos);

	if ( !upfound && !dnfound )
	    return false;
	else if ( upfound && dnfound )
	    targetz = fabs(targetz-upevent.pos)<fabs(targetz-dnevent.pos) ?
		upevent.pos : dnevent.pos;
	else 
	    targetz = upfound ? upevent.pos : dnevent.pos;
    }
    else if ( evtype==VSEvent::Max || evtype==VSEvent::Min )
    {
	float upampl;
	bool uploopskip;
	ValueSeriesEvent<float,float> upevent =
	    findExtreme(evfinder,uprg,threshold,upampl,uploopskip);

	float dnampl;
	bool dnloopskip;
	ValueSeriesEvent<float,float> dnevent =
	    findExtreme(evfinder,dnrg,threshold,dnampl,dnloopskip);

	const bool upfound = !Values::isUdf(upevent.pos);
	const bool dnfound = !Values::isUdf(dnevent.pos);

	if ( !upfound && !dnfound )
	    return false;
	else if ( upfound && dnfound )
	{
	    if ( uploopskip!=dnloopskip )
		targetz = uploopskip ? dnevent.pos : upevent.pos;
	    else
	    {
		if ( evtype==VSEvent::Min )
		{
		    upampl *= -1;
		    dnampl *= -1;
		}

		targetz = upampl>dnampl ?  upevent.pos : dnevent.pos;
	    }
	}
	else 
	    targetz = upfound ? upevent.pos : dnevent.pos;
    }
    else
    {
	pErrMsg("Event not handled");
	return false;
    }

    return true;
}


ValueSeriesEvent<float, float>
HorizonAdjuster::findExtreme(
	const ValueSeriesEvFinder<float, float>& eventfinder,
        const Interval<float>& rg, float threshold, float& avgampl,
	bool& hasloopskips ) const
{
    const SamplingData<float>& sd = eventfinder.samplingData();
    const ValueSeries<float>& valser = eventfinder.valueSeries();

    ValueSeriesEvent<float, float> ev;
    int occ=1;
    while ( true )
    {
	ev = eventfinder.find( evtype, rg, occ );
	if ( Values::isUdf(ev.pos) )
	    return ev;

	if ( !Values::isUdf(threshold) &&
	     ( (evtype==VSEvent::Min && ev.val>threshold) ||
	     (evtype==VSEvent::Max && ev.val<threshold)) )
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
    float prev = valser.value(amplsumrg.start);
    for ( int idx=amplsumrg.start;
	  inc>0?idx<=amplsumrg.stop:idx>=amplsumrg.stop; idx+=inc )
    {
	const float val = valser.value(idx);
	if ( !hasloopskips &&
		(evtype==VSEvent::Min && val>prev ) ||
		(evtype==VSEvent::Max && val<prev ) )
	    hasloopskips = true;

	avgampl += val;
    }

    avgampl /= amplsumrg.width()+1;
    return ev;
}


bool HorizonAdjuster::trackByAmplitude( const BinID& refbid,
				        const BinID& targetbid,
				        float& targetz ) const
{
    targetz =  horizon_.getPos( sectionid_, targetbid.getSerialized() ).z;
    if ( useAbsThreshold() )
	return snap( targetbid, amplitudeTreshold(), targetz );

    const float refdepth =
		horizon_.getPos( sectionid_, refbid.getSerialized() ).z;

    float threshold;
    if ( !attrdata->getValue( 0, BinIDValue(refbid,refdepth), &threshold,true) )
	return false;

    threshold *= (1-allowedvar_);
    return snap( targetbid, threshold, targetz );
}


#define mGetArray( prefix, extrarg ) \
    const int prefix##inlidx = \
		attrdata->inlsampling.nearestIndex( prefix##bid.inl ); \
    if ( prefix##inlidx<0 || prefix##inlidx>=attrdata->getInlSz() ) \
	return false; \
 \
    const int prefix##crlidx = \
	attrdata->crlsampling.nearestIndex( prefix##bid.crl ); \
    if ( prefix##crlidx<0 || prefix##crlidx>=attrdata->getCrlSz() ) \
	return false; \
 \
    const float prefix##depth = \
		horizon_.getPos( sectionid_, prefix##bid.getSerialized() ).z; \
    const int prefix##sample = mNINT(prefix##depth/zstep) - attrdata->z0; \
     \
    const Interval<int> prefix##rg = \
		Interval<int>( prefix##sample, prefix##sample ) + extrarg; \
    if ( prefix##rg.start<0 || prefix##rg.stop>=attrdata->getZSz()-1 ) \
	return false; \
 \
    const float* prefix##arr = cube.getData() + \
       cube.info().getMemPos(prefix##inlidx,prefix##crlidx,prefix##rg.start )

bool HorizonAdjuster::trackBySimilarity( const BinID& trefbid,
					 const BinID& targetbid,
					 float& targetz ) const
{
    const BinID refbid = trefbid.inl==-1 || trefbid.crl==-1
	? targetbid
	: trefbid;

    const Array3D<float>& cube = attrdata->getCube(0);

    const float zstep = attrdata->zstep;
    const Interval<int> similarityrg( mNINT( similaritywin_.start/zstep ),
				      mNINT( similaritywin_.stop/zstep ) );
    const int simlength = similarityrg.width()+1;

    mGetArray( ref, similarityrg );

    const Interval<int> testrange( mNINT(permzrange_.start/zstep),
	    			   mNINT(permzrange_.stop/zstep) );

    mGetArray( target, similarityrg+testrange );

    float upsample, upsim; bool upflatstart;
    const bool findup = findMaxSimilarity( refarr, targetarr-testrange.start,
	    				   simlength, -testrange.start+1,
					   -1, 1, upsample, upsim, upflatstart);
    float dnsample, dnsim; bool dnflatstart;
    const bool finddn = findMaxSimilarity( refarr, targetarr-testrange.start,
	    				   simlength, testrange.stop+1,
					   1, 1, dnsample, dnsim, dnflatstart);

    float bestmatch;
    if ( findup && finddn )
    {
	if ( upsim==dnsim )
	{
	    if ( upflatstart && dnflatstart )
		bestmatch = (upsample+dnsample) / 2;
	    else
		bestmatch = fabs(dnsample)<fabs(upsample) ? dnsample : upsample;
	}
	else
	    bestmatch = dnsim<upsim ? upsample : dnsample;
    }
    else if ( findup )
	bestmatch = upsample;
    else if ( finddn )
	bestmatch = dnsample;
    else
	return false;

    bestmatch += targetsample;

    targetz = (bestmatch+attrdata->z0) * zstep;

    const int bestidx = mNINT(bestmatch)-targetrg.start;
    return snap( targetbid, targetarr[bestidx], targetz );
}


void HorizonAdjuster::setHorizonPick(const BinID&  bid, float val )
{
    horizon_.setPos( sectionid_, bid.getSerialized(), Coord3(0,0,val), true );
}


void HorizonAdjuster::setAttributeSel( int idx, const Attrib::SelSpec& as )
{
    if ( idx )
	return;

    attribsel = as;

    if ( attrdata ) attrdata->unRef();
    attrdata = engine().getAttribCache( attribsel );
    if ( attrdata ) attrdata->ref();
}


void HorizonAdjuster::fillPar( IOPar& iopar ) const
{
    SectionAdjuster::fillPar( iopar );
    attribsel.fillPar( iopar );
    iopar.set( sKeyTrackEvent(), VSEvent::TypeRef(evtype) );
    iopar.set( sKeyPermittedZRange(), permzrange_.start, permzrange_.stop );
    iopar.set( sKeyValueThreshold(), ampthreshold_ );
    iopar.set( sKeyAllowedVariance(), allowedvar_);
    iopar.setYN( sKeyUseAbsThreshold(), useabsthreshold_ );
    iopar.set( sKeySimWindow(), similaritywin_.start, similaritywin_.stop );
    iopar.set( sKeySimThreshold(), similaritythreshold_ );
    iopar.setYN( sKeyTrackByValue(), trackbyvalue_ );
}


bool HorizonAdjuster::usePar( const IOPar& iopar )
{
    EnumRef tmpref = VSEvent::TypeRef(evtype);

    PtrMan<IOPar> oldpar = iopar.subselect("attrval.Attrib 0");
    if ( !oldpar || !attribsel.usePar(*oldpar ) )
    {
	if ( !attribsel.usePar( iopar ) )
	    return false;
    }

    return
	SectionAdjuster::usePar( iopar ) &&
	iopar.get( sKeyTrackEvent(),  tmpref ) &&
	iopar.get( sKeyPermittedZRange(),permzrange_.start,permzrange_.stop ) &&
	iopar.get( sKeyValueThreshold(), ampthreshold_ ) &&
	iopar.get( sKeyAllowedVariance(), allowedvar_) &&
	iopar.getYN( sKeyUseAbsThreshold(), useabsthreshold_ ) &&
	iopar.get( sKeySimWindow(),similaritywin_.start,similaritywin_.stop ) &&
	iopar.get( sKeySimThreshold(), similaritythreshold_ ) &&
	iopar.getYN( sKeyTrackByValue(), trackbyvalue_ );
}


}; // namespace MPE
