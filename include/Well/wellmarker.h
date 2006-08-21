#ifndef wellmarker_h
#define wellmarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellmarker.h,v 1.4 2006-08-21 17:14:45 cvsbert Exp $
________________________________________________________________________


-*/

#include "namedobj.h"
#include "color.h"

namespace Well
{

class Marker : public ::NamedObject
{
public:

			Marker( const char* nm= 0 )
			: ::NamedObject(nm), dah(0), istop(true)
						{}

    BufferString	desc;
    float		dah;
    bool		istop;
    Color		color;

    static const char*	sKeyDah;
};


}; // namespace Well

#endif
