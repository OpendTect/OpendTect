#ifndef wellmarker_h
#define wellmarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellmarker.h,v 1.3 2003-11-07 12:21:52 bert Exp $
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
