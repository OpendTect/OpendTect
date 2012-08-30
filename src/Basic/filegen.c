/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sept 1993
 * FUNCTION : file utilities
-*/

static const char* rcsID mUnusedVar = "$Id: filegen.c,v 1.93 2012-08-30 09:49:44 cvskris Exp $";

#include "filegen.h"
#include "string2_c.h"
#include "genc.h"
#include "mallocdefs.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#ifndef __msvc__
# include <dirent.h>
#endif

#ifdef __win__
# include <direct.h>
# include <windows.h>
# include <shlwapi.h>
# include <io.h>

# ifndef __msvc__
#  include <largeint.h>
# endif

# define stat _stati64
# define fstat _fstat
# define access _access

#else

# include <unistd.h>

# ifdef __lux__

#  include <sys/statfs.h>
#  define mStatFS statfs

# else

#  ifdef __mac__
#   include <sys/mount.h>
#   define mStatFS statfs
#  else
#   include <sys/statvfs.h>
#   define mStatFS statvfs
#  endif
# endif

 static struct mStatFS fsstatbuf;

#endif

static struct stat statbuf;


int File_exists( const char* fname )
{
#ifdef __win__
    return fname && *fname && PathFileExists( fname );
#else
    return fname && *fname && stat(fname,&statbuf) >= 0 ? mC_True : mC_False;
#endif
}


int File_isEmpty( const char* fname )
{
    if( !File_exists( fname ) ) return mC_True;
#ifdef __msvc__
    //pErrMsg("File_isEmpty not fully implemented!");
    return mC_False;
#else
    return stat(fname,&statbuf) < 0 || statbuf.st_size < 1 ? mC_True : mC_False;
#endif
}


int File_isDirectory( const char* dirname )
{
#ifdef __win__
    return PathIsDirectory( dirname );
#else
    if ( !dirname || !*dirname || !File_exists(dirname) ) return 0;
    return S_ISDIR(statbuf.st_mode);
#endif
}


int File_isRemote( const char* fname )
{
#ifdef __win__
    return mC_False;
#else
    if ( !File_exists(fname)
      || mStatFS(fname,&fsstatbuf) )
	return mC_False;

# ifdef __lux__
    /* return fsstatbuf.f_type == NFS_SUPER_MAGIC
	|| fsstatbuf.f_type == SMB_SUPER_MAGIC; */
    return fsstatbuf.f_type == 0x6969 || fsstatbuf.f_type == 0x517B;
# else
#  ifdef __mac__
#ifdef __debug__
	fprintf(stderr,"File_IsRemote untested. Please verify");
#endif
    return fsstatbuf.f_fstypename[0] == 'n' && fsstatbuf.f_fstypename[1] == 'f';
#  else
    return fsstatbuf.f_basetype[0] == 'n' && fsstatbuf.f_basetype[1] == 'f';
#  endif
# endif
#endif
}

#define mToKbFac (1.0 / 1024.0)

int File_getFreeMBytes( const char* dirnm )
{
#ifdef __win__
    ULARGE_INTEGER freeBytesAvail2User; 
    ULARGE_INTEGER totalNrBytes;
    ULARGE_INTEGER totalNrFreeBytes;
#endif 

    double fac = mToKbFac;
    double res;

    if ( !File_exists(dirnm) )
	return 0;

#ifdef __win__
    GetDiskFreeSpaceEx( dirnm, &freeBytesAvail2User,
			&totalNrBytes, &totalNrFreeBytes);

    res = freeBytesAvail2User.LowPart * fac * fac;
    res += ((double)freeBytesAvail2User.HighPart) * 2048;
#else

    if ( mStatFS(dirnm,&fsstatbuf) )
	return 0;

    res = fac * fac		/* to MB */
	* fsstatbuf.f_bavail	/* available blocks */
#ifdef __lux__
	* fsstatbuf.f_bsize;	/* block size */
#else
# ifdef __mac__
	* fsstatbuf.f_bsize;	/* fundamental file system block size */
# else
	* fsstatbuf.f_frsize;	/* 'real' block size */
# endif
#endif

#endif

    return (int)(res + .5);
}


int File_getKbSize( const char* fnm )
{
    double res = mToKbFac;

    if ( !File_exists(fnm) )
	return 0;
#ifdef __win__
    stat((char*)fnm,&statbuf);
#endif
    res *= statbuf.st_size;
    if ( res < 0 )
	res = - res * 2;
    return (int)(res + .5);

}


od_int64 File_getTimeInSeconds( const char* fnm )
{
    if ( !File_exists(fnm) )
	return -1;
#ifdef __win__
    stat((char*)fnm,&statbuf);
#endif
    return (od_int64)statbuf.st_mtime;
}


