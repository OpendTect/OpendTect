#ifndef wellfmtop_h
#define wellfmtop_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellmarker.h,v 1.1 2003-08-15 11:12:15 bert Exp $
________________________________________________________________________


-*/

#include "uidobj.h"
#include "color.h"

namespace Well
{

class Marker : public ::UserIDObject
{
public:

			Marker( const char* nm= 0 )
			: ::UserIDObject(nm), dah(0), istop(true)
						{}

    BufferString	desc;
    float		dah;
    bool		istop;
    Color		color;

};


}; // namespace Well

#endif
