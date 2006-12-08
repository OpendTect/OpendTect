#ifndef dirlist_H
#define dirlist_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		18-4-1996
 RCS:		$Id: dirlist.h,v 1.6 2006-12-08 13:58:42 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"


/*!\brief provides file and directory names in a certain directory.  */

class DirList : public BufferStringSet
{
public:

    enum Type		{ AllEntries, FilesOnly, DirsOnly };

			DirList(const char*,Type t=AllEntries,
				const char* msk=0);
				/*!< msk can be a glob expression */

    void		update();

    Type		type() const		{ return type_; }
    const char*		dirName() const		{ return dir_; }
    const char*		dirMask() const		{ return mask_; }
    const char*		fullPath(int) const;

private:

    Type		type_;
    BufferString	dir_;
    BufferString	mask_;

};


#endif
