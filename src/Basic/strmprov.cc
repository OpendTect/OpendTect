/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream Provider functions
-*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream.h>
#include <stdiostream.h>
#include "strmprov.h"
#include "filegen.h"
#include "string2.h"
#include "binidsel.h"
#include "strmoper.h"

static const char* rcsID = "$Id: strmprov.cc,v 1.1 2000-03-02 15:29:00 bert Exp $";

static FixedString<1024> oscommand;
#define exeCmd(comm) system((const char*)comm) ? NO : YES

DefineClassID(StreamProvider);
static const char* sStdIO = "$std$IO$";


void StreamData::close()
{
    if ( fp && fp != stdin && fp != stdout && fp != stderr )
	if ( ispipe ) pclose(fp); else fclose(fp);
    if ( sb )
	delete sb;
    if ( istrm && istrm != &cin )
	delete istrm;
    if ( ostrm && ostrm != &cout && ostrm != &cerr )
	delete ostrm;

    init();
}


void StreamData::init()
{
    fp = 0; sb = 0; istrm = 0; ostrm = 0;
}


int StreamData::usable() const
{
    if		( !istrm && !ostrm )				return NO;
    else if	( istrm && istrm->fail() )			return NO;
    else if	( ostrm && (ostrm->fail() || ostrm->eof()) )	return NO;
    else if	( sb && !fp )					return NO;

    return YES;
}


StreamProvider::StreamProvider( const char* devname )
{
    set( devname );
}


StreamProvider::StreamProvider( const char* hostnm, const char* fnm,
				StreamConn::Type typ )
	: isbad(NO)
	, type_(typ)
	, hostname(hostnm)
	, fname(fnm?fnm:sStdIO)
{
    if ( fname == "" ) isbad = YES;
}


void StreamProvider::set( const char* devname )
{
    type_ = StreamConn::File;
    isbad = NO;
    blocksize = 0;

    if ( !devname || !strcmp(devname,"stdin") || !strcmp(devname,"stdout") )
    {
	type_ = StreamConn::File;
	hostname = ""; fname = sStdIO;
	return;
    }
    else if ( !*devname )
    {
	isbad = YES;
	return;
    }

    char* ptr = (char*)devname;
    if ( *ptr == '@' ) { type_ = StreamConn::Command; ptr++; }
    fname = ptr;

    // separate hostname from filename
    ptr = strchr( (char*)fname, ':' );
    if ( ptr )
    {
	*ptr++ = '\0';
	hostname = fname;
    }
    else
	ptr = fname;

    if ( *ptr == '@' ) { type_ = StreamConn::Command; ptr++; }

    char* ptrname = fname;
    while ( *ptr ) *ptrname++ = *ptr++;
    *ptrname = '\0';

    if ( type_ != StreamConn::Command && matchString( "/dev/", fname ) )
	type_ = StreamConn::Device;
}


int StreamProvider::isNormalFile() const
{
    return type_ == StreamConn::File && hostname == "";
}


int StreamProvider::rewind() const
{
    if ( isbad ) return NO;
    else if ( type_ != StreamConn::Device ) return YES;

    if ( hostname[0] )
	sprintf( oscommand, "rsh %s \"mt -f %s rewind\"", (const char*)hostname,
			  (const char*)fname );
    else
	sprintf( oscommand, "mt -f %s rewind", (const char*)fname );
    return exeCmd(oscommand);
}


int StreamProvider::offline() const
{
    if ( isbad ) return NO;
    else if ( type_ != StreamConn::Device ) return YES;

    if ( hostname[0] )
	sprintf( oscommand, "rsh %s \"mt -f %s offline\"",
			    (const char*)hostname, (const char*)fname );
    else
	sprintf( oscommand, "mt -f %s offline", (const char*)fname );
    return exeCmd(oscommand);
}


int StreamProvider::skipFiles( int nr ) const
{
    if ( isbad ) return NO;
    if ( type_ != StreamConn::Device ) return NO;

    if ( hostname[0] )
	sprintf( oscommand, "rsh %s \"mt -f %s fsf %d\"", (const char*)hostname,
			  (const char*)fname, nr );
    else
	sprintf( oscommand, "mt -f %s fsf %d", (const char*)fname, nr );
    return exeCmd(oscommand);
}


const char* StreamProvider::fullName() const
{
    oscommand = "";
    if ( type_ == StreamConn::Command )
	oscommand += "@";
    if ( hostname[0] ) 
    {
	oscommand += hostname;
	oscommand += ":";
    }
    oscommand += fname;

    return oscommand;
}


