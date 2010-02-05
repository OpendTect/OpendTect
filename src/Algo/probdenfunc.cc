/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2010
-*/

static const char* rcsID = "$Id: probdenfunc.cc,v 1.5 2010-02-05 12:08:49 cvsnanne Exp $";

// Sampled:
// 1D currently does polynomial interpolation
// 2D currently does bi-linear interpolation
// ND currently does no interpolation at all, returns nearest
 

#include "sampledprobdenfunc.h"
#include "interpol1d.h"
#include "interpol2d.h"
#include "iopar.h"
#include "keystrs.h"
#include <math.h>

const char* ProbDenFunc::sKeyNrDim()	{ return "Nr dimensions"; }

void ProbDenFunc::fillPar( IOPar& par ) const
{
    par.set( sKey::Type, getTypeStr() );
    const int nrdim = nrDims();
    par.set( sKeyNrDim(), nrdim );
    for ( int idx=0; idx<nrdim; idx++ )
	par.set( IOPar::compKey(sKey::Name,idx), dimName(idx) );
}


ProbDenFunc1D& ProbDenFunc1D::operator =( const ProbDenFunc1D& pd )
{
    if ( this != & pd )
	varnm_ = pd.varnm_;
    return *this;
}


ProbDenFunc2D& ProbDenFunc2D::operator =( const ProbDenFunc2D& pd )
{
    if ( this != & pd )
    {
	dim0nm_ = pd.dim0nm_;
	dim1nm_ = pd.dim1nm_;
    }
    return *this;
}


const char* ProbDenFunc2D::dimName( int idim ) const
{
    const BufferString* bs = idim < 1 ? &dim0nm_ : &dim1nm_;
    if ( bs->isEmpty() )
    {
	static BufferString ret;
	ret = "Dim"; ret += idim ? "1" : "0";
	bs = &ret;
    }
    return bs->buf();
}


void ArrayNDProbDenFunc::fillPar( IOPar& par ) const
{
    const int nrdim = getData().info().getNDim();
    for ( int idx=0; idx<nrdim; idx++ )
    {
	par.set( IOPar::compKey(sKey::Size,idx), size(idx) );
	par.set( IOPar::compKey(sKey::Sampling,idx), sampling(idx) );
    }
}


void ArrayNDProbDenFunc::dump( std::ostream& strm ) const
{
    // TODO binary
    const ArrayND<float>& array = getData();
    const od_int64 totalsz = array.info().getTotalSz();
    const float* values = array.getData();
    if ( values )
    {
	for ( od_int64 idx=0; idx<totalsz; idx++ )
	    strm << values[idx] << '\n';
    }
}


bool ArrayNDProbDenFunc::obtain( std::istream& strm )
{
    // TODO binary

    ArrayND<float>& array = getData();
    const od_int64 totalsz = array.info().getTotalSz();
    if ( totalsz < 1 )
	return false;

    float* values = array.getData();
    if ( values )
    {
	for ( od_int64 idx=0; idx<totalsz; idx++ )
	    strm >> values[idx];
    }

    return true;
}

// 1D

SampledProbDenFunc1D::SampledProbDenFunc1D( const Array1D<float>& a1d )
    : ProbDenFunc1D("")
    , sd_(0,1)
    , bins_(a1d)
{
}


SampledProbDenFunc1D::SampledProbDenFunc1D( const TypeSet<float>& vals )
    : ProbDenFunc1D("")
    , sd_(0,1)
    , bins_(vals.size())
{
    for ( int idx=0; idx<vals.size(); idx++ )
	bins_.set( idx, vals[idx] );
}


SampledProbDenFunc1D::SampledProbDenFunc1D( const float* vals, int sz )
    : ProbDenFunc1D("")
    , sd_(0,1)
    , bins_(sz)
{
    for ( int idx=0; idx<sz; idx++ )
	bins_.set( idx, vals[idx] );
}


SampledProbDenFunc1D::SampledProbDenFunc1D( const SampledProbDenFunc1D& spdf )
    : ProbDenFunc1D(spdf.varnm_)
    , sd_(spdf.sd_)
    , bins_(spdf.bins_)
{
}


SampledProbDenFunc1D& SampledProbDenFunc1D::operator =(
					const SampledProbDenFunc1D& spdf )
{
    if ( this != &spdf )
    {
	ProbDenFunc1D::operator =( spdf );
	sd_ = spdf.sd_;
	bins_ = spdf.bins_;
    }
    return *this;
}


float SampledProbDenFunc1D::value( float pos ) const
{
    const int sz = size( 0 );
    if ( sz < 1 ) return 0;

    const float fidx = sd_.getIndex( pos );
    const int idx = (int)floor(fidx);
    if ( idx < -1 || idx > sz-1 )
	return 0;

    float v[4];
    v[0] = idx < 1	? 0 : bins_.get( idx-1 );
    v[1] = idx < 0	? 0 : bins_.get( idx );
    v[2] = idx > sz-2	? 0 : bins_.get( idx + 1 );
    v[3] = idx > sz-3	? 0 : bins_.get( idx + 2 );

    const float val = Interpolate::PolyReg1D<float>(v).apply( fidx - idx );
    return val < 0 ? 0 : val;
}


