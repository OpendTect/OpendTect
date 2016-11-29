/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		September 2016
________________________________________________________________________

-*/

#include "seisvolprovider.h"
#include "seistrctr.h"
#include "seisdatapack.h"
#include "dbman.h"
#include "iostrm.h"
#include "uistrings.h"
#include "file.h"

namespace Seis
{

class VolFetcher
{ mODTextTranslationClass(Seis::VolFetcher);
public:

VolFetcher( VolProvider& p )
    : prov_(p)
    , trl_(0)
    , ioobj_(0)
{
}

~VolFetcher()
{
    delete trl_;
    delete ioobj_;
}

    void		reset();
    void		findDataPack();
    void		openCube();
    Conn*		getFirstConn();
    void		getTranslator(Conn&);

    void		get(const TrcKey&,SeisTrc&);
    void		getNext(SeisTrc&);

    VolProvider&	prov_;
    IOObj*		ioobj_;
    SeisTrcTranslator*	trl_;
    RefMan<SeisDataPack> dp_;
    uiRetVal		uirv_;

};

} // namespace Seis


void Seis::VolFetcher::reset()
{
    uirv_.setEmpty();
    delete trl_; trl_ = 0;
    delete ioobj_; ioobj_ = 0;
    dp_ = 0;

    findDataPack();
    if ( !dp_ )
	openCube();
}


void Seis::VolFetcher::findDataPack()
{
    // find datapack for our prov_.dbky_
    // if it's there and it matches the subselection, we're cool
}


void Seis::VolFetcher::openCube()
{
    ioobj_ = DBM().get( prov_.dbky_ );
    if ( !ioobj_ )
    {
	uirv_ = uiStrings::phrCannotFindDBEntry( prov_.dbky_.toUiString() );
	return;
    }

    Conn* conn = getFirstConn();
    if ( conn )
	getTranslator( *conn );
}


Conn* Seis::VolFetcher::getFirstConn()
{
    Conn* conn = ioobj_->getConn( Conn::Read );
    const char* fnm = ioobj_->fullUserExpr( Conn::Read );
    if ( !conn || (conn->isBad() && !File::isDirectory(fnm)) )
    {
	delete conn; conn = 0;
	mDynamicCastGet(IOStream*,iostrm,ioobj_)
	if ( iostrm && iostrm->isMultiConn() )
	{
	    while ( !conn || conn->isBad() )
	    {
		delete conn; conn = 0;
		if ( !iostrm->toNextConnIdx() )
		    break;

		conn = ioobj_->getConn( Conn::Read );
	    }
	}
    }

    if ( !conn )
	uirv_ = uiStrings::phrCannotOpen( ioobj_->uiName() );
    return conn;
}


void Seis::VolFetcher::getTranslator( Conn& conn )
{
    delete trl_;
    Translator* trl = ioobj_->createTranslator();
    mDynamicCast( SeisTrcTranslator*, trl_, trl );
    if ( !trl_ )
    {
	uirv_ = tr("Cannot create appropriate data reader."
	    "\nThis is an installation problem or a data corruption issue.");
	return;
    }

    trl_->setSelData( prov_.seldata_ );
    if ( !trl_->initRead(&conn,prov_.readmode_) )
    {
	uirv_ = toUiString( trl_->errMsg() );
	return;
    }

    if ( prov_.selcomp_ >= 0 )
    {
	for ( int icd=0; icd<trl_->componentInfo().size(); icd++ )
	{
	    if ( icd != prov_.selcomp_ )
		trl_->componentInfo()[icd]->destidx = -1;
	}
    }

    if ( !trl_->commitSelections() )
    {
	uirv_ = toUiString( trl_->errMsg() );
	return;
    }
}


void Seis::VolFetcher::get( const TrcKey& trcky, SeisTrc& trc )
{
    if ( dp_ )
    {
	uirv_.set( mTODONotImplPhrase() );
	return;
    }

    if ( !trl_ )
    {
	uirv_.set( uiStrings::phrInternalError("trl_/Seismic Volume Fetcher" ));
	return;
    }

    uirv_.set( mTODONotImplPhrase() );
}


void Seis::VolFetcher::getNext( SeisTrc& trc )
{
    if ( dp_ )
    {
	uirv_.set( mTODONotImplPhrase() );
	return;
    }

    uirv_.set( mTODONotImplPhrase() );
}



Seis::VolProvider::VolProvider()
    : fetcher_(*new VolFetcher(*this))
{
}


Seis::VolProvider::~VolProvider()
{
    delete &fetcher_;
}


uiRetVal Seis::VolProvider::setInput( const DBKey& dbky )
{
    dbky_ = dbky;
    fetcher_.reset();
    return fetcher_.uirv_;
}


BufferStringSet Seis::VolProvider::getComponentInfo() const
{
    return BufferStringSet();
}


ZSampling Seis::VolProvider::getZSampling() const
{
    return ZSampling( 0.f, 0.f, 1.f );
}


TrcKeySampling Seis::VolProvider::getHSampling() const
{
    return TrcKeySampling();
}


void Seis::VolProvider::doUsePar( const IOPar& iop, uiRetVal& uirv )
{
}


void Seis::VolProvider::doGetNext( SeisTrc& trc, uiRetVal& uirv ) const
{
    fetcher_.getNext( trc );
    uirv = fetcher_.uirv_;
}


void Seis::VolProvider::doGet( const TrcKey& trcky, SeisTrc& trc,
				  uiRetVal& uirv ) const
{
    fetcher_.get( trcky, trc );
    uirv = fetcher_.uirv_;
}
