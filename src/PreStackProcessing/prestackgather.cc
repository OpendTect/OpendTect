/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackgather.cc,v 1.3 2007-05-09 21:34:53 cvskris Exp $";

#include "prestackgather.h"

#include "genericnumer.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "iopar.h"
#include "ioman.h"
#include "property.h"
#include "ptrman.h"
#include "samplfunc.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seisread.h"
#include "seistrcsel.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "unitofmeasure.h"

using namespace PreStack;

const char* Gather::sKeyIsAngleGather()	{ return "Angle Gather"; }
const char* Gather::sKeyIsNMO()		{ return "Is NMO Corrected"; }
const char* Gather::sKeyVelocityCubeID()	{ return "Velocity volume"; }
const char* Gather::sKeyZisTime()		{ return "Z Is Time"; }

Gather::Gather()
    : FlatDataPack( "Pre-Stack Gather" )
    , offsetisangle_( false )
    , iscorr_( false )
    , binid_( -1, -1 )
{}


Gather::~Gather()
{}


const char* Gather::dimName( bool dim0 ) const
{
    if ( dim0 )
	return "Offset";
    
    return zit_ ? "Time" : "Depth";
}


bool Gather::readFrom( const MultiID& mid, const BinID& bid, 
			   BufferString* errmsg )
{
    offsets_.erase();
    azimuths_.erase();
    traceorder_.erase();

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
    {
	if ( errmsg ) (*errmsg) = "No valid gather selected.";
	return false;
    }

    PtrMan<SeisPSReader> rdr = SPSIOPF().getReader( *ioobj, bid.inl );
    if ( !rdr )
    {
	if ( errmsg )
	    (*errmsg) = "This Pre-Stack data store cannot be handeled.";
	return false;
    }

    SeisTrcBuf tbuf;
    if ( !rdr->getGather(bid,tbuf) )
    {
	if ( errmsg ) (*errmsg) = rdr->errMsg();
	return false;
    }

    SeisTrcBuf tbuf2;
    while ( tbuf.size() )
    {
	SeisTrc* trc = tbuf.remove( 0 );
	const float offset = trc->info().offset;
	if ( mIsUdf(offset) )
	{
	    delete trc;
	    continue;
	}

	offsets_ += offset;
	azimuths_ += trc->info().azimuth;
	traceorder_ += traceorder_.size();
	tbuf2.add( trc );
    }

    if ( tbuf2.isEmpty() )
	return false;

    const int nrsamples = tbuf2.get(0)->size();
    if ( arr2d_ )
	delete arr2d_;

    arr2d_ = new Array2DImpl<float>( offsets_.size(), nrsamples );

    if ( offsets_.size()>1 )
	sort_coupled( offsets_.arr(), traceorder_.arr(), traceorder_.size() );

    float* offsetarr = new float[offsets_.size()];
    memcpy( offsetarr, offsets_.arr(), offsets_.size()*sizeof(float) );
    posdata_.setX1Pos( offsetarr, offsets_.size(), 0 );
    const StepInterval<float> zsd =
	tbuf2.get(0)->info().sampling.interval( nrsamples );
    posdata_.setRange( false,StepInterval<double>(zsd.start,zsd.stop,zsd.step));


    for ( int idy=0; idy<nrsamples; idy++ )
    {
	const float z = posdata_.range( false ).atIndex( idy );

	for ( int idx=0; idx<traceorder_.size(); idx++ )
	    arr2d_->set(idx, idy, tbuf2.get(traceorder_[idx])->getValue(z,0) );
    }

    offsetisangle_ = false;
    ioobj->pars().getYN(sKeyIsAngleGather(), offsetisangle_ );

    iscorr_ = false;
    ioobj->pars().getYN(sKeyIsNMO(), iscorr_ );

    zit_ = SI().zIsTime();
    ioobj->pars().getYN(sKeyZisTime(),zit_);

    ioobj->pars().get(sKeyVelocityCubeID(), velocitymid_ );
    
    binid_ = bid;
    setName( ioobj->name() );

    return true;
}


const StepInterval<double>& Gather::zSampling() const
{
    return posdata_.range( false );
}


bool Gather::getVelocityID(const MultiID& stor, MultiID& vid )
{
    PtrMan<IOObj> ioobj = IOM().get( stor );
    return ioobj ? ioobj->pars().get( sKeyVelocityCubeID(), vid ) : false;
}

