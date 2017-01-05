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
#include "seisselection.h"
#include "uistrings.h"
#include "posinfo2d.h"
#include "survgeom.h"


od_int64 Seis::Provider2D::getTotalNrInInput() const
{
    const int nrlines = nrLines();
    PosInfo::Line2DData ld;
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
    bool		getNextGetter();
    bool		readNextTraces();

    void		get(const TrcKey&,SeisTrc&);
    void		getNext(SeisTrc&);

    Seis2DTraceGetter*	getter_;

};

} // namespace Seis


void Seis::LineFetcher::reset()
{
    Fetcher2D::reset();
    delete getter_; getter_ = 0;

    openDataSet();
    if ( uirv_.isOK() )
    {
	if ( !getNextGetter() )
	    uirv_.set( tr("Empty 2D Data set") );
    }
}


bool Seis::LineFetcher::getNextGetter()
{
    delete getter_; getter_ = 0;

    if ( !toNextLine() )
	return false;

    getter_ = dataset_->traceGetter( curGeomID(), prov2D().selData(), uirv_ );
    return getter_ ? true : getNextGetter();
}


void Seis::LineFetcher::get( const TrcKey& tk, SeisTrc& trc )
{
    if ( !tk.hasValidGeomID() )
    {
	uirv_.set( tr("Invalid position requested") );
	return;
    }

    if ( !getter_ || tk.geomID() != curGeomID() )
    {
	curlidx_ = lineIdxFor( tk.geomID() );
	if ( curlidx_ < 0 )
	    { uirv_.set( tr("Requested position not available") ); return; }

	delete getter_;
	getter_ = dataset_->traceGetter( curGeomID(), prov().seldata_, uirv_ );
	if ( !getter_ )
	{
	    if ( uirv_.isEmpty() )
		uirv_.set( tr("Requested position not available") );
	    return;
	}
    }

    uirv_ = getter_->get( tk.trcNr(), trc );
}


void Seis::LineFetcher::getNext( SeisTrc& trc )
{
    if ( getter_ )
	uirv_ = getter_->getNext( trc );
    while ( !getter_ || uirv_.isError() )
    {
	if ( mIsSingleLine(prov().seldata_) || !getNextGetter() )
	{
	    if ( uirv_.isEmpty() )
		uirv_.set( uiStrings::sFinished() );
	    break;
	}
	uirv_ = getter_->getNext( trc );
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


int Seis::LineProvider::curLineIdx() const
{
    return fetcher_.curlidx_;
}


uiRetVal Seis::LineProvider::doGetComponentInfo( BufferStringSet& nms,
			TypeSet<Seis::DataType>& dts ) const
{
    return fetcher_.gtComponentInfo(nms,dts);
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


void Seis::LineProvider::getGeometryInfo( int iln,
					  PosInfo::Line2DData& ld ) const
{
    return fetcher_.gtGeometryInfo( iln, ld );
}


bool Seis::LineProvider::getRanges( int iln, StepInterval<int>& trcrg,
	                                 ZSampling& zsamp ) const
{
    return fetcher_.gtRanges( iln, trcrg, zsamp );
}


void Seis::LineProvider::doUsePar( const IOPar& iop, uiRetVal& uirv )
{
    uirv.set( mTODONotImplPhrase() );
}


void Seis::LineProvider::doReset( uiRetVal& uirv ) const
{
    fetcher_.reset();
    uirv = fetcher_.uirv_;
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
