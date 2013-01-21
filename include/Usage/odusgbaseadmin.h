#ifndef odusgbaseadmin_h
#define odusgbaseadmin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "usagemod.h"
#include "odusgadmin.h"


namespace Usage
{

mExpClass(Usage) BaseAdministrator : public Administrator
{
public:

    			BaseAdministrator()
			    : Administrator("")		{ reset(); }

    virtual bool	handle(Info&);

protected:

    void		reset();

};


} // namespace


#endif

