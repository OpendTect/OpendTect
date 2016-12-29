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
    int			curlidx_;
    bool		atend_;

};

} // namespace Seis


void Seis::PS2DFetcher::reset()
{
    Fetcher2D::reset();

    curlidx_ = 0;
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
			 prov().selcomps_.isEmpty() ? 0 : prov().selcomps_[0] );
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


int Seis::PS2DProvider::curLineIdx() const
{
    return fetcher_.curlidx_;
}


SeisPS2DReader* Seis::PS2DProvider::mkReader( Pos::GeomID geomid ) const
{
    PtrMan<IOObj> ioobj = fetcher_.getIOObj();
    return ioobj ? PS2DFetcher::getReader( *ioobj, geomid ) : 0;
}


uiRetVal Seis::PS2DProvider::doGetComponentInfo( BufferStringSet& nms,
			TypeSet<Seis::DataType>& dts ) const
{
    return fetcher_.gtComponentInfo(nms,dts);
}


int Seis::PS2DProvider::nrLines() const
{
    return fetcher_.gtNrLines();
}


Pos::GeomID Seis::PS2DProvider::geomID( int iln ) const
{
    return fetcher_.gtGeomID( iln );
}


int Seis::PS2DProvider::lineNr( Pos::GeomID geomid ) const
{
    return fetcher_.gtLineNr( geomid );
}


BufferString Seis::PS2DProvider::lineName( int iln ) const
{
    return fetcher_.gtLineName( iln );
}


void Seis::PS2DProvider::getGeometryInfo( int lidx,
					  PosInfo::Line2DData& ld ) const
{
    fetcher_.gtGeometryInfo( lidx, ld );
}


bool Seis::PS2DProvider::getRanges( int iln, StepInterval<int>& trcrg,
	                                 ZSampling& zsamp ) const
{
    return fetcher_.gtRanges( iln, trcrg, zsamp );
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
