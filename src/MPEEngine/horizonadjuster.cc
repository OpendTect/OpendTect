/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizonadjuster.cc,v 1.43 2007-05-22 03:23:23 cvsnanne Exp $";

#include "horizonadjuster.h"

#include "arrayndimpl.h"
#include "attribdatacubes.h"
#include "attribsel.h"
#include "emhorizon.h"
#include "emhorizon2d.h"
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
    : SectionAdjuster(sid)
    , horizon_(hor)
    , attribsel_(0)
    , attrdata_(0)
{
    float dist = 5 * SI().zStep();
    permzrange_ = Interval<float>(-dist,dist);
    dist = 10 * SI().zStep();
    similaritywin_ = Interval<float>(-dist, dist);
    
    ampthreshold_ = mUdf(float);
    allowedvar_ = 0.20;
    similaritythreshold_ = 0.80;
    trackbyvalue_ = true;
    evtype_ = VSEvent::Max;
    useabsthreshold_ = false;
}


HorizonAdjuster::~HorizonAdjuster()
{
    delete attribsel_;
    if ( attrdata_ ) attrdata_->unRef();
}


void HorizonAdjuster::setPermittedZRange( const Interval<float>& rg )
{ 
    permzrange_ = rg;
    permzrange_.sort(); 
}


Interval<float> HorizonAdjuster::permittedZRange() const
{ return permzrange_; }


void HorizonAdjuster::setTrackByValue( bool yn )
{ trackbyvalue_ = yn; }


bool HorizonAdjuster::trackByValue() const
{ return trackbyvalue_; }


void HorizonAdjuster::setTrackEvent( VSEvent::Type ev )
{ evtype_ = ev; }


VSEvent::Type HorizonAdjuster::trackEvent() const
{ return evtype_; }


void HorizonAdjuster::setAmplitudeThreshold( float th )
{ ampthreshold_ = th; }


float HorizonAdjuster::amplitudeThreshold() const
{ return ampthreshold_; }


void HorizonAdjuster::setAllowedVariance( float v )
{ allowedvar_ = v; }


float HorizonAdjuster::allowedVariance() const
{ return allowedvar_; }


void HorizonAdjuster::setUseAbsThreshold( bool abs )
{ useabsthreshold_ = abs; }


bool HorizonAdjuster::useAbsThreshold() const
{ return useabsthreshold_; }


void HorizonAdjuster::setSimilarityWindow( const Interval<float>& rg )
{ 
    similaritywin_ = rg;
    similaritywin_.sort();
}


Interval<float> HorizonAdjuster::similarityWindow() const
{ return similaritywin_; }


void HorizonAdjuster::setSimilarityThreshold( float th )
{ similaritythreshold_ = th; }


float HorizonAdjuster::similarityThreshold()
{ return similaritythreshold_; }


int HorizonAdjuster::getNrAttributes() const
{ return 1; }


const Attrib::SelSpec* HorizonAdjuster::getAttributeSel( int idx ) const
{ return !idx ? attribsel_ : 0; }


void HorizonAdjuster::reset()
{
    if ( attrdata_ ) attrdata_->unRef();
    attrdata_ = attribsel_ ? engine().getAttribCache( *attribsel_ ) : 0;
    if ( attrdata_ ) attrdata_->ref();
}


int HorizonAdjuster::nextStep()
{
    if ( !attrdata_ )
	return cErrorOccurred();

    int count = 0;
    for ( int idx=0; idx<pids_.size(); idx++ )
    {
	BinID bid;
	bid.setSerialized( pids_[idx] );
	float targetz;
	bool res;
	if ( pidsrc_.size() > idx )
	{
	    BinID refbid;
	    refbid.setSerialized( pidsrc_[idx] );
	    res = trackByValue() ? trackByAmplitude( refbid, bid, targetz )
			         : trackBySimilarity( refbid, bid, targetz );
	}
	else
	{
	    targetz = horizon_.getPos( sectionid_, bid.getSerialized() ).z;
	    res = snap( bid, amplitudeThreshold(), targetz );
	}

	if ( res )
	    setHorizonPick( bid, targetz );
	else if ( removeonfailure_ )
	    setHorizonPick(bid, mUdf(float) );
    }

    return cFinished();
}


