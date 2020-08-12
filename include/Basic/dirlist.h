#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		18-4-1996
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"


/*!
\brief Provides file and directory names in a certain directory.
*/

mExpClass(Basic) DirList : public BufferStringSet
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


