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

class IOObj;
namespace Well { class Data; class ReadAccess; class WriteAccess; }


/*!\brief Well::Data and sub-objects provider from data stores */

mExpClass(Well) WellDataIOProvider
{
public:

    virtual			~WellDataIOProvider();

    virtual bool		canWrite() const	{ return true; }

    virtual Well::ReadAccess*	makeReadAccess(const IOObj&,Well::Data&,
					       uiString&) const
				{ return nullptr; }
    virtual Well::WriteAccess*	makeWriteAccess(const IOObj&,
						const Well::Data&,
						uiString&) const
				{ return nullptr; }

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

    void			add(WellDataIOProvider*);

    const WellDataIOProvider*	provider(const char* typ) const;
				//!< Type should be the translator user name

    // Convenience functions
    Well::ReadAccess*		getReadAccess(const IOObj&,Well::Data&,
					      uiString&) const;
    Well::WriteAccess*		getWriteAccess(const IOObj&,const Well::Data&,
					       uiString&) const;

private:

    ManagedObjectSet<WellDataIOProvider> provs_;

public:
    mDeprecatedObs
    const ObjectSet<WellDataIOProvider>& providers() const
				{ return provs_; }

};

mGlobal(Well) WellDataIOProviderFactory& WDIOPF();
