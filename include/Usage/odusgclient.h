#ifndef odusgclient_h
#define odusgclient_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: odusgclient.h,v 1.2 2009-07-22 16:01:19 cvsbert Exp $
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
