/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : 14-3-2008
-*/


#include "bidvsetarrayadapter.h"
#include "survinfo.h"


BIDValSetArrAdapter::BIDValSetArrAdapter( const BinIDValueSet& bidvs, int colnr,
					  const BinID& step )
    : bidvs_( bidvs )
    , targetcolidx_( colnr )
{
    tks_.setInlRange( bidvs.inlRange() );
    tks_.setCrlRange( bidvs.crlRange() );
    tks_.step_ = step;
    arrinfo_ = Array2DInfoImpl( tks_.nrInl(), tks_.nrCrl() );
}


void BIDValSetArrAdapter::set( int inlidx, int crlidx, float value )
{
    BinID bid = tks_.atIndex( inlidx, crlidx );
    BinIDValueSet::SPos pos = bidvs_.find( bid );
    if ( !pos.isValid() || bidvs_.nrVals()<targetcolidx_ ) return;

    BinIDValueSet& ncset = const_cast<BinIDValueSet&>( bidvs_ );
    float* allvals = ncset.getVals( pos );
    allvals[targetcolidx_] = value;
    ncset.set( pos, allvals );
}


float BIDValSetArrAdapter::get( int inlidx, int crlidx ) const
{
    BinID bid = tks_.atIndex( inlidx, crlidx );
    BinIDValueSet::SPos pos = bidvs_.find( bid );
    if ( !pos.isValid() || bidvs_.nrVals()<targetcolidx_ )
	return mUdf(float);

    return bidvs_.getVal( pos, targetcolidx_ );
}
