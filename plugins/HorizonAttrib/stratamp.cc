/*
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Nageswara*
 *DATE     : Mar 2008
-*/

#include "stratamp.h"

#include "emhorizon3d.h"
#include "ioobj.h"
#include "seisselectionimpl.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seistrc.h"
#include "statruncalc.h"

CalcStratAmp::CalcStratAmp(const IOObj* seisobj, const EM::Horizon3D* tophor,
			   const EM::Horizon3D* bothor,
			   Stats::Type stattyp, const HorSampling& hs)
    : Executor("Computing Stratal amplitude...")
    , rdr_(0) 
    , tophorizon_(tophor)
    , bothorizon_(bothor)
    , usesinglehor_(true)
    , stattyp_(stattyp)
    , dataidx_(-1)
{
    if ( !seisobj ) return;

    rdr_= new SeisTrcReader( seisobj );
    CubeSampling cs;
    cs.hrg = hs;
    Seis::RangeSelData* sd = new Seis::RangeSelData( cs );
    rdr_->setSelData( sd );
    rdr_->prepareWork();

    totnr_ = hs.nrInl() * hs.nrCrl();
}


CalcStratAmp::~CalcStratAmp()
{
    delete rdr_;
    if ( tophorizon_ ) tophorizon_->unRef();
    if ( bothorizon_ ) bothorizon_->unRef();
}


int CalcStratAmp::totalNr() const
{
    
    return totnr_ < 0 ? -1 : totnr_;
}


int CalcStratAmp::init( const char* attribnm, bool addtotop )
{
    addtotop_ = addtotop;
    const EM::Horizon3D* addtohor_ = addtotop ? tophorizon_ : bothorizon_;
    if ( !addtohor_ ) return -1;

    dataidx_ = addtohor_->auxdata.auxDataIndex( attribnm );
    if ( dataidx_ < 0 ) dataidx_ = addtohor_->auxdata.addAuxData( attribnm );
    posid_.setObjectID( addtohor_->id() );
    posid_.setSectionID( addtohor_->sectionID(0) );
    return dataidx_;
}


int CalcStratAmp::nextStep()
{
    if ( !rdr_ || !tophorizon_ || ( !usesinglehor_ && !bothorizon_ ) )
	return Executor::ErrorOccurred;

    if ( dataidx_ < 0 ) return Executor::ErrorOccurred;

    SeisTrc trc;
    const int rv = rdr_->get( trc.info() );
    if ( rv == 0 ) return Executor::Finished;
    else if ( rv == -1 ) return Executor::ErrorOccurred;

    const BinID bid = trc.info().binid;
    if ( !rdr_->get(trc) ) return Executor::ErrorOccurred;

    const EM::SubID subid = bid.getSerialized();
    float z1 = tophorizon_->getPos(tophorizon_->sectionID(0),subid).z;
    float z2 = usesinglehor_ ? z1
		     : bothorizon_->getPos(bothorizon_->sectionID(0),subid).z;
    z1 += tophorshift_;
    z2 += bothorshift_;
    const bool isrev = z1 > z2;
    Interval<int> sampintv( trc.info().nearestSample( isrev ? z2 : z1 ),
	    		    trc.info().nearestSample( isrev ? z1 : z2 ) );

    if ( sampintv.start < 0 || sampintv.stop >= trc.info().nr )
	return Executor::MoreToDo;

    Stats::RunCalcSetup rcsetup;
    rcsetup.require( stattyp_ );
    Stats::RunCalc<float> runcalc( rcsetup );
    for ( int idx=sampintv.start; idx<=sampintv.stop; idx++ )
    {
	 const float val = trc.get( idx, 0 );
	 if ( mIsUdf(val) ) continue;

	 runcalc.addValue( val );
    }

    float outval;
    switch( stattyp_ )
    {
	case Stats::Min: outval = runcalc.min(); break;

	case Stats::Max: outval = runcalc.max(); break;

	case Stats::Average: outval = runcalc.average(); break;
	    
	case Stats::RMS: outval = runcalc.rms(); break;  
      
	default: break;
    }

    const EM::Horizon3D* addtohor_ = addtotop_ ? tophorizon_ : bothorizon_;
    posid_.setSubID( subid );
    addtohor_->auxdata.setAuxDataVal( dataidx_, posid_, outval );

    return Executor::MoreToDo;
}
