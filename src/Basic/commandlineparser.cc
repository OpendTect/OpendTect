/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2013
-*/


#include "commandlineparser.h"

#include "dbkey.h"
#include "oddirs.h"
#include "filepath.h"
#include "genc.h"
#include "varlenarray.h"
#include "envvars.h"
#include "survinfo.h"


CommandLineParser::CommandLineParser( const char* fullcommand )
{
    init( fullcommand );
}


CommandLineParser::CommandLineParser( int argc, char** argv )
{
    init( argc, argv );
}


CommandLineParser::CommandLineParser()
{
    init( GetArgC(), GetArgV() );
}


const OD::String& CommandLineParser::getExecutable() const
{ return executable_; }


const OD::String& CommandLineParser::getExecutableName() const
{
    return progname_;
}


bool CommandLineParser::hasKey( const char* key ) const
{
    return indexOf( key ) >= 0;
}


void CommandLineParser::setKeyHasValue( const char* key, int nrvals ) const
{
    const int nrvalsidx = keyswithvalue_.indexOf( key );
    if ( nrvalsidx >= 0 )
	mSelf().nrvalues_[nrvalsidx] = nrvals;
    else
    {
	mSelf().keyswithvalue_.add( key );
	mSelf().nrvalues_.add( nrvals );
    }
}


bool CommandLineParser::isPresent( const char* arg ) const
{
    return argv_.isPresent( arg );
}


const OD::String& CommandLineParser::lastArg() const
{
    if ( argv_.isEmpty() )
	return BufferString::empty();

    return *argv_.last();
}


bool CommandLineParser::isKeyValue( int idx ) const
{
    int keyidx = idx;
    while ( keyidx>=0 && !isKey(keyidx) )
	keyidx--;

    if ( keyidx==idx || keyidx<0 )
	return false;

    const char* keyptr = getArg( keyidx );
    while ( *keyptr=='-' )
	keyptr++;

    if ( !*keyptr )
	return false;

    const int nrvalsidx = keyswithvalue_.indexOf( keyptr );
    if ( nrvalsidx < 0 )
	return false;

    return nrvalues_[nrvalsidx]<1 || nrvalues_[nrvalsidx]>=idx-keyidx;
}


