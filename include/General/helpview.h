#ifndef helpview_h
#define helpview_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
-*/
 
#include "bufstring.h"


/*!\brief Pops up the help system

  The Help is sought in $DTECT_APPL/data/xxxDoc . The xxx is either the 'scope'
  usually taken from the window ID (e.g. "myplugin" from "myplugin:1.2.3"). If
  not present, applnm is used (which is by default "dTect").

  Thus, unscoped window IDs will by default be resolved in the dTectDoc/
  directory.

  */

class HelpViewer
{
public:

    static BufferString	getLinkNameForWinID(const char*);
    static BufferString	getURLForLinkName(const char*,const char* scope=0);

    static BufferString	getURLForWinID(const char*);
    			//!< Combines Link -> WinID -> URL

    static BufferString	basenm;
    			// Determines default scope= subdir from data/ directory
    static const char*	subdirNm(const char* scope=0);
    			// scope + "Doc" . If !scope || !*scope: applnm used
};

#endif
