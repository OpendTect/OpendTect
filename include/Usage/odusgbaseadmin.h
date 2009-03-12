#ifndef odusgbaseadmin_h
#define odusgbaseadmin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusgbaseadmin.h,v 1.1 2009-03-12 15:51:31 cvsbert Exp $
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
