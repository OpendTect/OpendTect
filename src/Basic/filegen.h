#ifndef filegen_H
#define filegen_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	File utitlities
 RCS:		$Id: filegen.h,v 1.2 1999-10-18 14:02:43 dgb Exp $
________________________________________________________________________

These functions deliver services related to files. In principle, they shield
from knowledge of the OS, and could so also be used on non-UNIX platforms.
Only UNIX is currently implemented, though.

@$*/

#include <gendefs.h>

#define sDirSep		"/"


#ifdef __cpp__
extern "C" {
#endif


int		File_exists		Pargs( (const char*) );
int		File_isEmpty		Pargs( (const char*) );
int		File_isDirectory	Pargs( (const char*) );
int		File_isAbsPath		Pargs( (const char*) );
int		File_createDir		Pargs( (const char*,int) );
int		File_rename		Pargs( (const char*,const char*) );
int		File_copy		Pargs( (const char*,const char*,int) );
int		File_link		Pargs( (const char*,const char*) );
int		File_remove		Pargs( (const char*,int,int) );

/*$@
The following functions return a pointer to a {\b static buffer}!
@$*/
const char*	File_getFullPath	Pargs( (const char*,const char*) );
const char*	File_getPathOnly	Pargs( (const char*) );
const char*	File_getFileName	Pargs( (const char*) );
const char*	File_getBaseName	Pargs( (const char*) );
const char*	File_getTempFileName	Pargs( (const char*,const char*,int) );


#ifdef __cpp__
}
#endif


/*$-*/
#endif
