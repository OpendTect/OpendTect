/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblocksdataglueer.h"
#include "seisstorer.h"
#include "arrayndimpl.h"
#include "survinfo.h"
#include "uistrings.h"


namespace Seis
{
namespace Blocks
{

class LineBuf
{
public:

    mUseType( DataGlueer,	Arr2D );
    mUseType( DataGlueer,	Arr3D );
    mUseType( DataGlueer,	idx_type );
    mUseType( DataGlueer,	pos_type );

			~LineBuf()
			{ deepErase(tiles_); deepErase(blocks_); }

idx_type idxOf( pos_type pos ) const
{
    for ( auto idx=0; idx<posns_.size(); idx++ )
	if ( posns_[idx] == pos )
	    return idx;
    return -1;
}

    TypeSet<pos_type>	posns_;
    ObjectSet<Arr2D>	tiles_;
    ObjectSet<Arr3D>	blocks_;

};

}
}


Seis::Blocks::DataGlueer::DataGlueer( Storer& strr )
    : storer_(strr)
    , zstep_(SI().zStep())
{
}


Seis::Blocks::DataGlueer::~DataGlueer()
{
    if ( !linebufs_.isEmpty() )
    {
	pErrMsg("Call finish() before destruction and check uiRetVal");
	auto uirv = finish();
	if ( !uirv.isOK() )
	    ErrMsg( uirv );
    }
    delete arrinfo_;
}


bool Seis::Blocks::DataGlueer::is2D() const
{
    return arrinfo_ ? arrinfo_->nrDims() == 2 : false;
}


void Seis::Blocks::DataGlueer::initGeometry( const ArrayNDInfo& inf )
{
    arrinfo_ = inf.clone();
}


uiRetVal Seis::Blocks::DataGlueer::addData( const Bin2D& b2d, z_type midz,
					    const Arr2D& arr )
{
    if ( !arrinfo_ )
	initGeometry( arr.info() );
    if ( arrinfo_->nrDims() != 2 )
	return mINTERNAL( "Requiring 2D tiles" );
    if ( !arrinfo_->isEqual(arr.info()) )
	return mINTERNAL( "Incompatible tile passed" );

    if ( curb2d_ == b2d )
	mergeZ( arr, midz );
    else
	addPos( b2d, arr, midz );

    curb2d_ = b2d;
    return storeFinished();
}


uiRetVal Seis::Blocks::DataGlueer::addData( const BinID& bid, z_type midz,
					    const Arr3D& arr )
{
    if ( !arrinfo_ )
	initGeometry( arr.info() );
    if ( arrinfo_->nrDims() != 3 )
	return mINTERNAL( "Requiring 3D cubelets" );
    if ( !arrinfo_->isEqual(arr.info()) )
	return mINTERNAL( "Incompatible cubelet passed" );

    if ( curbid_ == bid )
	mergeZ( arr, midz );
    else
	addPos( bid, arr, midz );

    curbid_ = bid;
    return storeFinished();
}


void Seis::Blocks::DataGlueer::mergeZ( const Arr2D& arr, z_type midz )
{
}


void Seis::Blocks::DataGlueer::mergeZ( const Arr3D& arr, z_type midz )
{
}


void Seis::Blocks::DataGlueer::addPos( const Bin2D& b2d,
					const Arr2D& arr, z_type midz )
{
}


void Seis::Blocks::DataGlueer::addPos( const BinID& bid,
					const Arr3D& arr, z_type midz )
{
}


uiRetVal Seis::Blocks::DataGlueer::storeFinished()
{
    return uiRetVal::OK();
}


uiRetVal Seis::Blocks::DataGlueer::finish()
{
    curbid_ = BinID::udf();
    mSetUdf( curb2d_.trcNr() );
    return storeFinished();
}
