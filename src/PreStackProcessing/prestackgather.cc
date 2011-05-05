/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackgather.cc,v 1.29 2011-05-05 13:35:14 cvsbruno Exp $";

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
#include "seistrctr.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "keystrs.h"

using namespace PreStack;

const char* Gather::sDataPackCategory()		{ return "Pre-Stack Gather"; }
const char* Gather::sKeyIsAngleGather()		{ return "Angle Gather"; }
const char* Gather::sKeyIsCorr()		{ return "Is Corrected"; }
const char* Gather::sKeyVelocityCubeID()	{ return "Velocity volume"; }
const char* Gather::sKeyZisTime()		{ return "Z Is Time"; }
const char* Gather::sKeyPostStackDataID()	{ return "Post Stack Data"; }
const char* Gather::sKeyStaticsID()		{ return "Statics"; }

Gather::Gather()
    : FlatDataPack( sDataPackCategory(), new Array2DImpl<float>(0,0) )
    , offsetisangle_( false )
    , iscorr_( false )
    , binid_( -1, -1 )
    , coord_( 0, 0 )
    , zit_( SI().zIsTime() )
{}



Gather::Gather( const Gather& gather )
    : FlatDataPack( gather )
    , offsetisangle_( gather.offsetisangle_ )
    , iscorr_( gather.iscorr_ )
    , binid_( gather.binid_ )
    , coord_( gather.coord_ )
    , zit_( gather.zit_ )
    , azimuths_( gather.azimuths_ )
    , velocitymid_( gather.velocitymid_ )
    , storagemid_( gather.storagemid_ )
    , linename_( gather.linename_ )
    , staticsmid_( gather.staticsmid_ )
{}


bool Gather::setSize( int nroff, int nrz )
{
    return true;
}


Gather::~Gather()
{}


