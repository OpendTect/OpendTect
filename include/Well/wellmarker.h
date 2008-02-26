#ifndef wellmarker_h
#define wellmarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellmarker.h,v 1.5 2008-02-26 09:17:36 cvsnanne Exp $
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
			: ::NamedObject(nm), dah_(0), istop_(true)
			, stratlevelid_(-1)
						{}

    BufferString	desc_;
    float		dah_;
    bool		istop_;
    Color		color_;
    int			stratlevelid_;

    static const char*	sKeyDah;
};


}; // namespace Well

#endif
