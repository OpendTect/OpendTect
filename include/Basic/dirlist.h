#ifndef dirlist_H
#define dirlist_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		18-4-1996
 RCS:		$Id: dirlist.h,v 1.3 2003-10-17 14:19:00 bert Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"


/*!\brief provides file and directory names in a certain directory.  */

class DirList : public BufferStringSet
{
public:
			DirList(const char*,int dirindic=0);
				/*!< dirindic > 0: only directories
				     dirindic < 0: no directories */

    void		update();
    const char*		dirName() const		{ return dir; }

private:

    FileNameString	dir;
    int			indic;

};


#endif
