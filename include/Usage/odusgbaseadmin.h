#ifndef odusgbaseadmin_h
#define odusgbaseadmin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusgbaseadmin.h,v 1.3 2012-08-03 13:00:43 cvskris Exp $
________________________________________________________________________

-*/

#include "usagemod.h"
#include "odusgadmin.h"


namespace Usage
{

mClass(Usage) BaseAdministrator : public Administrator
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

