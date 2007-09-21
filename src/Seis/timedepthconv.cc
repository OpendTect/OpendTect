/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : September 2007
-*/

static const char* rcsID = "$Id: timedepthconv.cc,v 1.1 2007-09-21 20:47:48 cvskris Exp $";

#include "timedepthconv.h"

#include "arraynd.h"
#include "cubesampling.h"
#include "datapackbase.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "seisread.h"
#include "survinfo.h"

const char* Time2DepthStretcher::sName() 	{ return "Time2Depth"; }

void Time2DepthStretcher::initClass()
{ ZATF().addCreator( create, sName() ); }


ZAxisTransform* Time2DepthStretcher::create()
{ return new Time2DepthStretcher;}


Time2DepthStretcher::Time2DepthStretcher()
    : velreader_( 0 )
    , flatdp_( 0 )
    , cubedp_( 0 )
{
    voidata_.allowNull( true );
}


Time2DepthStretcher::~Time2DepthStretcher()
{ releaseData(); }


bool Time2DepthStretcher::setVelData( const MultiID& mid )
{
    releaseData();
    PtrMan<IOObj> velioobj = IOM().get( mid );
    if ( !velioobj )
	return false;

    velreader_ = new SeisTrcReader( velioobj );
    if ( !velreader_->prepareWork() )
    {
	releaseData();
	return false;
    }

    veldesc_.type_ = VelocityDesc::Interval;
    veldesc_.samplerange_ = VelocityDesc::Centered;

    veldesc_.usePar ( velioobj->pars() );

    BufferString depthdomain = SI().zIsTime() ? sKey::Time : sKey::Depth;
    velioobj->pars().get( sKey::ZDomain, depthdomain );

    if ( depthdomain==sKey::Time )
	velintime_ = true;
    else if ( depthdomain==sKey::Depth )
	velintime_ = false;
    else
    {
	releaseData();
	return false;
    }


    //TODO: Reload eventual VOIs
    return true;
}


bool Time2DepthStretcher::setVelData( DataPack::ID id,const VelocityDesc& vd,
				      const char* depthkey )
{
    releaseData();

    DataPack* dp = DPM(DataPackMgr::FlatID).obtain( id, false );
    if ( dp )
	flatdp_ = (FlatDataPack*) dp;
    if ( !dp )
	cubedp_ = (CubeDataPack*) DPM(DataPackMgr::CubeID).obtain( id, false );

    if ( !flatdp_ && !cubedp_ )
	return false;

    BufferString depthdomain = SI().zIsTime() ? sKey::Time : sKey::Depth;
    if ( depthkey )
	depthdomain = depthkey;

    if ( depthdomain==sKey::Time )
	velintime_ = true;
    else if ( depthdomain==sKey::Depth )
	velintime_ = false;
    else
    {
	releaseData();
	return false;
    }

    veldesc_ = vd;

    //TODO: Recomp
    return true;
}


bool Time2DepthStretcher::isOK() const
{ return veldesc_.type_==VelocityDesc::Interval; }


int Time2DepthStretcher::addVolumeOfInterest(const CubeSampling& cs,
					     bool depth )
{
    int id = 0;
    while ( voiids_.indexOf(id)!=-1 ) id++;

    voidata_ += 0;
    voivols_ += cs;
    voidepth_ += depth;
    voiids_ += id;

    return id;
}


void Time2DepthStretcher::transform(const BinID&,const SamplingData<float>&,
			   int,float*) const
{}


void Time2DepthStretcher::transformBack(const BinID&,const SamplingData<float>&,
			   int,float*) const
{}


Interval<float> Time2DepthStretcher::getZInterval(bool from) const
{
    return Interval<float>( 0, 1 ); //TODO implement something real
}


void Time2DepthStretcher::releaseData()
{
    delete velreader_;
    velreader_ = 0;

    if ( flatdp_ ) DPM(DataPackMgr::FlatID).release( flatdp_->id() );
    if ( cubedp_ ) DPM(DataPackMgr::CubeID).release( cubedp_->id() );

    flatdp_ = 0;
    cubedp_ = 0;

    for ( int idx=0; idx<voidata_.size(); idx++ )
    {
	delete voidata_[idx];
	voidata_.replace( idx, 0 );
    }
}

    

