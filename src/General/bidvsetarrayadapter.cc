/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : 14-3-2008
-*/

static const char* rcsID = "$Id: bidvsetarrayadapter.cc,v 1.4 2010-11-23 06:13:43 cvsnageswara Exp $";

#include "bidvsetarrayadapter.h"
#include "survinfo.h"


BIDValSetArrAdapter::BIDValSetArrAdapter( const BinIDValueSet& bidvs, int colnr,
					  const BinID& step )
    : bidvs_( bidvs )
    , targetcolidx_( colnr )
{
    hrg_.setInlRange( bidvs.inlRange() );
    hrg_.setCrlRange( bidvs.crlRange() );
    hrg_.step = step;
    arrinfo_ = Array2DInfoImpl( hrg_.nrInl(), hrg_.nrCrl() );
}



void BIDValSetArrAdapter::set( int inlidx, int crlidx, float value )
{
    BinID bid = hrg_.atIndex( inlidx, crlidx );
    BinIDValueSet::Pos pos = bidvs_.findFirst( bid );
    if ( !pos.valid() || bidvs_.nrVals()<targetcolidx_ ) return;

    float* allvals = bidvs_.getVals( pos );
    allvals[targetcolidx_] = value;
    bidvs_.set( pos, allvals );
}


float BIDValSetArrAdapter::get( int inlidx, int crlidx ) const
{
    BinID bid = hrg_.atIndex( inlidx, crlidx );
    BinIDValueSet::Pos pos = bidvs_.findFirst( bid );
    if ( !pos.valid() || bidvs_.nrVals()<targetcolidx_ )
	return mUdf(float);

    return bidvs_.getVal( pos, targetcolidx_ );
}
