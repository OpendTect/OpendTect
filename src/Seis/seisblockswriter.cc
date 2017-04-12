/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblockswriter.h"
#include "seisblocksdata.h"
#include "survgeom3d.h"
#include "scaler.h"

static const unsigned short cHdrSz = 128;

Seis::Blocks::DataStorage::DataStorage( const Survey::Geometry3D* geom )
    : survgeom_(*(geom ? geom : static_cast<const Survey::Geometry3D*>(
				    &Survey::Geometry::default3D())))
    , dims_(Data::defDims())
{
}


Seis::Blocks::IdxType Seis::Blocks::DataStorage::idx4Inl( int inl ) const
{
    return (IdxType)(survgeom_.idx4Inl( inl ) / dims_.inl());
}


Seis::Blocks::IdxType Seis::Blocks::DataStorage::idx4Crl( int crl ) const
{
    return (IdxType)(survgeom_.idx4Crl( crl ) / dims_.crl());
}


Seis::Blocks::IdxType Seis::Blocks::DataStorage::idx4Z( float z ) const
{
    return (IdxType)(survgeom_.idx4Z( z ) / dims_.z());
}


Seis::Blocks::GlobIdx Seis::Blocks::DataStorage::getGlobIdx( const BinID& bid,
							float z ) const
{
    return GlobIdx( idx4Inl(bid.inl()), idx4Crl(bid.crl()), idx4Z(z) );
}


Seis::Blocks::Writer::Writer( const Survey::Geometry3D* geom )
    : DataStorage(geom)
    , scaler_(0)
    , component_(0)
    , needreset_(true)
{
}


Seis::Blocks::Writer::~Writer()
{
    delete scaler_;
}


void Seis::Blocks::Writer::setBasePath( const File::Path& fp )
{
    if ( fp != basepath_ )
    {
	basepath_ = fp;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setCubeName( const char* nm )
{
    if ( cubename_ != nm )
    {
	cubename_ = nm;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setComponent( int icomp )
{
    if ( component_ != icomp )
    {
	component_ = icomp;
	needreset_ = true;
    }
}


uiRetVal Seis::Blocks::Writer::reset()
{
    uiRetVal uirv;
    return uirv;
}


uiRetVal Seis::Blocks::Writer::add( const SeisTrc& trc )
{
    uiRetVal uirv;
    if ( needreset_ )
    {
	uirv = reset();
	if ( uirv.isError() )
	    return uirv;
    }

    GlobIdx globidx = getGlobIdx( trc.info().binID(), trc.startPos() );
    const IdxType stopzidx = idx4Z( trc.endPos() );

	globidx.z() = stopzidx;

    return uirv;
}
