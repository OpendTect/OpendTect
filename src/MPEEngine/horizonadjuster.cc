/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizonadjuster.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "emhorizon.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "genericnumer.h"
#include "iopar.h"
#include "mpeengine.h"
#include "samplingdata.h"
#include "seisdatapack.h"
#include "survinfo.h"

namespace MPE
{

HorizonAdjuster::HorizonAdjuster( EM::Horizon& hor )
    : SectionAdjuster()
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
    evtracker_.normalizeSimilarityValues( true );

    mDynamicCastGet(EM::Horizon2D*,hor2d,&horizon_)
    evtracker_.setCompareMethod( hor2d ? EventTracker::AdjacentParent
				       : EventTracker::SeedTrace );
}


HorizonAdjuster::~HorizonAdjuster()
{
    delete attribsel_;
    delete &evtracker_;
    dpm_.unRef( datapackid_ );
}


int HorizonAdjuster::getNrAttributes() const
{ return 1; }


const Attrib::SelSpec* HorizonAdjuster::getAttributeSel( int idx ) const
{ return !idx ? attribsel_ : 0; }


void HorizonAdjuster::reset()
{
    dpm_.unRef( datapackid_ );
    datapackid_ = attribsel_ ? engine().getAttribCacheID(*attribsel_)
			     : DataPack::cNoID();
    dpm_.ref( datapackid_ );
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


void HorizonAdjuster::allowAmplitudeSignChange( bool yn )
{ evtracker_.allowAmplitudeSignChange( yn ); }

bool HorizonAdjuster::isAmplitudeSignChangeAllowed() const
{ return evtracker_.isAmplitudeSignChangeAllowed(); }


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

void HorizonAdjuster::setSnapToEvent( bool yn )
{ evtracker_.setSnapToEvent( yn ); }

bool HorizonAdjuster::snapToEvent() const
{ return evtracker_.snapToEvent(); }


int HorizonAdjuster::nextStep()
{
    auto sdp = dpm_.get<SeisDataPack>( datapackid_ );
    if ( !sdp || sdp->isEmpty() )
	return ErrorOccurred();

    mDynamicCastGet(EM::Horizon3D*,hor3d,&horizon_)
    for ( int idx=0; idx<tks_.size(); idx++ )
    {
	const TrcKey& target = tks_[idx];
	float targetz = mUdf(float);
	bool res = false;
	if ( tksrc_.size() > idx )
	{
	    const TrcKey& ref = tksrc_[idx];
	    res = track( ref, target, targetz );
	    if ( res && hor3d )
		hor3d->setParent( target, ref );
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
	    const Interval<float> searchwin = searchWindow();
	    const float winsz = SI().zStep() * 50; // should be sufficient
	    setSearchWindow( Interval<float>(-winsz,winsz) );

	    const TrcKey tk = target.is3D() ? TrcKey( BinID::udf() )
					: TrcKey( target.geomID(), mUdf(int) );
	    res = track( tk, target, targetz );

	    evtracker_.useSimilarity( wasusingsim );
	    evtracker_.setCompareMethod( curmethod );
	    setSearchWindow( searchwin );

	    if ( res )
		setSeedPosition( target );
	}

	if ( res || removeonfailure_ )
	{
	    const float newz = res ? targetz : mUdf(float);
	    setHorizonPick( target, newz );
	}
    }

    return Finished();
}


bool HorizonAdjuster::track( const TrcKey& from, const TrcKey& to,
			     float& targetz ) const
{
    auto sdp = dpm_.get<SeisDataPack>( datapackid_ );
    if ( !sdp || sdp->isEmpty() )
	return false;

    const Array3D<float>& array = sdp->data( 0 );
    if ( !array.getStorage() ) return false;

    if ( !horizon_.hasZ(to) )
	return false;

    const int totrcidx = sdp->getGlobalIdx( to );
    if ( totrcidx < 0 ) return false;

    const OffsetValueSeries<float> tovs = sdp->getTrcStorage( 0, totrcidx );
    const float startz = horizon_.getZ( to );
    if ( mIsUdf(startz) )
	return false;

    const StepInterval<float> zsamp = sdp->zRange();
    const int nrz = zsamp.nrSteps() + 1;
    const SamplingData<float> sd( zsamp.start, zsamp.step );
    evtracker_.setRangeStep( sd.step );
    evtracker_.setTarget( &tovs, nrz, sd.getfIndex(startz) );

    if ( !mIsUdf(from.lineNr()) && !mIsUdf(from.trcNr()) )
    {
	if ( !horizon_.hasZ(from) )
	    return false;

	const int seedtrcidx = sdp->getGlobalIdx( seedtk_ );
	if ( seedtrcidx < 0 )
	    return false;

	OffsetValueSeries<float> seedvs = sdp->getTrcStorage( 0, seedtrcidx );
	if ( evtracker_.getCompareMethod() == EventTracker::SeedTrace )
	{
	    const float seedz = horizon_.getZ( seedtk_ );
	    evtracker_.setSeed( &seedvs, nrz, sd.getfIndex(seedz) );
	}
	else
	    evtracker_.setSeed( 0, -1, mUdf(float) );

	const int fromtrcidx = sdp->getGlobalIdx( from );
	if ( fromtrcidx < 0 )
	    return false;

	const OffsetValueSeries<float> fromvs =
			sdp->getTrcStorage( 0, fromtrcidx );
	const float fromz = horizon_.getZ( from );
	evtracker_.setSource( &fromvs, nrz, sd.getfIndex(fromz) );
	if ( !evtracker_.isOK() )
	    return false;

	const bool res = evtracker_.track();
	const float resz = sd.atIndex( evtracker_.targetDepth() );
	if ( !searchWindow().includes(resz-startz,false) )
	    return false;

	if ( res )
	    targetz = resz;

	return res;
    }

    evtracker_.setSource( 0, nrz, 0 );
    if ( !evtracker_.isOK() )
	return false;

    const bool res = evtracker_.snap( 0 );
    const float resz = sd.atIndex( evtracker_.targetDepth() );
    if ( !searchWindow().includes(resz-startz,false) )
	return false;

    if ( res ) targetz = resz;
    return true;
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


void HorizonAdjuster::setHorizonPick( const TrcKey& tk, float val )
{
    horizon_.setZAndNodeSourceType( tk, val, setundo_, EM::EMObject::Auto );

    mDynamicCastGet(EM::Horizon3D*,hor3d,&horizon_);
    if ( !hor3d ) return;

    hor3d->auxdata.setAuxDataVal( 0, tk, evtracker_.targetValue() );
    hor3d->auxdata.setAuxDataVal( 1, tk, evtracker_.quality() );
    hor3d->auxdata.setAuxDataVal( 2, tk, (float)seedid_ );
}


void HorizonAdjuster::setAttributeSel( int idx, const Attrib::SelSpec& as )
{
    if ( idx )
	return;

    if ( !attribsel_ ) attribsel_ = new Attrib::SelSpec;
    *attribsel_ = as;

    dpm_.unRef( datapackid_ );
    datapackid_ = engine().getAttribCacheID( *attribsel_ );
    dpm_.ref( datapackid_ );
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
