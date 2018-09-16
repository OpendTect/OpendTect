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
#include "seispreload.h"
#include "seisdatapack.h"
#include "seisselection.h"
#include "uistrings.h"
#include "posinfo2d.h"
#include "survgeom.h"


od_int64 Seis::Provider2D::getTotalNrInInput() const
{
    const int nrlines = nrLines();
    Line2DData ld;
    od_int64 ret = 0;
    for ( int iln=0; iln<nrlines; iln++ )
    {
	getGeometryInfo( iln, ld );
	ret += ld.size();
    }
    return ret;
}


namespace Seis
{

/*\brief Gets required traces from 2D lines. */

class LineFetcher : public Fetcher2D
{ mODTextTranslationClass(Seis::LineFetcher);
public:

LineFetcher( LineProvider& p )
    : Fetcher2D(p)
    , getter_(0)
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

    void		reset();
    void		findDataPack();
    bool		goTo(const TrcKey&);
    bool		getNextGetter();
    bool		readNextTraces();

    void		get(const TrcKey&,SeisTrc&);
    void		get(const TrcKey&,TraceData&,SeisTrcInfo*);
    void		getNext(SeisTrc&);

    RefMan<RegularSeisDataPack> dp_;
    Seis2DTraceGetter*	getter_;

private:

    void		doGet(const TrcKey&,SeisTrc*,TraceData&,SeisTrcInfo*);

};

} // namespace Seis


void Seis::LineFetcher::reset()
{
    Fetcher2D::reset();
    delete getter_; getter_ = 0;
    dp_ = 0;

    if ( uirv_.isOK() )
    {
	if ( !getNextGetter() )
	    uirv_.set( tr("Empty 2D Data set") );
    }
}


void Seis::LineFetcher::findDataPack()
{
    dp_ = Seis::PLDM().get<RegularSeisDataPack>( prov().dbky_, curGeomID() );
}


bool Seis::LineFetcher::goTo( const TrcKey& tk )
{
    uirv_.setEmpty();
    if ( !tk.hasValidGeomID() )
	{ uirv_.set( tr("Invalid position requested") ); return false; }

    int newlineidx = lineIdxFor( tk.geomID() );
    if ( newlineidx < 0 )
	{ uirv_.set(tr("Requested position not available")); return false; }

    if ( !getter_ || curlidx_!=newlineidx )
    {
	Seis2DTraceGetter* newgetter =
		dataset_->traceGetter( tk.geomID(), prov().seldata_, uirv_ );
	if ( !newgetter )
	{
	    if ( uirv_.isEmpty() )
		uirv_.set( tr("Requested position not available") );
	    return false;
	}

	delete getter_;
	getter_ = newgetter;
	curlidx_ = newlineidx;
    }


    if ( !iter_ || iter_->geomID()!=curGeomID() )
    {
	delete iter_;
	iter_ = new PosInfo::Line2DDataIterator( *line2ddata_[curlidx_] );
    }

    nexttrcky_ = tk;
    iter_->setTrcNr( tk.trcNr() );
    return true;
}


bool Seis::LineFetcher::getNextGetter()
{
    delete getter_; getter_ = 0;

    if ( !toNextLine() )
	return false;

    getter_ = dataset_->traceGetter( curGeomID(), prov2D().selData(), uirv_ );
    if ( !getter_ )
	return getNextGetter();

    findDataPack();
    return true;
}


void Seis::LineFetcher::get( const TrcKey& tk, SeisTrc& trc )
{
    doGet( tk, &trc, trc.data(), &trc.info() );
}


void Seis::LineFetcher::get( const TrcKey& tk, TraceData& data,
			     SeisTrcInfo* trcinfo )
{
    doGet( tk, 0, data, trcinfo );
}


void Seis::LineFetcher::doGet( const TrcKey& tk, SeisTrc* trc, TraceData& data,
			       SeisTrcInfo* trcinfo )
{
    if ( !goTo(tk) )
	return;

    if ( dp_ && dp_->sampling().hsamp_.includes(tk) )
    {
	if ( trc ) dp_->fillTrace( tk, *trc );
	else
	{
	    if ( trcinfo ) dp_->fillTraceInfo( tk, *trcinfo );
	    dp_->fillTraceData( tk, data );
	}
	return;
    }

    uirv_ = trc ? getter_->get( tk.trcNr(), *trc )
		: getter_->get( tk.trcNr(), data, trcinfo );
}


