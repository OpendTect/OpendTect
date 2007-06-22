#ifndef maddefs_h
#define maddefs_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
 * ID       : $Id: maddefs.h,v 1.1 2007-06-22 12:07:19 cvsbert Exp $
-*/

#include "gendefs.h"


class MadagascarDefs
{
public:

			MadagascarDefs(const char* rsfroot=0);
			~MadagascarDefs();

protected:

    BufferString	rsfroot_;
    BufferString	defdir;

};


#endif
