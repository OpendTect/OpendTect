#ifndef dirlist_H
#define dirlist_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		18-4-1996
 RCS:		$Id: dirlist.h,v 1.2 2001-02-13 17:15:45 bert Exp $
________________________________________________________________________

-*/

#include <uidobjset.h>


/*!\brief provides file and directory names in a certain directory.  */

class DirList : public UserIDObjectSet<UserIDObject>
{
public:
			DirList(const char*,int dirindic=0);
				/*!< dirindic > 0: only directories
				     dirindic < 0: no directories */

			~DirList();

    void		update();
    const char*		dirName() const		{ return dir; }

private:

    FileNameString	dir;
    int			indic;

};


#endif
