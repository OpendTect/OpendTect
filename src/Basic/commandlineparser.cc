/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "commandlineparser.h"

#include "oddirs.h"
#include "surveydisklocation.h"
#include "survinfo.h"
#include "varlenarray.h"

#include "genc.h"


CommandLineParser::CommandLineParser( const char* str )
{
    init( str );
}


CommandLineParser::CommandLineParser( int argc, char** argv )
{
    init( argc, argv );
}


CommandLineParser::CommandLineParser()
{
    init( GetArgC(), GetArgV() );
}


CommandLineParser::~CommandLineParser()
{}


const OD::String& CommandLineParser::getExecutable() const
{ return executable_; }


const OD::String& CommandLineParser::getExecutableName() const
{
    return progname_;
}


bool CommandLineParser::hasKey( const char* key ) const
{
    return indexOf( key )!=-1;
}


void CommandLineParser::setKeyHasValue( const char* key, int nrvals ) const
{
    ensureNrArgs( key, nrvals );
}


void CommandLineParser::ensureNrArgs( const char* key, int nrvals ) const
{
    const int nrvalsidx = keyswithvalue_.indexOf( key );
    if ( nrvalsidx >= 0 )
    {
	const int curnrvals = nrvalues_[nrvalsidx];
	if ( curnrvals < nrvals )
	    getNonConst(*this).nrvalues_[nrvalsidx] = nrvals;
    }
    else
    {
	getNonConst(*this).keyswithvalue_.add( key );
	getNonConst(*this).nrvalues_.add( nrvals );
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

    const OD::String& arg = getArg( idx );
    if ( arg.size()<2 )
	return false;

    if ( arg.size()>2 && arg.buf()[0]=='-' && arg.buf()[1]=='-' &&
	 !iswspace(arg.buf()[2]) )
    {
	return true;
    }

    if ( arg.buf()[0]=='-' )
    {
	const char nextchar = arg.buf()[1];
	if ( !iswspace(nextchar) && nextchar!='.' && !iswdigit(nextchar) )
	{
	    return true;
	}
    }

    return false;
}


int CommandLineParser::indexOf( const char* key, TypeSet<int>* idxs ) const
{
    if ( idxs )
	idxs->setEmpty();

    const BufferString searchkey1( "--", key );
    const BufferString searchkey2( "-", key );

    for ( int idx=argv_.size()-1; idx>=0; idx-- )
    {
	if ( searchkey1==(*argv_[idx]) ||
	     searchkey2==(*argv_[idx]) )
	{
	    if ( isKey(idx) )
	    {
		if ( idxs )
		    idxs->add( idx );
		else
		    return idx;
	    }
	}
    }

    return idxs && !idxs->isEmpty() ? idxs->first() : -1;
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


void CommandLineParser::init( const char* thecomm )
{
    BufferStringSet args;
    const BufferString comm( thecomm );
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

    init( args.size(), mVarLenArr(argarray) );
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


bool CommandLineParser::getVal( const char* key, BufferStringSet& vals,
				bool acceptnone ) const
{
    vals.setEmpty();
    TypeSet<int> keyidxs;
    const int keyidx = indexOf( key, &keyidxs );
    if ( keyidx<0 )
	return acceptnone;

    for ( int kidx=0; kidx<keyidxs.size(); kidx++ )
    {
	int validx = keyidxs[kidx] + 1;
	while ( argv_.validIdx( validx ) && !isKey(validx) )
	{
	    vals.add( argv_[validx]->buf() );
	    validx++;
	}
    }

    return acceptnone ? true : !vals.isEmpty();
}


bool CommandLineParser::getVal( const char* key, DBKey& dbkey,
				bool acceptnone, int valnr ) const
{
    BufferString str;
    bool res = getVal( key, str, acceptnone, valnr );
    if ( res )
	res = dbkey.fromString( str.buf() );
    else
	dbkey.setUdf();

    return res;
}


bool CommandLineParser::getVal( const char* key, FilePath& fp, bool acceptnone,
					int valnr ) const
{
    BufferString str;
    const bool res = getVal( key, str, acceptnone, valnr );
    fp = res ? FilePath( str.buf() ) : FilePath();
    return res;
}


void CommandLineParser::addKey( const char* key, BufferString& cmd,
				const char* valstr )
{
    if ( !cmd.isEmpty() )
	cmd.addSpace();

    cmd.add( createKey(key) );
    if ( !valstr )
	return;

    cmd.addSpace().add( valstr );
}


void CommandLineParser::addFilePath( const char* fp, BufferString& cmd )
{
    if ( !cmd.isEmpty() )
	cmd.addSpace();

    cmd.add( "\"" ).add( fp ).add( "\"" );
}


BufferString CommandLineParser::envVarBase() const
{
    FilePath fp( executable_ );
    BufferString envvarbase( fp.fileName() );
    envvarbase.replace( ' ', '_' );
    return envvarbase;
}


BufferString CommandLineParser::getFullSurveyPath( bool* iscur ) const
{
    BufferString cursurvfullpath( SI().diskLocation().fullPath() );
    if ( cursurvfullpath.isEmpty() )
    {
	const FilePath fp( GetBaseDataDir(), GetSurveyName() );
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

    const FilePath orgfp( cursurvfullpath );
    BufferString survdir( orgfp.fileName() );
    BufferString dataroot( orgfp.pathOnly() );

    if ( havesurvey )
	survdir = keyedString( sSurveyArg() );
    if ( havedataroot )
	dataroot = keyedString( sDataRootArg() );

    const FilePath fp( dataroot, survdir );
    if ( iscur )
	*iscur = fp == orgfp;

    return fp.fullPath();
}
