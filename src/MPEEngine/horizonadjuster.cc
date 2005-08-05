/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizonadjuster.cc,v 1.1 2005-08-05 03:56:52 cvsduntao Exp $";

#include "horizonadjuster.h"

#include "emhorizon.h"
#include "attribpsc.h"
#include "genericnumer.h"
#include "survinfo.h"
#include "linear.h"
#include "iopar.h"

#include <math.h>

namespace MPE {

const char* HorizonAdjuster::permzrgstr_ = "Permitted Z range";
const char* HorizonAdjuster::ampthresholdstr_ = "Value threshhold";
const char* HorizonAdjuster::similaritywinstr_ = "Similarity window";
const char* HorizonAdjuster::similaritythresholdstr_ = "Similarity threshhold";
const char* HorizonAdjuster::trackbyvalstr_ = "Track by value";

HorizonAdjuster::HorizonAdjuster( EM::Horizon& hor,
	const EM::SectionID& sid )
    : SectionAdjuster( sid )
    , horizon_( hor )
{
    computers_ += new AttribPositionScoreComputer();

    float dist = 8 * SI().zRange().step;
    permzrange_ = Interval<float>(-dist,dist);
    dist = 4 * SI().zRange().step;
    similaritywin_ = Interval<float>(-dist, dist);
    
    ampthreshold_ = mUndefValue;
    similaritythreshold_ = 0.75;
    trackbyvalue_ = true;
}


int HorizonAdjuster::nextStep()
{
    initTrackParam();
    horizon_.geometry.checkSupport(false);
    for ( int idx=0; idx<pids_.size(); idx++ )
    {
	BinID bid;	bid.setSerialized( pids_[idx] );
	BinID refbid;
	if ( refpos_ )
	    refbid.setSerialized(*refpos_);
	else if ( pidsrc_.size() > idx )
	    refbid.setSerialized( pidsrc_[idx] );
	else
	    refbid = bid;
	
	const float refz = horizon_.geometry.getPos(sectionid_,refbid).z;
	if ( Values::isUdf(refz) )
	    continue;

	float targetz;
 	if ( trackTrace(refbid, refz, bid, targetz, 0) )
	    setHorizonPick(bid, targetz);
	else //if ( stopbelowthrhold_ )
	    setHorizonPick(bid, mUndefValue);
    }
    horizon_.geometry.checkSupport(true);
    return 0;
}


bool HorizonAdjuster::trackTrace( const BinID& refbid, float refz,
				 const BinID& targetbid, float& targetz, 
				 float* refsamples )
{
    const Coord3 pos = horizon_.geometry.getPos(sectionid_,refbid);
    if ( !pos.isDefined() )	return false;
    targetz = pos.z;

    const AttribPositionScoreComputer* acomp = getAttribComputer();
    const PositionScoreComputerAttribData* attrdata = acomp->getAttribData();
    const HorSampling hrg = attrdata->getCubeSampling(0).hrg;
    const StepInterval<float> zrg = attrdata->getCubeSampling(0).zrg;
    if ( !hrg.includes(refbid) || !hrg.includes(targetbid) )
	return false;
    if ( !zrg.includes(refz) || !zrg.includes(targetz) )
	return false;

    float smplbuf[matchwinsamples_+2];
    if ( ! refsamples ) {
	BinID srcbid(refbid.inl, refbid.crl);
	if ( srcbid.inl == -1 && srcbid.crl == -1 )
	    srcbid = targetbid;
	if ( ! getCmpSamples(attrdata, srcbid, refz, smplbuf ) )
	    return false;
	refsamples = smplbuf;
    }

    Interval<double> permrange( targetz+permzrange_.start,
	    			targetz+permzrange_.stop );

    if ( permrange.start<zrg.start )	permrange.start = zrg.start;
    if ( permrange.stop>zrg.stop )	permrange.stop = zrg.stop;
    if ( permrange.start>permrange.stop || !permrange.includes(targetz) )
	return false;
    
    const int nrsamples = zrg.nrSteps();
    float trcbuffer[zrg.nrSteps() + 2 ];
    const Interval<int> samplerg( mMAX(zrg.nearestIndex(permrange.start),0),
			mMIN(zrg.nearestIndex(permrange.stop), nrsamples-1));
    
    int midsample = zrg.nearestIndex(targetz);
    int upmatchpos, downmatchpos;
    float upmatchv, downmatchv;
    bool upeq, downeq, upok, downok;
    if ( trackbyvalue_ ) 
    {
	upok = findMatchingSampleByValue( targetbid, midsample,
			    samplerg.start, *refsamples,
			    upmatchpos, upmatchv, upeq );
	downok = findMatchingSampleByValue( targetbid, midsample+1,
			    samplerg.stop, *refsamples,
			    downmatchpos, downmatchv, downeq );
    }
    else // track by similarity
    {
	Interval<double> matchrg( targetz+similaritywin_.start,
				 targetz+similaritywin_.stop );
	if ( !( permrange.includes(matchrg.start)
	     && permrange.includes(matchrg.stop) ) )
	    return false;
	if ( ! attrdata->getValueBySample(0, targetbid, trcbuffer,
	    			samplerg.start, samplerg.stop) )
	    return false;

	int stsmpl = zrg.nearestIndex(matchrg.start)-samplerg.start;

 	upok = findMatchingSampleBySimilarity( trcbuffer, stsmpl,
			    0, refsamples, upmatchpos, upmatchv, upeq );
	downok = findMatchingSampleBySimilarity( trcbuffer, stsmpl+1,
			    samplerg.stop-samplerg.start+1 - matchwinsamples_,
			    refsamples, downmatchpos, downmatchv, downeq );
    }
    
    int matchpos = -1;
    if ( upok &&  downok )
    {
	if ( downmatchv == upmatchv && upeq && downeq )
	    matchpos = ( upmatchpos + downmatchpos ) / 2;
	else if ( trackbyvalue_ )
	{
	    if ( acomp->trackHigh() )
		matchpos = downmatchv>upmatchv ? downmatchpos : upmatchpos;
	    else
		matchpos = downmatchv<upmatchv ? downmatchpos : upmatchpos;
	}
	else 
	    matchpos = downmatchv>upmatchv ? downmatchpos : upmatchpos;
    }
    else if ( upok )
	matchpos = upmatchpos;
    else if ( downok )
	matchpos = downmatchpos;

    if ( matchpos != -1 )
    {
	targetz = zrg.atIndex(matchpos);
	if ( ! trackbyvalue_ )
	    targetz += ((samplerg.start+simlaritymatchpt_)*zrg.step);
    }
    return matchpos != -1;
}


bool HorizonAdjuster::findMatchingSampleByValue( const BinID& bid,
			int startsample, int endsample, float refval,
			int& matchpos, float &matchval, bool& eqfromstart )
{
    const int trackfail = 1;
    const int tolerancesamples = 2;
    matchpos = -1;
    eqfromstart = false;
    int step = startsample>endsample ? -1 : 1;
    
    int numsamples = endsample - startsample;
    if ( numsamples<0 )	numsamples = -numsamples;
    numsamples++;
    
    const AttribPositionScoreComputer* acomp = getAttribComputer();
    const PositionScoreComputerAttribData* attrdata = acomp->getAttribData();
    const float alloweddev = fabs( refval * 0.20 );
    float prevdev = alloweddev;
    float prevval = attrdata->getValueBySample( 0, bid, startsample );
    
    int smpl = startsample;
    int eqsamples = 0;
    for ( int idx=0; idx<numsamples; idx++, smpl += step )
    {
	float sampleval = attrdata->getValueBySample( 0, bid, smpl );
	if ( Values::isUdf(sampleval) )
	    break;
	float dev =  fabs( sampleval - refval );
	try
	{
	    if ( !acomp->trackHigh() )
	    {
		if ( !Values::isUdf(ampthreshold_) && sampleval>ampthreshold_ )
		    throw trackfail;
		if ( Values::isUdf(ampthreshold_) && sampleval>refval
		    && sampleval - refval>alloweddev )
		    throw trackfail;
		if ( sampleval>prevval )
		    continue;
	    }
	    else
	    {
		if ( !Values::isUdf(ampthreshold_) && sampleval<ampthreshold_ )
		    throw trackfail;
		if ( Values::isUdf(ampthreshold_) && sampleval<refval
		    && refval - sampleval>alloweddev )
		    throw trackfail;
		if ( sampleval<prevval )
		    continue;
	    }
	}
	catch (int) 
	{
	    if ( matchpos == -1 && idx<tolerancesamples )
		continue;
	    else
		break;
	}
	
	if ( idx == 0 || prevval != sampleval )
	{
	    prevval = sampleval;
	    prevdev = dev;
	    matchpos = smpl;
	    matchval = sampleval;
	    eqsamples = 0;
	}
	else if ( idx && prevval == sampleval )
	    eqsamples++;
    }
    if ( matchpos != -1 && eqsamples )
    {
	eqfromstart = (eqsamples == (matchpos - startsample + 1));
	matchpos += ( eqsamples / 2 * step );
    }
    return matchpos != -1;
}


bool HorizonAdjuster::findMatchingSampleBySimilarity( float* srctrc,
			int startsample, int endsample, float* refval,
			int& matchpos, float &matchratio, bool& eqfromstart )
{
    matchpos = -1;    eqfromstart = false;

    int eqsamples = 0;    float prevratio = 0;
    int step = startsample>endsample ? -1 : 1;
    for ( int smpl=startsample;  ; smpl += step )
    {
	if ( step==1 && smpl>endsample || step==-1 && smpl<endsample )
	    break;
	
	float curratio = similarity<float*,float*>(refval, srctrc,
				matchwinsamples_, false, 0, smpl );
	if ( curratio<similaritythreshold_ )
	    break;
	if ( curratio<prevratio )
	    continue;
	
	if ( smpl==startsample || prevratio != curratio )
	{
	    matchpos = smpl;
	    matchratio = prevratio = curratio;
	    eqsamples = 0;
	}
	else if ( smpl!=startsample && prevratio == curratio )
	    eqsamples++;
    }
    if ( matchpos != -1 && eqsamples )
    {
	eqfromstart = (eqsamples == (matchpos - startsample + 1));
	matchpos += ( eqsamples / 2 * step );
    }
    if ( matchpos != -1
	 && *(refval+simlaritymatchpt_)**(srctrc+matchpos+simlaritymatchpt_)<0 )
	    matchpos = -1;
    return matchpos != -1;
}


void HorizonAdjuster::initTrackParam()
{
    if (trackbyvalue_)
    {
	matchwinsamples_ = 1;
	simlaritymatchpt_ = 0;
    }
    else {
	const AttribPositionScoreComputer* acomp = getAttribComputer();
	const PositionScoreComputerAttribData* attrdata = acomp->getAttribData();
	const StepInterval<float> zrg = attrdata->getCubeSampling(0).zrg;
	matchwinsamples_ = (int)( (similaritywin_.stop - similaritywin_.start)
    				  / zrg.step ) + 1;
	simlaritymatchpt_ = (int)(-similaritywin_.start / zrg.step);
    }
}


const AttribPositionScoreComputer* 
    HorizonAdjuster::getAttribComputer() const
{
    for ( int idx=0; idx<nrComputers(); idx++ )
    {
	mDynamicCastGet(const AttribPositionScoreComputer*,attr,
			getComputer(idx));
	if ( attr ) return attr;
    }

    return 0;
}


void HorizonAdjuster::setHorizonPick(const BinID&  bid, float val )
{
    horizon_.geometry.setPos( sectionid_, bid, Coord3(0,0,val), true );
}


bool HorizonAdjuster::getCmpSamples(
	const PositionScoreComputerAttribData*	attrdata,
	const BinID& bid, float z, float* buf )
{
    float *smplbuf = buf;
    if ( trackbyvalue_ )
    {
	const StepInterval<float> zrg = attrdata->getCubeSampling(0).zrg;
	int stsmpl = zrg.nearestIndex(z);
	*smplbuf = attrdata->getValueBySample( 0, bid, stsmpl );
	return ! Values::isUdf(*smplbuf);
    }
    else
    {
	const StepInterval<float> zrg = attrdata->getCubeSampling(0).zrg;
	int stsmpl = zrg.nearestIndex(z+similaritywin_.start);
	return attrdata->getValueBySample(0, bid, smplbuf, stsmpl,
					  stsmpl + matchwinsamples_ - 1);
    }
}


void HorizonAdjuster::fillPar( IOPar& iopar ) const
{
    SectionAdjuster::fillPar( iopar );
    iopar.set( permzrgstr_, permzrange_.start, permzrange_.stop );
    iopar.set( ampthresholdstr_, ampthreshold_ );
    iopar.set( similaritywinstr_, similaritywin_.start, similaritywin_.stop );
    iopar.set( similaritythresholdstr_, similaritythreshold_ );
    int byval = trackbyvalue_;
    iopar.set( trackbyvalstr_, byval );
}


bool HorizonAdjuster::usePar( const IOPar& iopar )
{
    SectionAdjuster::usePar( iopar );
    iopar.get( permzrgstr_, permzrange_.start, permzrange_.stop );
    iopar.get( ampthresholdstr_, ampthreshold_ );
    iopar.get( similaritywinstr_, similaritywin_.start, similaritywin_.stop );
    iopar.get( similaritythresholdstr_, similaritythreshold_ );
    int byval;
    iopar.get( trackbyvalstr_,  byval);
    trackbyvalue_ = byval;
    return true;
}


}; // namespace MPE
