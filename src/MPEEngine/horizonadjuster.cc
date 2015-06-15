/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2003
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "horizonadjuster.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "emhorizon.h"
#include "emhorizon2d.h"
#include "genericnumer.h"
#include "iopar.h"
#include "mpeengine.h"
#include "samplingdata.h"
#include "seisdatapack.h"
#include "survinfo.h"

namespace MPE
{

HorizonAdjuster::HorizonAdjuster( EM::Horizon& hor, EM::SectionID sid )
    : SectionAdjuster(sid)
    , horizon_(hor)
    , attribsel_(0)
    , datapackid_(DataPack::cNoID())
    , dpm_(DPM(DataPackMgr::SeisID()))
    , evtracker_(*new EventTracker)
{
    evtracker_.setSimilarityWindow(
	    Interval<float>(-10*SI().zStep(), 10*SI().zStep() ) );
    evtracker_.setPermittedRange(
	    Interval<float>(-2*SI().zStep(), 2*SI().zStep() ) );
    evtracker_.setRangeStep( SI().zStep() );
}


HorizonAdjuster::~HorizonAdjuster()
{
    delete attribsel_;
    delete &evtracker_;
    dpm_.release( datapackid_ );
}


int HorizonAdjuster::getNrAttributes() const
{ return 1; }


const Attrib::SelSpec* HorizonAdjuster::getAttributeSel( int idx ) const
{ return !idx ? attribsel_ : 0; }


void HorizonAdjuster::reset()
{
    dpm_.release( datapackid_ );
    datapackid_ = attribsel_ ? engine().getAttribCacheID(*attribsel_)
			     : DataPack::cNoID();
    dpm_.obtain( datapackid_ );
}


void HorizonAdjuster::setCompareMethod( EventTracker::CompareMethod cm )
{ evtracker_.setCompareMethod( cm ); }

EventTracker::CompareMethod HorizonAdjuster::getCompareMethod() const
{ return evtracker_.getCompareMethod(); }


void HorizonAdjuster::setSearchWindow( const Interval<float>& rg )
{ evtracker_.setPermittedRange( rg ); }

Interval<float> HorizonAdjuster::searchWindow() const
{ return evtracker_.permittedRange(); }


void HorizonAdjuster::setTrackByValue( bool yn )
{ evtracker_.useSimilarity( !yn ); }

bool HorizonAdjuster::trackByValue() const
{ return !evtracker_.usesSimilarity(); }


void HorizonAdjuster::setTrackEvent( VSEvent::Type ev )
{ evtracker_.setTrackEvent( ev ); }

VSEvent::Type HorizonAdjuster::trackEvent() const
{ return evtracker_.trackEvent(); }


void HorizonAdjuster::setAmplitudeThreshold( float th )
{ evtracker_.setAmplitudeThreshold( th ); }

float HorizonAdjuster::amplitudeThreshold() const
{ return evtracker_.amplitudeThreshold(); }


void HorizonAdjuster::setAmplitudeThresholds( const TypeSet<float>& ats )
{ evtracker_.setAmplitudeThresholds( ats ); }

TypeSet<float>& HorizonAdjuster::getAmplitudeThresholds()
{ return evtracker_.getAmplitudeThresholds(); }


void HorizonAdjuster::setAllowedVariances( const TypeSet<float>& avs )
{ evtracker_.setAllowedVariances( avs ); }

TypeSet<float>& HorizonAdjuster::getAllowedVariances()
{ return evtracker_.getAllowedVariances(); }


void HorizonAdjuster::setAllowedVariance( float v )
{ evtracker_.setAllowedVariance( v ); }

float HorizonAdjuster::allowedVariance() const
{ return evtracker_.allowedVariance(); }


void HorizonAdjuster::setUseAbsThreshold( bool abs )
{ evtracker_.setUseAbsThreshold( abs ); }

bool HorizonAdjuster::useAbsThreshold() const
{ return evtracker_.useAbsThreshold(); }


void HorizonAdjuster::setSimilarityWindow( const Interval<float>& rg )
{ evtracker_.setSimilarityWindow( rg ); }

Interval<float> HorizonAdjuster::similarityWindow() const
{ return evtracker_.similarityWindow(); }


void HorizonAdjuster::setSimilarityThreshold( float th )
{ evtracker_.setSimilarityThreshold( th ); }

float HorizonAdjuster::similarityThreshold() const
{ return evtracker_.similarityThreshold(); }


int HorizonAdjuster::nextStep()
{
    ConstDataPackRef<SeisDataPack> sdp = dpm_.obtain( datapackid_ );
    if ( !sdp || sdp->isEmpty() )
	return ErrorOccurred();

    for ( int idx=0; idx<pids_.size(); idx++ )
    {
	BinID targetbid = BinID::fromInt64( pids_[idx] );
	float targetz;
	bool res;
	if ( pidsrc_.size() > idx )
	{
	    BinID refbid = BinID::fromInt64( pidsrc_[idx] );
	    res = track( refbid, targetbid, targetz );
	}
	else // adjust picked seed
	{
	    // get current settings and temporarily change eventtracker
	    // for adjusting the seed
	    const bool wasusingsim = evtracker_.usesSimilarity();
	    evtracker_.useSimilarity( false );
	    EventTracker::CompareMethod curmethod =
				evtracker_.getCompareMethod();
	    evtracker_.setCompareMethod( EventTracker::None );

	    res = track( BinID(-1,-1), targetbid, targetz );

	    evtracker_.useSimilarity( wasusingsim );
	    evtracker_.setCompareMethod( curmethod );

	    if ( res )
		setSeedPosition( targetbid );
	}

	if ( res || removeonfailure_ )
	{
	    const float newz = res ? targetz : mUdf(float);
	    setHorizonPick( targetbid, newz );
	}
    }

    return Finished();
}


bool HorizonAdjuster::track( const BinID& from, const BinID& to,
			     float& targetz ) const
{
    ConstDataPackRef<SeisDataPack> sdp = dpm_.obtain( datapackid_ );
    if ( !sdp || sdp->isEmpty() )
	return false;

    const Array3D<float>& array = sdp->data( 0 );
    if ( !array.getStorage() ) return false;

    if ( !horizon_.isDefined(sectionid_,to.toInt64()) )
	return false;

    const int totrcidx = sdp->getGlobalIdx( getTrcKey(to) );
    if ( totrcidx < 0 ) return false;

    const OffsetValueSeries<float> tovs = sdp->getTrcStorage( 0, totrcidx );
    const float startz = (float) horizon_.getPos( sectionid_, to.toInt64() ).z;
    const StepInterval<float>& zsamp = sdp->getZRange();
    const int nrz = zsamp.nrSteps() + 1;
    const SamplingData<float> sd( zsamp.start, zsamp.step );
    evtracker_.setRangeStep( sd.step );
    evtracker_.setTarget( &tovs, nrz, sd.getfIndex(startz) );

    if ( from.inl()!=-1 && from.crl()!=-1 )
    {
	if ( !horizon_.isDefined(sectionid_,from.toInt64()) )
	    return false;

	const int seedtrcidx = sdp->getGlobalIdx( seedtk_ );
	if ( seedtrcidx < 0 ) return false;

	OffsetValueSeries<float> seedvs = sdp->getTrcStorage( 0, seedtrcidx );
	if ( evtracker_.getCompareMethod() == EventTracker::SeedTrace )
	{
	    const float seedz = horizon_.getZ( seedtk_ );
	    evtracker_.setSeed( &seedvs, nrz, sd.getfIndex(seedz) );
	}
	else
	    evtracker_.setSeed( 0, -1, mUdf(float) );

	const int fromtrcidx = sdp->getGlobalIdx( getTrcKey(from) );
	if ( fromtrcidx < 0 ) return false;

	const OffsetValueSeries<float> fromvs =
			sdp->getTrcStorage( 0, fromtrcidx );
	const float fromz = (float)horizon_.getPos(sectionid_,from.toInt64()).z;
	evtracker_.setSource( &fromvs, nrz, sd.getfIndex(fromz) );
	if ( !evtracker_.isOK() )
	    return false;

	const bool res = evtracker_.track();
	const float resz = sd.atIndex( evtracker_.targetDepth() );
	if ( !searchWindow().includes(resz-startz,false) )
	    return false;

	if ( res ) targetz = resz;
	return res;
    }

    evtracker_.setSource( 0, nrz, 0 );
    if ( !evtracker_.isOK() )
	return false;

    const bool res = evtracker_.track();
    const float resz = sd.atIndex( evtracker_.targetDepth() );
    if ( !searchWindow().includes(resz-startz,false) )
	return false;

    if ( res ) targetz = resz;
    return res;
}


void HorizonAdjuster::getNeededAttribs( TypeSet<Attrib::SelSpec>& specs ) const
{
    if ( !attribsel_ || !attribsel_->id().isValid() )
	return;

    for ( int idx=specs.size()-1; idx>=0; idx-- )
    {
	if ( specs[idx] == *attribsel_ )
	    return;
    }

    specs += *attribsel_;
}


TrcKeyZSampling
	HorizonAdjuster::getAttribCube( const Attrib::SelSpec& sp ) const
{
    if ( !attribsel_ || sp != *attribsel_ )
	return SectionAdjuster::getAttribCube( sp );

    TrcKeyZSampling res = engine().activeVolume();

    res.zsamp_.start += evtracker_.permittedRange().start;
    res.zsamp_.stop += evtracker_.permittedRange().stop;
    if ( !trackByValue() )
    {
	res.zsamp_.start += evtracker_.similarityWindow().start;
	res.zsamp_.stop += evtracker_.similarityWindow().stop;
    }

    res.snapToSurvey();
    return res;
}


const TrcKey HorizonAdjuster::getTrcKey( const BinID& bid ) const
{
    ConstDataPackRef<SeisDataPack> sdp = dpm_.obtain( datapackid_ );
    if ( !sdp ) return TrcKey::udf();

    return sdp->is2D() ?
      Survey::GM().traceKey(sdp->getTrcKey(0).geomID(),bid.crl()) :
      Survey::GM().traceKey(Survey::GM().default3DSurvID(),bid.inl(),bid.crl());
}


void HorizonAdjuster::setHorizonPick( const BinID& bid, float val )
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

    dpm_.release( datapackid_ );
    datapackid_ = engine().getAttribCacheID( *attribsel_ );
    dpm_.obtain( datapackid_ );
}


bool HorizonAdjuster::hasInitializedSetup() const
{
   return attribsel_ && attribsel_->id().isValid();
}


void HorizonAdjuster::fillPar( IOPar& iopar ) const
{
    SectionAdjuster::fillPar( iopar );
    IOPar trackerpar;
    evtracker_.fillPar( trackerpar );
    iopar.mergeComp( trackerpar, sKeyTracker() );
    if ( attribsel_ ) attribsel_->fillPar( iopar );
}


bool HorizonAdjuster::usePar( const IOPar& iopar )
{
    if ( !SectionAdjuster::usePar(iopar) )
	return false;

    if ( !attribsel_ ) attribsel_ = new Attrib::SelSpec;
    if ( !attribsel_->usePar(iopar) )
	return false;

    PtrMan<IOPar> trackerpar = iopar.subselect( sKeyTracker() );
    return trackerpar && evtracker_.usePar( *trackerpar );
}

} // namespace MPE
