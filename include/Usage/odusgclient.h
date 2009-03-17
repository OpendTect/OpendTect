#ifndef odusgclient_h
#define odusgclient_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusgclient.h,v 1.1 2009-03-17 12:53:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "odusginfo.h"


namespace Usage
{
class Info;

mClass Client
{
public:

    			Client( const char* nm )
			    : usginfo_(nm)	{}
    virtual		~Client()		{}

    const char*		name() const		{ return usginfo_.group_; }

protected:

    Info		usginfo_;
    bool		sendUsageInfo();

};


} // namespace


#endif
