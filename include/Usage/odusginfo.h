#ifndef odusginfo_h
#define odusginfo_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusginfo.h,v 1.2 2009-03-17 12:53:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include <iosfwd>


namespace Usage
{

mClass Info
{
public:

    			Info( const char* grp, const char* act=0,
			      const char* aux=0 )
			    : group_(grp)
			    , action_(act)
    			    , aux_(aux)
    			    , start_(true)		{}

    BufferString	group_;
    BufferString	action_;
    BufferString	aux_;
    bool		start_;

    std::ostream&	dump(std::ostream&) const;
    BufferString&	dump(BufferString&) const;

};


} // namespace


#endif
