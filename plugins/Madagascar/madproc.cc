/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
 * ID       : $Id: madproc.cc,v 1.9 2011/12/14 13:16:41 cvsbert Exp $
-*/


#include "madproc.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "madio.h"
#include "string2.h"
#include <cctype>

bool ODMad::Proc::progExists( const char* prog )
{
    const char* rsfroot = GetEnvVar( "RSFROOT" );
    if ( !rsfroot || !*rsfroot ) return false;

    FilePath fp( rsfroot, "bin", prog );
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
    char* buf = str.buf();
    mSkipBlanks( cmd );
    cmd = getNextWord( cmd, buf );
    if ( !buf || !*buf ) return;

    char* ptr = strchr( buf, '&' );
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
	cmd = getNextWord( cmd, buf );
	if ( !buf || !*buf ) break;

	int idx = 0;
	if ( buf[idx]=='&' && !buf[idx+1] )
	{
	    parstrs_.add( buf );
	    continue;
	}

	const char* startquote = strchr( buf, '"' );
	if ( startquote )
	{
	    BufferString newbuf( cmd );
	    bool foundmatch = false;
	    while ( cmd )
	    {
		const char* endquote = strrchr( buf, '"' );
		if ( endquote && endquote != startquote )
		{
		    foundmatch = true;
		    break;
		}

		cmd = getNextWord( cmd, newbuf.buf() );
		if ( !newbuf.buf() || !*newbuf.buf() ) break;

		strcat( buf, " " );
		strcat( buf, newbuf.buf() );
	    }

	    if ( !foundmatch ) break;
	}

	char* rsfstr = strstr( buf, ".rsf" );
	if ( rsfstr )
	    rsfstr = strchr( buf, '=' );

	if ( rsfstr )
	{
	    BufferString filenm( startquote ? rsfstr + 2 : rsfstr + 1 );
	    if ( startquote )
	    {
		char* endquote = strrchr( filenm.buf(), '"' );
		if ( endquote )
		    *endquote = '\0';
	    }

	    FilePath fp( filenm.buf() );
	    if ( !fp.isAbsolute() )
	    {
		BufferString filepath = fp.fullPath();
		fp.set( ODMad::FileSpec::defPath() );
		fp.add( filepath );
	    }
	    
	    if ( !File::exists(fp.fullPath()) )
	    {
		isvalid_ = false;
		errmsg_ = "Cannot find RSF file ";
		errmsg_ += fp.fullPath();
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
	*( str->buf() + 20 ) = '\0';
	*str += " | ";
	*str += auxcmd_;
    }

    *( str->buf() + 40 ) = '\0';
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
