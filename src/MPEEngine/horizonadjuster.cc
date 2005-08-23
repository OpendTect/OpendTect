/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizonadjuster.cc,v 1.7 2005-08-23 09:58:29 cvsduntao Exp $";

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
const char* HorizonAdjuster::trackeventstr_ = "Track event";

HorizonAdjuster::HorizonAdjuster( EM::Horizon& hor,
	const EM::SectionID& sid )
    : SectionAdjuster( sid )
    , horizon_( hor )
{
    computers_ += new AttribPositionScoreComputer();

    float dist = 8 * SI().zStep();
    permzrange_ = Interval<float>(-dist,dist);
    dist /= 2;
    similaritywin_ = Interval<float>(-dist, dist);
    
    ampthreshold_ = mUndefValue;
    similaritythreshold_ = 0.75;
    trackbyvalue_ = true;
    trackevent_ = VSEvent::Max;
}


int HorizonAdjuster::nextStep()
{
    initTrackParam();
    horizon_.geometry.checkSupport(false);
    int count = 0;
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
	
	float targetz;
 	if ( trackTrace(refbid, bid, targetz, 0) )
	    ++count, setHorizonPick(bid, targetz);
	else //if ( stopbelowthrhold_ )
	    setHorizonPick(bid, mUndefValue);
    }
    horizon_.geometry.checkSupport(true);

#ifdef __debug__
    BufferString msg( "Tracked horizon picks: " ); msg += count;
    ErrMsg( msg );
#endif
    return 0;
}


bool HorizonAdjuster::trackTrace( const BinID& refbid,
				 const BinID& targetbid, float& targetz, 
				 float* refsamples )
{
    const Coord3 pos = horizon_.geometry.getPos(sectionid_,targetbid);
    if ( !pos.isDefined() )	return false;
    targetz = pos.z;

    const AttribPositionScoreComputer* acomp = getAttribComputer();
    const PositionScoreComputerAttribData* attrdata = acomp->getAttribData();
    const HorSampling hrg = attrdata->getCubeSampling(0).hrg;
    const StepInterval<float> zrg = attrdata->getCubeSampling(0).zrg;
    if ( !zrg.includes(targetz) )
	return false;

    float smplbuf[matchwinsamples_+2];
    if ( ! refsamples ) {
	BinID srcbid(refbid.inl, refbid.crl);
	if ( srcbid.inl == -1 && srcbid.crl == -1 )
	    srcbid = targetbid;
	if ( !hrg.includes(srcbid) || !hrg.includes(srcbid) )
	    return false;
	if ( ! getCompSamples(attrdata, srcbid,
	    	horizon_.geometry.getPos(sectionid_,srcbid).z, smplbuf ) )
	    return false;
	refsamples = smplbuf;
    }

    Interval<double> permrange( targetz+permzrange_.start,
	    			targetz+permzrange_.stop );

    if ( permrange.start<zrg.start )	permrange.start = zrg.start;
    if ( permrange.stop>zrg.stop )	permrange.stop = zrg.stop;
    if ( permrange.start>permrange.stop || !permrange.includes(targetz) )
	return false;
    
    const int nrcubesamples = zrg.nrSteps();
    float trcbuffer[zrg.nrSteps() + 2 ];
    const Interval<int> samplerg( mMAX(zrg.nearestIndex(permrange.start),0),
			mMIN(zrg.nearestIndex(permrange.stop),nrcubesamples-1));
    if ( ! attrdata->getValueBySample( 0, targetbid, trcbuffer,
				       samplerg.start, samplerg.stop) )
    	return false;
    const int nrsamples = samplerg.stop - samplerg.start + 1;
    
    if ( trackevent_==VSEvent::ZC || trackevent_==VSEvent::ZCNegPos
	 || trackevent_==VSEvent::ZCPosNeg )
    {
	float zeropos = adjoiningZeroEvent(trcbuffer, nrsamples,
		zrg.nearestIndex(targetz)-samplerg.start, trackevent_);
	if ( Values::isUdf(zeropos) )
	    return false;
	targetz = zrg.atIndex(samplerg.start);
	targetz += (zeropos*zrg.step);
	return true;
    }
    
    int upmatchpos, downmatchpos;  upmatchpos = downmatchpos = -1;
    float upmatchv, downmatchv;    bool upeq, downeq;
    if ( trackbyvalue_ ) 
    {
	int midsample = zrg.nearestIndex(targetz)-samplerg.start;
	upmatchpos = adjoiningExtreme( trcbuffer, midsample,
			    0, *refsamples, upmatchv, upeq );
	downmatchpos = adjoiningExtreme( trcbuffer, midsample+1,
			    nrsamples-1, *refsamples, downmatchv, downeq );
    }
    else // track by similarity
    {
	Interval<double> matchrg( targetz+similaritywin_.start,
				 targetz+similaritywin_.stop );
	if ( !( permrange.includes(matchrg.start)
	     && permrange.includes(matchrg.stop) ) )
	    return false; 

	int stsmpl = zrg.nearestIndex(matchrg.start)-samplerg.start;

 	upmatchpos = matchingSampleBySimilarity( trcbuffer, stsmpl,
			    0, refsamples, upmatchv, upeq );
	downmatchpos = matchingSampleBySimilarity( trcbuffer, stsmpl+1,
			    nrsamples - matchwinsamples_,
			    refsamples, downmatchv, downeq );
    }
    
    int matchpos = -1;
    if ( upmatchpos!=-1 &&  downmatchpos!=-1 )
    {
	if ( downmatchv == upmatchv && upeq && downeq )
	    matchpos = ( upmatchpos + downmatchpos ) / 2;
	else if ( trackbyvalue_ )
	{
	    if ( trackevent_ == VSEvent::Max )
		matchpos = downmatchv>upmatchv ? downmatchpos : upmatchpos;
	    else
		matchpos = downmatchv<upmatchv ? downmatchpos : upmatchpos;
	}
	else 
	    matchpos = downmatchv>upmatchv ? downmatchpos : upmatchpos;
    }
    else if ( upmatchpos!=-1 )
	matchpos = upmatchpos;
    else if ( downmatchpos!=-1 )
	matchpos = downmatchpos;

    if ( matchpos != -1 )
    {
	float dstpos = fineTuneExtremePos( trcbuffer, nrsamples, matchpos,
					   trackevent_ );
	if ( !trackbyvalue_ )	    dstpos += simlaritymatchpt_;
	targetz = zrg.atIndex(samplerg.start);
	targetz += (dstpos*zrg.step);
    }
    return matchpos != -1;
}


