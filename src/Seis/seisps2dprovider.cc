/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisps2dprovider.h"
#include "seisfetcher.h"
#include "seistrctr.h"
#include "uistrings.h"
#include "seispacketinfo.h"
#include "seisbuf.h"
#include "seisselection.h"
#include "seispsioprov.h"
#include "seis2ddata.h"
#include "posinfo2d.h"
#include "keystrs.h"


namespace Seis
{

/*\brief Gets required traces from 2D data store.  */

class PS2DFetcher : public Fetcher2D
{ mODTextTranslationClass(Seis::PS2DFetcher);
public:

PS2DFetcher( PS2DProvider& p )
    : Fetcher2D(p)
    , rdr_(0)
    , lditer_(0)
{
}

~PS2DFetcher()
{
    delete lditer_;
    delete rdr_;
}

PS2DProvider& prov()
{
    return static_cast<PS2DProvider&>( prov_ );
}

const PS2DProvider& prov() const
{
    return static_cast<const PS2DProvider&>( prov_ );
}

    void		reset();

    bool		openReader(Pos::GeomID);
    void		moveNextTrcKey();
    bool		prepGetAt(const TrcKey&);
    void		getAt(const TrcKey&,SeisTrcBuf&);
    void		getSingleAt(const TrcKey&,SeisTrc&);
    static SeisPS2DReader*  getReader(const IOObj&,Pos::GeomID);

    void		get(const TrcKey&,SeisTrcBuf&);
    void		getSingle(const TrcKey&,SeisTrc&);
    void		getNext(SeisTrcBuf&);
    void		getNextSingle(SeisTrc&);

    SeisPS2DReader*	rdr_;
    PosInfo::Line2DDataIterator* lditer_;
    bool		atend_;

};

} // namespace Seis


void Seis::PS2DFetcher::reset()
{
    Fetcher2D::reset();

    delete lditer_; lditer_ = 0;
    delete rdr_; rdr_ = 0;
    atend_ = false;

    openDataSet();
}


SeisPS2DReader* Seis::PS2DFetcher::getReader( const IOObj& ioobj,
						Pos::GeomID geomid )
{
    return SPSIOPF().get2DReader( ioobj, geomid );
}


bool Seis::PS2DFetcher::openReader( Pos::GeomID geomid )
{
    uirv_.setEmpty();
    delete lditer_; lditer_ = 0;
    delete rdr_;
    rdr_ = getReader( *ioobj_, geomid );
    if ( !rdr_ )
    {
	uirv_ = tr( "Cannot find a reader for this type of data store" );
	return false;
    }

    lditer_ = new PosInfo::Line2DDataIterator( rdr_->posData() );
    nexttrcky_.setGeomID( geomid );
    if ( !lditer_->next() )
    {
	uirv_ = tr( "Empty line" );
	return false;
    }

    nexttrcky_.setTrcNr( lditer_->trcNr() );
    return true;
}


void Seis::PS2DFetcher::moveNextTrcKey()
{
    atend_ = !lditer_->next();

    if ( atend_ )
    {
	// At end of this line ...
	int linenr = dataset_->indexOf( nexttrcky_.geomID() );
	while ( atend_ && linenr < dataset_->nrLines()-1 )
	{
	    linenr++;
	    nexttrcky_.setGeomID( dataset_->geomID(linenr) );
	    if ( openReader(nexttrcky_.geomID()) )
		atend_ = !lditer_->next();
	}
    }

	// atend_ now indicates the end of everything
    if ( !atend_ )
	nexttrcky_.setLineNr( lditer_->trcNr() );
}


bool Seis::PS2DFetcher::prepGetAt( const TrcKey& tk )
{
    if ( rdr_ && rdr_->geomID() != tk.geomID() )
    {
	if ( !openReader( tk.geomID() ) )
	    return false;
    }

    nexttrcky_ = tk;
    return true;
}


void Seis::PS2DFetcher::getAt( const TrcKey& tk, SeisTrcBuf& tbuf )
{
    tbuf.deepErase();
    if ( !prepGetAt(tk) )
	return;

    if ( !rdr_->getGather(nexttrcky_,tbuf) )
	uirv_.set( rdr_->errMsg() );
    else
	moveNextTrcKey();
}


void Seis::PS2DFetcher::getSingleAt( const TrcKey& tk, SeisTrc& trc )
{
    if ( !prepGetAt(tk) )
	return;

    SeisTrc* rdtrc = rdr_->getTrace( nexttrcky_,
				     prov().selcomp_<0 ? 0 : prov().selcomp_ );
    if ( !rdtrc )
	uirv_.set( rdr_->errMsg() );
    else
    {
	trc = *rdtrc;
	moveNextTrcKey();
    }
}


void Seis::PS2DFetcher::get( const TrcKey& tk, SeisTrcBuf& tbuf )
{
    getAt( tk, tbuf );
}


void Seis::PS2DFetcher::getSingle( const TrcKey& tk, SeisTrc& trc )
{
    getSingleAt( tk, trc );
}