bool Gather::readFrom( const MultiID& mid, const BinID& bid, int comp,
		       BufferString* errmsg )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
    {
	if ( errmsg ) (*errmsg) = "No valid gather selected.";
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    return readFrom( *ioobj, bid, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, const BinID& bid, int comp,
		       BufferString* errmsg )
{
    PtrMan<SeisPSReader> rdr = SPSIOPF().get3DReader( ioobj, bid.inl );
    if ( !rdr )
    {
	if ( errmsg )
	    (*errmsg) = "This Pre-Stack data store cannot be handeled.";
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    linename_.setEmpty();
    return readFrom( ioobj, *rdr, bid, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, const int tracenr, 
		       const char* linename, int comp, BufferString* errmsg )
{
    PtrMan<SeisPSReader> rdr = SPSIOPF().get2DReader( ioobj, linename );
    if ( !rdr )
    {
	if ( errmsg )
	    (*errmsg) = "This Pre-Stack data store cannot be handeled.";
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    linename_ = linename;

    return readFrom( ioobj, *rdr, BinID(0,tracenr), comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, SeisPSReader& rdr, const BinID& bid, 
		       int comp, BufferString* errmsg )
{
    PtrMan<SeisTrcBuf> tbuf = new SeisTrcBuf( true );
    if ( !rdr.getGather(bid,*tbuf) )
    {
	if ( errmsg ) (*errmsg) = rdr.errMsg();
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    tbuf->sort( true, SeisTrcInfo::Offset );

    bool isset = false;
    StepInterval<double> zrg;
    Coord crd( coord_ );
    for ( int idx=tbuf->size()-1; idx>-1; idx-- )
    {
	const SeisTrc* trc = tbuf->get( idx );
	if ( mIsUdf( trc->info().offset ) )
	    delete tbuf->remove( idx );

	const int trcsz = trc->size();
	if ( !isset )
	{
	    isset = true;
	    zrg.setFrom( trc->info().sampling.interval( trcsz ) );
	    crd = trc->info().coord;
	}
	else
	{
	    zrg.start = mMIN( trc->info().sampling.start, zrg.start );
	    zrg.stop = mMAX( trc->info().sampling.atIndex(trcsz-1), zrg.stop );
	    zrg.step = mMIN( trc->info().sampling.step, zrg.step );
	}
    }

    if ( !isset )
    {
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    int nrsamples = zrg.nrSteps()+1;
    if ( zrg.atIndex(nrsamples-1)<zrg.stop )
	nrsamples++;

    const Array2DInfoImpl newinfo( tbuf->size(), nrsamples );
    if ( arr2d_ && !arr2d_->setInfo( newinfo ) )
    {
	delete arr2d_;
	arr2d_ = new Array2DImpl<float>( newinfo );
	if ( !arr2d_ || !arr2d_->isOK() )
	{
	    delete arr2d_; arr2d_ = 0;
	    return false;
	}
    }

    azimuths_.setSize( tbuf->size(), mUdf(float) );

    for ( int trcidx=tbuf->size()-1; trcidx>=0; trcidx-- )
    {
	const SeisTrc* trc = tbuf->get( trcidx );
	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    const float val = trc->getValue( zrg.atIndex( idx ), comp );
	    if ( arr2d_ ) arr2d_->set( trcidx, idx, val );
	}

	azimuths_[trcidx] = trc->info().azimuth;
    }

    double offset;
    float* offsets = tbuf->getHdrVals(SeisTrcInfo::Offset, offset);
    if ( !offsets )
	return false;
    posData().setX1Pos( offsets, tbuf->size(), offset );
    posData().setRange( false, zrg );

    offsetisangle_ = false;
    ioobj.pars().getYN(sKeyIsAngleGather(), offsetisangle_ );

    iscorr_ = false;
    if ( !ioobj.pars().getYN(sKeyIsCorr(), iscorr_ ) )
	ioobj.pars().getYN( "Is NMO Corrected", iscorr_ );

    zit_ = SI().zIsTime();
    ioobj.pars().getYN(sKeyZisTime(),zit_);

    velocitymid_.setEmpty();
    ioobj.pars().get( sKeyVelocityCubeID(), velocitymid_ );
    staticsmid_.setEmpty();
    ioobj.pars().get( sKeyStaticsID(), staticsmid_ );
    
    binid_ = bid;
    coord_ = crd;
    setName( ioobj.name() );

    storagemid_ = ioobj.key();

    return true;
}


const char* Gather::dimName( bool dim0 ) const
{ 
    return dim0 ? sKey::Offset : (SI().zIsTime() ? sKey::Time : sKey::Depth);
}


void Gather::getAuxInfo( int idim0, int idim1, IOPar& par ) const
{
    par.set( "X", coord_.x );
    par.set( "Y", coord_.y );
    float z = posData().position( false, idim1 );
    if ( zit_ ) z *= 1000;
    par.set( "Z", z );
    par.set( sKey::Offset, getOffset(idim0) );
    par.set( sKey::Azimuth, getAzimuth(idim0) );
    if ( !is3D() )
	par.set( sKey::TraceNr, binid_.crl );
    else
    {
	BufferString str( 128, false ); binid_.fill( str.buf() );
	par.set( sKey::Position, str );
    }
}



const char* Gather::getSeis2DName() const
{
    return linename_.isEmpty() ? 0 : linename_.buf();
}


float Gather::getOffset( int idx ) const
{ return posData().position( true, idx ); }


float Gather::getAzimuth( int idx ) const
{
    return azimuths_[idx];
}


OffsetAzimuth Gather::getOffsetAzimuth( int idx ) const
{
    return OffsetAzimuth( posData().position( true, idx ), 
	    		  azimuths_[idx] );
}


bool Gather::getVelocityID(const MultiID& stor, MultiID& vid )
{
    PtrMan<IOObj> ioobj = IOM().get( stor );
    return ioobj ? ioobj->pars().get( sKeyVelocityCubeID(), vid ) : false;
}

#define mIfNonZero \
const float val = data().get( offset, idz ); \
if ( val && !mIsUdf(val) )

void Gather::detectInnerMutes( int* res, int taperlen ) const
{
    const int nroffsets = size( !offsetDim() );
    const int nrz = size( offsetDim() );

    int muteend = 0;
    for ( int offset=0; offset<nroffsets; offset++ )
    {
	for ( int idz=nrz-1; idz>=muteend; idz-- )
	{
	    mIfNonZero
		muteend = idz+1;
	}

	res[offset] = muteend-taperlen;
    }
}


void Gather::detectOuterMutes( int* res, int taperlen ) const
{
    const int nroffsets = size( !offsetDim() );
    const int nrz = size( offsetDim() );

    int muteend = nrz-1;
    for ( int offset=nroffsets-1; offset>=0; offset-- )
    {
	for ( int idz=0; idz<=muteend; idz++ )
	{
	    mIfNonZero
		muteend = idz-1;
	}

	res[offset] = muteend + taperlen;
    }
}
