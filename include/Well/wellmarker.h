#ifndef wellmarker_h
#define wellmarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellmarker.h,v 1.2 2003-08-18 16:37:23 bert Exp $
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

    static const char*	sKeyDah;
};


}; // namespace Well

#endif
