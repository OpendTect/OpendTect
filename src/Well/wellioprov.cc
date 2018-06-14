/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : July 2014
-*/



#include "wellioprov.h"
#include "wellodreader.h"
#include "wellodwriter.h"
#include "wellhdf5reader.h"
#include "wellhdf5writer.h"
#include "welltransl.h"
#include "hdf5reader.h"
#include "hdf5writer.h"


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
	    Well::Data& wd, uiString& e ) const
{
    const WellDataIOProvider* prov = provider( ioobj.translator().str() );
    return prov ? prov->makeReadAccess(ioobj,wd,e) : 0;
}


Well::WriteAccess* WellDataIOProviderFactory::getWriteAccess(
	    const IOObj& ioobj, const Well::Data& wd, uiString& e ) const
{
    const WellDataIOProvider* prov = provider( ioobj.translator().str() );
    return prov ? prov->makeWriteAccess(ioobj,wd,e) : 0;
}


class odWellDataIOProvider : public WellDataIOProvider
{
public:
			odWellDataIOProvider()
			    : WellDataIOProvider("OpendTect")	{}

    Well::ReadAccess*	makeReadAccess(const IOObj&,Well::Data&,
				       uiString&) const;
    Well::WriteAccess*	makeWriteAccess(const IOObj&,const Well::Data&,
					uiString&) const;

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
	return 0;
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