int HorizonAdjuster::adjoiningExtreme( const float* srctrc,
			int startsample, int endsample, float refval,
			float &matchval, bool& eqfromstart )
{
    const int tolerancesamples = 1;
    int matchpos = -1;
    eqfromstart = false;
    int step = startsample>endsample ? -1 : 1;
    
    int numsamples = endsample - startsample;
    if ( numsamples<0 )	numsamples = -numsamples;
    numsamples++;
    
    const float alloweddev = fabs( refval * 0.20 );
    float prevdev = alloweddev;
    float prevval = srctrc[startsample];
    
    int smpl = startsample;
    int eqsamples = 0;
    for ( int idx=0; idx<numsamples; idx++, smpl += step )
    {
	float sampleval = srctrc[smpl];
	if ( Values::isUdf(sampleval) )	    break;
	float dev =  fabs( sampleval - refval );
	bool matchfail = false;
	if ( trackevent_ == VSEvent::Min )
	{
	    if ( !Values::isUdf(ampthreshold_) && sampleval>ampthreshold_ )
		matchfail = true;;
	    if ( Values::isUdf(ampthreshold_) && sampleval>refval
		&& sampleval - refval>alloweddev )
		matchfail = true;;
	    if ( sampleval>prevval )
		matchfail = true;;
	}
	else
	{
	    if ( !Values::isUdf(ampthreshold_) && sampleval<ampthreshold_ )
		matchfail = true;;
	    if ( Values::isUdf(ampthreshold_) && sampleval<refval
		&& refval - sampleval>alloweddev )
		matchfail = true;;
	    if ( sampleval<prevval )
		matchfail = true;;
	}
	if (matchfail) 
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
    return matchpos;
}


int HorizonAdjuster::matchingSampleBySimilarity( const float* srctrc,
			int startsample, int endsample, const float* refval,
			float &matchratio, bool& eqfromstart )
{
    int matchpos = -1;    eqfromstart = false;

    int eqsamples = 0;    float prevratio = 0;
    int step = startsample>endsample ? -1 : 1;
    for ( int smpl=startsample;  ; smpl += step )
    {
	if ( step==1 && smpl>endsample || step==-1 && smpl<endsample )
	    break;
	
	float curratio = similarity<const float*, const float*>(refval, srctrc,
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
    return matchpos;
}


float HorizonAdjuster::adjoiningZeroEvent( float *srctrc, int nrsamples,
				int startpos, VSEvent::Type ev )
{
    if ( !(ev==VSEvent::ZC || ev==VSEvent::ZCNegPos || ev==VSEvent::ZCPosNeg) )
	return mUndefValue;
    
    float dwpos = firstZeroEvent( srctrc, startpos, nrsamples - 1, ev );
    float uppos = firstZeroEvent( srctrc, startpos, 0, ev );

    float matchpos = mUndefValue;
    if ( !Values::isUdf(uppos) && !Values::isUdf(dwpos) )
	matchpos = (dwpos-startpos<startpos-uppos) ? dwpos : uppos;
    else if ( !Values::isUdf(uppos) && Values::isUdf(dwpos) )
	matchpos = uppos;
    else if ( Values::isUdf(uppos) && !Values::isUdf(dwpos) )
	matchpos = dwpos;
    
    return matchpos;
}


float HorizonAdjuster::firstZeroEvent( const float *srctrc, int startsample,
				    int endsample, VSEvent::Type ev )
{
    int step = startsample>endsample ? -1 : 1;
    for ( int smpl=startsample+step;  ; smpl += step )
    {
	if ( step==1 && smpl>endsample || step==-1 && smpl<endsample )
	    break;
	
	float prev, cur;
	if ( step > 0 )
	    prev = srctrc[smpl-step],	cur  = srctrc[smpl];
	else
	    prev = srctrc[smpl],	cur  = srctrc[smpl-step];
	if ( prev==0 && cur==0 )	continue;
	
	bool zeroev = false;
	if ( prev<=0 && cur>0 || prev==0 && cur>0 )
	{
	    if ( ev==VSEvent::ZCNegPos || ev==VSEvent::ZC )
		zeroev =  true;
	    if ( ev == VSEvent::ZCPosNeg )
		break;
	}
	if ( prev>=0 && cur<0 || prev==0 && cur<0 )
	{
	    if ( ev == VSEvent::ZCPosNeg || ev==VSEvent::ZC )
		zeroev = true;
	    if ( ev == VSEvent::ZCNegPos )
		break;
	}
	
	if ( zeroev )
	{
	    float pos = step == 1 ? smpl-1 : smpl;
	    pos += ( -prev/(cur-prev) );
	    return pos;
	}

	if ( ev==VSEvent::ZCNegPos && prev > cur 
	     || ev==VSEvent::ZCPosNeg && prev < cur )
	    break;
    }
    return mUndefValue;
}


float HorizonAdjuster::fineTuneExtremePos(const float *smplbuf, int nrsamples,
				     int pickpos, VSEvent::Type ev)
{
    if ( nrsamples < 4 || pickpos<1 || pickpos>=nrsamples-2 )
	return pickpos;
    ThirdOrderPoly poly;
    if ( ev == VSEvent::Min )
	poly.setFromSamples( -smplbuf[pickpos-1], -smplbuf[pickpos],
    		 -smplbuf[pickpos+1], -smplbuf[pickpos+2]);
    else
	poly.setFromSamples( smplbuf[pickpos-1], smplbuf[pickpos],
    		 smplbuf[pickpos+1], smplbuf[pickpos+2]);
    float extremepos, extremepos0, extremepos1;
    poly.getExtremePos( extremepos0, extremepos1 );
    PtrMan<SecondOrderPoly> firstder = poly.createDerivative();
    PtrMan<FloatMathFunction> secder = firstder->createDerivative();

    if ( secder->getValue(extremepos0)<0 )
	extremepos = extremepos0;
    else if ( secder->getValue(extremepos1)<0 )
	extremepos = extremepos1;

    if ( Values::isUdf(extremepos) || extremepos>=1 || extremepos<=-1 )
	return pickpos;
    
    return pickpos+extremepos;
}


void HorizonAdjuster::initTrackParam()
{
    const AttribPositionScoreComputer* acomp = getAttribComputer();
    
    if (trackbyvalue_)
    {
	matchwinsamples_ = 1;
	simlaritymatchpt_ = 0;
    }
    else {
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


bool HorizonAdjuster::getCompSamples(
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
    int event = trackevent_;
    iopar.set( trackeventstr_, event );
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
    int event;
    iopar.get( trackeventstr_,  event);
    trackevent_ = (VSEvent::Type)event;
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
