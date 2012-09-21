#ifndef uidesktopservices_h
#define uidesktopservices_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "commondefs.h"

mClass(uiBase) uiDesktopServices
{
public:
    static bool		openUrl(const char* url);
    			//!< url has to start with http://, file://,
			//!< ftp:// or mailto:
    			//!< ftp://user:passwd@ftp.example.com
    			//!< mailto:user@foo.com?subject=Test&body=Just a test

protected:
};


#endif

