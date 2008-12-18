#ifndef helpview_h
#define helpview_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		18-8-2000
 RCS:		$Id: helpview.h,v 1.11 2008-12-18 13:21:48 cvsbert Exp $
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
    static BufferString	getCreditsURLForWinID(const char* winid);
    			//!< Finds link in the appropriate credits page

protected:

    static BufferString	getLinkNameForWinID(const char*,const char*);
    static BufferString	getURLForLinkName(const char*,const char*);

};

#endif
