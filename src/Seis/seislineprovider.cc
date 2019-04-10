/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seislineprovider.h"
#include "seisfetcher.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seistrc.h"
#include "seisdatapack.h"
#include "seisseldata.h"
#include "uistrings.h"
#include "posinfo2d.h"
#include "survgeom.h"


namespace Seis
{

/*\brief Gets required traces from 2D lines. */

class LineFetcher : public Fetcher2D
{ mODTextTranslationClass(Seis::LineFetcher);
public:

LineFetcher( LineProvider& p )
    : Fetcher2D(p)
{
}

~LineFetcher()
{
    delete getter_;
}

LineProvider& prov()
{
    return static_cast<LineProvider&>( prov_ );
}

const LineProvider& prov() const
{
    return static_cast<const LineProvider&>( prov_ );
}

const RegularSeisDataPack& dp() const
{
    return *static_cast<const RegularSeisDataPack*>( dp_.ptr() );
}

    void		prepWork();
    bool		goTo(GeomID,trcnr_type);
    void		getAt(GeomID,trcnr_type,TraceData&,SeisTrcInfo&);
    void		getCur(TraceData&,SeisTrcInfo&);

    Seis2DTraceGetter*	getter_		= nullptr;

protected:

    bool		ensureGetter();

};

} // namespace Seis


void Seis::LineFetcher::prepWork()
{
    deleteAndZeroPtr( getter_ );
}


bool Seis::LineFetcher::goTo( GeomID gid, trcnr_type tnr )
{
    uirv_.setEmpty();

    const auto oldlidx = prov2D().curLineIdx();
    if ( !selectPosition(gid,tnr) )
	return false;

    if ( oldlidx != prov2D().curLineIdx() )
	deleteAndZeroPtr( getter_ );
    return true;
}


bool Seis::LineFetcher::ensureGetter()
{
    if ( getter_ )
	return true;

    ensureDataSet();
    const auto geomid = prov().curGeomID();
    if ( dataset_ )
	getter_ = dataset_->traceGetter( geomid, prov().selData(), uirv_ );
    if ( !getter_ && uirv_.isOK() )
	uirv_.set( uiStrings::phrCannotLoad( nameOf(geomid) ) );

    getDataPack();
    return uirv_.isOK();
}


void Seis::LineFetcher::getAt( GeomID gid, trcnr_type tnr, TraceData& data,
			       SeisTrcInfo& trcinfo )
{
    if ( goTo(gid,tnr) )
	getCur( data, trcinfo );
}


void Seis::LineFetcher::getCur( TraceData& data, SeisTrcInfo& trcinfo )
{

    const GeomID gid = prov().curGeomID();
    const auto tnr = prov().curTrcNr();
    const TrcKey tk( gid, tnr );

    if ( !dp_ || !dp().sampling().hsamp_.includes(tk) )
    {
	if ( ensureGetter() )
	    uirv_ = getter_->get( tnr, data, &trcinfo );
    }
    else
    {
	dp().fillTraceInfo( tk, trcinfo );
	dp().fillTraceData( tk, data );
    }
}



Seis::LineProvider::LineProvider()
    : fetcher_(*new LineFetcher(*this))
{
}


Seis::LineProvider::~LineProvider()
{
    delete &fetcher_;
}


Seis::Fetcher2D& Seis::LineProvider::fetcher() const
{
    return mNonConst( fetcher_ );
}


void Seis::LineProvider::prepWork( uiRetVal& uirv ) const
{
    fetcher_.prepWork();
    uirv = fetcher_.uirv_;
}


bool Seis::LineProvider::doGoTo( GeomID gid, trcnr_type tnr,
				 uiRetVal* uirv ) const
{
    if ( fetcher_.goTo(gid,tnr) )
	return true;

    if ( uirv )
	*uirv = fetcher_.uirv_;
    return false;
}


void Seis::LineProvider::gtCur( SeisTrc& trc, uiRetVal& uirv ) const
{
    fetcher_.getCur( trc.data(), trc.info() );
    uirv = fetcher_.uirv_;
}


void Seis::LineProvider::gtAt( GeomID gid, trcnr_type tnr, TraceData& td,
				  SeisTrcInfo& ti, uiRetVal& uirv ) const
{
    fetcher_.getAt( gid, tnr, td, ti );
    uirv = fetcher_.uirv_;
}


void Seis::LineProvider::gtComponentInfo( BufferStringSet& nms,
					  DataType& dt ) const
{
    fetcher_.getComponentInfo( nms, dt );
}


const SeisTrcTranslator* Seis::LineProvider::curTransl() const
{
    const Seis2DTraceGetter* getter2d = fetcher_.getter_;
    if ( !getter2d )
	return 0;

    getter2d->ensureTranslator();
    return getter2d->tr_;
}
