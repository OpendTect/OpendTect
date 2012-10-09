#ifndef odusgclient_h
#define odusgclient_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "odusginfo.h"


namespace Usage
{
class Info;

mClass Client
{
public:

    			Client( const char* grp )
			    : usginfo_(grp)	{}
    virtual		~Client()		{}

    virtual void	prepUsgStart( const char* act=0 ) const
			{ usginfo_.prepStart(act); }
    virtual void	prepUsgEnd( const char* act=0 ) const
			{ usginfo_.prepEnd(act); }

    Info&		usgInfo()		{ return usginfo_; }
    const Info&		usgInfo() const		{ return usginfo_; }

    virtual bool	sendUsgInfo() const;

protected:

    mutable Info	usginfo_;

};


} // namespace


#endif
