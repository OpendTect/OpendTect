/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2016
________________________________________________________________________

-*/

#include "seisfetcher.h"
#include "seisprovider.h"
#include "seisioobjinfo.h"
#include "seisselection.h"
#include "dbman.h"
#include "ioobj.h"
#include "uistrings.h"


Seis::Fetcher::Fetcher( Provider& p )
    : prov_(p)
    , ioobj_(0)
{
}

Seis::Fetcher::~Fetcher()
{
    delete ioobj_;
}


void Seis::Fetcher::reset()
{
    uirv_.setEmpty();
    delete ioobj_; ioobj_ = 0;
}


IOObj* Seis::Fetcher::getIOObj() const
{
    return DBM().get( prov_.dbky_ );
}


bool Seis::Fetcher::fillIOObj()
{
    delete ioobj_;
    ioobj_ = getIOObj();
    if ( !ioobj_ )
    {
	uirv_ = uiStrings::phrCannotFindDBEntry( prov_.dbky_.toUiString() );
	return false;
    }
    return true;
}


Seis::Provider3D& Seis::Fetcher3D::prov3D()
{
    return static_cast<Provider3D&>( prov_ );
}


const Seis::Provider3D& Seis::Fetcher3D::prov3D() const
{
    return static_cast<const Provider3D&>( prov_ );
}


void Seis::Fetcher3D::reset()
{
    Fetcher::reset();

    getReqCS();

    nextbid_ = reqcs_.hsamp_.start_ - reqcs_.hsamp_.step_;
    if ( !moveNextBinID() )
	uirv_.set( tr("No traces available for current selection") );
}


TrcKeyZSampling Seis::Fetcher3D::getDefaultCS() const
{
    return TrcKeyZSampling( true );
}


void Seis::Fetcher3D::getReqCS()
{
    // set the default to everything; also sets proper steps
    SeisIOObjInfo objinf( prov_.dbky_ );
    if ( !objinf.getRanges(reqcs_) )
	reqcs_ = getDefaultCS();

    if ( prov_.seldata_ && !prov_.seldata_->isAll() )
    {
	const Seis::SelData& seldata = *prov_.seldata_;
	const Interval<float> reqzrg( seldata.zRange() );
	const Interval<int> reqinlrg( seldata.inlRange() );
	const Interval<int> reqcrlrg( seldata.crlRange() );
	reqcs_.zsamp_.start = reqzrg.start;
	reqcs_.zsamp_.stop = reqzrg.stop;
	reqcs_.hsamp_.start_.inl() = reqinlrg.start;
	reqcs_.hsamp_.stop_.inl() = reqinlrg.stop;
	reqcs_.hsamp_.start_.crl() = reqcrlrg.start;
	reqcs_.hsamp_.stop_.crl() = reqcrlrg.stop;
    }
}


bool Seis::Fetcher3D::isSelectedBinID( const BinID& bid ) const
{
    return !prov_.seldata_ || prov_.seldata_->isOK(bid);
}


bool Seis::Fetcher3D::moveNextBinID()
{
    while ( true )
    {
	if ( !reqcs_.hsamp_.toNext(nextbid_) )
	    return false;
	if ( isSelectedBinID(nextbid_) )
	    return true;
    }
}
