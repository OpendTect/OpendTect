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
#include "valseriestracker.h"

namespace MPE
{

HorizonAdjuster::HorizonAdjuster( EM::Horizon& hor, EM::SectionID sid )
    : SectionAdjuster(sid)
    , horizon_(hor)
    , attribsel_(0)
    , datapackid_(DataPack::cNoID())
    , dpm_(DPM(DataPackMgr::SeisID()))
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
    delete tracker_;
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
    ConstDataPackRef<RegularSeisDataPack> regsdp = dpm_.obtain( datapackid_ );
    if ( !regsdp || regsdp->isEmpty() )
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
	    setHorizonPick( targetbid, mUdf(float) );
    }

    return Finished();
}


bool HorizonAdjuster::track( const BinID& from, const BinID& to,
			     float& targetz) const
{
    ConstDataPackRef<RegularSeisDataPack> regsdp = dpm_.obtain( datapackid_ );
    if ( !regsdp || regsdp->isEmpty() )
	return false;

    const Array3D<float>& array = regsdp->data( 0 );
    if ( !array.getStorage() ) return false;

    if ( !horizon_.isDefined(sectionid_, to.toInt64()) )
	return false;

    const int totrcidx = regsdp->getGlobalIdx( getTrcKey(to) );
    if ( totrcidx < 0 ) return false;

    const OffsetValueSeries<float> tovs = regsdp->getTrcStorage( 0, totrcidx );
    const float startz = (float) horizon_.getPos( sectionid_, to.toInt64() ).z;
    const TrcKeyZSampling& tkzs = regsdp->sampling();
    const SamplingData<float> sd( tkzs.zsamp_.start, tkzs.zsamp_.step );
    tracker_->setRangeStep( sd.step );
    tracker_->setTarget( &tovs, tkzs.nrZ(), sd.getfIndex(startz) );
    if ( from.inl()!=-1 && from.crl()!=-1 )
    {
	if ( !horizon_.isDefined(sectionid_, from.toInt64()) )
	    return false;

	const int fromtrcidx = regsdp->getGlobalIdx( getTrcKey(from) );
	if ( fromtrcidx < 0 ) return false;

	const OffsetValueSeries<float> fromvs =
			regsdp->getTrcStorage( 0, fromtrcidx );
	const float fromz = (float)horizon_.getPos(sectionid_,from.toInt64()).z;
	tracker_->setSource( &fromvs, tkzs.nrZ(), sd.getfIndex(fromz) );
	if ( !tracker_->isOK() )
	    return false;

	const bool res = tracker_->track();
	const float resz = sd.atIndex( tracker_->targetDepth() );
	if ( !permittedZRange().includes(resz-startz,false) )
	    return false;

	if ( res ) targetz = resz;
	return res;
    }

    tracker_->setSource( 0, tkzs.nrZ(), 0 );
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


TrcKeyZSampling
	HorizonAdjuster::getAttribCube( const Attrib::SelSpec& sp ) const
{
    if ( !attribsel_ || sp != *attribsel_ )
	return SectionAdjuster::getAttribCube( sp );

    TrcKeyZSampling res = engine().activeVolume();

    res.zsamp_.start += tracker_->permittedRange().start;
    res.zsamp_.stop += tracker_->permittedRange().stop;
    if ( !trackByValue() )
    {
	res.zsamp_.start += tracker_->similarityWindow().start;
	res.zsamp_.stop += tracker_->similarityWindow().stop;
    }

    res.snapToSurvey();
    return res;
}


const TrcKey HorizonAdjuster::getTrcKey( const BinID& bid ) const
{
    ConstDataPackRef<RegularSeisDataPack> regsdp = dpm_.obtain( datapackid_ );
    if ( !regsdp ) return TrcKey::udf();

    return regsdp->is2D() ?
      Survey::GM().traceKey(regsdp->getTrcKey(0).geomID(),bid.crl()) :
      Survey::GM().traceKey(Survey::GM().default3DSurvID(),bid.inl(),bid.crl());
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
    tracker_->fillPar( trackerpar );
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
    return trackerpar && tracker_->usePar( *trackerpar );
}

} // namespace MPE
