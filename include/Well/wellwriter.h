#ifndef wellwriter_h
#define wellwriter_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellwriter.h,v 1.1 2003-08-15 15:29:22 bert Exp $
________________________________________________________________________


-*/

#include "wellio.h"
#include <iosfwd>

namespace Well
{
class Data;


class Writer : public IO
{
public:

			Writer(const char* fnm,const Data&);

protected:

    const Data&		wd;

};

}; // namespace Well

#endif
