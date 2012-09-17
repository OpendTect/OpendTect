#ifndef odusgbaseadmin_h
#define odusgbaseadmin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusgbaseadmin.h,v 1.2 2009/07/22 16:01:19 cvsbert Exp $
________________________________________________________________________

-*/

#include "odusgadmin.h"


namespace Usage
{

mClass BaseAdministrator : public Administrator
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
