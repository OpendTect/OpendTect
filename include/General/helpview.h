#ifndef helpview_h
#define helpview_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		18-8-2000
 RCS:		$Id: helpview.h,v 1.13 2009-01-20 04:38:46 cvsranojay Exp $
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


};

#endif
