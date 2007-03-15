/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackgather.cc,v 1.1 2007-03-15 17:28:52 cvskris Exp $";

#include "prestackgather.h"

#include "genericnumer.h"
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

const char* Gather::sKeyIsAngeGather()	{ return "Angle Gather"; }
const char* Gather::sKeyIsNMO()		{ return "Is NMO Corrected"; }
const char* Gather::sKeyVelocityCubeID()	{ return "Velocity volume"; }
const char* Gather::sKeyZisTime()		{ return "Z Is Time"; }

Gather::Gather()
    : offsetisangle_( false )
    , iscorr_( false )
    , binid_( -1, -1 )
    , data_( 1, 1 )
{}


Gather::~Gather()
{}


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
    data_.setSize( offsets_.size(), nrsamples );
    zsampling_ = tbuf2.get(0)->info().sampling;

    if ( offsets_.size()>1 )
	sort_coupled( offsets_.arr(), traceorder_.arr(), traceorder_.size() );

    for ( int idy=0; idy<nrsamples; idy++ )
    {
	const float z = zsampling_.atIndex(idy);

	for ( int idx=0; idx<traceorder_.size(); idx++ )
	    data_.set(idx, idy, tbuf2.get(traceorder_[idx])->getValue( z, 0 ) );
    }

    offsetisangle_ = false;
    ioobj->pars().getYN(sKeyIsAngeGather(), offsetisangle_ );

    iscorr_ = false;
    ioobj->pars().getYN(sKeyIsNMO(), iscorr_ );

    zit_ = SI().zIsTime();
    ioobj->pars().getYN(sKeyZisTime(),zit_);

    ioobj->pars().get(sKeyVelocityCubeID(), velocitymid_ );
    
    binid_ = bid;

    return true;
}


bool Gather::getVelocityID(const MultiID& stor, MultiID& vid )
{
    PtrMan<IOObj> ioobj = IOM().get( stor );
    return ioobj ? ioobj->pars().get( sKeyVelocityCubeID(), vid ) : false;
}

