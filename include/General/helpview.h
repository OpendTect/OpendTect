#ifndef helpview_h
#define helpview_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		18-8-2000
 RCS:		$Id$
________________________________________________________________________

-*/


#include "bufstring.h"
class IOPar;

/*!\brief access help/about/credit files

  The files are sought in $DTECT_APPL/doc/DIR/xxx/ . The xxx is the 'scope'
  taken from the window ID (e.g. "myplugin" from "myplugin:1.2.3"). If
  no scope present, 'base' is used (which is the OpendTect manual).
  For user doc, DIR=User. For credits, DIR=Credits.
*/


namespace HelpViewer
{

mGlobal BufferString	getURLForWinID(const char* winid);
    			//!< Combines Link -> WinID -> URL

mGlobal BufferString	getCreditsURLForWinID(const char* winid);
    			//!< Finds link in the appropriate credits page


mGlobal BufferString	getLinkNameForWinID(const char*,const char*);
mGlobal BufferString	getURLForLinkName(const char*,const char*);
mGlobal BufferString	getCreditsFileName(const char* winid);
mGlobal bool		getCreditsData(const char* filenm,IOPar&);
mGlobal bool		hasSpecificCredits(const char* winid);
mGlobal const char*	getCreditsSpecificFileName(const char* winid);
mGlobal BufferString	getWebUrlFromLocal(const char*);

};

#endif