bool HorizonAdjuster::findMaxSimilarity( const ValueSeries<float>& fixedarr,
					 const ValueSeries<float>& slidearr,
					 int nrsamples, int nrtests, int step,
					 int nrgracetests,
       					 float& res, float& maxsim,
       					 bool& flatstart ) const
{
    if ( !nrtests )
	return false;

    int gracecount = 0;
    int nreqsamples = 0;

    for ( int idx=0; idx<nrtests; idx++ )
    {

	const OffsetValueSeries<float> 
	      slidingarr( const_cast<ValueSeries<float>&>(slidearr), idx*step );

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
    if ( !attribsel_ || attribsel_->id()<0 )
	return;

    for ( int idx=specs.size()-1; idx>=0; idx-- )
    {
	if ( *specs[idx] == *attribsel_ )
	    return;
    }

    specs += attribsel_;
}


CubeSampling HorizonAdjuster::getAttribCube( const Attrib::SelSpec& sp ) const
{
    if ( !attribsel_ || sp != *attribsel_ )
	return SectionAdjuster::getAttribCube( sp );

    CubeSampling res = engine().activeVolume();

    res.zrg.start += permzrange_.start;
    res.zrg.stop += permzrange_.stop;
    if ( !trackByValue() )
    {
	res.zrg.start += similaritywin_.start;
	res.zrg.stop += similaritywin_.stop;
    }

    res.snapToSurvey();
    return res;
}


bool HorizonAdjuster::is2D() const
{
    mDynamicCastGet(const EM::Horizon2D*,hor2d,&horizon_)
    return hor2d;
}


const BinID HorizonAdjuster::attrDataBinId( const BinID& bid ) const 
{
    return is2D() && attrdata_->getInlSz()==1 ? 
	   BinID( attrdata_->inlsampling.start, bid.crl ) : bid;
}


bool HorizonAdjuster::snap( const BinID& bid,
			    float threshold,
			    float& targetz ) const
{
    const StepInterval<float> zrg( attrdata_->z0*attrdata_->zstep,
			(attrdata_->z0+attrdata_->getZSz())* attrdata_->zstep,
			attrdata_->zstep );

    if ( !zrg.includes(targetz) )
	return false;

    const int inlidx = 
		attrdata_->inlsampling.nearestIndex( attrDataBinId(bid).inl );
    if ( inlidx<0 || inlidx>=attrdata_->getInlSz() )
	return false;

    const int crlidx = 
		attrdata_->crlsampling.nearestIndex( attrDataBinId(bid).crl );
    if ( crlidx<0 || crlidx>=attrdata_->getCrlSz() )
	return false;

    const Array3D<float>& cube = attrdata_->getCube(0);

    const ValueSeries<float>* storage = cube.getStorage();
    if ( !storage ) return false; 

    const OffsetValueSeries<float> valarr( 
				const_cast<ValueSeries<float>&>(*storage), 
				cube.info().getOffset( inlidx, crlidx, 0 ) ); 

    const SamplingData<float> sd( zrg.start, zrg.step );
    ValueSeriesEvFinder<float, float>
			    evfinder( valarr, attrdata_->getZSz()-1, sd);

    const float zstep = fabs( zrg.step );
    const float upbound = targetz + permzrange_.start - 0.01*zstep;
    const float dnbound = targetz + permzrange_.stop  + 0.01*zstep;

    const Interval<float> uprg( targetz, mMAX(zrg.start,upbound-zstep) );
    const Interval<float> dnrg( targetz, mMIN(zrg.stop, dnbound+zstep) );

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
	    eventpos = fabs(targetz-upevent.pos)<fabs(targetz-dnevent.pos) ?
		upevent.pos : dnevent.pos;
	else 
	    eventpos = upfound ? upevent.pos : dnevent.pos;
    }
    else if ( evtype_==VSEvent::Max || evtype_==VSEvent::Min )
    {
	float upampl;
	bool uploopskip;
	float uptroughampl;
	ValueSeriesEvent<float,float> upevent =
	    findExtreme(evfinder,uprg,threshold,upampl,uploopskip,uptroughampl);

	float dnampl;
	bool dnloopskip;
	float dntroughampl;
	ValueSeriesEvent<float,float> dnevent =
	    findExtreme(evfinder,dnrg,threshold,dnampl,dnloopskip,dntroughampl);

	float troughthreshold = !mIsUdf(threshold) ? -0.1*threshold : 0;
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
		eventpos = uploopskip && dnevent.pos<=dnbound ? 
						  dnevent.pos : upevent.pos;
	    }
	    else
	    {
		if ( upampl==dnampl && fabs(upevent.pos-dnevent.pos)<zstep )
		    eventpos = (upevent.pos + dnevent.pos) / 2;
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
    
    targetz = eventpos;
    return true;
}


ValueSeriesEvent<float,float> HorizonAdjuster::findExtreme(
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
	if ( !troughamplset ||
		(evtype_==VSEvent::Min && val>troughampl ) ||
		(evtype_==VSEvent::Max && val<troughampl ) )
	{
	    troughamplset = true;
	    troughampl = val;
	}

	if ( !hasloopskips &&
		((evtype_==VSEvent::Min && val>prev ) ||
		 (evtype_==VSEvent::Max && val<prev )) )
	{
	    hasloopskips = true;
	}

	avgampl += val;
    }

    avgampl /= amplsumrg.width()+1;
    return ev;
}


