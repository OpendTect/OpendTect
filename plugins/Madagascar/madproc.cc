/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
-*/


#include "madproc.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "madio.h"
#include "string2.h"
#include <cctype>
#include <string.h>

bool ODMad::Proc::progExists( const char* prog )
{
    const BufferString rsfroot = GetEnvVar( "RSFROOT" );
    if ( !rsfroot || !*rsfroot ) return false;

    File::Path fp( rsfroot, "bin", prog );
#if __win__
    fp.setExtension( "exe" );
#endif
    return File::exists( fp.fullPath() );
}


ODMad::Proc::Proc( const char* cmd, const char* auxcmd )
    : isvalid_(false)
    , inptype_(ODMad::Proc::Madagascar)
    , outptype_(ODMad::Proc::Madagascar)
{
    makeProc( cmd, auxcmd );
}


void ODMad::Proc::makeProc( const char* cmd, const char* auxcmd )
{
    if ( auxcmd && *auxcmd )
	auxcmd_ = auxcmd;

    BufferString str = cmd;
    char* buf = str.getCStr();
    mSkipBlanks( cmd );
    cmd = getNextNonBlanks( cmd, buf );
    if ( !buf || !*buf ) return;

    char* ptr = firstOcc( buf, '&' );
    if ( ptr )*ptr = '\0';

    if ( progExists(buf) )
	isvalid_ = true;

    progname_ = buf;
    if ( ptr )
    {
	parstrs_.add( "&" );
	return;
    }

    while ( cmd && *cmd )
    {
	cmd = getNextNonBlanks( cmd, buf );
	if ( !buf || !*buf ) break;

	int idx = 0;
	if ( buf[idx]=='&' && !buf[idx+1] )
	{
	    parstrs_.add( buf );
	    continue;
	}

	const char* startquote = firstOcc( buf, '"' );
	if ( startquote )
	{
	    BufferString newbuf( cmd );
	    bool foundmatch = false;
	    while ( cmd )
	    {
		const char* endquote = lastOcc( buf, '"' );
		if ( endquote && endquote != startquote )
		{
		    foundmatch = true;
		    break;
		}

		cmd = getNextNonBlanks( cmd, newbuf.getCStr() );
		if ( !*newbuf.buf() ) break;

#ifdef __win__
		strcat_s( buf, str.bufSize(), " " );
		strcat_s( buf, str.bufSize(), newbuf.buf() );
#else
		strcat( buf, " " );
		strcat( buf, newbuf.buf() );
#endif
	    }

	    if ( !foundmatch ) break;
	}

	char* rsfstr = firstOcc( buf, ".rsf" );
	if ( rsfstr )
	    rsfstr = firstOcc( buf, '=' );

	if ( rsfstr )
	{
	    BufferString filenm( startquote ? rsfstr + 2 : rsfstr + 1 );
	    if ( startquote )
	    {
		char* endquote = filenm.findLast( '"' );
		if ( endquote )
		    *endquote = '\0';
	    }

	    File::Path fp( filenm.buf() );
	    if ( !fp.isAbsolute() )
	    {
		BufferString filepath = fp.fullPath();
		fp.set( ODMad::FileSpec::defPath() );
		fp.add( filepath );
	    }

	    if ( !File::exists(fp.fullPath()) )
	    {
		isvalid_ = false;
		errmsg_ = tr("Cannot find RSF file %1")
			.arg(fp.fullPath());
		return;
	    }

	    *(rsfstr+1) = '\0';
	    BufferString parstr( buf );
	    parstr += "\"";
	    parstr += fp.fullPath();
	    parstr += "\"";
	    parstrs_.add( parstr );
	    continue;
	}

	parstrs_.add( buf );
    }
}


ODMad::Proc::~Proc()
{}


const char* ODMad::Proc::parStr( int idx ) const
{
    if ( idx < 0 || idx >= parstrs_.size() )
	return 0;

    return parstrs_.get( idx );
}


const char* ODMad::Proc::getCommand() const
{
    BufferString* ret = new BufferString( progname_ );
    for ( int idx=0; idx<parstrs_.size(); idx++ )
    {
	*ret += " ";
	*ret += parstrs_.get( idx );
    }

    return ret->buf();
}


const char* ODMad::Proc::getSummary() const
{
    BufferString* str = new BufferString( getCommand() );
    if ( !auxcmd_.isEmpty() )
    {
	*( str->getCStr() + 20 ) = '\0';
	*str += " | ";
	*str += auxcmd_;
    }

    *( str->getCStr() + 40 ) = '\0';
    return str->buf();
}


void ODMad::Proc::fillPar( IOPar& par ) const
{
    par.set( sKeyCommand(), getCommand() );
    if ( !auxcmd_.isEmpty() )
	par.set( sKeyAuxCommand(), auxcmd_ );
}


bool ODMad::Proc::usePar( const IOPar& par )
{
    const char* cmd = par.find( sKeyCommand() );
    if ( !cmd || !*cmd )
	return false;

    const char* auxcmd = par.find( sKeyAuxCommand() );
    makeProc( cmd, auxcmd );
    return true;
}
