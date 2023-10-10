/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellelasticmodelcomputer.h"

#include "survinfo.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltrack.h"


Well::ElasticModelComputer::ElasticModelComputer( const Well::Data& wd )
    : wd_(&wd)
{
}


Well::ElasticModelComputer::ElasticModelComputer( const Well::Data& wd,
						  const Well::Log& vel,
						  const Well::Log* den,
						  const Well::Log* svel )
    : wd_(&wd)
{
    setLogs( vel, den, svel );
}


Well::ElasticModelComputer::~ElasticModelComputer()
{
    inplogs_.erase();
    uomset_.erase();
    delete ls_;
    delete lsnearest_;
}


#define mPVelIdx	0
#define mDenIdx		1
#define mSVelIdx	2
#define mErrRet(msg) { errmsg_ = msg; return false; }


bool Well::ElasticModelComputer::setVelLog( const Well::Log& log )
{
    if ( inplogs_.size() != 0 )
	mErrRet( tr("Internal: Velocity log must be set first. "
		    "Another log was set first" ) );

    inplogs_ += &log;

    return true;
}


bool Well::ElasticModelComputer::setDenLog( const Well::Log& log )
{
    if ( inplogs_.size() != 1 )
	mErrRet( tr("Internal: Velocity log must be set first.") );

    inplogs_ += &log;

    return true;
}


bool Well::ElasticModelComputer::setSVelLog( const Well::Log& log )
{
    if ( inplogs_.size() != 2 )
	mErrRet( tr("Internal:Velocity and density logs must be set first." ) );

    inplogs_ += &log;

    return true;
}


void Well::ElasticModelComputer::setLogs( const Well::Log& vellog,
					  const Well::Log* denlog,
					  const Well::Log* svellog )
{
    setVelLog( vellog );

    if ( denlog )
	setDenLog( *denlog );
    else
	inplogs_ += nullptr;

    if ( svellog )
	setSVelLog( *svellog );
}


void Well::ElasticModelComputer::setZrange( const Interval<float>& zrange,
					    bool istime )
{
    zrange_ = zrange;
    zrgistime_ = istime;
}


void Well::ElasticModelComputer::setExtractionPars( float zstep,
						    bool extractinistime )
{
    zstep_ = zstep;
    extractintime_ = extractinistime;
}


bool Well::ElasticModelComputer::computeFromLogs()
{
    if ( !inplogs_.size() )
	mErrRet( tr("Internal: No input logs provided") )

    if ( !getLogUnits() )
	return false;

    if ( zrange_.isUdf() || mIsUdf(zstep_) )
	mErrRet( tr("Please set the extraction range") )

    zrange_.start += zstep_ / 2.f;
    zrange_.stop -= zstep_ / 2.f;

    if ( !extractLogs() )
	mErrRet( tr("Cannot extract logs") )

    emodel_.erase();
    const int nrsteps = ls_->nrZSamples();
    const float dz = zrgistime_ ? zstep_ / 2.f : zstep_;
    int erroridx = -1;
    const bool needswave = inplogs_.validIdx( mSVelIdx ) &&
			   inplogs_.get(mSVelIdx);
    RefLayer* newlayer = nullptr;
    const UnitOfMeasure* depthuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    for ( int idl=0; idl<nrsteps; idl++ )
    {
	const float thickness = zrgistime_
			      ? depthuom->getSIValue( ls_->getThickness(idl) )
			      : dz;

	const float velp = getVelp( idl );
	const float den = getDensity( idl );
	if ( needswave )
	    newlayer = new ElasticLayer( thickness, velp, getSVel(idl), den );
	else
	    newlayer = new AILayer( thickness, velp, den );

	if ( !newlayer->isOK(true,false) && erroridx == -1 )
	    erroridx = idl;

	emodel_.add( newlayer );
    }

    if ( emodel_.isEmpty() )
	mErrRet( tr("Returning empty elastic model") )

    if ( erroridx != -1 )
    {
	const float depth =
		UnitOfMeasure::surveyDefDepthUnit()->getUserValueFromSI(
					  ls_->getDah(erroridx) +
					  ls_->getThickness(erroridx)/2.f );
	emodel_.interpolate( true, true, false );

	warnmsg_ = tr("Invalid log values found.\n"
		      "First occurence at MD: %1%2\n"
		      "Invalid values will be interpolated.")
		  .arg( toString(depth,2) )
		  .arg( UnitOfMeasure::surveyDefDepthUnitAnnot(true,false) );
    }

    float startdepth = zrange_.start;
    const float srd = SI().seismicReferenceDatum();
    if ( zrgistime_ )
    {
	const float startdah = ls_->getDah(0);
	startdepth = mCast(float,wd_->track().getPos( startdah ).z) -
		     ls_->getThickness(0) / 2.f;
    }

    emodel_.first()->setThickness( emodel_.first()->getThickness()
				+ depthuom->getSIValue(startdepth + srd) );

    return true;
}


