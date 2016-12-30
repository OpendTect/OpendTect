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
#include "seisselection.h"
#include "seisbuf.h"
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
    , tbuf_(false)
{
}

~LineFetcher()
{
    tbuf_.deepErase();
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
    bool		createGetter();
    bool		getNextGetter();
    bool		getFromBuf(int,SeisTrc&);
    bool		readNextTraces();

    void		get(const TrcKey&,SeisTrc&);
    void		getNext(SeisTrc&);

    Executor*		getter_;
    SeisTrcBuf		tbuf_;

};

} // namespace Seis


void Seis::LineFetcher::reset()
{
    Fetcher2D::reset();
    delete getter_; getter_ = 0;

    openDataSet();
    if ( !uirv_.isOK() )
	return;

    if ( !getNextGetter() )
	{ uirv_ = tr( "No selected data found" ); return; }
}


bool Seis::LineFetcher::getNextGetter()
{
    delete getter_; getter_ = 0;
    tbuf_.deepErase();

    if ( !toNextLine() )
	return false;

    getter_ = dataset_->lineGetter( curGeomID(), tbuf_, 1, prov2D().selData() );
    return getter_ ? true : getNextGetter();
}


bool Seis::LineFetcher::getFromBuf( int trcnr, SeisTrc& trc )
{
    while ( !tbuf_.isEmpty() )
    {
	SeisTrc* buftrc = tbuf_.remove( 0 );
	const bool ismatch = buftrc->info().trcNr() == trcnr;
	if ( ismatch )
	    trc = *buftrc;
	delete buftrc;
	if ( ismatch )
	    return true;
    }
    return false;
}


bool Seis::LineFetcher::createGetter()
{
    delete getter_; tbuf_.deepErase();
    getter_ = dataset_->lineGetter( curGeomID(), tbuf_, 1, prov().seldata_ );
    if ( getter_ )
	return true;

    uirv_.set( uiStrings::phrCannotOpen(
		    toUiString(dataset_->lineName(curlidx_)) ) );
    return false;
}


bool Seis::LineFetcher::readNextTraces()
{
    if ( !getter_ && !createGetter() )
	return false;

    int res = getter_->doStep();
    if ( res < 0 )
    {
	uirv_.set( getter_->message() );
	return false;
    }

    return !tbuf_.isEmpty();
}


void Seis::LineFetcher::get( const TrcKey& tk, SeisTrc& trc )
{
    if ( !tk.hasValidGeomID() )
    {
	uirv_.set( tr("Invalid position requested") );
	return;
    }

    if ( tk.geomID() == curGeomID() && getFromBuf(tk.trcNr(),trc) )
	return;

    tbuf_.deepErase();
    curlidx_ = lineIdxFor( tk.geomID() );
    if ( curlidx_ < 0 )
    {
	uirv_.set( tr("Requested position not available") );
	return;
    }

    if ( !createGetter() )
	return;

    while ( true )
    {
	if ( !readNextTraces() )
	{
	    uirv_.set( tr("Requested position not available") );
	    return;
	}
	if ( getFromBuf(tk.trcNr(),trc) )
	    break;
    }
}


void Seis::LineFetcher::getNext( SeisTrc& trc )
{
    if ( tbuf_.isEmpty() )
    {
        while ( !readNextTraces() )
	{
	    if ( mIsSingleLine(prov().seldata_) || !getNextGetter() )
		{ uirv_.set( uiStrings::sFinished() ); return; }
	}
    }

    SeisTrc* buftrc = tbuf_.remove( 0 );
    trc = *buftrc;
    delete buftrc;
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
