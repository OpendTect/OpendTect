/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "commandlineparser.h"

#include "filepath.h"
#include "string2.h"


CommandLineParser::CommandLineParser( int argc, char** argv )
{
    init( argc, argv );
}


CommandLineParser::CommandLineParser()
{
    init( GetArgC(), GetArgV() );
}


const BufferString& CommandLineParser::getExecutable() const
{ return executable_; }


const BufferString& CommandLineParser::getExecutableName() const
{
    return progname_;
}


bool CommandLineParser::hasKey( const char* key ) const
{
    return indexOf( key )!=-1;
}


void CommandLineParser::setKeyHasValue( const char* key )
{
    keyswithvalue_.addIfNew( key );
}


bool CommandLineParser::isPresent( const char* arg ) const
{
    return argv_.isPresent( arg );
}


const BufferString& CommandLineParser::lastArg() const
{
    if ( argv_.isEmpty() )
	return BufferString::empty();
    
    return *argv_.last();
}


bool CommandLineParser::isKeyValue( int idx ) const
{
    if ( !isKey(idx-1) || isKey(idx) )
	return false;
    
    const char* keyptr = getArg( idx-1 );
    while ( *keyptr=='-' )
	keyptr++;
    
    if ( !*keyptr )
	return false;
    
    return keyswithvalue_.isPresent(keyptr);
}


bool CommandLineParser::isKey( int idx ) const
{
    if ( !argv_.validIdx( idx ) )
	return false;
    
    const BufferString& arg = getArg( idx );
    if ( arg.size()<2 )
	return false;
    
    if ( arg.size()>2 && arg.buf()[0]=='-' && arg.buf()[1]=='-' &&
	 !isspace(arg.buf()[2]) )
    {
	return true;
    }
    
    if ( arg.buf()[0]=='-' )
    {
	const char nextchar = arg.buf()[1];
	if ( !isspace(nextchar) && nextchar!='.' && !isdigit(nextchar) )
	{
	    return true;
	}
    }
    
    return false;
}


int CommandLineParser::indexOf( const char* key ) const
{
    const BufferString searchkey1( "--", key );
    const BufferString searchkey2( "-", key );
    
    for ( int idx=argv_.size()-1; idx>=0; idx-- )
    {
	if ( searchkey1==(*argv_[idx]) ||
	     searchkey2==(*argv_[idx]) )
	{
	    if ( isKey(idx) )
		return idx;
	}
    }
    
    return -1;
}



void CommandLineParser::init( int argc, char** argv )
{
    argv_.erase();
    
    if ( argc )
    {
	executable_ = argv[0];
	progname_ = FilePath( executable_ ).fileName();
    }
    else
    {
	executable_.setEmpty();
	progname_.setEmpty();
    }
    
    for ( int idx=1; idx<argc; idx++ )
	argv_.add( argv[idx] );
}


void CommandLineParser::getNormalArguments( BufferStringSet& res ) const
{
    res.erase();
    for ( int idx=0; idx<nrArgs(); idx++ )
    {
	if ( !isKey(idx) && !isKeyValue(idx) )
	    res.add( getArg( idx ).str() );
    }
}