void Seis::LineFetcher::getNext( SeisTrc& trc )
{
    if ( !moveNextBinID() && (mIsSingleLine(prov().seldata_) ||
			     !getNextGetter()) )
	{ uirv_.set( uiStrings::sFinished() ); return; }

    uirv_.setEmpty();
    if ( dp_ && dp_->sampling().hsamp_.includes(nexttrcky_.binID()) )
	{ dp_->fillTrace( nexttrcky_, trc ); return; }

    if ( getter_ )
	uirv_ = getter_->get( nexttrcky_.trcNr(), trc );
    if ( !getter_ || uirv_.isError() )
	getNext( trc );
}



Seis::LineProvider::LineProvider()
    : fetcher_(*new LineFetcher(*this))
{
}


Seis::LineProvider::~LineProvider()
{
    delete &fetcher_;
}


int Seis::LineProvider::curLineIdx() const
{
    return fetcher_.curlidx_;
}


uiRetVal Seis::LineProvider::doGetComponentInfo( BufferStringSet& nms,
						 DataType& dt ) const
{
    return fetcher_.gtComponentInfo(nms,dt);
}


int Seis::LineProvider::nrLines() const
{
    return fetcher_.gtNrLines();
}


Pos::GeomID Seis::LineProvider::geomID( int iln ) const
{
    return fetcher_.gtGeomID( iln );
}


int Seis::LineProvider::lineNr( Pos::GeomID geomid ) const
{
    return fetcher_.gtLineNr( geomid );
}


BufferString Seis::LineProvider::lineName( int iln ) const
{
    return fetcher_.gtLineName( iln );
}


void Seis::LineProvider::getGeometryInfo( int iln, Line2DData& ld ) const
{
    return fetcher_.gtGeometryInfo( iln, ld );
}


bool Seis::LineProvider::getRanges( int iln, StepInterval<int>& trcrg,
	                                 ZSampling& zsamp ) const
{
    return fetcher_.gtRanges( iln, trcrg, zsamp );
}


SeisTrcTranslator* Seis::LineProvider::getCurrentTranslator() const
{
    const Seis2DTraceGetter* getter2d = fetcher_.getter_;
    if ( !getter2d )
	return 0;

    return getter2d->tr_ ? getter2d->tr_
			 : ( getter2d->ensureTranslator() ? getter2d->tr_ : 0 );
}


void Seis::LineProvider::doFillPar( IOPar& iop, uiRetVal& uirv ) const
{
    Seis::Provider2D::doFillPar( iop, uirv );

    IOPar par;
    fetcher_.doFillPar( par, uirv );
    iop.merge( par );
}


void Seis::LineProvider::doUsePar( const IOPar& iop, uiRetVal& uirv )
{
    Seis::Provider2D::doUsePar( iop, uirv );

    fetcher_.doUsePar( iop, uirv );
}


void Seis::LineProvider::doReset( uiRetVal& uirv ) const
{
    fetcher_.reset();
    uirv = fetcher_.uirv_;
}


TrcKey Seis::LineProvider::doGetCurPosition() const
{
    return TrcKey( curGeomID(), fetcher_.iter_->trcNr() );
}


bool Seis::LineProvider::doGoTo( const TrcKey& tk )
{
    return fetcher_.goTo( tk );
}


void Seis::LineProvider::doGetNext( SeisTrc& trc, uiRetVal& uirv ) const
{
    fetcher_.getNext( trc );
    uirv = fetcher_.uirv_;
}


void Seis::LineProvider::doGet( const TrcKey& trcky, SeisTrc& trc,
				  uiRetVal& uirv ) const
{
    fetcher_.get( trcky, trc );
    uirv = fetcher_.uirv_;
}


void Seis::LineProvider::doGetData( const TrcKey& trcky, TraceData& data,
				    SeisTrcInfo* trcinfo, uiRetVal& uirv ) const
{
    fetcher_.get( trcky, data, trcinfo );
    uirv = fetcher_.uirv_;
}
