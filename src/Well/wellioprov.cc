/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : July 2014
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "wellioprov.h"
#include "wellodreader.h"
#include "wellodwriter.h"
#include "welltransl.h"


WellDataIOProviderFactory& WDIOPF()
{
    mDefineStaticLocalObject( WellDataIOProviderFactory, theinst, );
    return theinst;
}


const WellDataIOProvider* WellDataIOProviderFactory::provider(
						const char* typ ) const
{
    if ( provs_.isEmpty() )
	return 0;
    else if ( !typ || !*typ || FixedString(typ) == "dGB" )
	return provs_[0];

    for ( int idx=0; idx<provs_.size(); idx++ )
	if ( provs_[idx]->type() == typ )
	    return provs_[idx];

    return 0;
}


Well::ReadAccess* WellDataIOProviderFactory::getReadAccess( const IOObj& ioobj,
	    Well::Data& wd, BufferString& e ) const
{
    const WellDataIOProvider* prov = provider( ioobj.translator().str() );
    return prov ? prov->makeReadAccess(ioobj,wd,e) : 0;
}


Well::WriteAccess* WellDataIOProviderFactory::getWriteAccess(
	    const IOObj& ioobj, const Well::Data& wd, BufferString& e ) const
{
    const WellDataIOProvider* prov = provider( ioobj.translator().str() );
    return prov ? prov->makeWriteAccess(ioobj,wd,e) : 0;
}


class odWellDataIOProvider : public WellDataIOProvider
{
public:
			odWellDataIOProvider()
			    : WellDataIOProvider("OpendTect")	{}

    Well::ReadAccess*	makeReadAccess( const IOObj& ioobj,
				Well::Data& wd, BufferString& e ) const
			{ return new Well::odReader(ioobj,wd,e); }
    Well::WriteAccess*	makeWriteAccess( const IOObj& ioobj,
				const Well::Data& wd, BufferString& e ) const
			{ return new Well::odWriter(ioobj,wd,e); }

    static int		factid_;
};

int odWellDataIOProvider::factid_ = WDIOPF().add( new odWellDataIOProvider );


const WellDataIOProvider& odWellTranslator::getProv() const
{
    return *WDIOPF().providers()[odWellDataIOProvider::factid_];
}
