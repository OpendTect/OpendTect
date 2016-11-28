/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		September 2016
________________________________________________________________________

-*/

#include "seisvolprovider.h"
#include "seistrctr.h"

namespace Seis
{

class VolFetcher
{
public:

inline VolFetcher( VolProvider& p )
    : prov_(p)
    , trl_(0)
{
}

    void		reset();
    void		get(const TrcKey&,SeisTrc&);
    void		getNext(SeisTrc&);

    VolProvider&	prov_;
    SeisTrcTranslator*	trl_;
    uiRetVal		uirv_;

};

} // namespace Seis


void Seis::VolFetcher::reset()
{
    delete trl_; trl_ = 0;
    // create new trl_ from IOObj, also Conn
    // set trl_ subsel
    // set trl_ to honor selcomp_
    // commit selections
}


void Seis::VolFetcher::get( const TrcKey& trcky, SeisTrc& trc )
{
}


void Seis::VolFetcher::getNext( SeisTrc& trc )
{
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
