#ifndef filegen_H
#define filegen_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	File utitlities
 RCS:		$Id: filegen.h,v 1.4 2000-03-07 08:37:24 bert Exp $
________________________________________________________________________

These functions deliver services related to files. In principle, they shield
from knowledge of the OS, and could so also be used on non-UNIX platforms.
Only UNIX is currently implemented, though.

-*/

#include <gendefs.h>

#ifdef __win__
# define sDirSep	"\\"
#else
# define sDirSep	"/"
#endif


#ifdef __cpp__
extern "C" {
#endif


/* Functions returning YES/true on success */
int	File_exists(const char*);
int	File_isEmpty(const char*);
int	File_isDirectory(const char*);
int	File_isAbsPath(const char*);
int	File_createDir(const char*,int mode /* 0755 when 0 passed */);
int	File_rename(const char* oldname,const char* newname);
int	File_copy(const char* from,const char* to,int recursive_downward);
int	File_link(const char* from,const char* to);
int	File_remove(const char*,int force,int recursive_downward);
int	File_makeWritable(const char*,int recursive,int yesno);

/*
The following functions return a pointer to the same static buffer, meaning you
cannot mix them directly!
*/
const char*	File_getFullPath(const char* pathname,const char* filename);
const char*	File_getPathOnly(const char* fullpath);
const char*	File_getFileName(const char* fullpath);
const char*	File_getTempFileName(const char* id_unique_for_process,
				     const char* extension,int full_path);
const char*	File_getBaseName(const char* filename);
		/* returns all extensions and path prefixes removed */


#ifdef __cpp__
}
#endif


#endif
