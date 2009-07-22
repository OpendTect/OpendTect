/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : 14-3-2008
-*/

static const char* rcsID = "$Id: bidvsetarrayadapter.cc,v 1.3 2009-07-22 16:01:32 cvsbert Exp $";

#include "bidvsetarrayadapter.h"
#include "survinfo.h"


BIDValSetArrAdapter::BIDValSetArrAdapter( const BinIDValueSet& bidvs, int colnr)
    : bidvs_( bidvs )
    , targetcolidx_( colnr )
{
    inlrg_ = bidvs.inlRange();
    crlrg_ = bidvs.crlRange();
    //TODO we use SI steps, if needed it can be replaced by a user-defined one
    const int inlsz = inlrg_.width()/SI().inlStep()+1;
    const int crlsz = crlrg_.width()/SI().crlStep()+1;
    arrinfo_ = Array2DInfoImpl( inlsz, crlsz );
}



void BIDValSetArrAdapter::set( int inlidx, int crlidx, float value )
{
    BinID bid;
    bid.inl = inlrg_.start + inlidx*SI().inlStep();
    bid.crl = crlrg_.start + crlidx*SI().crlStep();
    BinIDValueSet::Pos pos = bidvs_.findFirst( bid );
    if ( !pos.valid() || bidvs_.nrVals()<targetcolidx_ ) return;

    float* allvals = bidvs_.getVals( pos );
    allvals[targetcolidx_] = value;
    bidvs_.set( pos, allvals );
}


float BIDValSetArrAdapter::get( int inlidx, int crlidx ) const
{
    BinID bid;
    bid.inl = inlrg_.start + inlidx*SI().inlStep();
    bid.crl = crlrg_.start + crlidx*SI().crlStep();
    BinIDValueSet::Pos pos = bidvs_.findFirst( bid );
    if ( !pos.valid() || bidvs_.nrVals()<targetcolidx_ )
	return mUdf(float);

    return bidvs_.getVal( pos, targetcolidx_ );
}