void StreamProvider::addPathIfNecessary( const char* path )
{
    if ( isbad ) return;
    if ( type_ != StreamConn::File || !path || ! *path
      || File_isAbsPath(fname) )
	return;
    else if ( fname == sStdIO ) return;

    FileNameString pth( path );
    fname = File_getFullPath( pth, fname );
}


StreamData StreamProvider::makeIStream() const
{
    StreamData sd;
    if ( isbad ) return sd;
    if ( !*(const char*)fname ) return sd;

    if ( type_ == StreamConn::Command || hostname[0] )
    {
	if ( hostname[0] )
	{
	    switch ( type_ )
	    {
	    case StreamConn::Device:
		if ( blocksize )
		    sprintf( oscommand, "rsh %s dd if=%s ibs=%ld",
				(const char*)hostname, (const char*)fname,
				blocksize );
		else
		    sprintf( oscommand, "rsh %s dd if=%s",
				(const char*)hostname, (const char*)fname );
	    break;
	    case StreamConn::Command:
		sprintf( oscommand, "rsh %s %s", (const char*)hostname,
					(const char*)fname );
	    break;
	    case StreamConn::File:
		sprintf( oscommand, "rsh %s cat %s", (const char*)hostname,
					(const char*)fname );
	    break;
	    }
	}
	else
	    strcpy( oscommand, (const char*)fname );

	sd.fp = popen( oscommand, "r" );
	sd.ispipe = YES;
	if ( sd.fp )
	{
	    sd.sb = new stdiobuf( sd.fp );
	    sd.istrm = new istream( sd.sb );
	}
    }
    else
    {
	if ( fname == sStdIO )
	    sd.istrm = &cin;
	else if ( File_exists(fname) )
	    sd.istrm = new ifstream( fname );
    }

    return sd;
}


StreamData StreamProvider::makeOStream() const
{
    StreamData sd;
    if ( isbad ||  !*(const char*)fname ) return sd;

    if ( type_ == StreamConn::Command || hostname[0] )
    {
	if ( hostname[0] )
	{
	    switch ( type_ )
	    {
	    case StreamConn::Device:
		if ( blocksize )
		    sprintf( oscommand, "rsh %s dd of=%s obs=%ld",
				      (const char*)hostname, (const char*)fname,
					blocksize );
		else
		    sprintf( oscommand, "rsh %s dd of=%s",
				    (const char*)hostname, (const char*)fname );
	    break;
	    case StreamConn::Command:
		sprintf( oscommand, "rsh %s %s", (const char*)hostname,
					(const char*)fname );
	    break;
	    case StreamConn::File:
		sprintf( oscommand, "rsh %s tee %s > /dev/null",
				  (const char*)hostname, (const char*)fname );
	    break;
	    }
	}
	else
	    strcpy( oscommand, (const char*)fname );

	sd.fp = popen( oscommand, "w" );
	sd.ispipe = YES;
	if ( sd.fp )
	{
	    sd.sb = new stdiobuf( sd.fp );
	    sd.ostrm = new ostream( sd.sb );
	}
    }
    else
    {
	if ( fname == sStdIO )
	    sd.ostrm = &cout;
	else
	    sd.ostrm = new ofstream( fname );
    }

    return sd;
}


int StreamProvider::exists( int fr ) const
{
    if ( isbad ) return NO;

    if ( type_ == StreamConn::Command )
	return fr;

    if ( hostname[0] )
    {
	sprintf( oscommand, "rsh %s 'test -%c %s && echo 1'",
		  (const char*)hostname, fr ? 'r' : 'w', (const char*)fname );
	FILE* fp = popen( oscommand, "r" );
	int i = 0;
	fscanf( fp, "%d", &i );
	pclose( fp );
	return i;
    }
    else
	return fname == sStdIO ? YES : File_exists( (const char*)fname );
}


int StreamProvider::remove() const
{
    if ( isbad || type_ != StreamConn::File ) return NO;

    if ( !hostname[0] )
	return fname == sStdIO ? NO :
			File_remove( (const char*)fname, YES, YES );
    else
    {
	sprintf( oscommand, "rsh %s '/bin/rm -f %s && echo 1'",
		  (const char*)hostname, (const char*)fname );
	FILE* fp = popen( oscommand, "r" );
	char c;
	fscanf( fp, "%c", &c );
	pclose( fp );
	return c == '1';
    }
}
