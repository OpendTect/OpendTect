/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara Rao
 Date:		March 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: stratamp.cc,v 1.6 2009-07-22 16:01:27 cvsbert Exp $";

#include "stratamp.h"

#include "cubesampling.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "ioobj.h"
#include "seisread.h"
#include "statruncalc.h"
#include "seisselectionimpl.h"
#include "seistrc.h"


StratAmpCalc::StratAmpCalc( const IOObj& seisobj, const EM::Horizon3D* tophor,
			    const EM::Horizon3D* bothor,
			    Stats::Type stattyp, const HorSampling& hs )
    : Executor("Computing Stratal amplitude...")
    , rdr_(0) 
    , tophorizon_(tophor)
    , bothorizon_(bothor)
    , stattyp_(stattyp)
    , dataidx_(-1)
    , nrdone_(0)
{
    rdr_ = new SeisTrcReader( &seisobj );
    CubeSampling cs;
    cs.hrg = hs;
    rdr_->setSelData( new Seis::RangeSelData(cs) );
    rdr_->prepareWork();

    totnr_ = hs.nrInl() * hs.nrCrl();

    if ( tophor ) tophor->ref();
    if ( bothor ) bothor->ref();
}


StratAmpCalc::~StratAmpCalc()
{
    delete rdr_;
    if ( tophorizon_ ) tophorizon_->unRef();
    if ( bothorizon_ ) bothorizon_->unRef();
}


int StratAmpCalc::init( const char* attribnm, bool addtotop )
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


int StratAmpCalc::nextStep()
{
    if ( !rdr_ || !tophorizon_ || dataidx_<0 )
	return Executor::ErrorOccurred();

    SeisTrc trc;
    const int rv = rdr_->get( trc.info() );
    if ( rv == 0 ) return Executor::Finished();
    else if ( rv == -1 ) return Executor::ErrorOccurred();

    const BinID bid = trc.info().binid;
    if ( !rdr_->get(trc) ) return Executor::ErrorOccurred();

    const EM::SubID subid = bid.getSerialized();
    float z1 = tophorizon_->getPos(tophorizon_->sectionID(0),subid).z;
    float z2 = !bothorizon_ ? z1
		     : bothorizon_->getPos(bothorizon_->sectionID(0),subid).z;
    z1 += tophorshift_;
    z2 += bothorshift_;
    Interval<int> sampintv( trc.info().nearestSample(z1),
	    		    trc.info().nearestSample(z2) );

    if ( sampintv.start < 0 )
	sampintv.start = 0;
    if ( sampintv.stop >= trc.size() )
	sampintv.stop = trc.size()-1;

    Stats::RunCalcSetup rcsetup;
    rcsetup.require( stattyp_ );
    Stats::RunCalc<float> runcalc( rcsetup );
    for ( int idx=sampintv.start; idx<=sampintv.stop; idx++ )
    {
	const float val = trc.get( idx, 0 );
	if ( !mIsUdf(val) )
	    runcalc.addValue( val );
    }

    float outval = mUdf( float );
    switch ( stattyp_ )
    {
	case Stats::Min: outval = runcalc.min(); break;
	case Stats::Max: outval = runcalc.max(); break;
	case Stats::Average: outval = runcalc.average(); break;
	case Stats::RMS: outval = runcalc.rms(); break;  
	case Stats::Sum: outval = runcalc.sum(); break;  
	default: break;
    }

    const EM::Horizon3D* addtohor_ = addtotop_ ? tophorizon_ : bothorizon_;
    posid_.setSubID( subid );
    addtohor_->auxdata.setAuxDataVal( dataidx_, posid_, outval );
    nrdone_++;
    return Executor::MoreToDo();
}