bool CommandLineParser::isKey( int idx ) const
{
    if ( !argv_.validIdx( idx ) )
	return false;

    const BufferString arg = getArg( idx );
    if ( arg.size()<2 )
	return false;

    if ( arg.size()>2 && arg.buf()[0]=='-' && arg.buf()[1]=='-' &&
	 !iswspace(arg.buf()[2]) )
    {
	return true;
    }

    if ( arg.firstChar()=='-' )
    {
	const char nextchar = arg.buf()[1];
	if ( !iswspace(nextchar) && nextchar!='.' && !iswdigit(nextchar) )
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

    if ( argc > 0 )
    {
	executable_ = argv[0];
	progname_ = File::Path( executable_ ).fileName();
    }
    else
    {
	executable_.setEmpty();
	progname_.setEmpty();
	return;
    }

    for ( int idx=1; idx<argc; idx++ )
	argv_.add( argv[idx] );

    File::Path fp( executable_ );
    BufferString envvarbase( fp.fileName() );
    envvarbase.replace( ' ', '_' );
    BufferString envvarnm( envvarbase, "_ARGS" );
    overruleArgsIfEnvVarSet( envvarnm );

    envvarnm.set( envvarbase ).add( "_EXTRA_ARGS" );
    BufferString envvarval = GetEnvVar( envvarnm );
    if ( !envvarval.isEmpty() )
    {
	envvarval.insertAt( 0, "X " );
	CommandLineParser clp( envvarval );
	argv_.append( clp.argv_ );
    }
}


void CommandLineParser::init( const char* fullcommand )
{
    BufferStringSet args;
    const BufferString comm( fullcommand );
    BufferString tmp;

    char inquotes = 0; //0 - not in quotes, otherwise "'" or """ )

    for ( int idx=0; idx<comm.size(); idx++ )
    {
	if ( inquotes )
	{
	    if ( comm[idx]==inquotes )
	    {
		inquotes=0;
		continue;
	    }
	}
	else
	{
	    if ( comm[idx] == '"' )
	    {
		inquotes = '"';
		continue;
	    }
	    if ( comm[idx] == '\'' )
	    {
		inquotes = '\'';
		continue;
	    }

	    if ( isspace(comm[idx]) )
	    {
		if ( !tmp.isEmpty() )
		{
		    args.add(tmp);
		    tmp.setEmpty();
		}

		continue;
	    }
	}

	tmp += comm[idx];
    }

    if ( !tmp.isEmpty() )
	args.add(tmp);

    mAllocVarLenArr( char*, argarray, args.size() );
    for ( int idx=0; idx<args.size(); idx++ )
    {
	argarray[idx] = args.get(idx).getCStr();
    }

    init( args.size(), argarray );
}


void CommandLineParser::overruleArgs( const BufferStringSet& newargs,
				      int startat )
{
    const BufferStringSet oldargs( argv_ );
    argv_.setEmpty();

    for ( int iarg=0; iarg<startat; iarg++ )
    {
	if ( iarg < oldargs.size() )
	    argv_.add( oldargs.get(iarg) );
	else
	    break;
    }

    argv_.append( newargs );
}


void CommandLineParser::overruleArgsIfEnvVarSet( const char* envvarnm )
{
    BufferString envvarval( GetEnvVar(envvarnm) );
    if ( envvarval.isEmpty() )
	return;

    envvarval.insertAt( 0, "X " );
    CommandLineParser clp( envvarval );
    overruleArgs( clp.argv_ );
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



static char* getArgvStr( const BufferString& inp )
{
    const int sz = inp.size();
    char* ret = new char [sz+1];
    const char* inpptr = inp.buf();
    char* outptr = ret;
    while ( *inpptr )
    {
	*outptr = *inpptr;
	outptr++; inpptr++;
    }
    *outptr = '\0';
    return ret;
}


char** CommandLineParser::getArgv() const
{
    const int argc = getArgc();
    char** ret = new char* [argc+1];
    ret[0] = getArgvStr( executable_ );
    for ( int iarg=1; iarg<argc; iarg++ )
	ret[iarg] = getArgvStr( argv_.get(iarg-1) );
    ret[argc] = 0;
    return ret;
}


bool CommandLineParser::getDBKey( const char* key, DBKey& dbky,
				  bool acceptnone, int valnr ) const
{
    BufferString dbkystr;
    bool ret = getVal( key, dbkystr, acceptnone, valnr );
    dbky = DBKey( dbkystr );
    return ret;
}


bool CommandLineParser::getVal( const char* key, BufferString& val,
				bool acceptnone, int valnr ) const
{
    const int keyidx = indexOf( key );
    if ( keyidx<0 )
	return acceptnone;

    const int validx = keyidx + mMAX(valnr,1);
    if ( !argv_.validIdx( validx ) || isKey(validx) )
	return false;

    val.set( argv_[validx]->buf() );
    return true;
}


BufferString CommandLineParser::getFullSurveyPath( bool* iscur ) const
{
    BufferString cursurvfullpath( SI().diskLocation().fullPath() );
    if ( cursurvfullpath.isEmpty() )
    {
	const File::Path fp( GetBaseDataDir(), GetLastSurveyDirName() );
	cursurvfullpath = fp.fullPath();
    }

    const bool havedataroot = hasKey( sDataRootArg() );
    const bool havesurvey = hasKey( sSurveyArg() );
    if ( !havedataroot && !havesurvey )
    {
	if ( iscur )
	    *iscur = true;
	return cursurvfullpath;
    }

    const File::Path orgfp( cursurvfullpath );
    BufferString survdir( orgfp.fileName() );
    BufferString dataroot( orgfp.pathOnly() );

    if ( havesurvey )
    {
	mSelf().setKeyHasValue( sSurveyArg(), 1 );
	getVal( sSurveyArg(), survdir );
    }
    if ( havedataroot )
    {
	mSelf().setKeyHasValue( sDataRootArg(), 1 );
	getVal( sDataRootArg(), dataroot );
    }

    const File::Path fp( dataroot, survdir );
    if ( iscur )
	*iscur = fp == orgfp;

    return fp.fullPath();
}
