#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "bufstring.h"
#include "manobjectset.h"
#include "wellreadaccess.h"
#include "wellwriteaccess.h"

class IOObj;
namespace Well { class Data; }


/*!\brief Well::Data and sub-objects provider from data stores */

mExpClass(Well) WellDataIOProvider
{
public:

    virtual			~WellDataIOProvider();

    virtual bool		canWrite() const	{ return false; }
				/*!< Not used (yet) in this branch,
					will become true */

    virtual Well::ReadAccess*	makeReadAccess(const IOObj&,Well::Data&,
					       uiString&) const
				{ return 0; }
    virtual Well::WriteAccess*	makeWriteAccess(const IOObj&,
						const Well::Data&,
						uiString&) const
				{ return 0; }

    const OD::String&		type() const		{ return type_; }

protected:

				WellDataIOProvider(const char* type);
				//!< Type should be the translator user name

    const BufferString		type_;

};


mExpClass(Well) WellDataIOProviderFactory
{
public:
				~WellDataIOProviderFactory();

    int				add( WellDataIOProvider* prov )
				{ provs_ += prov; return provs_.size() - 1; }

    mDeprecatedObs
    const ObjectSet<WellDataIOProvider>& providers() const
				{ return provs_; }

    // Convenience functions
    const WellDataIOProvider*	provider(const char* typ) const;
				//!< Type should be the translator user name
    Well::ReadAccess*		getReadAccess(const IOObj&,Well::Data&,
					      uiString&) const;
    Well::WriteAccess*		getWriteAccess(const IOObj&,const Well::Data&,
					       uiString&) const;

protected:

    ManagedObjectSet<WellDataIOProvider> provs_;

};

mGlobal(Well) WellDataIOProviderFactory& WDIOPF();
