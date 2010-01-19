/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2010
-*/
 
static const char* rcsID = "$Id: probdenfunc.cc,v 1.2 2010-01-19 12:06:40 cvsbert Exp $";


#include "sampledprobdenfunc.h"
#include "interpol1d.h"
#include "interpol2d.h"


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
    if ( fidx < -0.5 || fidx > sz-0.5 )
	return 0;

    float v[4];
    const int idx = (int)fidx;
    v[0] = bins_.get( idx < 2 ? 0 : idx-1 );
    v[1] = bins_.get( idx );
    v[2] = idx < sz-1 ? bins_.get( idx + 1 ) : v[1];
    v[3] = idx < sz-2 ? bins_.get( idx + 2 ) : v[2];

    return Interpolate::PolyReg1D<float>(v).apply( fidx - idx );
}


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
    if ( fidxx < -0.5 || fidxx > szx-0.5 || fidxy < -0.5 || fidxy > szy-0.5 )
	return 0;

    const int idxx = (int)fidxx; const int idxy = (int)fidxy;
    const float v00 = bins_.get( idxx, idxy );
    const float v01 = idxy < szy-1 ? bins_.get( idxx, idxy+1 ) : v00;
    const float v10 = idxx < szx-1 ? bins_.get( idxx+1, idxy ) : v00;
    const float v11 = idxy < szy-1 && idxx < szx-1 ? bins_.get( idxx+1, idxy+1 )
		: (v01+v10) * .5;

    return Interpolate::LinearReg2D<float>(v00,v01,v10,v11)
		.apply( fidxx - idxx, fidxy - idxy );
}


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
	idxs += sds_[idx].nearestIndex( vals[idx] );

    return bins_.getND( idxs.arr() );
}
