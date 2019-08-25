/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2017
-*/


#include "seisposprovider.h"
#include "seisprovider.h"
#include "iopar.h"
#include "keystrs.h"
#include "uistrings.h"



Pos::SeisProvider3D::SeisProvider3D()
    : Pos::Provider3D()
    , zsamp_(0.f,0.f,1.f)
    , curzidx_(-1)
{
    reset();
}


Pos::SeisProvider3D::SeisProvider3D( const SeisProvider3D& oth )
    : Pos::Provider3D(oth)
{
    *this = oth;
}


Pos::SeisProvider3D::~SeisProvider3D()
{
}


Pos::SeisProvider3D& Pos::SeisProvider3D::operator =(
					const SeisProvider3D& oth )
{
    if ( &oth != this )
	return *this;

    Pos::Provider3D::operator = ( oth );

    cubedata_ = oth.cubedata_;
    zsamp_ = oth.zsamp_;
    curpos_ = oth.curpos_;
    curzidx_ = oth.curzidx_;

    return *this;
}


uiRetVal Pos::SeisProvider3D::setSeisID( const DBKey& dbky )
{
    uiRetVal uirv;
    if ( dbky == id_ )
	return uirv;

    PtrMan<Seis::Provider> prov = Seis::Provider::create( dbky, &uirv );
    if ( !prov )
	return uirv;
    if ( prov->is2D() )
    {
	uirv.set( uiStrings::phrSelectObjectWrongType( toUiString("2D") ) );
	return uirv;
    }

    cubedata_ = prov.ptr()->as3D()->possibleCubeData();
    id_ = dbky;
    zsamp_ = prov->zRange();
    reset();
    return uirv;
}


void Pos::SeisProvider3D::reset()
{
    curpos_.toPreStart();
    curzidx_ = zsamp_.nrSteps();
}


bool Pos::SeisProvider3D::toNextPos()
{
    curzidx_ = 0;
    return cubedata_.toNext( curpos_ );
}


bool Pos::SeisProvider3D::toNextZ()
{
    curzidx_++;
    if ( curzidx_ >= nrSamples() )
	return toNextPos();

    return true;
}


BinID Pos::SeisProvider3D::curBinID() const
{
    return curpos_.isValid() ? cubedata_.binID( curpos_ ) : BinID(0,0);
}


float Pos::SeisProvider3D::curZ() const
{
    return zsamp_.atIndex( curzidx_ );
}


bool Pos::SeisProvider3D::includes( const BinID& bid, float z ) const
{
    if ( !cubedata_.includes(bid) )
	return false;
    if ( mIsUdf(z) )
	return true;

    const float eps = 1e-6f * zsamp_.step;
    return z < zsamp_.stop+eps && z > zsamp_.start-eps;
}



#define mGetSP3DKey(ky) IOPar::compKey(sKeyType(),ky)

void Pos::SeisProvider3D::usePar( const IOPar& iop )
{
    DBKey dbky( id_ ); ZSampling zsamp( zsamp_ );
    iop.get( mGetSP3DKey(sKey::ID()), dbky );
    const bool havezsamp = iop.get( mGetSP3DKey(sKey::ZRange()), zsamp );
    setSeisID( dbky );
    if ( havezsamp )
	zsamp_ = zsamp;
}


void Pos::SeisProvider3D::fillPar( IOPar& iop ) const
{
    iop.set( mGetSP3DKey(sKey::ID()), id_ );
    iop.set( mGetSP3DKey(sKey::ZRange()), zsamp_ );
}


void Pos::SeisProvider3D::getSummary( uiString& txt ) const
{
    if ( id_.isInvalid() )
	txt = tr("No selection").embedFinalState();
    else
	txt = toUiString( "=> %1" ).arg( id_.name() );
    const int nrsamps = nrSamples();
    if ( nrsamps > 1 )
	txt.appendPhrase( toUiString("%1 %2").arg( nrsamps )
	   .arg(uiStrings::sSample(mPlural).toLower()).parenthesize() );
}


void Pos::SeisProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    Interval<int> inlrg, crlrg;
    cubedata_.getRanges( inlrg, crlrg );
    start.inl() = inlrg.start; start.crl() = crlrg.start;
    stop.inl() = inlrg.stop; stop.crl() = crlrg.stop;
}


void Pos::SeisProvider3D::getZRange( Interval<float>& zrg ) const
{
    assign( zrg, zsamp_ );
    mDynamicCastGet(StepInterval<float>*,szrg,&zrg)
    if ( szrg )
	szrg->step = zsamp_.step;
}


od_int64 Pos::SeisProvider3D::estNrPos() const
{
    return cubedata_.totalSize();
}


int Pos::SeisProvider3D::estNrZPerPos() const
{
    return nrSamples();
}


void Pos::SeisProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, sKeyType(), dispType() );
}
