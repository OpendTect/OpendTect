/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2008
-*/


#include "seiscubeposprovider.h"
#include "seisprovider.h"
#include "survinfo.h"
#include "iopar.h"



Pos::SeisCubeProvider3D::SeisCubeProvider3D()
    : zsamp_(SI().zRange(false))
    , curzidx_(0)
{
    reset();
}


Pos::SeisCubeProvider3D::SeisCubeProvider3D( const SeisCubeProvider3D& oth )
{
    *this = oth;
}


Pos::SeisCubeProvider3D::~SeisCubeProvider3D()
{
}


Pos::SeisCubeProvider3D& Pos::SeisCubeProvider3D::operator =(
					const SeisCubeProvider3D& oth )
{
    if ( &oth != this )
    {
	cubedata_ = p.cubedata_;
	zsamp_ = oth.zsamp_;
	curpos_ = p.curpos_;
	curzidx_ = p.curzidx_;
    }
    return *this;
}


void Pos::SeisCubeProvider3D::setZSampling( const ZSampling& zrg )
{
    zsamp_ = zrg;
}


void Pos::SeisCubeProvider3D::reset()
{
    curbid_ = BinID( tkzs_.hsamp_.start_.inl(),
		     tkzs_.hsamp_.start_.crl()-tkzs_.hsamp_.step_.crl() );
    curzidx_ = zsamp_.nrSteps();
}


bool Pos::SeisCubeProvider3D::toNextPos()
{
    curbid_.crl() += tkzs_.hsamp_.step_.crl();
    if ( curbid_.crl() > tkzs_.hsamp_.stop_.crl() )
    {
	curbid_.inl() += tkzs_.hsamp_.step_.inl();
	if ( curbid_.inl() > tkzs_.hsamp_.stop_.inl() )
	    return false;
	curbid_.crl() = tkzs_.hsamp_.start_.crl();
    }

    curzidx_ = 0;
    return true;
}


#define mZrgEps (1e-6*tkzs_.zsamp_.step)

bool Pos::SeisCubeProvider3D::toNextZ()
{
    curzidx_++;
    if ( curzidx_>=zsampsz_ )
	return toNextPos();

    return true;
}


float Pos::SeisCubeProvider3D::curZ() const
{
    if ( curzidx_<0 || curzidx_>=zsampsz_ )
	return mUdf(float);

    return tkzs_.zsamp_.atIndex( curzidx_ );
}


bool Pos::SeisCubeProvider3D::includes( const BinID& bid, float z ) const
{
    bool issel = tkzs_.hsamp_.includes(bid);
    if ( !issel ) return false;
    if ( mIsUdf(z) ) return true;

    return z < tkzs_.zsamp_.stop+mZrgEps && z > tkzs_.zsamp_.start - mZrgEps;
}


void Pos::SeisCubeProvider3D::usePar( const IOPar& iop )
{
    tkzs_.usePar( iop );
    zsampsz_ = tkzs_.zsamp_.nrSteps()+1;
}


void Pos::SeisCubeProvider3D::fillPar( IOPar& iop ) const
{
    tkzs_.fillPar( iop );
}


void Pos::SeisCubeProvider3D::getSummary( BufferString& txt ) const
{
    txt.set( tkzs_.hsamp_.start_.toString() ).add( "-" );
    txt.add( tkzs_.hsamp_.stop_.toString() ); // needs to be a separate line
    const int nrsamps = zsampsz_;
    if ( nrsamps > 1 )
	txt.add( " (" ).add( nrsamps ).add( " samples)" );
}


void Pos::SeisCubeProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = tkzs_.hsamp_.start_; stop = tkzs_.hsamp_.stop_;
}


void Pos::SeisCubeProvider3D::getZRange( Interval<float>& zrg ) const
{
    assign( zrg, tkzs_.zsamp_ );
    mDynamicCastGet(StepInterval<float>*,szrg,&zrg)
    if ( szrg )
	szrg->step = tkzs_.zsamp_.step;
}


od_int64 Pos::SeisCubeProvider3D::estNrPos() const
{
    return tkzs_.hsamp_.totalNr();
}


int Pos::SeisCubeProvider3D::estNrZPerPos() const
{
    return zsampsz_;
}


void Pos::SeisCubeProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, sKey::Range() );
}
