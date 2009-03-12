#ifndef odusginfo_h
#define odusginfo_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusginfo.h,v 1.1 2009-03-12 15:51:31 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"


namespace Usage
{

mClass Info
{
public:

    			Info( const char* grp, const char* act )
			    : group_(grp)
			    , action_(act)	{}

    BufferString	group_;
    BufferString	action_;

    BufferString	auxinfo_;

};


} // namespace


#endif
