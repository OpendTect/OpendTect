/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellioprov.h"

#include "wellhdf5reader.h"
#include "wellhdf5writer.h"
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
	return nullptr;

    const StringView typstr( typ );
    if ( typstr.isEmpty() || typstr == "dGB" )
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
    return prov ? prov->makeWriteAccess(ioobj,wd,errmsg) : nullptr;
}


class odWellDataIOProvider : public WellDataIOProvider
{
public:
			odWellDataIOProvider()
			    : WellDataIOProvider("OpendTect")	{}

    Well::ReadAccess*	makeReadAccess(const IOObj&,Well::Data&,
				       uiString& errmsg) const override;
    Well::WriteAccess*	makeWriteAccess(const IOObj&,const Well::Data&,
					uiString& errmsg) const override;

    static int		factid_;
};

int odWellDataIOProvider::factid_ = WDIOPF().add( new odWellDataIOProvider );


const WellDataIOProvider& odWellTranslator::getProv() const
{
    return *WDIOPF().providers()[odWellDataIOProvider::factid_];
}


Well::ReadAccess* odWellDataIOProvider::makeReadAccess( const IOObj& ioobj,
				Well::Data& wd, uiString& emsg ) const
{
    const BufferString fnm( ioobj.mainFileName() );
    if ( !HDF5::isHDF5File(fnm) )
	return new Well::odReader( ioobj, wd, emsg );

    if ( !HDF5::isAvailable() )
    {
	emsg = HDF5::Access::sHDF5NotAvailable( fnm );
	return nullptr;
    }

    return new Well::HDF5Reader( ioobj, wd, emsg );
}


Well::WriteAccess* odWellDataIOProvider::makeWriteAccess( const IOObj& ioobj,
				const Well::Data& wd, uiString& emsg ) const
{
    bool usehdf = Well::HDF5Writer::useHDF5( ioobj, emsg );
    if ( !emsg.isEmpty() )
	usehdf = false;

    if ( usehdf )
	return new Well::HDF5Writer( ioobj, wd, emsg );

    return new Well::odWriter( ioobj, wd, emsg );
}
