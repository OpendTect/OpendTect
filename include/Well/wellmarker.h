#ifndef wellmarker_h
#define wellmarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellmarker.h,v 1.6 2009-01-06 10:57:11 cvsranojay Exp $
________________________________________________________________________


-*/

#include "namedobj.h"
#include "color.h"

namespace Well
{

mClass Marker : public ::NamedObject
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