bool Well::ElasticModelComputer::getLogUnits()
{
    if ( inplogs_.isEmpty() )
	mErrRet( tr("Internal: No logs in log set yet") )

    for ( int idx=0; idx<inplogs_.size(); idx++ )
    {
	if ( !inplogs_.validIdx(idx) )
	    mErrRet( tr("Internal: Error reading log in log set") );

	uomset_ += inplogs_[idx] ? inplogs_[idx]->unitOfMeasure() : 0;
    }

    velpissonic_ = uomset_.validIdx(mPVelIdx) && uomset_[mPVelIdx] &&
		   uomset_[mPVelIdx]->propType() == Mnemonic::Son;

    return true;
}


bool Well::ElasticModelComputer::extractLogs()
{
    if ( !wd_->d2TModel() )
	mErrRet( tr("Well has no valid time-depth model") )

    const float srddepth = -1.f * UnitOfMeasure::surveyDefSRDStorageUnit()->
				getSIValue(  SI().seismicReferenceDatum() );
    if ( (!zrgistime_ && zrange_.start < srddepth) ||
	  (zrgistime_ && zrange_.start < 0.f) )
	mErrRet( tr("Extraction interval should not start above SRD") )

    const Interval<float> zrange = zrange_;

    ls_ = new Well::LogSampler( wd_->d2TModel(), &wd_->track(), zrange,
				zrgistime_, zstep_, extractintime_,
				Stats::UseAvg, inplogs_ );

    lsnearest_ = new Well::LogSampler( wd_->d2TModel(), &wd_->track(), zrange,
				       zrgistime_, zstep_, extractintime_,
				       Stats::TakeNearest, inplogs_ );

    if ( !ls_ || !lsnearest_ )
	mErrRet( tr("Internal: Could not initialize log sampler") );

    if ( !ls_->execute() )
	mErrRet( ls_->errMsg() )

    if ( !lsnearest_->execute() )
	mErrRet( lsnearest_->errMsg() )

    return true;
}


float Well::ElasticModelComputer::getLogVal( int logidx, int sampidx ) const
{
    if ( logidx > ls_->nrZSamples() )
	return mUdf(float);

    const float val = ls_->getLogVal( logidx, sampidx );
    const float valintp = !lsnearest_ || logidx > lsnearest_->nrZSamples()
			? mUdf(float)
			: lsnearest_->getLogVal( logidx, sampidx );
    return mIsUdf(val) ? valintp : val;
}

#define mTestLogIdx(logidx) \
{ \
    if ( !inplogs_.validIdx(logidx) || !uomset_.validIdx(logidx) ) \
	return mUdf(float); \
\
    const Well::Log* inplog = inplogs_[logidx]; \
    if ( !inplog ) \
	return mUdf(float); \
}

float Well::ElasticModelComputer::getVelp( int idx ) const
{
    const int logidx = mPVelIdx;
    mTestLogIdx(logidx);
    float logval = getLogVal( logidx, idx );
    const UnitOfMeasure* uom = uomset_[logidx];
    logval = uom ? uom->getSIValue( logval ) : logval;

    return velpissonic_ ? 1.f / logval : logval;
}


float Well::ElasticModelComputer::getDensity( int idx ) const
{
    const int logidx = mDenIdx;
    mTestLogIdx(logidx);
    const float logval = getLogVal( logidx, idx );
    const UnitOfMeasure* uom = uomset_[logidx];
    return uom ? uom->getSIValue( logval ) : logval;
}


float Well::ElasticModelComputer::getSVel( int idx ) const
{
    const int logidx = mSVelIdx;
    mTestLogIdx(logidx);
    const float logval = getLogVal( logidx, idx );
    const UnitOfMeasure* uom = uomset_[logidx];
    return uom ? uom->getSIValue( logval ) : logval;
}