bool HorizonAdjuster::trackByAmplitude( const BinID& refbid,
				        const BinID& targetbid,
				        float& targetz ) const
{
    targetz = horizon_.getPos( sectionid_, targetbid.getSerialized() ).z;
    if ( useAbsThreshold() )
	return snap( targetbid, amplitudeThreshold(), targetz );

    const float refdepth =
		horizon_.getPos( sectionid_, refbid.getSerialized() ).z;
    const BinIDValue refbidval( attrDataBinId(refbid), refdepth );

    float threshold;
    if ( !attrdata_->getValue(0, refbidval, &threshold, true) )
	return false;

    threshold *= (1-allowedvar_);
    return snap( targetbid, threshold, targetz );
}


#define mGetArray( prefix, extrarg ) \
    const int prefix##inlidx = \
	attrdata_->inlsampling.nearestIndex( attrDataBinId(prefix##bid).inl ); \
    if ( prefix##inlidx<0 || prefix##inlidx>=attrdata_->getInlSz() ) \
	return false; \
 \
    const int prefix##crlidx = \
	attrdata_->crlsampling.nearestIndex( attrDataBinId(prefix##bid).crl ); \
    if ( prefix##crlidx<0 || prefix##crlidx>=attrdata_->getCrlSz() ) \
	return false; \
 \
    const float prefix##depth = \
		horizon_.getPos( sectionid_, prefix##bid.getSerialized() ).z; \
    const int prefix##sample = mNINT(prefix##depth/zstep) - attrdata_->z0; \
     \
    const Interval<int> prefix##rg = \
		Interval<int>( prefix##sample, prefix##sample ) + extrarg; \
    if ( prefix##rg.start<0 || prefix##rg.stop>=attrdata_->getZSz()-1 ) \
	return false; \
 \
    OffsetValueSeries<float> prefix##arr( \
	const_cast<ValueSeries<float>&>( *cube.getStorage() ), \
	cube.info().getOffset(prefix##inlidx,prefix##crlidx,prefix##rg.start ))

bool HorizonAdjuster::trackBySimilarity( const BinID& trefbid,
					 const BinID& targetbid,
					 float& targetz ) const
{
    const BinID refbid = trefbid.inl==-1 || trefbid.crl==-1
	? targetbid
	: trefbid;

    const Array3D<float>& cube = attrdata_->getCube(0);

    const float zstep = attrdata_->zstep;
    const Interval<int> similarityrg( mNINT( similaritywin_.start/zstep ),
				      mNINT( similaritywin_.stop/zstep ) );
    const int simlength = similarityrg.width()+1;

    mGetArray( ref, similarityrg );

    const Interval<int> testrange( mNINT(permzrange_.start/zstep),
	    			   mNINT(permzrange_.stop/zstep) );

    mGetArray( target, similarityrg+testrange );
    OffsetValueSeries<float> slidearr( targetarr, -testrange.start );

    float upsample, upsim; bool upflatstart;
    const bool findup = findMaxSimilarity( refarr, slidearr, simlength, 
	    				   -testrange.start+1, -1, 1, 
					   upsample, upsim, upflatstart);
    float dnsample, dnsim; bool dnflatstart;
    const bool finddn = findMaxSimilarity( refarr, slidearr, simlength, 
					   testrange.stop+1, 1, 1, 
					   dnsample, dnsim, dnflatstart);

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

    targetz = (bestmatch+attrdata_->z0) * zstep;

    const int bestidx = mNINT(bestmatch)-targetrg.start;
    return snap( targetbid, targetarr[bestidx], targetz );
}


void HorizonAdjuster::setHorizonPick(const BinID&  bid, float val )
{
    Coord3 pos = horizon_.getPos( sectionid_, bid.getSerialized() );
    pos.z = val;
    horizon_.setPos( sectionid_, bid.getSerialized(), pos, true );
}


void HorizonAdjuster::setAttributeSel( int idx, const Attrib::SelSpec& as )
{
    if ( idx )
	return;

    if ( !attribsel_ ) attribsel_ = new Attrib::SelSpec;
    *attribsel_ = as;

    if ( attrdata_ ) attrdata_->unRef();
    attrdata_ = engine().getAttribCache( *attribsel_ );
    if ( attrdata_ ) attrdata_->ref();
}


bool HorizonAdjuster::hasInitializedSetup() const
{
   return ( attribsel_ && attribsel_->id()>=0 );
}


void HorizonAdjuster::fillPar( IOPar& iopar ) const
{
    SectionAdjuster::fillPar( iopar );
    if ( attribsel_ ) attribsel_->fillPar( iopar );
    iopar.set( sKeyTrackEvent(), eString(VSEvent::Type,evtype_) );
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
    PtrMan<IOPar> oldpar = iopar.subselect("attrval.Attrib 0");
    if ( !attribsel_ ) attribsel_ = new Attrib::SelSpec;
    if ( !oldpar || !attribsel_->usePar(*oldpar) )
    {
	if ( !attribsel_->usePar(iopar) )
	    return false;
    }

    const char* res = iopar.find( sKeyTrackEvent() );
    if ( res && *res ) evtype_ = eEnum(VSEvent::Type,res);
    iopar.get( sKeyPermittedZRange(),permzrange_.start,permzrange_.stop );
    iopar.get( sKeyValueThreshold(), ampthreshold_ );
    iopar.get( sKeyAllowedVariance(), allowedvar_);
    iopar.getYN( sKeyUseAbsThreshold(), useabsthreshold_ );
    iopar.get( sKeySimWindow(),similaritywin_.start,similaritywin_.stop );
    iopar.get( sKeySimThreshold(), similaritythreshold_ );
    iopar.getYN( sKeyTrackByValue(), trackbyvalue_ );

    return SectionAdjuster::usePar( iopar );
}


}; // namespace MPE
