/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
 * ID       : $Id: madproc.cc,v 1.2 2009-05-26 10:34:31 cvsraman Exp $
-*/


#include "madproc.h"
#include "envvars.h"
#include "filegen.h"
#include "filepath.h"
#include "iopar.h"
#include "string2.h"
#include <cctype>

bool ODMad::Proc::progExists( const char* prog )
{
    const char* rsfroot = GetEnvVar( "RSFROOT" );
    if ( !rsfroot || !*rsfroot ) return false;

    FilePath fp( rsfroot );
    fp.add ( "bin" );
    fp.add( prog );

#if __win__
    fp.setExtension( "exe" );
#endif

    return File_exists( fp.fullPath() );
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

	while ( buf[idx++] ) 
	{
	    if ( buf[idx]=='=' && buf[idx+1] )
	    {
		parstrs_.add( buf );
		break;
	    }
	}
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
