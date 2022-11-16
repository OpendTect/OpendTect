/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisposprovider.h"
#include "seistrctr.h"
#include "seispacketinfo.h"
#include "conn.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "uistrings.h"



Pos::SeisProvider3D::SeisProvider3D()
    : zsamp_(0.f,0.f,1.f)
    , curzidx_(-1)
{
    reset();
}


Pos::SeisProvider3D::SeisProvider3D( const SeisProvider3D& oth )
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
    {
	cubedata_ = oth.cubedata_;
	zsamp_ = oth.zsamp_;
	curpos_ = oth.curpos_;
	curzidx_ = oth.curzidx_;
    }
    return *this;
}


uiRetVal Pos::SeisProvider3D::setSeisID( const MultiID& dbky )
{
    uiRetVal uirv = uiRetVal::OK();
    if ( dbky == id_ )
	return uirv;

    PtrMan<IOObj> ioobj = IOM().get( dbky );
    if ( !ioobj )
    {
	uirv.set( tr("Cannot find selected cube for positions in database") );
	return uirv;
    }
    PtrMan<SeisTrcTranslator> trl
		    = (SeisTrcTranslator*)ioobj->createTranslator();
    if ( !trl )
    {
	uirv.set( tr("Cannot create reader for position cube") );
	return uirv;
    }

    Conn* conn = ioobj->getConn( true );
    if ( !conn )
    {
	uirv.set( tr("Cannot open cube for positions") );
	return uirv;
    }

    if ( !trl->initRead(conn,Seis::Scan) )
    {
	uirv.set( trl->errMsg() );
	return uirv;
    }

    const PosInfo::CubeData* cd = trl->packetInfo().cubedata;
    if ( !cd )
    {
	uirv.set( tr("Cube for positions does not contain required info") );
	return uirv;
    }

    cubedata_ = *cd;
    id_ = dbky;
    zsamp_ = trl->packetInfo().zrg;
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
    return curpos_.isValid() ? cubedata_.binID( curpos_ ) : BinID::udf();
}


float Pos::SeisProvider3D::curZ() const
{
    return zsamp_.atIndex( curzidx_ );
}


bool Pos::SeisProvider3D::includes( const BinID& bid, float z ) const
{
    if ( !cubedata_.includes( bid.inl(), bid.crl() ) )
	return false;
    if ( mIsUdf(z) )
	return true;

    const float eps = 1e-6f * zsamp_.step;
    return z < zsamp_.stop+eps && z > zsamp_.start-eps;
}



#define mGetSP3DKey(ky) IOPar::compKey(sKeyType(),ky)

void Pos::SeisProvider3D::usePar( const IOPar& iop )
{
    MultiID dbky( id_ ); ZSampling zsamp( zsamp_ );
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


void Pos::SeisProvider3D::getSummary( BufferString& txt ) const
{
    if ( id_.isUdf() || id_.isUdf() )
	txt.set( "<No selection>" );
    else
	txt.set( "=> " ).add( IOM().nameOf(id_) );
    const int nrsamps = nrSamples();
    if ( nrsamps > 1 )
	txt.add( " (" ).add( nrsamps ).add( " samples)" );
}


void Pos::SeisProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    StepInterval<int> inlrg, crlrg;
    cubedata_.getInlRange( inlrg );
    cubedata_.getCrlRange( crlrg );
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
    Pos::Provider3D::factory().addCreator( create, sKeyType() );
}
