/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellioprov.h"

#include "transl.h"


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


void WellDataIOProviderFactory::add( WellDataIOProvider* prov )
{
    provs_.add( prov );
}


const WellDataIOProvider* WellDataIOProviderFactory::provider(
						const char* typ ) const
{
    if ( provs_.isEmpty() )
	return nullptr;

    const StringView typstr( typ );
    if ( typstr.isEmpty() )
	return provs_.first();

    for ( const auto* prov : provs_ )
    {
	if ( prov->type() == typ )
	    return prov;
    }

    return nullptr;
}


Well::ReadAccess* WellDataIOProviderFactory::getReadAccess( const IOObj& ioobj,
	    Well::Data& wd, uiString& errmsg ) const
{
    const WellDataIOProvider* prov = provider( ioobj.translator().str() );
    return prov ? prov->makeReadAccess(ioobj,wd,errmsg) : nullptr;
}


Well::WriteAccess* WellDataIOProviderFactory::getWriteAccess(
	    const IOObj& ioobj, const Well::Data& wd, uiString& errmsg ) const
{
    const WellDataIOProvider* prov = provider( ioobj.translator().str() );
    return prov && prov->canWrite() ? prov->makeWriteAccess(ioobj,wd,errmsg)
				    : nullptr;
}

// Base implementations are in welltransl.cc
