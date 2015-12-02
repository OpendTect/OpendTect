#ifndef wellioprov_h
#define wellioprov_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"

#include "manobjectset.h"
#include "wellreadaccess.h"
#include "wellwriteaccess.h"
#include "bufstring.h"
class IOObj;
namespace Well { class Data; }


/*!\brief Well::Data and sub-objects provider from data stores */

mExpClass(Well) WellDataIOProvider
{
public:

    virtual			~WellDataIOProvider()	{}

    virtual bool		canWrite() const	{ return false; }

    virtual Well::ReadAccess*	makeReadAccess( const IOObj&, Well::Data&,
						BufferString& ) const
				{ return 0; }
    virtual Well::WriteAccess*	makeWriteAccess( const IOObj&,
						 const Well::Data&,
						 BufferString& ) const
				{ return 0; }

    const OD::String&		type() const		{ return type_; }

protected:

				WellDataIOProvider( const char* t )
				    : type_(t)			{}

    const BufferString		type_;

};


mExpClass(Well) WellDataIOProviderFactory
{
public:

    int				add( WellDataIOProvider* prov )
				{ provs_ += prov; return provs_.size() - 1; }
    const ObjectSet<WellDataIOProvider>& providers() const
				{ return provs_; }

    // Convenience functions
    const WellDataIOProvider*	provider(const char* typ) const;
    Well::ReadAccess*		getReadAccess(const IOObj&,Well::Data&,
						BufferString&) const;
    Well::WriteAccess*		getWriteAccess(const IOObj&,const Well::Data&,
						BufferString&) const;

protected:

    ManagedObjectSet<WellDataIOProvider> provs_;

};

mGlobal(Well) WellDataIOProviderFactory& WDIOPF();


#endif