void Seis::PS2DFetcher::getNext( SeisTrcBuf& tbuf )
{
    if ( atend_ )
	uirv_.set( uiStrings::sFinished() );
    else
	getAt( nexttrcky_, tbuf );
}


void Seis::PS2DFetcher::getNextSingle( SeisTrc& trc )
{
    if ( atend_ )
	uirv_.set( uiStrings::sFinished() );
    else
	getSingleAt( nexttrcky_, trc );
}



Seis::PS2DProvider::PS2DProvider()
    : fetcher_(*new PS2DFetcher(*this))
{
}


Seis::PS2DProvider::~PS2DProvider()
{
    delete &fetcher_;
}


SeisPS2DReader* Seis::PS2DProvider::mkReader( Pos::GeomID geomid ) const
{
    PtrMan<IOObj> ioobj = fetcher_.getIOObj();
    return ioobj ? PS2DFetcher::getReader( *ioobj, geomid ) : 0;
}


static void addCompName( BufferStringSet& compnms, bool isoffs, float val )
{
    BufferString nm( isoffs?sKey::Offset():sKey::Azimuth(), " " );
    nm.add( val );
    compnms.add( nm );
}


BufferStringSet Seis::PS2DProvider::getComponentInfo() const
{
    PtrMan<SeisPS2DReader> rdrptrman;
    const SeisPS2DReader* rdr = 0;
    if ( fetcher_.rdr_ )
	rdr = fetcher_.rdr_;
    else
    {
	//TODO
	// rdrptrman = mkReader( first_geomid );
	// rdr = rdrptrman;
    }

    BufferStringSet compnms;
    if ( !rdr )
	return compnms;

    const PosInfo::Line2DData& ld = rdr->posData();
    if ( ld.positions().size() < 1 )
	return compnms;

    //TODO remove
    addCompName( compnms, true, 0.f );

    /*
    const int linenr = ld.size() / 2;
    const PosInfo::LineData& ld = *ld[linenr];
    const int segnr = ld.segments_.size() / 2;
    const TrcKey tk( ld.linenr_, ld.segments_[segnr].center() );
    SeisTrcBuf tbuf( true );
    if ( !rdr->getGather(tk,tbuf) || tbuf.isEmpty() )
	return compnms;

    float prevoffs = tbuf.get(0)->info().offset_;
    float prevazim = tbuf.get(0)->info().azimuth_;
    const int nrtrcs = tbuf.size();
    bool useoffs = true;
    if ( nrtrcs == 1 )
	addCompName( compnms, true, prevoffs );
    else
    {
	float offs = tbuf.get(1)->info().offset_;
	float azim = tbuf.get(1)->info().azimuth_;
	if ( !mIsEqual(azim,prevazim,1e-4f) )
	    useoffs = !mIsEqual(offs,prevoffs,1e-4f);
	for ( int idx=0; idx<tbuf.size(); idx++ )
	    addCompName( compnms, useoffs,
			 useoffs ? tbuf.get(idx)->info().offset_
				 : tbuf.get(idx)->info().azimuth_ );
    }
    */

    return compnms;
}


ZSampling Seis::PS2DProvider::getZSampling() const
{
    ZSampling ret;
    if ( fetcher_.rdr_ )
	ret = fetcher_.rdr_->getZRange();
    else
    {
	//TODO
	// PtrMan<SeisPS2DReader> rdr = mkReader();
	// ret = rdr ? rdr->getZRange() : TrcKeyZSampling(true).zsamp_;
    }
    return ret;
}


int Seis::PS2DProvider::nrLines() const
{
    return getNrLines( fetcher_ );
}


void Seis::PS2DProvider::getGeometryInfo( int lidx,
					  PosInfo::Line2DData& ld ) const
{
    ld.setEmpty();
	//TODO
	// PtrMan<SeisPS2DReader> rdr = mkReader();
	// if ( rdr )
	    // ld = rdr->posData();
	// else
	// {
	    // There is no fallback, right?
	// }
}


void Seis::PS2DProvider::doUsePar( const IOPar& iop, uiRetVal& uirv )
{
    uirv.set( mTODONotImplPhrase() );
}


void Seis::PS2DProvider::doReset( uiRetVal& uirv ) const
{
    fetcher_.reset();
    uirv = fetcher_.uirv_;
}


void Seis::PS2DProvider::doGetNextGather( SeisTrcBuf& tbuf,
					  uiRetVal& uirv ) const
{
    fetcher_.getNext( tbuf );
    uirv = fetcher_.uirv_;
}


void Seis::PS2DProvider::doGetGather( const TrcKey& trcky, SeisTrcBuf& tbuf,
				  uiRetVal& uirv ) const
{
    fetcher_.get( trcky.binID(), tbuf );
    uirv = fetcher_.uirv_;
}


void Seis::PS2DProvider::doGetNext( SeisTrc& trc, uiRetVal& uirv ) const
{
    fetcher_.getNextSingle( trc );
    uirv = fetcher_.uirv_;
}


void Seis::PS2DProvider::doGet( const TrcKey& trcky, SeisTrc& trc,
				uiRetVal& uirv ) const
{
    fetcher_.getSingle( trcky.binID(), trc );
    uirv = fetcher_.uirv_;
}
