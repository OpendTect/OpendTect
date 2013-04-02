/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Arnaud
 Date:		Mar 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "wellelasticmodelcomputer.h"

#include "survinfo.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltrack.h"


namespace Well
{

Well::ElasticModelComputer::ElasticModelComputer( const Well::Data& wd )
    : wd_(wd)
{
    init();
}

    
Well::ElasticModelComputer::ElasticModelComputer( const Well::Data& wd,
       						  const Well::Log& vel,
						  const Well::Log* den,
						  const Well::Log* svel )
    : wd_(wd)
{
    init();
    setLogs( vel, den, svel );
}


Well::ElasticModelComputer::~ElasticModelComputer()
{
    inplogs_.erase();
    uomset_.erase();
    if ( ls_ )
	delete ls_;
    if ( lsnearest_ )
	delete lsnearest_;
}


#define mPVelIdx	0
#define mDenIdx		1
#define mSVelIdx	2
#define mErrRet(msg) { errmsg_ = msg; return false; }


bool Well::ElasticModelComputer::setVelLog( const Well::Log& log )
{
    if ( inplogs_.size() != 0 )
	mErrRet( "Velocity log must be set first. Another log was set first" );

    inplogs_ += &log;

    return true;
}


bool Well::ElasticModelComputer::setDenLog( const Well::Log& log )
{
    if ( inplogs_.size() != 1 )
	mErrRet( "Velocity log must be set first." );

    inplogs_ += &log;

    return true;
}


bool Well::ElasticModelComputer::setSVelLog( const Well::Log& log )
{
    if ( inplogs_.size() != 2 )
	mErrRet( "Velocity and density logs must be set first." );

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
	inplogs_ += 0;

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


void Well::ElasticModelComputer::init()
{
    emodel_ = ElasticModel();
    zrange_ = Interval<float>( mUdf(float), mUdf(float) );
    zrgistime_ = false;
    zstep_ = mUdf(float);
    extractintime_ = false;
    ls_ = 0;
    lsnearest_ = 0;
    velpissonic_ = false;
}


bool Well::ElasticModelComputer::computeFromLogs()
{
    if ( !inplogs_.size() )
	mErrRet( "No input logs provided" )

    if ( !getLogUnits() )
	return false;

    if ( zrange_.isUdf() || mIsUdf(zstep_) )
	mErrRet( "Please set the extraction range" )

    if ( !extractLogs() )
	mErrRet( "Cannot extract logs" )

    const float convfact = SI().zDomain().isDepth() && SI().depthsInFeet()
    			 ? mFromFeetFactorF : 1.0f;

    emodel_.erase();
    const int nrsteps = ls_->nrZSamples();
    const float dz = !zrgistime_ ? zstep_ * convfact : zstep_ / 2.f;
    for ( int idl=0; idl<nrsteps; idl++ )
    {
	const float velp = getVelp( idl );
	const float den = getDensity( idl );
	const float svel = getSVel( idl );
	const float thickness = !zrgistime_ ? dz : dz * velp;

	ElasticLayer layer( thickness, velp, svel, den );
	emodel_ += layer;
    }

    if ( !emodel_.size() )
	mErrRet( "Returning empty elastic model" )

    float startdepth = zrange_.start;
    const float srddepth = -1.f * (float)SI().seismicReferenceDatum();
    if ( zrgistime_ )
    {
	const float startdah = wd_.d2TModel()->getDah( startdepth );
	startdepth = mCast(float,wd_.track().getPos( startdah ).z);
    }

    emodel_[0].thickness_ += ( startdepth - srddepth ) * convfact;

    return true;
}


bool Well::ElasticModelComputer::getLogUnits()
{
    if ( inplogs_.isEmpty() )
	mErrRet( "No logs in log set yet" )

    for ( int idx=0; idx<inplogs_.size(); idx++ )
    {
	if ( !inplogs_.validIdx(idx) )
	    mErrRet( "Error reading log in log set" );

	uomset_ += inplogs_[idx] ? inplogs_[idx]->unitOfMeasure() : 0;
    }

    velpissonic_ = uomset_[mPVelIdx] &&
       		   uomset_[mPVelIdx]->propType() == PropertyRef::Son;

    return true;
}


bool Well::ElasticModelComputer::extractLogs()
{
    if ( !wd_.d2TModel() )
	mErrRet( "Well has no valid time-depth model" )

    const float srddepth = -1.f * (float)SI().seismicReferenceDatum();
    if ( (!zrgistime_ && zrange_.start < srddepth) ||
	  (zrgistime_ && zrange_.start < 0.f) )
	mErrRet( "Extraction interval should not start above SRD" )

    const Interval<float> zrange = zrange_;

    ls_ = new Well::LogSampler( wd_.d2TModel(), &wd_.track(), zrange,
	   			zrgistime_, zstep_, true, Stats::UseAvg,
				inplogs_ );

    lsnearest_ = new Well::LogSampler( wd_.d2TModel(), &wd_.track(), zrange,
	    			       zrgistime_, zstep_, true,
				       Stats::TakeNearest, inplogs_ );

    if ( !ls_ || !lsnearest_ )
	mErrRet( "Could not initialize log sampler" );

    if ( !ls_->execute() )
	mErrRet( ls_->errMsg() )

    if ( !lsnearest_->execute() )
	mErrRet( lsnearest_->errMsg() )

    return true;
}


float Well::ElasticModelComputer::getLogVal( int logidx, int sampidx ) const
{
    const float val = ls_->getLogVal( logidx, sampidx );
    const float valintp = !lsnearest_
       			? mUdf(float)
			: lsnearest_->getLogVal( logidx, sampidx );
    return mIsUdf(val) ? valintp : val;
}


float Well::ElasticModelComputer::getVelp( int idx ) const
{
    const int logidx = mPVelIdx;
    if ( !inplogs_.validIdx(logidx) || !uomset_.validIdx(logidx) )
	return mUdf(float);

    const Well::Log* inplog = inplogs_[logidx];
    if ( !inplog )
	return mUdf(float);

    float logval = getLogVal( logidx, idx );
    const UnitOfMeasure* uom = uomset_[logidx];
    logval = uom ? uom->getSIValue( logval ) : logval;

    return velpissonic_ ? 1.f / logval : logval;
}


float Well::ElasticModelComputer::getDensity( int idx ) const
{
    const int logidx = mDenIdx;
    if ( !inplogs_.validIdx(logidx) || !uomset_.validIdx(logidx) )
	return mUdf(float);

    const Well::Log* inplog = inplogs_[logidx];
    if ( !inplog )
	return mUdf(float);

    const float logval = getLogVal( logidx, idx );
    const UnitOfMeasure* uom = uomset_[logidx];
    return uom ? uom->getSIValue( logval ) : logval;
}


float Well::ElasticModelComputer::getSVel( int idx ) const
{
    const int logidx = mSVelIdx;
    if ( !inplogs_.validIdx(logidx) || !uomset_.validIdx(logidx) )
	return mUdf(float);

    const Well::Log* inplog = inplogs_[logidx];
    if ( !inplog )
	return mUdf(float);

    const float logval = getLogVal( logidx, idx );
    const UnitOfMeasure* uom = uomset_[logidx];
    return uom ? uom->getSIValue( logval ) : logval;
}

}; //namespace Well

