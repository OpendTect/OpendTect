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
#include "survgeom.h"


namespace Seis
{

class LineFetcher : public Fetcher2D
{
public:

			LineFetcher( LineProvider& p )
			    : Fetcher2D(p)	    {}

			~LineFetcher()		    { delete getter_; }

    bool		isPS() const		    { return false; }
    void		prepWork() override;
    const STTrl*	curTransl() const override;

    bool		setPosition(const Bin2D&) override;
    void		getTrc(TraceData&,SeisTrcInfo&);

protected:

    Seis2DTraceGetter*	getter_		= nullptr;
    Bin2D		dpbin2d_;

    bool		ensureRightDataSource(GeomID);
    const RegularSeisDataPack& dp() const
		{ return *static_cast<const RegularSeisDataPack*>(dp_.ptr()); }

};

} // namespace Seis


void Seis::LineFetcher::prepWork()
{
    uirv_.setOK();
    // let's not do anything until we know the geomid
}


bool Seis::LineFetcher::setPosition( const Bin2D& b2d )
{
    curb2d_.trcNr() = -1;
    if ( !ensureRightDataSource(b2d.geomID()) )
	return false;

    curb2d_ = b2d;
    return true;
}


void Seis::LineFetcher::getTrc( TraceData& data, SeisTrcInfo& trcinfo )
{
    if ( !haveDP()
      || !dp().sampling().hsamp_.includes(BinID(curb2d_.idxPair())) )
	uirv_ = getter_->get( curb2d_.trcNr(), data, &trcinfo );
    else
    {
	const TrcKey tk( curb2d_ );
	dp().fillTraceInfo( tk, trcinfo );
	dp().fillTraceData( tk, data );
    }
}


const SeisTrcTranslator* Seis::LineFetcher::curTransl() const
{
    if ( !getter_ )
	return nullptr;

    getter_->ensureTranslator();
    return getter_->translator();
}


bool Seis::LineFetcher::ensureRightDataSource( GeomID geomid )
{
    if ( getter_ && getter_->geomID() == geomid )
	return true;

    deleteAndZeroPtr( getter_ );
    ensureDataSet();
    if ( dataset_ )
    {
	getter_ = dataset_->traceGetter( geomid, nullptr, uirv_ );
	ensureDPIfAvailable( prov2D().lineIdx(geomid) );
    }

    if ( !getter_ && uirv_.isOK() )
	uirv_.set( uiStrings::phrCannotLoad( geomid.name() ) );

    return uirv_.isOK();
}


mDefNonPSProvFns( 2D, Line )
