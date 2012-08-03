#ifndef filegen_h
#define filegen_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	File utitlities
 RCS:		$Id: filegen.h,v 1.28 2012-08-03 13:01:34 cvskris Exp $
________________________________________________________________________

-*/

#include "gendefs.h"


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
mGlobal( Basic ) int File_exists(const char*);
mGlobal( Basic ) int File_isEmpty(const char*);
mGlobal( Basic ) int File_isDirectory(const char*);
mGlobal( Basic ) int File_isLink(const char*);
mGlobal( Basic ) int File_isRemote(const char*);
mGlobal( Basic ) int File_isWritable(const char*);

/*! Functions returning mC_True on success */
mGlobal( Basic ) int File_createDir(const char*,int mode /* 0755 when 0 passed */); 
mGlobal( Basic ) int File_rename(const char* oldname,const char* newname);
mGlobal( Basic ) int File_createLink(const char* from,const char* to);
mGlobal( Basic ) int File_copy(const char* from,const char* to,int recursive_downward);
mGlobal( Basic ) int File_remove(const char*,int recursive_downward);
mGlobal( Basic ) int File_makeWritable(const char*,int recursive,int yesno);
mGlobal( Basic ) int File_setPermissions(const char*,const char* perms,int recursive);
						// Not for Windows
#define mFile_NotRecursive 0
#define mFile_Recursive 1

/*! Size/disk space. Returns 0 on any error */
mGlobal( Basic ) int File_getKbSize(const char*);
mGlobal( Basic ) int File_getFreeMBytes(const char*);

/*! Modification time. The seconds are for compare, getTime for user display. */
mGlobal( Basic ) const char*	File_getTime(const char*);
mGlobal( Basic ) od_int64 	File_getTimeInSeconds(const char*);

/*! Functions returning path */
mGlobal( Basic ) const char* File_linkTarget(const char* linkname);
		/* returns what a symbolic link points to */
mGlobal( Basic ) const char* File_getCurrentDir(void);


#ifdef __cpp__
}
#endif


#endif
