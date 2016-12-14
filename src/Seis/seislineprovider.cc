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
#include "seisselection.h"
#include "posinfo2d.h"


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
    void		getNextGetter();

    void		get(const TrcKey&,SeisTrc&);
    void		getNext(SeisTrc&);

    Seis2DLineGetter*	getter_;
    SeisTrcBuf		tbuf_;

};

} // namespace Seis


void Seis::LineFetcher::reset()
{
    Fetcher2D::reset();
    delete getter_; getter_ = 0;
}


void Seis::LineFetcher::openFirst()
{
    if ( !fillIOObj() )
	return;

    getNextGetter();

    if ( !getter_ )
	{ uirv_ = tr( "No selected data found" ); return; }
}


void Seis::LineFetcher::getNextGetter()
{
}


void Seis::LineFetcher::get( const TrcKey& tk, SeisTrc& trc )
{
}


void Seis::LineFetcher::getNext( SeisTrc& trc )
{
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
