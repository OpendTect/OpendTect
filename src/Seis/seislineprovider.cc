/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seislineprovider.h"
#include "seisfetcher.h"
#include "seis2dlineio.h"
#include "seis2ddata.h"
#include "seisbuf.h"
#include "uistrings.h"
#include "seisselectionimpl.h"
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


int Seis::Provider2D::getNrLines( const Fetcher2D& fetcher )
{
    if ( fetcher.dataset_ )
	return fetcher.dataset_->nrLines();
    else
    {
	PtrMan<Seis2DDataSet> ds = fetcher.mkDataSet();
	return ds ? ds->nrLines() : 0;
    }
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
    void		openFirst();
    bool		createGetter();
    bool		getNextGetter();
    bool		getFromBuf(int,SeisTrc&);
    bool		readNextTraces();
    int			lineIdxFor(Pos::GeomID) const;

    void		get(const TrcKey&,SeisTrc&);
    void		getNext(SeisTrc&);

    Executor*		getter_;
    SeisTrcBuf		tbuf_;
    int			curlidx_;
    Pos::GeomID		curGeomID() const
			{ return dataset_ && curlidx_>=0
			    ? dataset_->geomID( curlidx_ ) : mUdfGeomID; }

};

} // namespace Seis


void Seis::LineFetcher::reset()
{
    Fetcher2D::reset();
    delete getter_; getter_ = 0;
    curlidx_ = -1;
    openFirst();
}


void Seis::LineFetcher::openFirst()
{
    openDataSet();
    if ( !uirv_.isOK() )
	return;

    if ( !getNextGetter() )
	{ uirv_ = tr( "No selected data found" ); return; }
}


int Seis::LineFetcher::lineIdxFor( Pos::GeomID geomid ) const
{
    const int nrlines = dataset_->nrLines();
    for ( int lidx=0; lidx<nrlines; lidx++ )
    {
	if ( dataset_->geomID(lidx) == geomid )
	    return lidx;
    }
    return -1;
}


#define mIsSingleLine(sd) (sd && !mIsUdfGeomID(sd->geomID()))


bool Seis::LineFetcher::getNextGetter()
{
    delete getter_; getter_ = 0;
    tbuf_.deepErase();

    curlidx_++;
    if ( curlidx_ >= dataset_->nrLines() )
	return false;

    const Seis::SelData* sd = prov().seldata_;
    const bool issingleline = mIsSingleLine( sd );
    const bool istable = sd && sd->type() == Seis::Table;

    if ( issingleline )
    {
	curlidx_ = lineIdxFor( sd->geomID() );
	if ( curlidx_ < 0 )
	    return false;
    }
    else if ( istable )
    {
	mDynamicCastGet(const Seis::TableSelData*,tsd,sd)
	while ( !dataset_->haveMatch(dataset_->geomID(curlidx_),
				     tsd->binidValueSet()) )
	{
	    curlidx_++;
	    if ( curlidx_ >= dataset_->nrLines() )
		return false;
	}
    }

    getter_ = dataset_->lineGetter( curGeomID(), tbuf_, 1, sd );
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


BufferStringSet Seis::LineProvider::getComponentInfo() const
{
    BufferStringSet compnms;
    return compnms;
}


ZSampling Seis::LineProvider::getZSampling() const
{
    ZSampling ret;
    return ret;
}


int Seis::LineProvider::nrLines() const
{
    return 0;
}


void Seis::LineProvider::getGeometryInfo( int iln,
					  PosInfo::Line2DData& ld ) const
{
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