const char* File_getTime( const char* fnm )
{
    static char buf[64];
   
#ifdef __win__
    HANDLE hfile;
    FILETIME ftCreate, ftAccess, ftWrite;
    SYSTEMTIME stUTC, stLocal;

    // get the file from string
    const int len = strlen( fnm );
    TCHAR* fname1 = (TCHAR*) malloc( (len+1) * sizeof(TCHAR) );
    int i;
#endif

    if ( !File_exists(fnm) )
	return 0;

 #ifdef __win__

    for ( i=0; i<len; i++ )
	fname1[i] = (TCHAR)fnm[i];

    fname1[len] = (TCHAR) '\0';

    // Opening the existing file
    hfile = CreateFile(fname1, GENERIC_READ, FILE_SHARE_READ, NULL,
	    		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if ( hfile == INVALID_HANDLE_VALUE )
    {
	CloseHandle( hfile );
	return 0;
    }

    if ( !GetFileTime(hfile,&ftCreate,&ftAccess,&ftWrite) )
    {
	CloseHandle( hfile );
	return 0;
    }

    // convert the created time to local time
    FileTimeToSystemTime( &ftWrite, &stUTC );
    SystemTimeToTzSpecificLocalTime( NULL, &stUTC, &stLocal);

    sprintf( buf, "%02d-%02d-%d %02d:%02d", stLocal.wDay, stLocal.wMonth,
	     stLocal.wYear, stLocal.wHour, stLocal.wMinute );
    CloseHandle( hfile );
    return buf;

#else

    ctime_r( &statbuf.st_mtime, buf );
    return buf;

#endif
}


int File_isWritable( const char* fnm )
{
    int acc;
    if ( !File_exists(fnm) )
	return 0;

#ifdef __msvc__
     acc = access( fnm, 02 );
#else
     acc = access( fnm, W_OK );
#endif
    return !acc;
}


int File_createDir( const char* dirname, int mode )
{
    if ( !dirname || !*dirname ) return mC_False;
    if ( mode == 0 ) mode = 0755;

#ifdef __win__
    return CreateDirectory( dirname, 0 );
#else
    return mkdir( dirname, (mode_t)mode ) < 0 ? mC_False : mC_True;
#endif
}


#define mRet(v) { mFREE(cmd); return v; }

int File_rename( const char* from, const char* to )
{
    int rv, len;
    char* cmd = 0;

    if ( !from || !*from || !to || !*to ) return mC_False;
    if ( !File_exists(from) ) return mC_True;

#ifdef __win__

    rv = MoveFile( from, to );

# ifdef __debug__
    if( !rv )
    {

	LPVOID lpMsgBuf;

	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	    NULL,
	    GetLastError(),
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL 
	);

	// Display the string.
	MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

	// Free the buffer.
	LocalFree( lpMsgBuf );
	 
    }
# endif

    if ( rv>0 )
	rv = File_exists( to );

    return rv;

#else

    if ( !rename(from,to) )
    {
	if ( File_exists(to) )
	    return mC_True;
	else if ( !File_exists(from) )
	    return mC_False;
    }

    // Failed. May be to other disk. Let's try again via shell.
    len = strlen( from ) + strlen( to ) + 25;
    cmd = mMALLOC(len,char);
    
    strcpy( cmd, "/bin/mv -f '" );
    strcat( cmd, from );
    strcat( cmd, "' '" );
    strcat( cmd, to );
    strcat( cmd, "'" );

    rv = system( cmd ) != -1 ? mC_True : mC_False;
    if ( rv )
	rv = File_exists( to );
    mRet( rv )

#endif
}


int File_copy( const char* from, const char* to, int recursive )
{
#ifdef __win__
    char* cmd = 0;
    int len, retval;

    if ( !from || !*from || !to || !*to ) return mC_False;
    if ( !File_exists(from) ) return mC_True;

    if ( recursive )
    { 
	if ( !from || !*from || !to || !*to ) return mC_False;
	if ( !File_exists(from) ) return mC_True;
	if ( File_exists(to) ) return mC_False;

	len = strlen( from ) + strlen( to ) + 128;
	cmd = mMALLOC(len,char);
	
	strcpy( cmd, "xcopy /E /I /Q /H " );
	strcat( cmd, " \"" );
	strcat( cmd, from );
	strcat( cmd, "\" \"" );
	strcat( cmd, to );
	strcat( cmd, "\"" );

	retval = system( cmd ) != -1 ? mC_True : mC_False;
	if ( retval ) retval = File_exists( to );
	mFREE(cmd);
	return retval;
    }

    return CopyFile( from, to, FALSE );

#else

    char* cmd = 0;
    int len, retval;
    if ( !from || !*from || !to || !*to ) return mC_False;
    if ( !File_exists(from) ) return mC_True;

    len = strlen( from ) + strlen( to ) + 25;
    cmd = mMALLOC(len,char);
    
    strcpy( cmd, "/bin/cp " );
    if ( recursive ) strcat( cmd, "-r " );
    strcat( cmd, " '" );
    strcat( cmd, from );
    strcat( cmd, "' '" );
    strcat( cmd, to );
    strcat( cmd, "'" );

    retval = system( cmd ) != -1 ? mC_True : mC_False;
    if ( retval ) retval = File_exists( to );
    mRet( retval )

#endif
}


