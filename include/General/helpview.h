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


#include "generalmod.h"
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

mGlobal(General) void		init();
    
mGlobal(General) BufferString	getURLForWinID(const char* winid);
    			//!< Combines Link -> WinID -> URL

mGlobal(General) BufferString	getCreditsURLForWinID(const char* winid);
    			//!< Finds link in the appropriate credits page


mGlobal(General) BufferString	getLinkNameForWinID(const char*,const char*);
mGlobal(General) BufferString	getURLForLinkName(const char*,const char*);
mGlobal(General) BufferString	getCreditsFileName(const char* winid);
mGlobal(General) bool		getCreditsData(const char* filenm,IOPar&);
mGlobal(General) bool		hasSpecificCredits(const char* winid);
mGlobal(General) const char*	getCreditsSpecificFileName(const char* winid);
mGlobal(General) BufferString	getWebUrlFromLocal(const char*);

};

#endif

