#ifndef uidesktopservices_h
#define uidesktopservices_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: uidesktopservices.h,v 1.4 2009/07/22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "commondefs.h"

mClass uiDesktopServices
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