void SampledProbDenFunc1D::fillPar( IOPar& par ) const
{
    ProbDenFunc::fillPar( par );
    ArrayNDProbDenFunc::fillPar( par );
}


bool SampledProbDenFunc1D::usePar( const IOPar& par )
{
    int sz = -1;
    par.get( IOPar::compKey(sKey::Size,0), sz );
    bins_.setSize( sz );

    par.get( IOPar::compKey(sKey::Sampling,0), sd_ );
    par.get( IOPar::compKey(sKey::Name,0), varnm_ );
    return sz>0;
}


void SampledProbDenFunc1D::dump( std::ostream& strm ) const
{ ArrayNDProbDenFunc::dump( strm ); }

bool SampledProbDenFunc1D::obtain( std::istream& strm )
{ return ArrayNDProbDenFunc::obtain( strm ); }


// 2D

SampledProbDenFunc2D::SampledProbDenFunc2D( const Array2D<float>& a2d )
    : ProbDenFunc2D("","")
    , sd0_(0,1)
    , sd1_(0,1)
    , bins_(a2d)
{
}


SampledProbDenFunc2D::SampledProbDenFunc2D( const SampledProbDenFunc2D& spdf )
    : ProbDenFunc2D(spdf.dim0nm_,spdf.dim1nm_)
    , sd0_(spdf.sd0_)
    , sd1_(spdf.sd1_)
    , bins_(spdf.bins_)
{
}


SampledProbDenFunc2D& SampledProbDenFunc2D::operator =(
					const SampledProbDenFunc2D& spdf )
{
    if ( this != &spdf )
    {
	ProbDenFunc2D::operator =( spdf );
	sd0_ = spdf.sd0_;
	sd1_ = spdf.sd1_;
	bins_ = spdf.bins_;
    }
    return *this;
}


float SampledProbDenFunc2D::value( float px, float py ) const
{
    const int szx = size( 0 ); const int szy = size( 1 );
    if ( szx < 1 || szy < 1 ) return 0;

    const float fidxx = sd0_.getIndex( px );
    const float fidxy = sd1_.getIndex( py );
    const int idxx = (int)floor(fidxx); const int idxy = (int)floor(fidxy);
    if ( idxx < -1 || idxx > szx-1 || idxy < -1 || idxy > szy-1 )
	return 0;

    float v[4];
    v[0] = idxx < 0 || idxy < 0		? 0 : bins_.get( idxx, idxy );
    v[1] = idxy > szy-2 || idxx < 0	? 0 : bins_.get( idxx, idxy+1 );
    v[2] = idxx > szx-2 || idxy < 0	? 0 : bins_.get( idxx+1, idxy );
    v[3] = idxx > szx-2 || idxy > szy-2	? 0 : bins_.get( idxx+1, idxy+1 );

    const float val = Interpolate::LinearReg2D<float>(v)
			.apply( fidxx - idxx, fidxy - idxy );
    return val < 0 ? 0 : val;
}


void SampledProbDenFunc2D::fillPar( IOPar& par ) const
{
    ProbDenFunc::fillPar( par );
    ArrayNDProbDenFunc::fillPar( par );
}


bool SampledProbDenFunc2D::usePar( const IOPar& par )
{
    int sz0 = -1; int sz1 = -1;
    par.get( IOPar::compKey(sKey::Size,0), sz0 );
    par.get( IOPar::compKey(sKey::Size,1), sz1 );
    bins_.setSize( sz0, sz1 );

    par.get( IOPar::compKey(sKey::Sampling,0), sd0_ );
    par.get( IOPar::compKey(sKey::Sampling,1), sd1_ );

    par.get( IOPar::compKey(sKey::Name,0), dim0nm_ );
    par.get( IOPar::compKey(sKey::Name,1), dim1nm_ );

    return sz0>0 && sz1>0;
}


void SampledProbDenFunc2D::dump( std::ostream& strm ) const
{ ArrayNDProbDenFunc::dump( strm ); }

bool SampledProbDenFunc2D::obtain( std::istream& strm )
{ return ArrayNDProbDenFunc::obtain( strm ); }


// ND


SampledProbDenFuncND& SampledProbDenFuncND::operator =(
					const SampledProbDenFuncND& spdf )
{
    if ( this != &spdf )
    {
	sds_ = spdf.sds_;
	bins_ = spdf.bins_;
	dimnms_ = spdf.dimnms_;
    }
    return *this;
}


const char* SampledProbDenFuncND::dimName( int dim ) const
{
    if ( dim >= 0 && dim < dimnms_.size() )
	return dimnms_.get( dim ).buf();

    static BufferString ret; ret = "Dim";
    ret += dim;
    return ret.buf();
}


float SampledProbDenFuncND::value( const TypeSet<float>& vals ) const
{
    if ( vals.size() < sds_.size() )
	return 0;

    TypeSet<int> idxs;
    for ( int idx=0; idx<sds_.size(); idx++ )
    {
	const int sdidx = sds_[idx].nearestIndex( vals[idx] );
	if ( sdidx < 0 || sdidx > size(idx) )
	    return 0;

	idxs += sdidx;
    }

    return bins_.getND( idxs.arr() );
}
