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

    static void		use(const char* url);
    static BufferString	getURLForWinID(const char*);

};


#endif
