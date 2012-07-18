/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2003
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: horizonadjuster.cc,v 1.70 2012-07-18 09:16:15 cvsjaap Exp $";

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
#include "valseriestracker.h"

#include <math.h>

namespace MPE {


HorizonAdjuster::HorizonAdjuster( EM::Horizon& hor,
	const EM::SectionID& sid )
    : SectionAdjuster(sid)
    , horizon_(hor)
    , attribsel_(0)
    , attrdata_(0)
    , tracker_( new EventTracker )
{
    tracker_->setSimilarityWindow(
	    Interval<float>(-10*SI().zStep(), 10*SI().zStep() ) );
    tracker_->setPermittedRange(
	    Interval<float>(-3*SI().zStep(), 3*SI().zStep() ) );
    tracker_->setRangeStep( SI().zStep() );
}


HorizonAdjuster::~HorizonAdjuster()
{
    delete attribsel_;
    if ( attrdata_ ) attrdata_->unRef();
    delete tracker_;
}


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


void HorizonAdjuster::setPermittedZRange( const Interval<float>& rg )
{ tracker_->setPermittedRange( rg ); }


Interval<float> HorizonAdjuster::permittedZRange() const
{ return tracker_->permittedRange(); }


void HorizonAdjuster::setTrackByValue( bool yn )
{ tracker_->useSimilarity( !yn ); }


bool HorizonAdjuster::trackByValue() const
{ return !tracker_->usesSimilarity(); }


void HorizonAdjuster::setTrackEvent( VSEvent::Type ev )
{ tracker_->setTrackEvent( ev ); }


VSEvent::Type HorizonAdjuster::trackEvent() const
{ return tracker_->trackEvent(); }


void HorizonAdjuster::setAmplitudeThreshold( float th )
{ tracker_->setAmplitudeThreshold( th ); }


float HorizonAdjuster::amplitudeThreshold() const
{ return tracker_->amplitudeThreshold(); }


void HorizonAdjuster::setAmplitudeThresholds( const TypeSet<float>& ats )
{ tracker_->setAmplitudeThresholds( ats ); }


TypeSet<float>& HorizonAdjuster::getAmplitudeThresholds()
{ return tracker_->getAmplitudeThresholds(); }


void HorizonAdjuster::setAllowedVariances( const TypeSet<float>& avs )
{ tracker_->setAllowedVariances( avs ); }


TypeSet<float>& HorizonAdjuster::getAllowedVariances()
{ return tracker_->getAllowedVariances(); }


void HorizonAdjuster::setAllowedVariance( float v )
{ tracker_->setAllowedVariance( v ); }


float HorizonAdjuster::allowedVariance() const
{ return tracker_->allowedVariance(); }


void HorizonAdjuster::setUseAbsThreshold( bool abs )
{ tracker_->setUseAbsThreshold( abs ); }


bool HorizonAdjuster::useAbsThreshold() const
{ return tracker_->useAbsThreshold(); }


void HorizonAdjuster::setSimilarityWindow( const Interval<float>& rg )
{ tracker_->setSimilarityWindow( rg ); }


Interval<float> HorizonAdjuster::similarityWindow() const
{ return tracker_->similarityWindow(); }


void HorizonAdjuster::setSimilarityThreshold( float th )
{ tracker_->setSimilarityThreshold( th ); }


float HorizonAdjuster::similarityThreshold() const
{ return tracker_->similarityThreshold(); }


int HorizonAdjuster::nextStep()
{
    if ( !attrdata_ || !attrdata_->nrCubes() )
	return ErrorOccurred();

    for ( int idx=0; idx<pids_.size(); idx++ )
    {
	BinID targetbid;
	targetbid.fromInt64( pids_[idx] );
	float targetz;
	bool res;
	if ( pidsrc_.size() > idx )
	{
	    BinID refbid;
	    refbid.fromInt64( pidsrc_[idx] );
	    res = track( refbid, targetbid, targetz );
	}
	else
	{
	    const bool wasusingsim = tracker_->usesSimilarity();
	    tracker_->useSimilarity( false );
	    res = track( BinID(-1,-1), targetbid, targetz );
	    tracker_->useSimilarity( wasusingsim );
	}

	if ( res )
	    setHorizonPick( targetbid, targetz );
	else if ( removeonfailure_ )
	    setHorizonPick(targetbid, mUdf(float) );
    }

    return Finished();
}


bool HorizonAdjuster::track( const BinID& from, const BinID& to,
			     float& targetz) const 
{
    //const Array3D<float>& cube = attrdata_->getCube(0);
    CubeSampling cs = attrdata_->getCubeSampling();
    const int toinlidx = cs.hrg.inlRange().nearestIndex( attrDataBinId(to).inl);
    if ( toinlidx<0 || toinlidx>=cs.nrInl() )
	return false;

    const int tocrlidx = cs.hrg.crlRange().nearestIndex( attrDataBinId(to).crl);
    if ( tocrlidx<0 || tocrlidx>=cs.nrCrl() )
	return false;

    const ValueSeries<float>* storage = 0;
    od_int64 tooffset = 0;
    if ( !attrdata_->is2D() && attrdata_->get3DData() )
    {
	const Array3D<float>& arr = attrdata_->get3DData()->getCube(0);
	storage = arr.getStorage();
	tooffset = arr.info().getOffset( toinlidx, tocrlidx, 0 );
    }
    else if ( attrdata_->is2D() && attrdata_->get2DData() )
    {
	const int dhidx = attrdata_->get2DData()->indexOf( to.crl );
	if ( dhidx<0 )
	    return false;

	const Array3D<float>& arr = *attrdata_->get2DData()->dataset_;
	storage = arr.getStorage();
	const int component = 0;
	tooffset = arr.info().getOffset( component, dhidx, 0 );
    }

    if ( !storage ) return false; 

    const int zsz = cs.nrZ();

    const SamplingData<double> sd( cs.zrg.start,cs.zrg.step );

    const OffsetValueSeries<float> toarr( 
		    const_cast<ValueSeries<float>&>(*storage), tooffset ); 

    if ( !horizon_.isDefined(sectionid_, to.toInt64()) )
	return false;
    const float startz = horizon_.getPos( sectionid_, to.toInt64() ).z;
    tracker_->setRangeStep( sd.step );

    tracker_->setTarget( &toarr, zsz, sd.getfIndex(startz) );

    if ( from.inl!=-1 && from.crl!=-1 )
    {
	const int frominlidx = 
	    cs.hrg.inlRange().nearestIndex(attrDataBinId(from).inl);
	if ( frominlidx<0 || frominlidx>=cs.nrInl() )
	    return false;

	const int fromcrlidx = 
	    cs.hrg.crlRange().nearestIndex(attrDataBinId(from).crl);
	if ( fromcrlidx<0 || fromcrlidx>=cs.nrCrl() )
	    return false;

	od_int64 fromoffset;
	if ( !attrdata_->is2D() )
	    fromoffset = attrdata_->get3DData()->getCube(0).info().getOffset(
						frominlidx, fromcrlidx, 0 );

	if ( attrdata_->is2D() && attrdata_->get2DData() )
	{
	    const int dhidx = attrdata_->get2DData()->indexOf( from.crl );
	    if ( dhidx<0 )
		return false;

	    const Array3D<float>& arr = *attrdata_->get2DData()->dataset_;
	    const int component = 0;
	    fromoffset = arr.info().getOffset( component, dhidx, 0 );
	}

	const OffsetValueSeries<float> fromarr( 
		    const_cast<ValueSeries<float>&>(*storage), fromoffset ); 
	if ( !horizon_.isDefined(sectionid_, from.toInt64()) )
	    return false;
	const float fromz = horizon_.getPos(sectionid_,from.toInt64()).z;
	tracker_->setSource( &fromarr, zsz, sd.getfIndex(fromz) );

	if ( !tracker_->isOK() )
	    return false;

	const bool res = tracker_->track();
	const float resz = sd.atIndex( tracker_->targetDepth() );

	if ( !permittedZRange().includes(resz-startz,false) )
	    return false;

	if ( res ) targetz = resz;
	return res;
    }

    tracker_->setSource( 0, zsz, 0 );

    if ( !tracker_->isOK() )
	return false;

    const bool res = tracker_->track();
    const float resz = sd.atIndex( tracker_->targetDepth() );

    if ( !permittedZRange().includes(resz-startz,false) )
	return false;

    if ( res ) targetz = resz;
    return res;
}


void HorizonAdjuster::getNeededAttribs(
	ObjectSet<const Attrib::SelSpec>& specs ) const
{
    if ( !attribsel_ || !attribsel_->id().isValid() )
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

    res.zrg.start += tracker_->permittedRange().start;
    res.zrg.stop += tracker_->permittedRange().stop;
    if ( !trackByValue() )
    {
	res.zrg.start += tracker_->similarityWindow().start;
	res.zrg.stop += tracker_->similarityWindow().stop;
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
    return is2D() && attrdata_->getCubeSampling().nrInl()==1 
	? BinID( attrdata_->getCubeSampling().hrg.inlRange().start, bid.crl )
	: bid;
}


void HorizonAdjuster::setHorizonPick(const BinID&  bid, float val )
{
    Coord3 pos = horizon_.getPos( sectionid_, bid.toInt64() );
    pos.z = val;
    horizon_.setPos( sectionid_, bid.toInt64(), pos, setundo_ );
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
   return ( attribsel_ && attribsel_->id().isValid() );
}


void HorizonAdjuster::fillPar( IOPar& iopar ) const
{
    SectionAdjuster::fillPar( iopar );
    IOPar trackerpar;
    tracker_->fillPar( trackerpar );
    iopar.mergeComp( trackerpar, sKeyTracker() );
    if ( attribsel_ ) attribsel_->fillPar( iopar );
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

    PtrMan<IOPar> trackerpar = iopar.subselect( sKeyTracker() );
    if ( trackerpar )
    {
	if ( !tracker_->usePar( *trackerpar ) )
	    return false;
    }
    else
    {
	//OD3 format (old)

	VSEvent::Type eventtype;
	if ( VSEvent::parseEnumType( iopar.find( "Track event" ), eventtype ) )
	   tracker_->setTrackEvent( eventtype );

	float valthreshold;
	if ( iopar.get( "Value threshhold", valthreshold ) )
	    tracker_->setAmplitudeThreshold( valthreshold );
	float variance;
	if ( iopar.get( "Allowed variance", variance) )
	    tracker_->setAllowedVariance( variance );
	bool absthreshold;
	if ( iopar.getYN( "Use abs threshhold", absthreshold ) )
	    tracker_->setUseAbsThreshold( absthreshold );
	float simthreshold;
	if ( iopar.get( "Similarity threshhold", simthreshold ) )
	    tracker_->setSimilarityThreshold( simthreshold );
	bool byvalue;
	if ( iopar.getYN( "Track by value", byvalue ) )
	    tracker_->useSimilarity( !byvalue );
    }

    //The ranges was written in OD3.2, so this can be 
    //removed when OD5 is released.
    //Range is now stored with tracker
    Interval<float> permzrange;
    if ( iopar.get( "Permitted Z range", permzrange ) )
	tracker_->setPermittedRange( permzrange );

    Interval<float> similaritywin;
    if ( iopar.get( "Similarity window", similaritywin ) )
	tracker_->setSimilarityWindow(similaritywin);

    return SectionAdjuster::usePar( iopar );
}


}; // namespace MPE
