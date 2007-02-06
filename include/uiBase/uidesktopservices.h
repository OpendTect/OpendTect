#ifndef uidesktopservices_h
#define uidesktopservices_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: uidesktopservices.h,v 1.1 2007-02-06 21:27:05 cvsnanne Exp $
________________________________________________________________________

-*/

class QDesktopServices;

class uiDesktopServices
{
public:
    			uiDesktopServices();
			~uiDesktopServices()		{}

    void		openUrl(const char* url);
    			//!< url has to start with http://, file://,
			//!< ftp:// or mailto:
    			//!< ftp://user:passwd@ftp.example.com
    			//!< mailto:user@foo.com?subject=Test&body=Just a test

protected:
    QDesktopServices*	qdesktopservices_;
};


#endif
