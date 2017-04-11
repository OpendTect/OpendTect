/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblocksdata.h"
#include "databuf.h"
#include "scaler.h"
#include "datainterp.h"
#include "odmemory.h"

static const unsigned short cDefDim = 80;

Seis::Blocks::Dimensions Seis::Blocks::Data::defDims()
{
    return Dimensions( cDefDim, cDefDim, cDefDim );
}


Seis::Blocks::Data::Data( GlobIdx gidx, Dimensions dims, OD::FPDataRepType fpr )
    : globidx_(gidx)
    , dims_(dims)
    , scaler_(0)
    , totsz_((((int)dims.inl())*dims.crl()) * dims.z())
    , dbuf_(*new DataBuffer(0))
{
    const DataCharacteristics dc( fpr );
    interp_ = DataInterpreter<float>::create( dc, true );
    const int bytesperval = (int)dc.nrBytes();
    dbuf_.reByte( bytesperval, false );
    dbuf_.reSize( totsz_ );
}


Seis::Blocks::Data::~Data()
{
    delete interp_;
    delete scaler_;
    delete &dbuf_;
}


int Seis::Blocks::Data::getBufIdx( const SampIdx& sidx ) const
{
    const int nrsampsoninl = ((int)sidx.crl()) * dims_.z() + sidx.z();
    return sidx.inl() ? sidx.inl()*nrSampsPerInl() + nrsampsoninl
		      : nrsampsoninl;
}


float Seis::Blocks::Data::value( const SampIdx& sidx ) const
{
    return !scaler_ ? interp_->get( dbuf_.data(), getBufIdx(sidx) )
	: (float)scaler_->scale( interp_->get(dbuf_.data(),getBufIdx(sidx)) );
}


void Seis::Blocks::Data::getVert( SampIdx sampidx, float* vals,
				  int arrsz ) const
{
    const int startbufidx = getBufIdx( sampidx );
    sampidx.z() = (IdxType)dims_.z();
    const int stopbufidx = getBufIdx( sampidx );

    if ( interp_->isF32() && !scaler_ )
    {
	int nr2copy = stopbufidx - startbufidx + 1;
	if ( nr2copy > arrsz )
	    nr2copy = arrsz;
	OD::sysMemCopy( vals, dbuf_.data() + sizeof(float) * startbufidx,
		        nr2copy * sizeof(float) );
    }
    else
    {
	int arridx = 0;
	for ( int bufidx=startbufidx; bufidx<=stopbufidx; bufidx++ )
	{
	    if ( arridx >= arrsz )
		break;
	    if ( !scaler_ )
		vals[arridx] = interp_->get( dbuf_.data(), bufidx );
	    else
		vals[arridx] = (float)scaler_->scale(
				    interp_->get( dbuf_.data(), bufidx ) );
	    arridx++;
	}
    }
}


void Seis::Blocks::Data::setValue( const SampIdx& sidx, float val )
{
    if ( !scaler_ )
	interp_->put( dbuf_.data(), getBufIdx(sidx), val );
    else
	interp_->put( dbuf_.data(), getBufIdx(sidx),
			    (float)scaler_->unScale(val) );
}


void Seis::Blocks::Data::setVert( SampIdx sampidx, const float* vals,
				  int arrsz )
{
    const int startbufidx = getBufIdx( sampidx );
    sampidx.z() = (IdxType)dims_.z();
    const int stopbufidx = getBufIdx( sampidx );

    if ( interp_->isF32() && !scaler_ )
    {
	int nr2copy = stopbufidx - startbufidx + 1;
	if ( nr2copy > arrsz )
	    nr2copy = arrsz;
	OD::sysMemCopy( dbuf_.data() + sizeof(float) * startbufidx,
		        vals, nr2copy * sizeof(float) );
    }
    else
    {
	int arridx = 0;
	for ( int bufidx=startbufidx; bufidx<=stopbufidx; bufidx++ )
	{
	    if ( arridx >= arrsz )
		break;
	    if ( !scaler_ )
		interp_->put( dbuf_.data(), bufidx, vals[arridx] );
	    else
		interp_->put( dbuf_.data(), bufidx,
			      (float)scaler_->unScale( vals[arridx] ) );
	    arridx++;
	}
    }
}
