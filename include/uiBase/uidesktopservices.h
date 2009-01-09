#ifndef uidesktopservices_h
#define uidesktopservices_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: uidesktopservices.h,v 1.3 2009-01-09 04:26:14 cvsnanne Exp $
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