int File_remove( const char* fname, int recursive )
{
#ifdef __win__

    if ( !File_exists(fname) )
	return mC_True;

    if ( File_isDirectory(fname) )
    { 
	char* cmd;
	int len, retval;

	len = strlen( fname ) + 30;
	cmd = mMALLOC(len,char);

	strcpy( cmd, "rd /Q" );
	if ( recursive )
	    strcat( cmd, " /S" );
	strcat( cmd, " \"" );
	strcat( cmd, fname );
	strcat( cmd, "\"" );

	retval = system( cmd ) ? mC_False : mC_True;

	if ( retval && File_exists( fname ) ) retval = mC_False;
	mFREE(cmd);
	return retval;
    }

    return DeleteFile( fname );

#else

    char* cmd = 0;
    char* ptr;
    int len, retval;
    FileNameString targ_fname;

    if ( !File_exists(fname) )
	return mC_True;

    if ( !recursive )
	return unlink((char*)fname) ? mC_False : mC_True;

    len = strlen( fname ) + 30;
    cmd = mMALLOC(len,char);
    strcpy( cmd, "/bin/rm -rf '" );

    if ( File_isLink(fname) ) 
    {
	strcpy( targ_fname, File_linkTarget(fname) );
        if ( File_exists( targ_fname ) ) 
	{
	    ptr = cmd + strlen(cmd);
	    strcat( cmd, targ_fname );
	    strcat( cmd, "'" );
	    if ( system(cmd)<0 || File_exists(targ_fname) )
		mRet(mC_False)
	    *ptr = '\0';
	}
    }

    strcat( cmd, fname );
    strcat( cmd, "'" );
    retval = system( cmd ) ? mC_False : mC_True;
    if ( retval && File_exists( fname ) ) retval = mC_False;
    mRet( retval )

#endif
}


int File_setPermissions( const char* fname, const char* perms, int recursive )
{
#ifndef __win__

    FileNameString cmd;
    strcpy( cmd, "chmod " );
    if ( recursive ) strcat( cmd, "-R ");
    strcat( cmd, perms );
    strcat( cmd, " " );
    strcat( cmd, fname );
    if ( system(cmd) != -1 )
	return mC_True;

#endif

    return mC_False;
}


int File_makeWritable( const char* fname, int recursive, int yn )
{
#ifdef __win__

    FileNameString cmd;
    strcpy( cmd, "attrib " );
    strcat( cmd, yn ? " -R " : " +R " );
    strcat( cmd, "\"" );
    strcat( cmd, fname );
    strcat( cmd, "\"" );
    if ( recursive && File_isDirectory(fname) ) strcat( cmd, "\\*.* /S ");
    return system( cmd ) != -1 ? mC_True : mC_False;

#else

    FileNameString cmd;
    strcpy( cmd, "chmod " );
    if ( recursive ) strcat( cmd, "-R ");
    strcat( cmd, yn ? "ug+w '" : "a-w '" );
    strcat( cmd, fname );
    strcat( cmd, "'" );
    return system( cmd ) != -1 ? mC_True : mC_False;

#endif
}


int File_isLink( const char* fname )
{
#ifdef __win__

    FileNameString fnm;
    if ( !fname || !*fname ) return 0;
    if ( strstr(fname,".lnk") || strstr(fname,".LNK") )
	return mC_True;

    strcpy( fnm, fname ); strcat( fnm, ".lnk" );
    if ( File_exists(fnm) ) return mC_True;
    strcpy( fnm, fname ); strcat( fnm, ".LNK" );
    return File_exists(fnm);

#else
    return fname && lstat(fname,&statbuf) >= 0 && S_ISLNK(statbuf.st_mode)
	 ? mC_True : mC_False;
#endif
}


int File_createLink( const char* from, const char* to )
{
#ifdef __win__
    return mC_False;
#else
    char cmd[512];
    if ( !from || !to || !*from || !*to ) return mC_False;

    strcpy( cmd, "ln -s '" );
    strcat( cmd, from );
    strcat( cmd, "' '" );
    strcat( cmd, to );
    strcat( cmd, "'" );
    if ( system( cmd ) == -1 ) return -1;
    return File_isLink( to );
#endif
}


const char* File_linkTarget( const char* fname )
{
#ifndef __win__
    static FileNameString pathbuf;
    return File_isLink(fname) && readlink(fname,pathbuf,256) != -1
	 ? pathbuf : fname;
#endif
    return fname;
}


const char* File_getCurrentDir()
{
    static FileNameString pathbuf;
    getcwd( pathbuf, mMaxFilePathLength );
    return pathbuf;
}
