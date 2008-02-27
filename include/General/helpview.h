#ifndef helpview_h
#define helpview_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		18-8-2000
 RCS:		$Id: helpview.h,v 1.10 2008-02-27 10:22:06 cvsnanne Exp $
________________________________________________________________________

-*/


#include "bufstring.h"

/*!\brief Pops up the help system

  The Help is sought in $DTECT_APPL/doc/User/xxx . The xxx is the 'scope'
  taken from the window ID (e.g. "myplugin" from "myplugin:1.2.3"). If
  no scope present, 'base' is used (which is the OpendTect manual).
*/


class HelpViewer
{
public:
    static BufferString	getURLForWinID(const char* winid);
    			//!< Combines Link -> WinID -> URL
protected:
    static BufferString	getLinkNameForWinID(const char*,const char*);
    static BufferString	getURLForLinkName(const char*,const char*);
};

#endif
