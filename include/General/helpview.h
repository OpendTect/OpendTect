#ifndef helpview_h
#define helpview_h
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
-*/
 
#include <bufstring.h>


class HelpViewer
{
public:

    static BufferString	getLinkNameForWinID(const char*);
    static BufferString	getURLForLinkName(const char*);

    static BufferString	getURLForWinID(const char*);
    			//!< Combines Link -> WinID -> URL

    static void		use(const char* url,const char* wintitl=0);
    			//!< Pops up help viewer for URL.
    			//!< Jan 2003: no real internet-URLs yet, just filenames
    			//!< Pass url==null for main help index for app.

};

#endif
