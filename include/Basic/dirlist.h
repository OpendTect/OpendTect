#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		18-4-1996
________________________________________________________________________

-*/

#include "basicmod.h"
#include "file.h"
#include "bufstringset.h"


/*!\brief Provides file and directory names in a certain directory. */

mExpClass(Basic) DirList : public BufferStringSet
{
public:

    typedef File::DirListType	DLType;

			DirList(const char*,DLType t=File::AllEntriesInDir,
				const char* msk=0);
				    /*!< msk should be a glob expression */

    void		update();

    DLType		type() const		{ return type_; }
    const char*		dirName() const		{ return dir_; }
    const char*		dirMask() const		{ return mask_; }
    const char*		fullPath(int) const;

private:

    DLType		type_;
    BufferString	dir_;
    BufferString	mask_;

public:

    enum		Type { AllEntries, FilesOnly, DirsOnly };
    mDeprecated		DirList(const char*,Type,const char* msk=0);

};
