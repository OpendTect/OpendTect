/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 11-4-1994
 * FUNCTION : print errno
-*/

static const char* rcsID = "$Id: errnomsg.c,v 1.6 2012/04/13 12:43:29 cvsbert Exp $";

#include "gendefs.h"

#include <stdio.h>

#ifndef __win__

#include <errno.h>

#ifdef sun5
static const char* msgs[] = { "",
"Not owner", "No such file or directory", "No such process",
"Interrupted system call", "I/O error", "No such device or address",
"Arg list too long", "Exec format error", "Bad file number", "No children",
"No more processes", "Not enough core", "Permission denied", "Bad address",
"Block device required", "Mount device busy", "File exists",
"Cross-device link", "No such device", "Not a directory", "Is a directory",
"Invalid argument", "File table overflow", "Too many open files",
"Not a typewriter", "Text file busy", "File too large",
"No space left on device", "Illegal seek", "Read-only file system",
"Too many links", "Broken pipe", "Argument too large", "Result too large",
"Operation would block", "Operation now in progress",
"Operation already in progress", "Socket operation on non-socket",
"Destination address required", "Message too long",
"Protocol wrong type for socket", "Protocol not available",
"Protocol not supported", "Socket type not supported",
"Operation not supported on socket", "Protocol family not supported",
"Address family not supported by protocol family", "Address already in use",
"Cannot assign requested address", "Network is down", "Network is unreachable",
"Network dropped connection on reset", "Software caused connection abort",
"Connection reset by peer", "No buffer space available",
"Socket is already connected", "Socket is not connected",
"Cannot send after socket shutdown", "Too many references: cannot splice",
"Connection timed out", "Connection refused",
"Too many levels of symbolic links", "File name too long", "Host is down",
"No route to host", "Directory not empty", "Too many processes",
"Too many users", "Disc quota exceeded", "Stale NFS file handle",
"Too many levels of remote in path", "Device is not a stream",
"Timer expired", "Out of streams resources", "No message of desired type",
"Trying to read unreadable message", "Identifier removed",
"Deadlock condition.", "No record locks available.",
"Machine is not on the network", "Object is remote",
"the link has been severed", "advertise error", "srmount error",
"Communication error on send", "Protocol error", "multihop attempted",
"Cross mount point (not an error)", "Remote address changed",
"function not implemented",
0 };

#endif

#endif



const char* errno_message();
const char* errno_message()
{
    static char buf[80];
    sprintf( buf,

#ifdef sun5

	"%s (errno=%d)", msgs[errno], errno );

#else

# ifdef __win__

	"" );

# else

	"errno=%d", errno );

# endif

#endif

    return buf;
}
