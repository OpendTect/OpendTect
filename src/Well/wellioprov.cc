/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellioprov.h"
#include "wellodreader.h"
#include "wellodwriter.h"
#include "welltransl.h"


// WellDataIOProvider
WellDataIOProvider::WellDataIOProvider( const char* tp )
    : type_(tp)
{}


WellDataIOProvider::~WellDataIOProvider()
{}


// WellDataIOProviderFactory
WellDataIOProviderFactory& WDIOPF()
{
    mDefineStaticLocalObject( WellDataIOProviderFactory, theinst, );
    return theinst;
}


WellDataIOProviderFactory::~WellDataIOProviderFactory()
{}


const WellDataIOProvider* WellDataIOProviderFactory::provider(
						const char* typ ) const
{
    if ( provs_.isEmpty() )
	return 0;
    else if ( !typ || !*typ || StringView(typ) == "dGB" )
	return provs_[0];

    for ( int idx=0; idx<provs_.size(); idx++ )
	if ( provs_[idx]->type() == typ )
	    return provs_[idx];

    return 0;
}


Well::ReadAccess* WellDataIOProviderFactory::getReadAccess( const IOObj& ioobj,
	    Well::Data& wd, uiString& errmsg ) const
{
    const WellDataIOProvider* prov = provider( ioobj.translator().str() );
    return prov ? prov->makeReadAccess(ioobj,wd,errmsg) : 0;
}


Well::WriteAccess* WellDataIOProviderFactory::getWriteAccess(
	    const IOObj& ioobj, const Well::Data& wd, uiString& errmsg ) const
{
    const WellDataIOProvider* prov = provider( ioobj.translator().str() );
    return prov ? prov->makeWriteAccess(ioobj,wd,errmsg) : 0;
}


class odWellDataIOProvider : public WellDataIOProvider
{
public:
			odWellDataIOProvider()
			    : WellDataIOProvider("OpendTect")	{}

    Well::ReadAccess*	makeReadAccess( const IOObj& ioobj,
			    Well::Data& wd, uiString& errmsg ) const override
			{ return new Well::odReader(ioobj,wd,errmsg); }
    Well::WriteAccess*	makeWriteAccess( const IOObj& ioobj,
					 const Well::Data& wd,
					 uiString& errmsg ) const override
			{ return new Well::odWriter(ioobj,wd,errmsg); }

    static int		factid_;
};

int odWellDataIOProvider::factid_ = WDIOPF().add( new odWellDataIOProvider );


const WellDataIOProvider& odWellTranslator::getProv() const
{
    return *WDIOPF().providers()[odWellDataIOProvider::factid_];
}
