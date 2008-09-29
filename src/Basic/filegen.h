#ifndef filegen_H
#define filegen_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	File utitlities
 RCS:		$Id: filegen.h,v 1.19 2008-09-29 13:23:47 cvsbert Exp $
________________________________________________________________________

-*/

#include <gendefs.h>


#ifdef __cpp__
extern "C" {
#endif

/*!
These functions deliver services related to files. In principle, they shield
from knowledge of the OS, and could so also be used on non-UNIX platforms.
Only UNIX is currently fully implemented, most notably recursive copy/remove
and links are lacking on windows.
*/

/*! Queries returning mC_True or mC_False */
int	File_exists(const char*);
int	File_isEmpty(const char*);
int	File_isDirectory(const char*);
int	File_isLink(const char*);
int	File_isRemote(const char*);
int	File_isWritable(const char*);

/*! Functions returning mC_True on success */
int	File_createDir(const char*,int mode /* 0755 when 0 passed */);
int	File_rename(const char* oldname,const char* newname);
int	File_createLink(const char* from,const char* to);
int	File_copy(const char* from,const char* to,int recursive_downward);
int	File_remove(const char*,int recursive_downward);
int	File_makeWritable(const char*,int recursive,int yesno);
#define mFile_NotRecursive 0
#define mFile_Recursive 1

/*! Size/disk space. Returns 0 on any error */
int	File_getKbSize(const char*);
int	File_getFreeMBytes(const char*);

const char*	File_getTime(const char*);

/*! Functions returning path */
const char*	File_linkTarget(const char* linkname);
		/* returns what a symbolic link points to */
const char*	File_getCurrentDir();


#ifdef __cpp__
}
#endif


#endif
