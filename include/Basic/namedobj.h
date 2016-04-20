#ifndef namedobj_h
#define namedobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 1994, Aug 2006, Mar 2015
________________________________________________________________________

-*/

#include "basicmod.h"
#include "monitor.h"
#include "bufstring.h"


/*!\brief Monitorable object with a name.  */

mExpClass(Basic) NamedObject : public Monitorable
{
public:

			NamedObject(const char* nm=0);
			NamedObject(const NamedObject&);
    virtual		~NamedObject();
    bool		operator ==(const NamedObject&) const;

    virtual BufferString getName() const;
    virtual void	setName(const char*);

    bool		getNameFromPar(const IOPar&);
    void		putNameInPar(IOPar&) const;

    mDeclInstanceCreatedNotifierAccess( NamedObject );

			//! not MT-safe
    virtual const OD::String& name() const	{ return name_; }

protected:

    BufferString	name_;

};


#endif
