/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisproviderimpl.h"
#include "linesubsel.h"
#include "seisfetcher.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seistrc.h"
#include "seisdatapack.h"
#include "seisrangeseldata.h"
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
    const RegularSeisDataPack& dp() const { return regSeisDP(); }

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


void Seis::LineFetcher::getTrc( TraceData& td, SeisTrcInfo& ti )
{
    if ( useDP(curb2d_) )
	fillFromDP( curb2d_, ti, td );
    else if ( getter_->translator()->supportsGoTo() )
	uirv_ = getter_->get( curb2d_.trcNr(), td, &ti );
    else
    {
	uirv_ = getter_->getNext( td, ti );
	if ( uirv_.isOK() )
	    curb2d_ = ti.bin2D();
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
	const auto lidx = prov2D().lineIdx( geomid );
	Seis::RangeSelData rsd( geomid );
	rsd.zSubSel().limitTo( prov_.zSubSel(lidx) );
	getter_ = dataset_->traceGetter( geomid, &rsd, uirv_ );
	handleGeomIDChange( lidx );
    }

    if ( !getter_ && uirv_.isOK() )
	uirv_.set( uiStrings::phrCannotLoad( geomid.name() ) );

    return uirv_.isOK();
}


#include "seisproviderimpldefs.h"
//mDefNonPSProvFns( 2D, Line )
mDefProvStdFns( 2D, Line )

void Seis::LineProvider::gtTrc( TraceData& td, SeisTrcInfo& ti,
				uiRetVal& uirv ) const
{
    if ( !fetcher_.setPosition(trcpos_) )
	uirv.set( uiStrings::phrUnexpected(uiStrings::sPosition(),
					   trcpos_.usrDispStr()) );
    else
    {
	fetcher_.getTrc( td, ti );
	uirv = fetcher_.uirv_;
    }
}
